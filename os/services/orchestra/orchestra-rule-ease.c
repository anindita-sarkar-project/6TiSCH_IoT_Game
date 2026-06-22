/*
 * Copyright (c) 2024.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DAMAGES ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE.
 */

/**
 * \file
 *         EASE: Energy-Aware Autonomous Scheduling, a two-zone autonomous
 *         6TiSCH scheduler. The unicast slotframe of length SF is split into:
 *           - Zone 1 (timeslots [0, SF_s)): shared cells. A node's shared cell
 *             towards a parent P is hash(P, ASFN); the parent listens there and
 *             all of P's children contend (CSMA) to send their first packet.
 *           - Zone 2 (timeslots [SF_s, SF)): dedicated cells. A child-parent
 *             pair (P, C) gets cells at hash(alpha*(P,C) + i + ASFN) for
 *             additional packets.
 *         The schedule is time-varying: rebuilt every absolute slotframe number
 *         (ASFN), reusing the TSCH-core autonomous-scheduling machinery.
 *
 *         This file implements the SCHEDULING (cell placement). The CUSUM
 *         traffic prediction (how many dedicated cells per child) is added
 *         inline in tsch-schedule.c, and the game-theoretic budget allocation /
 *         token-bucket congestion control in tsch-packet.c (later phases). For
 *         now a fixed baseline of one dedicated cell per neighbour is used.
 *
 *         Designed for RPL storing mode (needs knowledge of children).
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/mac/tsch/tsch-ease.h"
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "EASE"
#define LOG_LEVEL LOG_LEVEL_MAC

#if TSCH_WITH_EASE && (UIP_MAX_ROUTES != 0)

/* Total unicast slotframe length SF, and the shared / dedicated zone split.
 * SF = SF_s + SF_x. By default the slotframe is divided into two equal zones. */
#define EASE_SF              ORCHESTRA_UNICAST_PERIOD
#define EASE_SHARED_LEN      (EASE_SF / 2)               /* SF_s, zone 1 */
#define EASE_DEDICATED_LEN   (EASE_SF - EASE_SHARED_LEN) /* SF_x, zone 2 */

/* Baseline number of dedicated cells per neighbour (replaced by the CUSUM /
 * game-theoretic count in later phases). */
#define EASE_BASELINE_DEDICATED 1

/* Absolute slotframe number currently used to derive the schedule. */
static uint16_t asfn_schedule = 0;

static uint16_t slotframe_handle = 0;
static struct tsch_slotframe *sf_unicast;

static const uint8_t link_option_rx = LINK_OPTION_RX;
static const uint8_t link_option_tx = LINK_OPTION_TX | LINK_OPTION_SHARED;

/*---------------------------------------------------------------------------*/
/* Hash of a single node address (stands in for hash(EUI64)). */
static uint32_t
ease_addr_hash(const linkaddr_t *a)
{
  uint32_t h = 0;
  int i;
  for(i = 0; i < LINKADDR_SIZE; i++) {
    h = h * 31 + a->u8[i];
  }
  return h;
}
/*---------------------------------------------------------------------------*/
static uint16_t
ease_channel_offset(uint32_t h)
{
#if ORCHESTRA_ONE_CHANNEL_OFFSET
  return slotframe_handle;
#else
  int num_ch = (sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE) / sizeof(uint8_t)) - 1;
  if(num_ch <= 0) {
    return slotframe_handle;
  }
  return 1 + real_hash5(h, num_ch);
#endif
}
/*---------------------------------------------------------------------------*/
/* Shared cell (zone 1) towards parent P: timeslot in [0, SF_s). */
static uint16_t
ease_shared_timeslot(const linkaddr_t *parent)
{
  return real_hash5(ease_addr_hash(parent) + (uint32_t)asfn_schedule, EASE_SHARED_LEN);
}
static uint16_t
ease_shared_choff(const linkaddr_t *parent)
{
  return ease_channel_offset(ease_addr_hash(parent) + (uint32_t)asfn_schedule);
}
/*---------------------------------------------------------------------------*/
/* Dedicated cell (zone 2) for the directed pair (parent P, child C), packet
 * index i: timeslot in [SF_s, SF). Both endpoints compute it with P first and C
 * second, so sender and receiver agree. */
