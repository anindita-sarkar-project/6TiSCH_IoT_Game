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
 * \addtogroup tsch
 * @{
 * \file
 *         A3: Adaptive autonomous allocation of TSCH slots. Layered on top of
 *         ALICE, A3 estimates the traffic load on each link (without control
 *         messages) from the Tx/Rx outcomes observed in its cells, and doubles
 *         or halves the number of cells per link accordingly.
 *
 *         This header declares the load counters maintained by the TSCH core
 *         (per child neighbor in struct uip_ds6_nbr, and the globals below for
 *         the preferred parent) and the hooks implemented in a3-management.c.
 *         Everything is gated on A3_MANAGEMENT.
 *
 *         Ported to Contiki-NG from https://github.com/skimskimskim/A3. Based on
 *         "A3: Adaptive autonomous allocation of TSCH slots", Seohyang Kim,
 *         Hyung-Sin Kim, Chongkwon Kim, ACM/IEEE IPSN 2021.
 */

#ifndef A3_MANAGEMENT_H_
#define A3_MANAGEMENT_H_

#include "contiki.h"

#ifdef A3_MANAGEMENT

#include "net/linkaddr.h"

/* Maximum number of cells A3 may allocate to a single link (per direction).
 * Must be a power of two (2, 4 or 8). */
#ifndef A3_UNICAST_MAX_REGION
#define A3_UNICAST_MAX_REGION 4
#endif

/* Load state for the preferred parent (the per-child equivalents live in
 * struct uip_ds6_nbr). "_cur" is the current number of allocated cells;
 * "_ewma" are exponentially weighted moving averages of the per-slotframe
 * attempt/success ratios; "_num_*pkt_*" are the raw counts accumulated over the
 * current slotframe and reset by a3_management_recalculate(). */
extern uint8_t p_num_rx_cur;
extern uint8_t p_num_tx_cur;
extern double p_num_tx_ewma;
extern double p_num_rx_ewma;
extern double p_tx_collision_ewma;
extern double p_tx_attempt_ewma;
extern double p_rx_attempt_ewma;
extern uint8_t p_num_txpkt_success;
extern uint8_t p_num_txpkt_collision;
extern uint8_t p_num_rxpkt_success;
extern uint8_t p_num_rxpkt_collision;
extern uint8_t p_num_rxpkt_idle;
extern uint8_t p_num_rxpkt_unscheduled;
extern uint8_t p_num_rxpkt_others;

/**
 * \brief Recompute, for the preferred parent and every child, the number of
 *        cells per link from the load measured over the slotframe that just
 *        ended, then reset the per-slotframe counters. Called once per absolute
 *        slotframe from the TSCH core.
 */
void a3_management_recalculate(void);

/**
 * \brief Reset the preferred-parent load state. Call on a time-source change.
 */
void a3_management_new_time_source(void);

/**
 * \brief Reset the load state of a newly added child neighbor.
 */
void a3_management_child_added(const linkaddr_t *addr);

/**
 * \brief Record the outcome of a unicast Tx in an ALICE cell. Attributes the
 *        success/collision to the preferred parent or the destination child.
 * \param dest  The link-layer destination of the transmitted packet
 * \param tx_ok Nonzero if the transmission was acknowledged (MAC_TX_OK)
 */
void a3_management_count_tx(const linkaddr_t *dest, int tx_ok);

/**
 * \brief Record the outcome of an ALICE Rx cell.
 * \param cell_neighbor The neighbor this Rx cell was scheduled for
 * \param src           The actual frame source, or NULL if none decoded
 * \param rx_result     0 = idle, 1 = success, 2 = others, 3 = collision
 */
void a3_management_count_rx(const linkaddr_t *cell_neighbor,
                            const linkaddr_t *src, uint8_t rx_result);

#endif /* A3_MANAGEMENT */
#endif /* A3_MANAGEMENT_H_ */
/** @} */
