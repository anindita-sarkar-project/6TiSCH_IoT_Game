/*
 * Copyright (c) 2024.
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
 * \addtogroup tsch
 * @{
 * \file
 *         Interface between the TSCH core and EASE (Energy-Aware Autonomous
 *         Scheduling). EASE is a two-zone autonomous 6TiSCH scheduler:
 *           - Zone 1: shared cells, one per parent, hashed from EUI64(P)+ASFN;
 *                     a child sends its first packet here with CSMA/CA.
 *           - Zone 2: dedicated cells, hashed from EUI64(P)+EUI64(C)+ASFN, for
 *                     additional packets, sized by a CUSUM traffic predictor and
 *                     a game-theoretic RDC-budget allocation.
 *         The schedule is time-varying (rebuilt every absolute slotframe number,
 *         ASFN = ASN / slotframe length), reusing the TSCH-core machinery shared
 *         with ALICE. The CUSUM prediction lives inline in tsch-schedule.c and
 *         the game / token-bucket enforcement in tsch-packet.c /
 *         tsch-slot-operation.c. The scheduling rule itself is in
 *         os/services/orchestra/orchestra-rule-ease.c.
 *
 *         Reference: "Energy-Aware Autonomous Scheduling for 6TiSCH IoT
 *         networks using a Game-Theoretic approach" (see 6TiSCH_Game_IoT PDF).
 *
 *         All of this compiles only under TSCH_WITH_EASE.
 */

#ifndef TSCH_EASE_H_
#define TSCH_EASE_H_

#include "contiki.h"
#include "net/mac/tsch/tsch-conf.h"

#if TSCH_WITH_EASE

#include "net/linkaddr.h"
#include "net/mac/tsch/tsch-types.h"

/* Slotframe handle of the EASE unicast slotframe. Must match the position of
 * the EASE rule in ORCHESTRA_CONF_RULES. */
#ifdef TSCH_CONF_EASE_SF_ID
#define EASE_SF_ID TSCH_CONF_EASE_SF_ID
#else
#define EASE_SF_ID 2
#endif

/* Number of zones the slotframe is split into. The paper uses two: zone 1 for
 * shared cells, zone 2 for dedicated cells. */
#define EASE_NUM_ZONES 2

/* Direction coefficient alpha used in the dedicated-cell hash to separate the
 * upward (child->parent) and downward (parent->child) schedules. */
#ifdef TSCH_CONF_EASE_ALPHA
#define EASE_ALPHA TSCH_CONF_EASE_ALPHA
#else
#define EASE_ALPHA 3
#endif

/* Radio duty-cycle fraction delta (percent) used for the parent's RX budget
 * B = round(delta/100 * SF) in the game-theoretic allocation. */
#ifdef TSCH_CONF_EASE_RDC_PERCENT
#define EASE_RDC_PERCENT TSCH_CONF_EASE_RDC_PERCENT
#else
#define EASE_RDC_PERCENT 20
#endif

/* Absolute slotframe number the EASE schedule is currently built for (advanced
 * by the TSCH core in tsch_schedule_get_next_active_link). */
extern uint16_t ease_curr_asfn;

/**
 * \brief Hook implemented by the EASE orchestra rule. Called once per absolute
 *        slotframe, to (re)build the two-zone EASE slotframe for ASFN sfid.
 */
void ease_callback_slotframe_start(uint16_t sfid, uint16_t sfsize);

/**
 * \brief Hook implemented by the EASE orchestra rule. Re-derives the
 *        (timeslot, channel offset) at which a packet to rx_lladdr should be
 *        sent under the current ASFN. Returns 1 if rx_lladdr is a scheduled
 *        EASE neighbor (parent or child), 0 otherwise.
 */
int ease_callback_packet_selection(uint16_t *ts, uint16_t *choff,
                                   const linkaddr_t *rx_lladdr);

