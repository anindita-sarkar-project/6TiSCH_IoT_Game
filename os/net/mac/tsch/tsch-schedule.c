/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         IEEE 802.15.4 TSCH MAC schedule manager.
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         Beshr Al Nahas <beshr@sics.se>
 *         Atis Elsts <atis.elsts@edi.lv>
 */

/**
 * \addtogroup tsch
 * @{
*/

#include "contiki.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "net/nbr-table.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-alice.h"
#include "net/mac/tsch/tsch-ease.h"
#include "net/mac/framer/frame802154.h"
#include "sys/process.h"
#include "sys/rtimer.h"
#include <string.h>

#ifdef EASE_MANAGEMENT
/* EASE runs its CUSUM traffic predictor inline here (the same place the A3
 * reference keeps its EWMA), so it reaches the routing neighbor tables. */
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#endif /* EASE_MANAGEMENT */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH Sched"
#define LOG_LEVEL LOG_LEVEL_MAC

/* Pre-allocated space for links */
MEMB(link_memb, struct tsch_link, TSCH_SCHEDULE_MAX_LINKS);
/* Pre-allocated space for slotframes */
MEMB(slotframe_memb, struct tsch_slotframe, TSCH_SCHEDULE_MAX_SLOTFRAMES);
/* List of slotframes (each slotframe holds its own list of links) */
LIST(slotframe_list);

