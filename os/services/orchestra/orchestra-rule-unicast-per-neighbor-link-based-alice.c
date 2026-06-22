/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         ALICE: a time-varying, link-based unicast slotframe for Orchestra.
 *
 *         For each RPL neighbor (preferred parent and children), and for each
 *         absolute slotframe number (ASFN), the timeslot and channel offset of
 *         the up/down cells are re-derived from hash(link, ASFN). This spreads
 *         link cells over time, reducing persistent collisions compared to the
 *         static link-based rule. When A3_MANAGEMENT is enabled, the number of
 *         cells allocated per link adapts to the measured traffic load (see
 *         a3-management.c).
 *
 *         Designed for RPL storing mode only (needs knowledge of children).
 *
 *         Ported to Contiki-NG from the A3 codebase
 *         (https://github.com/skimskimskim/A3). Based on:
 *         1) "ALICE: autonomous link-based cell scheduling for TSCH",
 *            Seohyang Kim, Hyung-Sin Kim, Chongkwon Kim, ACM/IEEE IPSN 2019.
 *         2) "An Empirical Survey of Autonomous Scheduling Methods for TSCH",
 *            Atis Elsts, Seohyang Kim, Hyung-Sin Kim, Chongkwon Kim,
 *            IEEE Access, 2020.
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "net/mac/tsch/tsch-alice.h"
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ALICE"
#define LOG_LEVEL LOG_LEVEL_MAC

/*
 * Compiled only when ALICE is enabled and routes are available. "nbr_routes"
 * exists only when UIP_MAX_ROUTES != 0 (see uip-ds6-route.c).
 */
#if TSCH_WITH_ALICE && (UIP_MAX_ROUTES != 0)

/* ALICE scheduling mode: 1 = ALICE (link-based), 2 = Orchestra receiver-based,
 * 3 = Orchestra sender-based. Defaults to ALICE. */
#ifdef ORCHESTRA_CONF_ALICE1_ORB2_OSB3
#define ALICE1_ORB2_OSB3 ORCHESTRA_CONF_ALICE1_ORB2_OSB3
#else
#define ALICE1_ORB2_OSB3 1
#endif

#define sq256 65536 /* 256 * 256, used to decorrelate per-cid hashes */

/* Absolute slotframe number currently used to derive the schedule. Updated by
 * alice_callback_slotframe_start. */
static uint16_t asfn_schedule = 0;
#ifdef ALICE_STATIC_ASFN
#define ALICE_ASFN 0  /* diagnostic: freeze schedule (no time-varying) */
#else
#define ALICE_ASFN asfn_schedule
#endif

static uint16_t slotframe_handle = 0;
static struct tsch_slotframe *sf_unicast;

static const uint8_t link_option_rx = LINK_OPTION_RX;
static const uint8_t link_option_tx = LINK_OPTION_TX | LINK_OPTION_SHARED;

#if ALICE1_ORB2_OSB3 != 1
static linkaddr_t addrZero;
#endif