#ifdef EASE_MANAGEMENT
/* CUSUM traffic predictor (paper eqs 2-5). Each interval (slotframe / ASFN):
 *   z   = D - mu
 *   S   = max(0, S + z)
 *   mu  = (S > H) ? D : rho*mu + (1-rho)*D
 *   c   = round(max(0, mu))   (dedicated cells to allocate next interval)
 * Tunables: threshold H and tracking coefficient rho. */
#ifndef EASE_CUSUM_H
#define EASE_CUSUM_H 2.0
#endif
#ifndef EASE_CUSUM_RHO
#define EASE_CUSUM_RHO 0.5
#endif
/* Upper bound on dedicated cells per link (also bounded by the dedicated zone).
 * Paper (Section H): SF=101, delta=0.2 -> B=20. Unconstrained ki* can reach up
 * to wi/lambda-1 ~ B, so cap at B = round(delta*SF) = 20. */
#ifndef EASE_MAX_DEDICATED
#define EASE_MAX_DEDICATED 20
#endif

/* Extra Rx cells a parent opens for each child beyond its received-demand CUSUM
 * estimate. Breaks the Tx/Rx predictor deadlock (the child sizes Tx from what it
 * sends, the parent sizes Rx from what it receives; without headroom the parent
 * never opens enough Rx cells for the child to ramp up). Bounded by
 * EASE_MAX_DEDICATED and the dedicated zone. */
/* Dedicated downlink (parent->child) cells per child. EASE's dedicated zone is
 * directional: the hash is asymmetric, so the pair (P,C) gets a different slot
 * for the uplink C->P (hash(P,C)) than for the downlink P->C (hash(C,P)). The
 * uplink count is demand-driven (CUSUM); the downlink count is a small fixed
 * constant, which lets the parent (Tx) and child (Rx) agree on exactly the same
 * slots with no headroom. One cell carries the low-load request/response echo;
 * two leaves margin for occasional control traffic (DAO, ND). */
#ifndef EASE_DOWNLINK_CELLS
#define EASE_DOWNLINK_CELLS 2
#endif

/* One extra Rx cell per child beyond the received-demand estimate. Just enough
 * to bootstrap: a fresh child's received-demand CUSUM is 0, so without headroom
 * the parent opens no Rx cell and never hears the child's first dedicated-cell
 * packet (the child can't ramp, the parent can't measure -- deadlock). One cell
 * breaks that, while keeping the parent's total Rx footprint ~1 cell per child
 * so the shared dedicated zone stays uncontended (low ETX -> clean star). */
#ifndef EASE_RX_HEADROOM
#define EASE_RX_HEADROOM 2
#endif

/* Child-side state: this node's own predicted uplink demand toward its parent.
 * Defined in tsch-schedule.c (where the CUSUM runs inline, per the reference
 * placement of A3's EWMA). ease_p_demand is incremented by the TSCH slot
 * operation; ease_p_cells is read by the EASE rule. */
extern uint8_t ease_p_demand;  /* D_t: packets sent to parent this interval */
extern uint8_t ease_p_cells;   /* c:   dedicated uplink cells to keep */

/* Run the CUSUM update for the parent link and every child. Called once per
 * ASFN from tsch_schedule_get_next_active_link before the schedule is rebuilt. */
void ease_cusum_recalculate(void);

/* Game-theoretic RDC-budget allocation (paper eqs 6-17), implemented in
 * tsch-packet.c. Recomputes each child's Nash-equilibrium quota q_i and seeds
 * its per-slotframe token bucket. Called once per ASFN (after the CUSUM). */
void ease_game_recompute(void);

/* Token-bucket congestion control: called from the RX slot when a data frame is
 * received from a child. Returns 1 if a token was available (consume it and ACK
 * normally) or 0 if the child's tokens are exhausted (the parent withholds the
 * ACK as a congestion signal, paper eq 17). */
int ease_token_consume(const linkaddr_t *child);
#endif /* EASE_MANAGEMENT */

#endif /* TSCH_WITH_EASE */
#endif /* TSCH_EASE_H_ */
/** @} */