/* Adds and returns a slotframe (NULL if failure) */
struct tsch_slotframe *
tsch_schedule_add_slotframe(uint16_t handle, uint16_t size)
{
  if(size == 0) {
    return NULL;
  }

  if(tsch_schedule_get_slotframe_by_handle(handle)) {
    /* A slotframe with this handle already exists */
    return NULL;
  }

  if(tsch_get_lock()) {
    struct tsch_slotframe *sf = memb_alloc(&slotframe_memb);
    if(sf != NULL) {
      /* Initialize the slotframe */
      sf->handle = handle;
      TSCH_ASN_DIVISOR_INIT(sf->size, size);
      LIST_STRUCT_INIT(sf, links_list);
      /* Add the slotframe to the global list */
      list_add(slotframe_list, sf);
    }
    LOG_INFO("Adding slotframe %u, size %u\n", handle, size);
    tsch_release_lock();
    return sf;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Removes all slotframes, resulting in an empty schedule */
int
tsch_schedule_remove_all_slotframes(void)
{
  struct tsch_slotframe *sf;
  while((sf = list_head(slotframe_list))) {
    if(tsch_schedule_remove_slotframe(sf) == 0) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/* Removes a slotframe Return 1 if success, 0 if failure */
int
tsch_schedule_remove_slotframe(struct tsch_slotframe *slotframe)
{
  if(slotframe != NULL) {
    /* Remove all links belonging to this slotframe */
    struct tsch_link *l;
    while((l = list_head(slotframe->links_list))) {
      tsch_schedule_remove_link(slotframe, l);
    }

    /* Now that the slotframe has no links, remove it. */
    if(tsch_get_lock()) {
      LOG_INFO("Remove slotframe %u, size %u\n",
               slotframe->handle, slotframe->size.val);
      memb_free(&slotframe_memb, slotframe);
      list_remove(slotframe_list, slotframe);
      tsch_release_lock();
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Looks for a slotframe from a handle */
struct tsch_slotframe *
tsch_schedule_get_slotframe_by_handle(uint16_t handle)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    while(sf != NULL) {
      if(sf->handle == handle) {
        return sf;
      }
      sf = list_item_next(sf);
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Looks for a link from a handle */
struct tsch_link *
tsch_schedule_get_link_by_handle(uint16_t handle)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    while(sf != NULL) {
      struct tsch_link *l = list_head(sf->links_list);
      /* Loop over all items. Assume there is max one link per timeslot */
      while(l != NULL) {
        if(l->handle == handle) {
          return l;
        }
        l = list_item_next(l);
      }
      sf = list_item_next(sf);
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static const char *
print_link_options(uint16_t link_options)
{
  static char buffer[20];
  unsigned length;

  buffer[0] = '\0';
  if(link_options & LINK_OPTION_TX) {
    strcat(buffer, "Tx|");
  }
  if(link_options & LINK_OPTION_RX) {
    strcat(buffer, "Rx|");
  }
  if(link_options & LINK_OPTION_SHARED) {
    strcat(buffer, "Sh|");
  }
  length = strlen(buffer);
  if(length > 0) {
    buffer[length - 1] = '\0';
  }

  return buffer;
}
/*---------------------------------------------------------------------------*/
static const char *
print_link_type(uint16_t link_type)
{
  switch(link_type) {
  case LINK_TYPE_NORMAL:
    return "NORMAL";
  case LINK_TYPE_ADVERTISING:
    return "ADV";
  case LINK_TYPE_ADVERTISING_ONLY:
    return "ADV_ONLY";
  default:
    return "?";
  }
}
/*---------------------------------------------------------------------------*/
/* Adds a link to a slotframe, return a pointer to it (NULL if failure) */
struct tsch_link *
tsch_schedule_add_link(struct tsch_slotframe *slotframe,
                       uint8_t link_options, enum link_type link_type, const linkaddr_t *address,
                       uint16_t timeslot, uint16_t channel_offset, uint8_t do_remove)
{
  struct tsch_link *l = NULL;
  if(slotframe != NULL) {
    /* We currently support only one link per timeslot in a given slotframe. */

    /* Validation of specified timeslot and channel_offset */
    if(timeslot > (slotframe->size.val - 1)) {
      LOG_ERR("! add_link invalid timeslot: %u\n", timeslot);
      return NULL;
    }

    if(do_remove) {
      /* Start with removing any link currently installed at this timeslot
       * (needed to keep neighbor state in sync with link options etc.). We
       * don't check for channel offset because only one link per timeslot
       * is allowed in a given slotframe */
      l = tsch_schedule_get_link_by_timeslot(slotframe, timeslot);
      if(l != NULL) {
        tsch_schedule_remove_link(slotframe, l);
        l = NULL;
      }
    }
    if(!tsch_get_lock()) {
      LOG_ERR("! add_link memb_alloc couldn't take lock\n");
    } else {
      l = memb_alloc(&link_memb);
      if(l == NULL) {
        LOG_ERR("! add_link memb_alloc failed\n");
        tsch_release_lock();
      } else {
        static int current_link_handle = 0;
        struct tsch_neighbor *n;
        /* Add the link to the slotframe */
        list_add(slotframe->links_list, l);
        /* Initialize link */
        l->handle = current_link_handle++;
        l->link_options = link_options;
        l->link_type = link_type;
        l->slotframe_handle = slotframe->handle;
        l->timeslot = timeslot;
        l->channel_offset = channel_offset;
        l->data = NULL;
        if(address == NULL) {
          address = &linkaddr_null;
        }
        linkaddr_copy(&l->addr, address);

        LOG_INFO("add_link sf=%u opt=%s type=%s ts=%u ch=%u addr=",
                 slotframe->handle,
                 print_link_options(link_options),
                 print_link_type(link_type), timeslot, channel_offset);
        LOG_INFO_LLADDR(address);
        LOG_INFO_("\n");
        /* Release the lock before we update the neighbor (will take the lock) */
        tsch_release_lock();

        if(l->link_options & LINK_OPTION_TX) {
          n = tsch_queue_add_nbr(&l->addr);
          /* We have a tx link to this neighbor, update counters */
          if(n != NULL) {
            n->tx_links_count++;
            if(!(l->link_options & LINK_OPTION_SHARED)) {
              n->dedicated_tx_links_count++;
            }
          }
        }
      }
    }
  }
  return l;
}
/*---------------------------------------------------------------------------*/
/* Removes a link from slotframe. Return 1 if success, 0 if failure */
int
tsch_schedule_remove_link(struct tsch_slotframe *slotframe, struct tsch_link *l)
{
  if(slotframe != NULL && l != NULL && l->slotframe_handle == slotframe->handle) {
    if(tsch_get_lock()) {
      uint8_t link_options;
      linkaddr_t addr;

      /* Save link option and addr in local variables as we need them
       * after freeing the link */
      link_options = l->link_options;
      linkaddr_copy(&addr, &l->addr);

      /* The link to be removed is scheduled as next, set it to NULL
       * to abort the next link operation */
      if(l == current_link) {
        current_link = NULL;
      }
      LOG_INFO("remove_link sf=%u opt=%s type=%s ts=%u ch=%u addr=",
               slotframe->handle,
               print_link_options(l->link_options),
               print_link_type(l->link_type), l->timeslot, l->channel_offset);
      LOG_INFO_LLADDR(&l->addr);
      LOG_INFO_("\n");

      list_remove(slotframe->links_list, l);
      memb_free(&link_memb, l);

      /* Release the lock before we update the neighbor (will take the lock) */
      tsch_release_lock();

      /* This was a tx link to this neighbor, update counters */
      if(link_options & LINK_OPTION_TX) {
        struct tsch_neighbor *n = tsch_queue_get_nbr(&addr);
        if(n != NULL) {
          n->tx_links_count--;
          if(!(link_options & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count--;
          }
        }
      }

      return 1;
    } else {
      LOG_ERR("! remove_link memb_alloc couldn't take lock\n");
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Removes a link from slotframe and timeslot + channel offset. Return a 1 if
 * success, 0 if failure */
int
tsch_schedule_remove_link_by_offsets(struct tsch_slotframe *slotframe,
                                     uint16_t timeslot, uint16_t channel_offset)
{
  int ret = 0;
  if(!tsch_is_locked()) {
    if(slotframe != NULL) {
      struct tsch_link *l = list_head(slotframe->links_list);
      /* Loop over all items and remove all matching links */
      while(l != NULL) {
        struct tsch_link *next = list_item_next(l);
        if(l->timeslot == timeslot && l->channel_offset == channel_offset) {
          if(tsch_schedule_remove_link(slotframe, l)) {
            ret = 1;
          }
        }
        l = next;
      }
    }
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
/* Looks within a slotframe for a link with a given timeslot and channel
 * offset */
struct tsch_link *
tsch_schedule_get_link_by_offsets(struct tsch_slotframe *slotframe,
                                  uint16_t timeslot, uint16_t channel_offset)
{
  if(!tsch_is_locked()) {
    if(slotframe != NULL) {
      struct tsch_link *l = list_head(slotframe->links_list);
      /* Loop over all items. Assume there is max one link per timeslot
         and channel_offset */
      while(l != NULL) {
        if(l->timeslot == timeslot && l->channel_offset == channel_offset) {
          return l;
        }
        l = list_item_next(l);
      }
      return l;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Looks within a slotframe for a link with a given timeslot */
struct tsch_link *
tsch_schedule_get_link_by_timeslot(struct tsch_slotframe *slotframe,
                                  uint16_t timeslot)
{
  if(!tsch_is_locked()) {
    if(slotframe != NULL) {
      struct tsch_link *l = list_head(slotframe->links_list);
      /* Loop over all items. Assume there is max one link per timeslot */
      while(l != NULL) {
        if(l->timeslot == timeslot) {
          return l;
        }
        l = list_item_next(l);
      }
      return l;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
#if TSCH_WITH_AUTONOMOUS
/*---------------------------------------------------------------------------*/
/* Shared support for the time-varying autonomous schedulers (ALICE/EASE).
 * Originally ported from the A3 codebase (https://github.com/skimskimskim/A3).
 * Compiles only under TSCH_WITH_ALICE || TSCH_WITH_EASE; with both disabled the
 * scheduler behaves exactly as upstream. */

/* Absolute slotframe number bookkeeping (see tsch-alice.h). */
uint16_t alice_curr_asfn = 0;
uint16_t alice_next_asfn = 0;
uint16_t alice_limit_asfn = 0;

/*---------------------------------------------------------------------------*/
/* Integer hash (variant of Thomas Wang's 32-bit mix) used by ALICE to map a
 * (link, ASFN) pair onto a timeslot/channel offset. */
uint16_t
real_hash5(uint32_t value, uint16_t mod)
{
  uint32_t a = value;
  a = ((((a + (a >> 16)) ^ (a >> 9)) ^ (a << 3)) ^ (a >> 5));
  return (uint16_t)(a % (uint32_t)mod);
}
/*---------------------------------------------------------------------------*/
uint16_t
alice_tsch_schedule_get_current_asfn(struct tsch_slotframe *sf)
{
  if(sf != NULL) {
    return TSCH_ASN_DIVISION(tsch_current_asn, sf->size);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Merge link_options into any link already installed at (timeslot,
 * channel_offset). Used when an up- and a down-link map onto the same cell:
 * the cell must then carry both the Tx and Rx options. */
struct tsch_link *
tsch_schedule_set_link_option_by_offsets(struct tsch_slotframe *slotframe,
                                         uint16_t timeslot, uint16_t channel_offset,
                                         uint8_t *link_options)
{
  if(slotframe != NULL) {
    struct tsch_link *l = list_head(slotframe->links_list);
    while(l != NULL) {
      if(l->timeslot == timeslot && l->channel_offset == channel_offset) {
        *link_options |= l->link_options;
        l->link_options |= *link_options;
      }
      l = list_item_next(l);
    }
    return l;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Like tsch_schedule_add_link, but additionally records the RPL neighbor this
 * link serves (l->neighbor) so per-cell Tx/Rx outcomes can be attributed to it,
 * and merges options with any co-located link (l->link_option_alice keeps this
 * link's own, unmerged options). Never auto-removes: the ALICE rule clears the
 * whole unicast slotframe before rebuilding it. */
struct tsch_link *
tsch_schedule_add_link_alice(struct tsch_slotframe *slotframe,
                             uint8_t link_options, enum link_type link_type,
                             const linkaddr_t *address, const linkaddr_t *neighbor,
                             uint16_t timeslot, uint16_t channel_offset)
{
  struct tsch_link *l = NULL;
  if(slotframe != NULL) {
    /* This runs from tsch_schedule_get_next_active_link, i.e. inside the slot
     * operation (tsch_in_slot_operation == 1). Acquiring the TSCH lock here
     * would busy-wait forever, so we deliberately operate lock-free. This is
     * safe because the slot operation is the sole writer in this context.
     * ALICE links always use the broadcast address for l->addr (shared links),
     * so tsch_queue_get_nbr below hits the always-present broadcast neighbor. */
    l = memb_alloc(&link_memb);
    if(l == NULL) {
      LOG_ERR("! add_link_alice memb_alloc failed\n");
    } else {
      static int current_link_handle = 0;
      struct tsch_neighbor *n;
      list_add(slotframe->links_list, l);
      l->handle = current_link_handle++;
      l->link_option_alice = link_options;
      tsch_schedule_set_link_option_by_offsets(slotframe, timeslot, channel_offset, &link_options);
      l->link_options = link_options;
      l->link_type = link_type;
      l->slotframe_handle = slotframe->handle;
      l->timeslot = timeslot;
      l->channel_offset = channel_offset;
      l->data = NULL;
      if(address == NULL) {
        address = &linkaddr_null;
      }
      linkaddr_copy(&l->addr, address);
      if(neighbor == NULL) {
        neighbor = &linkaddr_null;
      }
      linkaddr_copy(&l->neighbor, neighbor);

      if(l->link_options & LINK_OPTION_TX) {
        n = tsch_queue_get_nbr(&l->addr);
        if(n != NULL) {
          n->tx_links_count++;
          if(!(l->link_options & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count++;
          }
        }
      }
    }
  }
  return l;
}
/*---------------------------------------------------------------------------*/
/* Lock-free link removal used by the ALICE rule to clear the unicast slotframe
 * before rebuilding it. Like tsch_schedule_remove_link but without taking the
 * TSCH lock (see the rationale in tsch_schedule_add_link_alice). */
int
tsch_schedule_remove_link_alice(struct tsch_slotframe *slotframe, struct tsch_link *l)
{
  if(slotframe != NULL && l != NULL && l->slotframe_handle == slotframe->handle) {
    uint8_t link_options = l->link_options;
    linkaddr_t addr;
    linkaddr_copy(&addr, &l->addr);

    if(l == current_link) {
      current_link = NULL;
    }
    list_remove(slotframe->links_list, l);
    memb_free(&link_memb, l);

    if(link_options & LINK_OPTION_TX) {
      struct tsch_neighbor *n = tsch_queue_get_nbr(&addr);
      if(n != NULL) {
        n->tx_links_count--;
        if(!(link_options & LINK_OPTION_SHARED)) {
          n->dedicated_tx_links_count--;
        }
      }
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#endif /* TSCH_WITH_AUTONOMOUS */

#if TSCH_WITH_EASE
/* EASE: absolute slotframe number currently materialised in the schedule. */
uint16_t ease_curr_asfn = 0;

#ifdef EASE_MANAGEMENT
/* Child-side CUSUM state: this node's own predicted uplink demand to its parent.
 * ease_p_demand is incremented by tsch-slot-operation.c on each Tx to the parent;
 * the rest is private CUSUM state. */
uint8_t ease_p_demand = 0;
uint8_t ease_p_cells = 0;
static double ease_p_s = 0;
static double ease_p_mu = 0;

/*---------------------------------------------------------------------------*/
/* One CUSUM update (paper eqs 2-5) for a single link: given the demand D
 * observed over the interval, advance (S, mu) and return the new cell count c.
 * Resets the demand counter through its pointer. */
static uint8_t
ease_cusum_step(uint8_t *demand, double *s, double *mu)
{
  double d = (double)*demand;
  double z = d - *mu;
  double snew = *s + z;
  if(snew < 0) {
    snew = 0;
  }
  *s = snew;
  if(snew > (double)EASE_CUSUM_H) {
    *mu = d;                                   /* abrupt upward shift detected */
  } else {
    *mu = EASE_CUSUM_RHO * (*mu) + (1.0 - EASE_CUSUM_RHO) * d;
  }
  *demand = 0;
  double c = *mu < 0 ? 0 : *mu;
  long cells = (long)(c + 0.5);   /* round to nearest, c >= 0 */
  if(cells > EASE_MAX_DEDICATED) {
    cells = EASE_MAX_DEDICATED;
  }
  return (uint8_t)cells;
}
/*---------------------------------------------------------------------------*/
void
ease_cusum_recalculate(void)
{
  /* Parent link: this node's own uplink demand (packets delivered to the parent
   * this interval). At low load this settles to ~1 cell, which is exactly right;
   * the CUSUM ramps it up only under sustained high load. (Folding queue backlog
   * in here was tried and reverted: on the autonomous hash-scheduled dedicated
   * zone, extra cells from different links collide rather than add throughput, so
   * inflating the count over-subscribes the zone and makes delivery worse.) */
  uint8_t old_p = ease_p_cells;
  ease_p_cells = ease_cusum_step(&ease_p_demand, &ease_p_s, &ease_p_mu);
  if(ease_p_cells != old_p) {
    LOG_INFO("EASE CUSUM: parent dedicated cells %u->%u (mu %d/100)\n",
             old_p, ease_p_cells, (int)(ease_p_mu * 100));
  }

  /* Each child (route next hop): the demand we received from it. */
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *addr = nbr_table_get_lladdr(nbr_routes, item);
    if(addr != NULL) {
      uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)addr);
      if(it != NULL) {
        uint8_t old_c = it->ease_cells;
        it->ease_cells = ease_cusum_step(&it->ease_demand, &it->ease_s, &it->ease_mu);
        if(it->ease_cells != old_c) {
          LOG_INFO("EASE CUSUM: child %u dedicated cells %u->%u (mu %d/100)\n",
                   addr->u8[LINKADDR_SIZE - 1], old_c, it->ease_cells,
                   (int)(it->ease_mu * 100));
        }
      }
    }
    item = nbr_table_next(nbr_routes, item);
  }
}
#endif /* EASE_MANAGEMENT */
#endif /* TSCH_WITH_EASE */
/*---------------------------------------------------------------------------*/
static struct tsch_link *
default_tsch_link_comparator(struct tsch_link *a, struct tsch_link *b)
{
  if(!(a->link_options & LINK_OPTION_TX)) {
    /* None of the links are Tx: simply return the first link */
    return a;
  }

  /* Two Tx links at the same slotframe; return the one with most packets to send */
  if(!linkaddr_cmp(&a->addr, &b->addr)) {
    struct tsch_neighbor *an = tsch_queue_get_nbr(&a->addr);
    struct tsch_neighbor *bn = tsch_queue_get_nbr(&b->addr);
    int a_packet_count = an ? ringbufindex_elements(&an->tx_ringbuf) : 0;
    int b_packet_count = bn ? ringbufindex_elements(&bn->tx_ringbuf) : 0;
    /* Compare the number of packets in the queue */
    return a_packet_count >= b_packet_count ? a : b;
  }

  /* Same neighbor address; simply return the first link */
  return a;
}

/*---------------------------------------------------------------------------*/
/* Returns the next active link after a given ASN, and a backup link (for the same ASN, with Rx flag) */
struct tsch_link *
tsch_schedule_get_next_active_link(struct tsch_asn_t *asn, uint16_t *time_offset,
    struct tsch_link **backup_link)
{
  uint16_t time_to_curr_best = 0;
  struct tsch_link *curr_best = NULL;
  struct tsch_link *curr_backup = NULL; /* Keep a back link in case the current link
  turns out useless when the time comes. For instance, for a Tx-only link, if there is
  no outgoing packet in queue. In that case, run the backup link instead. The backup link
  must have Rx flag set. */
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    /* For each slotframe, look for the earliest occurring link */
    while(sf != NULL) {
#if TSCH_WITH_ALICE
      /* ALICE: rebuild the time-varying unicast schedule when the ASN crosses
       * into a new absolute slotframe number (ASFN). The trigger is the
       * ASN-derived ASFN, a global quantity, so every node transitions at the
       * same ASN. This keeps a sender and its receiver on the same schedule, so
       * their hashed cell positions always match (an earlier per-node trigger
       * based on each node's last link desynchronized them, causing NoAcks). */
      if(sf->handle == ALICE_UNICAST_SF_ID) {
        uint16_t mod = TSCH_ASN_MOD(*asn, sf->size);
        struct tsch_asn_t newasn = *asn;
        TSCH_ASN_DEC(newasn, mod);
        uint16_t asfn = TSCH_ASN_DIVISION(newasn, sf->size);
        if(asfn != alice_curr_asfn) {
          alice_curr_asfn = asfn;
          alice_next_asfn = asfn;
#ifdef A3_MANAGEMENT
          a3_management_recalculate();
#endif
          alice_callback_slotframe_start(asfn, sf->size.val);
        }
      }
#endif /* TSCH_WITH_ALICE */
#if TSCH_WITH_EASE
      /* EASE: same ASN-derived (global) trigger as ALICE; rebuild the two-zone
       * EASE slotframe when the ASN crosses into a new absolute slotframe. */
      if(sf->handle == EASE_SF_ID) {
        uint16_t mod = TSCH_ASN_MOD(*asn, sf->size);
        struct tsch_asn_t newasn = *asn;
        TSCH_ASN_DEC(newasn, mod);
        uint16_t asfn = TSCH_ASN_DIVISION(newasn, sf->size);
        if(asfn != ease_curr_asfn) {
          ease_curr_asfn = asfn;
#ifdef EASE_MANAGEMENT
          ease_cusum_recalculate();   /* CUSUM demand prediction (eqs 2-5) */
          ease_game_recompute();      /* game quotas + token seeding (eqs 6-17) */
#endif
          ease_callback_slotframe_start(asfn, sf->size.val);
        }
      }
#endif /* TSCH_WITH_EASE */
      /* Get timeslot from ASN, given the slotframe length */
      uint16_t timeslot = TSCH_ASN_MOD(*asn, sf->size);
      struct tsch_link *l = list_head(sf->links_list);
      while(l != NULL) {
        uint16_t time_to_timeslot =
          l->timeslot > timeslot ?
          l->timeslot - timeslot :
          sf->size.val + l->timeslot - timeslot;
        if(curr_best == NULL || time_to_timeslot < time_to_curr_best) {
          time_to_curr_best = time_to_timeslot;
          curr_best = l;
          curr_backup = NULL;
        } else if(time_to_timeslot == time_to_curr_best) {
          struct tsch_link *new_best = NULL;
          /* Two links are overlapping, we need to select one of them.
           * By standard: prioritize Tx links first, second by lowest handle */
          if((curr_best->link_options & LINK_OPTION_TX) == (l->link_options & LINK_OPTION_TX)) {
            /* Both or neither links have Tx, select the one with lowest handle */
            if(l->slotframe_handle != curr_best->slotframe_handle) {
              if(l->slotframe_handle < curr_best->slotframe_handle) {
                new_best = l;
              }
#if TSCH_WITH_ALICE
            } else if(l->slotframe_handle == ALICE_UNICAST_SF_ID) {
              /* ALICE links are shared (addr == broadcast), so compare by the
               * RPL neighbor each link serves: prefer the longer queue. */
              if(tsch_queue_nbr_packet_count(tsch_queue_get_nbr(&l->neighbor))
                 > tsch_queue_nbr_packet_count(tsch_queue_get_nbr(&curr_best->neighbor))) {
                new_best = l;
              }
#endif /* TSCH_WITH_ALICE */
            } else {
              /* compare the link against the current best link and return the newly selected one */
              new_best = TSCH_LINK_COMPARATOR(curr_best, l);
            }
          } else {
            /* Select the link that has the Tx option */
            if(l->link_options & LINK_OPTION_TX) {
              new_best = l;
            }
          }

          /* Maintain backup_link */
          /* Check if 'l' best can be used as backup */
          if(new_best != l && (l->link_options & LINK_OPTION_RX)) { /* Does 'l' have Rx flag? */
            if(curr_backup == NULL || l->slotframe_handle < curr_backup->slotframe_handle) {
              curr_backup = l;
            }
          }
          /* Check if curr_best can be used as backup */
          if(new_best != curr_best && (curr_best->link_options & LINK_OPTION_RX)) { /* Does curr_best have Rx flag? */
            if(curr_backup == NULL || curr_best->slotframe_handle < curr_backup->slotframe_handle) {
              curr_backup = curr_best;
            }
          }

          /* Maintain curr_best */
          if(new_best != NULL) {
            curr_best = new_best;
          }
        }

        l = list_item_next(l);
      }
      sf = list_item_next(sf);
    }
    if(time_offset != NULL) {
      *time_offset = time_to_curr_best;
    }
  }
  if(backup_link != NULL) {
    *backup_link = curr_backup;
  }
  return curr_best;
}
/*---------------------------------------------------------------------------*/
/* Module initialization, call only once at startup. Returns 1 is success, 0 if failure. */
int
tsch_schedule_init(void)
{
  if(tsch_get_lock()) {
    memb_init(&link_memb);
    memb_init(&slotframe_memb);
    list_init(slotframe_list);
    tsch_release_lock();
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
/* Create a 6TiSCH minimal schedule */
void
tsch_schedule_create_minimal(void)
{
  struct tsch_slotframe *sf_min;
  /* First, empty current schedule */
  tsch_schedule_remove_all_slotframes();
  /* Build 6TiSCH minimal schedule.
   * We pick a slotframe length of TSCH_SCHEDULE_DEFAULT_LENGTH */
  sf_min = tsch_schedule_add_slotframe(0, TSCH_SCHEDULE_DEFAULT_LENGTH);
  /* Add a single Tx|Rx|Shared slot using broadcast address (i.e. usable for unicast and broadcast).
   * We set the link type to advertising, which is not compliant with 6TiSCH minimal schedule
   * but is required according to 802.15.4e if also used for EB transmission.
   * Timeslot: 0, channel offset: 0. */
  tsch_schedule_add_link(sf_min,
      (LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING),
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      0, 0, 1);
}
/*---------------------------------------------------------------------------*/
struct tsch_slotframe *
tsch_schedule_slotframe_head(void)
{
  return list_head(slotframe_list);
}
/*---------------------------------------------------------------------------*/
struct tsch_slotframe *
tsch_schedule_slotframe_next(struct tsch_slotframe *sf)
{
  return list_item_next(sf);
}
/*---------------------------------------------------------------------------*/
/* Prints out the current schedule (all slotframes and links) */
void
tsch_schedule_print(void)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);

    LOG_PRINT("----- start slotframe list -----\n");

    while(sf != NULL) {
      struct tsch_link *l = list_head(sf->links_list);

      LOG_PRINT("Slotframe Handle %u, size %u\n", sf->handle, sf->size.val);

      while(l != NULL) {
        LOG_PRINT("* Link Options %s, type %s, timeslot %u, " \
                  "channel offset %u, address ",
                  print_link_options(l->link_options),
                  print_link_type(l->link_type),
                  l->timeslot, l->channel_offset);
        LOG_PRINT_LLADDR(&l->addr);
        LOG_PRINT_("\n");
        l = list_item_next(l);
      }

      sf = list_item_next(sf);
    }

    LOG_PRINT("----- end slotframe list -----\n");
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
