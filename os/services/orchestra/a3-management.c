/*
 * Copyright (c) 2021, the A3 authors.
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
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS
 * BE LIABLE FOR ANY DAMAGES ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *         A3: adaptive autonomous allocation of TSCH slots on top of ALICE.
 *
 *         Each absolute slotframe, a3_management_recalculate() turns the raw
 *         Tx/Rx outcome counts gathered by the TSCH core (per child in struct
 *         uip_ds6_nbr, per parent in the p_* globals here) into per-link cell
 *         counts: an EWMA estimates the channel-attempt and success ratios, and
 *         the number of cells per link is doubled (high load) or halved (low
 *         load / collisions), bounded by [1, A3_UNICAST_MAX_REGION].
 *
 *         Ported to Contiki-NG from https://github.com/skimskimskim/A3.
 */

#include "contiki.h"

#ifdef A3_MANAGEMENT

#include "orchestra.h"
#include "net/mac/tsch/a3-management.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/linkaddr.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "A3"
#define LOG_LEVEL LOG_LEVEL_MAC

/* Preferred-parent load state (per-child equivalents live in uip_ds6_nbr). */
uint8_t p_num_rx_cur = A3_MANAGEMENT;
uint8_t p_num_tx_cur = A3_MANAGEMENT;
double p_num_tx_ewma = 0.4;
double p_num_rx_ewma = 0.4;
double p_tx_collision_ewma = 0.2;
double p_tx_attempt_ewma = 0.5;
double p_rx_attempt_ewma = 0.5;
uint8_t p_num_txpkt_success = 0;
uint8_t p_num_txpkt_collision = 0;
uint8_t p_num_rxpkt_success = 0;
uint8_t p_num_rxpkt_collision = 0;
uint8_t p_num_rxpkt_idle = 0;
uint8_t p_num_rxpkt_unscheduled = 0;
uint8_t p_num_rxpkt_others = 0;

/* Adaptation thresholds (see the A3 paper). */
#define A3_TX_INCREASE_THRESH 0.75
#define A3_TX_DECREASE_THRESH 0.36
#define A3_RX_INCREASE_THRESH 0.65
#define A3_RX_DECREASE_THRESH 0.29
#define A3_MAX_COLLISION_RATIO 0.5 /* halve Tx cells above this collision prob. */