#ifdef A3_MANAGEMENT
static uint16_t CID_PERIOD; /* ORCHESTRA_UNICAST_PERIOD / A3_UNICAST_MAX_REGION */
#if A3_UNICAST_MAX_REGION == 2
static const uint8_t cid_map[2] = { 0, 1 };
#elif A3_UNICAST_MAX_REGION == 4
static const uint8_t cid_map[4] = { 0, 2, 1, 3 };
#elif A3_UNICAST_MAX_REGION == 8
static const uint8_t cid_map[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
#endif
#endif /* A3_MANAGEMENT */

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const linkaddr_t *addr1, const linkaddr_t *addr2, uint8_t cid)
{
#if ALICE1_ORB2_OSB3 == 2
  addr1 = &addrZero;
#elif ALICE1_ORB2_OSB3 == 3
  addr2 = &addrZero;
#endif

  if(addr1 != NULL && addr2 != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
#ifdef A3_MANAGEMENT
    /* Place the cell in zone "shift" (one of A3_UNICAST_MAX_REGION zones), then
     * at a hashed offset within that zone. The zone hash and the intra-zone
     * offset hash MUST use different inputs: real_hash5(x, m) computes
     * mix(x) % m, so reusing the same x makes (offset % zone_count) == zone,
     * collapsing every cell onto only CID_PERIOD distinct timeslots and making
     * a link's up/down cells collide. The sq256 salt decorrelates them so the
     * timeslot spans the whole slotframe. */
    uint16_t shift = (real_hash5(((uint32_t)ORCHESTRA_LINKADDR_HASH2(addr1, addr2) + (uint32_t)ALICE_ASFN),
                                 A3_UNICAST_MAX_REGION) + cid_map[cid]) % A3_UNICAST_MAX_REGION;
    return (CID_PERIOD) * shift
           + real_hash5(((uint32_t)ORCHESTRA_LINKADDR_HASH2(addr1, addr2) + (uint32_t)ALICE_ASFN + sq256), (CID_PERIOD));
#else
    return real_hash5(((uint32_t)ORCHESTRA_LINKADDR_HASH2(addr1, addr2) + (uint32_t)ALICE_ASFN + sq256 * (uint32_t)cid),
                      (ORCHESTRA_UNICAST_PERIOD));
#endif
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_node_channel_offset(const linkaddr_t *addr1, const linkaddr_t *addr2, uint8_t cid)
{
#if ORCHESTRA_ONE_CHANNEL_OFFSET == 1
  return slotframe_handle;
#endif

#if ALICE1_ORB2_OSB3 == 2
  addr1 = &addrZero;
#elif ALICE1_ORB2_OSB3 == 3
  addr2 = &addrZero;
#endif

  int num_ch = (sizeof(TSCH_DEFAULT_HOPPING_SEQUENCE) / sizeof(uint8_t)) - 1;
  if(addr1 != NULL && addr2 != NULL && num_ch > 0) {
    return 1 + real_hash5(((uint32_t)ORCHESTRA_LINKADDR_HASH2(addr1, addr2) + (uint32_t)ALICE_ASFN + sq256 * (uint32_t)cid),
                          num_ch);
  }
  return slotframe_handle;
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
/* Remove the whole unicast slotframe and rebuild it for the current ASFN. Runs
 * inside the slot operation, so uses the lock-free schedule helpers. */
static void
alice_schedule_unicast_slotframe(void)
{
  uint16_t timeslot_us, timeslot_ds, channel_offset_us, channel_offset_ds;
  uint8_t link_option_up, link_option_down;
  uint8_t cid = 0;

  /* Remove all links currently scheduled in the unicast slotframe. */
  struct tsch_link *l = list_head(sf_unicast->links_list);
  while(l != NULL) {
    tsch_schedule_remove_link_alice(sf_unicast, l);
    l = list_head(sf_unicast->links_list);
  }

  if(is_root() != 1) {
    /* Links between this node and its preferred parent. */
#ifdef A3_MANAGEMENT
    for(cid = 0; cid < p_num_tx_cur; cid++) {
#endif
      timeslot_us = get_node_timeslot(&linkaddr_node_addr, &orchestra_parent_linkaddr, cid);
      channel_offset_us = get_node_channel_offset(&linkaddr_node_addr, &orchestra_parent_linkaddr, cid);
      link_option_up = link_option_tx;
      tsch_schedule_add_link_alice(sf_unicast, link_option_up, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, &orchestra_parent_linkaddr,
                                   timeslot_us, channel_offset_us);
#ifdef A3_MANAGEMENT
    }
    for(cid = 0; cid < p_num_rx_cur; cid++) {
#endif
      timeslot_ds = get_node_timeslot(&orchestra_parent_linkaddr, &linkaddr_node_addr, cid);
      channel_offset_ds = get_node_channel_offset(&orchestra_parent_linkaddr, &linkaddr_node_addr, cid);
      link_option_down = link_option_rx;
      tsch_schedule_add_link_alice(sf_unicast, link_option_down, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, &orchestra_parent_linkaddr,
                                   timeslot_ds, channel_offset_ds);
#ifdef A3_MANAGEMENT
    }
#endif
  }

  /* Links between this node and its children (all route next hops). */
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *addr = nbr_table_get_lladdr(nbr_routes, item);
#ifdef A3_MANAGEMENT
    uint8_t cid_cur_tx = 1;
    uint8_t cid_cur_rx = 1;
    uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)addr);
    if(it != NULL) {
      cid_cur_rx = it->num_rx_cur;
      cid_cur_tx = it->num_tx_cur;
    }
#endif

#ifdef A3_MANAGEMENT
    for(cid = 0; cid < cid_cur_rx; cid++) {
#endif
      timeslot_us = get_node_timeslot(addr, &linkaddr_node_addr, cid);
      channel_offset_us = get_node_channel_offset(addr, &linkaddr_node_addr, cid);
      link_option_up = link_option_rx;
      tsch_schedule_add_link_alice(sf_unicast, link_option_up, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, addr, timeslot_us, channel_offset_us);
#ifdef A3_MANAGEMENT
    }
    for(cid = 0; cid < cid_cur_tx; cid++) {
#endif
      timeslot_ds = get_node_timeslot(&linkaddr_node_addr, addr, cid);
      channel_offset_ds = get_node_channel_offset(&linkaddr_node_addr, addr, cid);
      link_option_down = link_option_tx;
      tsch_schedule_add_link_alice(sf_unicast, link_option_down, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, addr, timeslot_ds, channel_offset_ds);
#ifdef A3_MANAGEMENT
    }
#endif

    item = nbr_table_next(nbr_routes, item);
  }
}
/*---------------------------------------------------------------------------*/
/* Hook called by the TSCH core once per absolute slotframe (see tsch-alice.h). */
void
alice_callback_slotframe_start(uint16_t sfid, uint16_t sfsize)
{
  asfn_schedule = sfid;
  alice_schedule_unicast_slotframe();
}
/*---------------------------------------------------------------------------*/
/* Hook called by the TSCH core to (re-)derive the cell for an outgoing packet
 * under the current ASFN (see tsch-alice.h). Returns 1 if rx_lladdr is still a
 * scheduled RPL neighbor. */