static uint16_t
ease_dedicated_timeslot(const linkaddr_t *p, const linkaddr_t *c, uint8_t i)
{
  uint32_t h = EASE_ALPHA * (uint32_t)ORCHESTRA_LINKADDR_HASH2(p, c)
               + (uint32_t)i + (uint32_t)asfn_schedule;
  return EASE_SHARED_LEN + real_hash5(h, EASE_DEDICATED_LEN);
}
static uint16_t
ease_dedicated_choff(const linkaddr_t *p, const linkaddr_t *c, uint8_t i)
{
  uint32_t h = EASE_ALPHA * (uint32_t)ORCHESTRA_LINKADDR_HASH2(p, c)
               + (uint32_t)i + (uint32_t)asfn_schedule;
  return ease_channel_offset(h);
}
/*---------------------------------------------------------------------------*/
static uint16_t
is_root(void)
{
  rpl_instance_t *instance = rpl_get_default_instance();
  if(instance != NULL && instance->current_dag != NULL) {
    if(instance->min_hoprankinc == (uint16_t)instance->current_dag->rank) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Number of dedicated cells this node should keep for a neighbour. With
 * EASE_MANAGEMENT this is the CUSUM-predicted count: our own predicted uplink
 * demand for the parent, or the demand we estimate from a child. Without it,
 * a fixed baseline is used. */
static uint8_t
ease_num_dedicated(const linkaddr_t *neighbor)
{
#ifdef EASE_MANAGEMENT
  if(linkaddr_cmp(neighbor, &orchestra_parent_linkaddr)) {
    /* Uplink: schedule Tx on our own predicted demand for the parent, but keep
     * at least one dedicated cell. The dedicated hash mixes in our own address,
     * so every child of the same parent gets a DISTINCT slot; the single shared
     * cell (hash of the parent alone) is the same slot for all siblings, so at
     * low load -- one packet per slotframe, always a "first" packet -- every
     * child would pile that packet onto the one shared cell and collide every
     * slotframe. Guaranteeing one dedicated cell gives each child a private,
     * collision-free slot from the first packet. */
    /* Floor of 2: with a single dedicated cell the only collision-free uplink
     * opportunity per slotframe is that one ASFN-varying slot, which intermittently
     * collides with a sibling's slot (zone-2 birthday collision). The shared cell
     * is no help at steady state (every child emits one packet per slotframe, so it
     * is a guaranteed N-way collision every frame). One cell therefore gives
     * throughput just under the 1 pkt/slotframe arrival rate, with no slack to
     * recover, so the queue drifts to full and never drains. Two cells push
     * throughput clearly above arrival: a child only stalls if BOTH its slots
     * collide in the same frame (rare), so backlogs drain. */
    return ease_p_cells > 2 ? ease_p_cells : 2;
  }
  /* Downlink (parent role): schedule Rx cells for a child. The child sizes its
   * Tx cells from its OWN CUSUM (what it sends); we size our Rx from a DIFFERENT
   * CUSUM (what we receive). Those two predictors observe different things, so
   * without headroom they deadlock: we won't open an Rx cell until we receive
   * more, but the child can't deliver more until we open the Rx cell, pinning us
   * at ~1 cell while the child piles up Tx cells (the "queue 63/64" overflow).
   * Listening on a few cells beyond our current estimate lets the child ramp and
   * the two counts converge upward (bounded by EASE_MAX_DEDICATED / the zone). */
  uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)neighbor);
  if(it == NULL) {
    return 0;
  }
  int rx = (int)it->ease_cells + EASE_RX_HEADROOM;
  if(rx > EASE_MAX_DEDICATED) {
    rx = EASE_MAX_DEDICATED;
  }
  return (uint8_t)rx;
#else
  (void)neighbor;
  return EASE_BASELINE_DEDICATED;
#endif
}
/*---------------------------------------------------------------------------*/
/* Rebuild the whole EASE slotframe for the current ASFN. Runs inside the slot
 * operation, so uses the lock-free schedule helpers. */