/*---------------------------------------------------------------------------*/
void
a3_management_new_time_source(void)
{
  p_num_rx_cur = A3_MANAGEMENT;
  p_num_tx_cur = A3_MANAGEMENT;
  p_num_tx_ewma = 0.4;
  p_num_rx_ewma = 0.4;
  p_tx_collision_ewma = 0.2;
  p_tx_attempt_ewma = 0.5;
  p_rx_attempt_ewma = 0.5;
  p_num_txpkt_success = 0;
  p_num_txpkt_collision = 0;
  p_num_rxpkt_success = 0;
  p_num_rxpkt_collision = 0;
  p_num_rxpkt_idle = 0;
  p_num_rxpkt_unscheduled = 0;
  p_num_rxpkt_others = 0;
}
/*---------------------------------------------------------------------------*/
void
a3_management_child_added(const linkaddr_t *addr)
{
  if(addr == NULL) {
    return;
  }
  uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)addr);
  if(it != NULL) {
    it->num_rx_cur = A3_MANAGEMENT;
    it->num_tx_cur = A3_MANAGEMENT;
    it->num_tx_ewma = 0.4;
    it->num_rx_ewma = 0.4;
    it->c_tx_collision_ewma = 0.2;
    it->c_tx_attempt_ewma = 0.5;
    it->c_rx_attempt_ewma = 0.5;
    it->c_num_txpkt_success = 0;
    it->c_num_txpkt_collision = 0;
    it->c_num_rxpkt_success = 0;
    it->c_num_rxpkt_collision = 0;
    it->c_num_rxpkt_idle = 0;
    it->c_num_rxpkt_others = 0;
    it->c_num_rxpkt_unscheduled = 0;
  }
}
/*---------------------------------------------------------------------------*/
void
a3_management_count_tx(const linkaddr_t *dest, int tx_ok)
{
  if(dest == NULL || linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  if(linkaddr_cmp(dest, &orchestra_parent_linkaddr)) {
    if(tx_ok) {
      p_num_txpkt_success++;
    } else {
      p_num_txpkt_collision++;
    }
    return;
  }
  if(nbr_table_get_from_lladdr(nbr_routes, dest) != NULL) {
    uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)dest);
    if(it != NULL) {
      if(tx_ok) {
        it->c_num_txpkt_success++;
      } else {
        it->c_num_txpkt_collision++;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
a3_management_count_rx(const linkaddr_t *cell_neighbor,
                       const linkaddr_t *src, uint8_t rx_result)
{
  if(cell_neighbor == NULL || linkaddr_cmp(cell_neighbor, &linkaddr_null)) {
    return;
  }
  /* A success from a neighbor other than the cell owner counts as "others". */
  if(rx_result == 1 && src != NULL && !linkaddr_cmp(src, cell_neighbor)) {
    rx_result = 2;
  }

  uint8_t *success, *collision, *idle, *others;
  if(linkaddr_cmp(cell_neighbor, &orchestra_parent_linkaddr)) {
    success = &p_num_rxpkt_success;
    collision = &p_num_rxpkt_collision;
    idle = &p_num_rxpkt_idle;
    others = &p_num_rxpkt_others;
  } else if(nbr_table_get_from_lladdr(nbr_routes, cell_neighbor) != NULL) {
    uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)cell_neighbor);
    if(it == NULL) {
      return;
    }
    success = &it->c_num_rxpkt_success;
    collision = &it->c_num_rxpkt_collision;
    idle = &it->c_num_rxpkt_idle;
    others = &it->c_num_rxpkt_others;
  } else {
    return;
  }

  switch(rx_result) {
  case 0: (*idle)++; break;
  case 1: (*success)++; break;
  case 2: (*others)++; break;
  case 3: (*collision)++; break;
  default: break;
  }
}
/*---------------------------------------------------------------------------*/
/* Update Tx/Rx attempt EWMAs and the cell count for one link, given the raw
 * counts for the slotframe that just ended. Operates on pointers so it serves
 * both the parent globals and per-child fields. */
static void
a3_adapt_link(double alpha, double alpha_plus,
              uint8_t *num_tx_cur, uint8_t *num_rx_cur,
              double *num_tx_ewma, double *tx_attempt_ewma, double *rx_attempt_ewma,
              uint8_t txpkt_success, uint8_t txpkt_collision,
              uint8_t rxpkt_success, uint8_t rxpkt_collision,
              uint8_t rxpkt_idle, uint8_t rxpkt_others)
{
  double newval;
  double dynamic_alpha;
  uint8_t sumtx, sumrx, diffrx;

  /* --- Tx side --- */
  sumtx = txpkt_success + txpkt_collision;
  newval = (double)sumtx / (double)(*num_tx_cur);
  if(newval > 1) {
    newval = 1;
  }
  *tx_attempt_ewma = (1 - alpha) * (*tx_attempt_ewma) + alpha * newval;

  newval = (double)txpkt_success / (double)(*num_tx_cur);
  if(newval > 1) {
    newval = 1;
  }
  *num_tx_ewma = (1 - alpha) * (*num_tx_ewma) + alpha * newval;

  /* --- Rx side --- */
  sumrx = rxpkt_collision + rxpkt_success + rxpkt_idle + rxpkt_others;
  diffrx = (*num_rx_cur > sumrx) ? (*num_rx_cur - sumrx) : 0;
  sumrx += diffrx; /* account for cells that saw no activity at all */

  newval = ((double)rxpkt_success + (*rx_attempt_ewma) * (double)(rxpkt_collision + diffrx))
           / (double)(sumrx == 0 ? 1 : sumrx);
  if(newval > 1) {
    newval = 1;
  }
  dynamic_alpha = alpha;
  if(newval > *rx_attempt_ewma) {
    dynamic_alpha += alpha_plus;
  }
  *rx_attempt_ewma = (1 - dynamic_alpha) * (*rx_attempt_ewma) + dynamic_alpha * newval;

  /* --- Tx cell count adaptation --- */
  int tx_changed = 0;
  if((*tx_attempt_ewma) > 0
     && ((*tx_attempt_ewma) - (*num_tx_ewma)) / (*tx_attempt_ewma) > A3_MAX_COLLISION_RATIO
     && *num_tx_cur > 1) {
    *num_tx_cur /= 2;
    tx_changed = 1;
    *tx_attempt_ewma = 0.5;
    *num_tx_ewma = 0.4;
  }
  if(!tx_changed) {
    if(*tx_attempt_ewma > A3_TX_INCREASE_THRESH && *num_tx_cur < A3_UNICAST_MAX_REGION) {
      *num_tx_cur *= 2;
      *tx_attempt_ewma /= 2;
    } else if(*tx_attempt_ewma < A3_TX_DECREASE_THRESH && *num_tx_cur > 1) {
      *num_tx_cur /= 2;
      *tx_attempt_ewma *= 2;
    }
  }

  /* --- Rx cell count adaptation --- */
  if(*rx_attempt_ewma > A3_RX_INCREASE_THRESH && *num_rx_cur < A3_UNICAST_MAX_REGION) {
    *num_rx_cur *= 2;
    *rx_attempt_ewma /= 2;
  } else if(*rx_attempt_ewma < A3_RX_DECREASE_THRESH && *num_rx_cur > 1) {
    *num_rx_cur /= 2;
    *rx_attempt_ewma *= 2;
  }
}
/*---------------------------------------------------------------------------*/
void
a3_management_recalculate(void)
{
  /* EWMA weight grows slowly with slotframe length, capped at 0.15. */
  double alpha = 0.02 + ((double)ORCHESTRA_UNICAST_PERIOD * 0.001);
  if(alpha > 0.15) {
    alpha = 0.15;
  }
  double alpha_plus = alpha * 0.2;

  /* Preferred parent. */
  uint8_t old_p_tx = p_num_tx_cur, old_p_rx = p_num_rx_cur;
  a3_adapt_link(alpha, alpha_plus, &p_num_tx_cur, &p_num_rx_cur,
                &p_num_tx_ewma, &p_tx_attempt_ewma, &p_rx_attempt_ewma,
                p_num_txpkt_success, p_num_txpkt_collision,
                p_num_rxpkt_success, p_num_rxpkt_collision,
                p_num_rxpkt_idle, p_num_rxpkt_others);
  if(p_num_tx_cur != old_p_tx || p_num_rx_cur != old_p_rx) {
    LOG_INFO("A3 adapt: parent cells tx %u->%u rx %u->%u\n",
             old_p_tx, p_num_tx_cur, old_p_rx, p_num_rx_cur);
  }
  p_num_txpkt_success = 0;
  p_num_txpkt_collision = 0;
  p_num_rxpkt_success = 0;
  p_num_rxpkt_collision = 0;
  p_num_rxpkt_idle = 0;
  p_num_rxpkt_unscheduled = 0;
  p_num_rxpkt_others = 0;

  /* Each child (route next hop). */
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *addr = nbr_table_get_lladdr(nbr_routes, item);
    if(addr != NULL) {
      uip_ds6_nbr_t *it = uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)addr);
      if(it != NULL) {
        uint8_t old_c_tx = it->num_tx_cur, old_c_rx = it->num_rx_cur;
        a3_adapt_link(alpha, alpha_plus, &it->num_tx_cur, &it->num_rx_cur,
                      &it->num_tx_ewma, &it->c_tx_attempt_ewma, &it->c_rx_attempt_ewma,
                      it->c_num_txpkt_success, it->c_num_txpkt_collision,
                      it->c_num_rxpkt_success, it->c_num_rxpkt_collision,
                      it->c_num_rxpkt_idle, it->c_num_rxpkt_others);
        if(it->num_tx_cur != old_c_tx || it->num_rx_cur != old_c_rx) {
          LOG_INFO("A3 adapt: child %u cells tx %u->%u rx %u->%u\n",
                   addr->u8[LINKADDR_SIZE - 1], old_c_tx, it->num_tx_cur,
                   old_c_rx, it->num_rx_cur);
        }
        it->c_num_txpkt_success = 0;
        it->c_num_txpkt_collision = 0;
        it->c_num_rxpkt_success = 0;
        it->c_num_rxpkt_collision = 0;
        it->c_num_rxpkt_idle = 0;
        it->c_num_rxpkt_unscheduled = 0;
        it->c_num_rxpkt_others = 0;
      }
    }
    item = nbr_table_next(nbr_routes, item);
  }
}
/*---------------------------------------------------------------------------*/
#endif /* A3_MANAGEMENT */