int
alice_callback_packet_selection(uint16_t *ts, uint16_t *choff, const linkaddr_t *rx_lladdr)
{
  uint16_t cur_ts = *ts;
  uint16_t cur_choff = *choff;
  int is_neighbor = 0;
  uint8_t cid = 0;

  /* Is the destination our preferred parent? */
  if(linkaddr_cmp(&orchestra_parent_linkaddr, rx_lladdr)) {
    is_neighbor = 1;
#ifdef A3_MANAGEMENT
    for(cid = 0; cid < p_num_tx_cur; cid++) {
#endif
      *ts = get_node_timeslot(&linkaddr_node_addr, &orchestra_parent_linkaddr, cid);
      *choff = get_node_channel_offset(&linkaddr_node_addr, &orchestra_parent_linkaddr, cid);
      if(*ts == cur_ts && *choff == cur_choff) {
        return is_neighbor;
      }
#ifdef A3_MANAGEMENT
    }
#endif
    return is_neighbor;
  }

  /* Is the destination one of our children? */
  nbr_table_item_t *item = nbr_table_get_from_lladdr(nbr_routes, rx_lladdr);
  if(item != NULL) {
    is_neighbor = 1;
#ifdef A3_MANAGEMENT
    uint8_t cid_cur_tx = 1;
    uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)rx_lladdr);
    if(it != NULL) {
      cid_cur_tx = it->num_tx_cur;
    }
    for(cid = 0; cid < cid_cur_tx; cid++) {
#endif
      *ts = get_node_timeslot(&linkaddr_node_addr, rx_lladdr, cid);
      *choff = get_node_channel_offset(&linkaddr_node_addr, rx_lladdr, cid);
      if(*ts == cur_ts && *choff == cur_choff) {
        return is_neighbor;
      }
#ifdef A3_MANAGEMENT
    }
#endif
    return is_neighbor;
  }

  /* Not an RPL neighbor of ours. */
  *ts = 0;
  *choff = slotframe_handle;
  return is_neighbor; /* 0 */
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
static void
child_added(const linkaddr_t *linkaddr)
{
#ifdef A3_MANAGEMENT
  a3_management_child_added(linkaddr);
#endif
  alice_schedule_unicast_slotframe();
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
  alice_schedule_unicast_slotframe();
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
    if(timeslot != NULL) {
      *timeslot = get_node_timeslot(&linkaddr_node_addr, dest, 0);
    }
    if(channel_offset != NULL) {
      *channel_offset = get_node_channel_offset(&linkaddr_node_addr, dest, 0);
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
#ifdef A3_MANAGEMENT
  a3_management_new_time_source();
#endif

  if(new != old) {
    const linkaddr_t *new_addr = new != NULL ? tsch_queue_get_nbr_address(new) : NULL;
    if(new_addr != NULL) {
      linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
    } else {
      linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
    }
    alice_schedule_unicast_slotframe();

    /* Re-synchronize the ASFN bookkeeping to the current ASN. */
    uint16_t mod = TSCH_ASN_MOD(tsch_current_asn, sf_unicast->size);
    struct tsch_asn_t newasn;
    newasn = tsch_current_asn;
    TSCH_ASN_DEC(newasn, mod);
    alice_curr_asfn = TSCH_ASN_DIVISION(newasn, sf_unicast->size);
    alice_next_asfn = alice_curr_asfn;
  }
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  slotframe_handle = sf_handle;
  /* Slotframe for unicast transmissions */
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);

#if ALICE1_ORB2_OSB3 != 1
  linkaddr_copy(&addrZero, &linkaddr_node_addr);
  addrZero.u8[LINKADDR_SIZE - 2] = 0;
  addrZero.u8[LINKADDR_SIZE - 1] = 0;
#endif

  /* ASFN wrap-around: 65536 / unicast_period. */
  alice_limit_asfn = (uint16_t)((uint32_t)65536 / (uint32_t)ORCHESTRA_UNICAST_PERIOD);

#ifdef A3_MANAGEMENT
  CID_PERIOD = ORCHESTRA_UNICAST_PERIOD / A3_UNICAST_MAX_REGION;
  LOG_INFO("A3 enabled: max region %u, CID period %u\n", A3_UNICAST_MAX_REGION, CID_PERIOD);
#endif

  asfn_schedule = alice_tsch_schedule_get_current_asfn(sf_unicast);
  LOG_INFO("ALICE unicast period %u, limit ASFN %u\n", ORCHESTRA_UNICAST_PERIOD, alice_limit_asfn);
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_link_based_alice = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  NULL,
  NULL,
  "unicast per neighbor link based ALICE",
  ORCHESTRA_UNICAST_PERIOD,
};

#endif /* TSCH_WITH_ALICE && (UIP_MAX_ROUTES != 0) */
