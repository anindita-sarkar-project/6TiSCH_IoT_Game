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
 * \addtogroup tsch
 * @{
 * \file
 *         Interface between the TSCH core and ALICE (Autonomous Link-based Cell
 *         scheduling), a time-varying extension of Orchestra. The ALICE rule
 *         itself lives in os/services/orchestra; this header declares the hooks
 *         the TSCH core calls into, plus the shared absolute-slotframe-number
 *         (ASFN) bookkeeping. All of it compiles only under TSCH_WITH_ALICE.
 *
 *         Ported to Contiki-NG from the A3 codebase
 *         (https://github.com/skimskimskim/A3), originally:
 *         "ALICE: autonomous link-based cell scheduling for TSCH",
 *         Seohyang Kim, Hyung-Sin Kim, Chongkwon Kim, ACM/IEEE IPSN 2019, and
 *         "A3: Adaptive autonomous allocation of TSCH slots", IPSN 2021.
 */

#ifndef TSCH_ALICE_H_
#define TSCH_ALICE_H_

#include "contiki.h"
#include "net/mac/tsch/tsch-conf.h"

#if TSCH_WITH_ALICE

#include "net/linkaddr.h"
#include "net/mac/tsch/tsch-types.h"

/* Slotframe handle of the ALICE unicast slotframe. Must match the position of
 * the ALICE rule in ORCHESTRA_CONF_RULES. */
#ifdef TSCH_CONF_ALICE_UNICAST_SF_ID
#define ALICE_UNICAST_SF_ID TSCH_CONF_ALICE_UNICAST_SF_ID
#else
#define ALICE_UNICAST_SF_ID 2
#endif


/* Absolute slotframe number (ASFN) bookkeeping, shared between tsch-schedule.c
 * (which advances it as ASN progresses) and the ALICE rule. */
extern uint16_t alice_curr_asfn;  /* ASFN of the slotframe currently running */
extern uint16_t alice_next_asfn;  /* ASFN the unicast slotframe is scheduled for */
extern uint16_t alice_limit_asfn; /* ASFN wrap-around value: 65536 / unicast_period */

/**
 * \brief Returns the current ASFN for a slotframe, derived from the current ASN.
 */
uint16_t alice_tsch_schedule_get_current_asfn(struct tsch_slotframe *sf);

/**
 * \brief Hook implemented by the ALICE orchestra rule. Called once per absolute
 *        slotframe, just before the unicast slotframe is rebuilt for ASFN sfid.
 */
void alice_callback_slotframe_start(uint16_t sfid, uint16_t sfsize);

/**
 * \brief Hook implemented by the ALICE orchestra rule. Re-evaluates the
 *        (timeslot, channel offset) at which a packet to rx_lladdr should be
 *        sent under the current ASFN. Returns 1 if rx_lladdr is still a
 *        scheduled RPL neighbor, 0 otherwise.
 */
int alice_callback_packet_selection(uint16_t *ts, uint16_t *choff,
                                    const linkaddr_t *rx_lladdr);

#ifdef A3_MANAGEMENT
/* A3 adaptive slot allocation on top of ALICE: load counters and hooks. */
#include "net/mac/tsch/a3-management.h"
#endif /* A3_MANAGEMENT */

#endif /* TSCH_WITH_ALICE */
#endif /* TSCH_ALICE_H_ */
/** @} */
