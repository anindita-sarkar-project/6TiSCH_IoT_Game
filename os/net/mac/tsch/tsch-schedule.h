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
 *	TSCH scheduling engine
*/

#ifndef TSCH_SCHEDULE_H_
#define TSCH_SCHEDULE_H_

/********** Includes **********/

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/tsch/tsch-conf.h"
#include "net/mac/tsch/tsch-types.h"

/********** Functions *********/

/**
 * \brief Module initialization, call only once at init
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_init(void);
/**
 * \brief Create a 6tisch minimal schedule with length TSCH_SCHEDULE_DEFAULT_LENGTH
 */
void tsch_schedule_create_minimal(void);
/**
 * \brief Prints out the current schedule (all slotframes and links)
 */
void tsch_schedule_print(void);


/**
 * \brief Creates and adds a new slotframe
 * \param handle the slotframe handle
 * \param size the slotframe size
 * \return the new slotframe, NULL if failure
 */
struct tsch_slotframe *tsch_schedule_add_slotframe(uint16_t handle, uint16_t size);

/**
 * \brief Looks up a slotframe by handle
 * \param handle the slotframe handle
 * \return the slotframe with required handle, if any. NULL otherwise.
 */
struct tsch_slotframe *tsch_schedule_get_slotframe_by_handle(uint16_t handle);

/**
 * \brief Removes a slotframe
 * \param slotframe The slotframe to be removed
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_slotframe(struct tsch_slotframe *slotframe);

/**
 * \brief Removes all slotframes, resulting in an empty schedule
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_all_slotframes(void);

struct tsch_link *tsch_schedule_add_link(struct tsch_slotframe *slotframe,
                                         uint8_t link_options, enum link_type link_type, const linkaddr_t *address,
                                         uint16_t timeslot, uint16_t channel_offset, uint8_t do_remove);

struct tsch_link *tsch_schedule_get_link_by_handle(uint16_t handle);


struct tsch_link *tsch_schedule_get_link_by_offsets(struct tsch_slotframe *slotframe,
                                                    uint16_t timeslot, uint16_t channel_offset);


struct tsch_link *tsch_schedule_get_link_by_timeslot(struct tsch_slotframe *slotframe,
                                                     uint16_t timeslot);


int tsch_schedule_remove_link(struct tsch_slotframe *slotframe, struct tsch_link *l);

int tsch_schedule_remove_link_by_offsets(struct tsch_slotframe *slotframe,
                                         uint16_t timeslot, uint16_t channel_offset);


struct tsch_link * tsch_schedule_get_next_active_link(struct tsch_asn_t *asn, uint16_t *time_offset,
    struct tsch_link **backup_link);


struct tsch_slotframe *tsch_schedule_slotframe_head(void);


struct tsch_slotframe *tsch_schedule_slotframe_next(struct tsch_slotframe *sf);

#if TSCH_WITH_AUTONOMOUS

uint16_t real_hash5(uint32_t value, uint16_t mod);


struct tsch_link *tsch_schedule_add_link_alice(struct tsch_slotframe *slotframe,
                                               uint8_t link_options, enum link_type link_type,
                                               const linkaddr_t *address, const linkaddr_t *neighbor,
                                               uint16_t timeslot, uint16_t channel_offset);


struct tsch_link *tsch_schedule_set_link_option_by_offsets(struct tsch_slotframe *slotframe,
                                                           uint16_t timeslot, uint16_t channel_offset,
                                                           uint8_t *link_options);


int tsch_schedule_remove_link_alice(struct tsch_slotframe *slotframe, struct tsch_link *l);
#endif /* TSCH_WITH_AUTONOMOUS */

#endif /* TSCH_SCHEDULE_H_ */
/** @} */