static void
ease_schedule_unicast_slotframe(void)
{
  uint8_t i;

  /* Remove all links currently in the EASE slotframe. */
  struct tsch_link *l = list_head(sf_unicast->links_list);
  while(l != NULL) {
    tsch_schedule_remove_link_alice(sf_unicast, l);
    l = list_head(sf_unicast->links_list);
  }

  if(is_root() != 1) {
    /* Child role: cells towards our preferred parent. */
    const linkaddr_t *parent = &orchestra_parent_linkaddr;
    /* Zone 1: one shared Tx cell (first packet, CSMA). */
    tsch_schedule_add_link_alice(sf_unicast, link_option_tx, LINK_TYPE_NORMAL,
                                 &tsch_broadcast_address, parent,
                                 ease_shared_timeslot(parent), ease_shared_choff(parent));
    /* Zone 2: dedicated Tx cells (additional packets). Uplink slot hash(P,C). */
    for(i = 0; i < ease_num_dedicated(parent); i++) {
      tsch_schedule_add_link_alice(sf_unicast, link_option_tx, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, parent,
                                   ease_dedicated_timeslot(parent, &linkaddr_node_addr, i),
                                   ease_dedicated_choff(parent, &linkaddr_node_addr, i));
    }
    /* Zone 2: dedicated Rx cells to receive downlink from the parent. The
     * downlink slot is the swapped hash(C,P), distinct from the uplink slot. */
    for(i = 0; i < EASE_DOWNLINK_CELLS; i++) {
      tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, parent,
                                   ease_dedicated_timeslot(&linkaddr_node_addr, parent, i),
                                   ease_dedicated_choff(&linkaddr_node_addr, parent, i));
    }
  }

  /* Parent role: cells towards our children (route next hops). */
  if(nbr_table_head(nbr_routes) != NULL) {
    /* Zone 1: one shared Rx cell, shared by all our children
     * (hash of our own address == each child's shared-Tx cell). */
    tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                 &tsch_broadcast_address, &linkaddr_node_addr,
                                 ease_shared_timeslot(&linkaddr_node_addr),
                                 ease_shared_choff(&linkaddr_node_addr));
  }
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *child = nbr_table_get_lladdr(nbr_routes, item);
    if(child != NULL) {
      /* Zone 2: dedicated Rx cells to receive uplink from this child. Uplink
       * slot hash(P,C). */
      for(i = 0; i < ease_num_dedicated(child); i++) {
        tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                     &tsch_broadcast_address, child,
                                     ease_dedicated_timeslot(&linkaddr_node_addr, child, i),
                                     ease_dedicated_choff(&linkaddr_node_addr, child, i));
      }
      /* Zone 2: dedicated Tx cells to send downlink to this child. The downlink
       * slot is the swapped hash(C,P), distinct from the uplink slot, and the
       * child listens on exactly these same EASE_DOWNLINK_CELLS slots. */
      for(i = 0; i < EASE_DOWNLINK_CELLS; i++) {
        tsch_schedule_add_link_alice(sf_unicast, link_option_tx, LINK_TYPE_NORMAL,
                                     &tsch_broadcast_address, child,
                                     ease_dedicated_timeslot(child, &linkaddr_node_addr, i),
                                     ease_dedicated_choff(child, &linkaddr_node_addr, i));
      }
    }
    item = nbr_table_next(nbr_routes, item);
  }
}
/*---------------------------------------------------------------------------*/
void
ease_callback_slotframe_start(uint16_t sfid, uint16_t sfsize)
{
  asfn_schedule = sfid;
  ease_schedule_unicast_slotframe();
}
/*---------------------------------------------------------------------------*/
int
ease_callback_packet_selection(uint16_t *ts, uint16_t *choff, const linkaddr_t *rx_lladdr)
{
  uint16_t cur_ts = *ts;
  uint16_t cur_choff = *choff;
  uint8_t i;

  /* Destination is our preferred parent: try our Tx cells towards it. Prefer
   * the dedicated cells -- each child of a parent hashes to a DISTINCT dedicated
   * slot, so siblings never collide. The shared cell (one slot per parent, every
   * child hashes to it) is only a bootstrap fallback for when we have no
   * dedicated cell yet; matching it first would funnel every sibling's packet
   * onto the same slot and collide every slotframe. */
  if(linkaddr_cmp(&orchestra_parent_linkaddr, rx_lladdr)) {
    uint8_t nd = ease_num_dedicated(rx_lladdr);
    for(i = 0; i < nd; i++) {
      *ts = ease_dedicated_timeslot(rx_lladdr, &linkaddr_node_addr, i);
      *choff = ease_dedicated_choff(rx_lladdr, &linkaddr_node_addr, i);
      if(*ts == cur_ts && *choff == cur_choff) {
        return 1;
      }
    }
    if(nd == 0) {
      /* No dedicated cell yet: fall back to the shared cell. */
      *ts = ease_shared_timeslot(rx_lladdr);
      *choff = ease_shared_choff(rx_lladdr);
    }
    return 1;
  }

  /* Destination is one of our children: send downlink on the swapped-hash
   * slot hash(C,P) where the child is listening (NOT the uplink slot hash(P,C),
   * where we only have an Rx link). */
  if(nbr_table_get_from_lladdr(nbr_routes, rx_lladdr) != NULL) {
    for(i = 0; i < EASE_DOWNLINK_CELLS; i++) {
      *ts = ease_dedicated_timeslot(rx_lladdr, &linkaddr_node_addr, i);
      *choff = ease_dedicated_choff(rx_lladdr, &linkaddr_node_addr, i);
      if(*ts == cur_ts && *choff == cur_choff) {
        return 1;
      }
    }
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
neighbor_has_uc_link(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL && !linkaddr_cmp(linkaddr, &linkaddr_null)) {
    if(orchestra_parent_knows_us && linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr)) {
      return 1;
    }
    if(nbr_table_get_from_lladdr(nbr_routes, (linkaddr_t *)linkaddr) != NULL) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && neighbor_has_uc_link(dest)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    /* Default to the dedicated cell; the link selector re-derives per ASFN. */
    if(timeslot != NULL) {
      if(linkaddr_cmp(&orchestra_parent_linkaddr, dest)) {
        /* Uplink to parent: slot hash(parent, self). */
        *timeslot = ease_dedicated_timeslot(dest, &linkaddr_node_addr, 0);
      } else {
        /* Downlink to child: swapped slot hash(child, self) where it listens. */
        *timeslot = ease_dedicated_timeslot(dest, &linkaddr_node_addr, 0);
      }
    }
    if(channel_offset != NULL) {
      *channel_offset = ease_dedicated_choff(dest, &linkaddr_node_addr, 0);
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
child_added(const linkaddr_t *linkaddr)
{
  ease_schedule_unicast_slotframe();
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
  ease_schedule_unicast_slotframe();
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  if(new != old) {
    const linkaddr_t *new_addr = new != NULL ? tsch_queue_get_nbr_address(new) : NULL;
    if(new_addr != NULL) {
      linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
    } else {
      linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
    }
    ease_schedule_unicast_slotframe();

    /* Resync the ASFN bookkeeping to the current ASN. */
    uint16_t mod = TSCH_ASN_MOD(tsch_current_asn, sf_unicast->size);
    struct tsch_asn_t newasn = tsch_current_asn;
    TSCH_ASN_DEC(newasn, mod);
    ease_curr_asfn = TSCH_ASN_DIVISION(newasn, sf_unicast->size);
  }
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  slotframe_handle = sf_handle;
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, EASE_SF);
  asfn_schedule = (sf_unicast != NULL)
    ? TSCH_ASN_DIVISION(tsch_current_asn, sf_unicast->size) : 0;
  LOG_INFO("EASE init: SF %u (shared %u + dedicated %u), alpha %u, RDC %u%%\n",
           EASE_SF, EASE_SHARED_LEN, EASE_DEDICATED_LEN, EASE_ALPHA, EASE_RDC_PERCENT);
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_ease = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  NULL,
  NULL,
  "unicast per neighbor EASE",
  EASE_SF,
};

#endif /* TSCH_WITH_EASE && (UIP_MAX_ROUTES != 0) */
