/*
 * Project configuration for the ALICE + A3 example.
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* ALICE unicast slotframe handle. Must match the index of the ALICE rule in
 * ORCHESTRA_CONF_RULES (eb=0, alice=1, default_common=2). The ALICE rule must
 * precede default_common, which otherwise claims every unicast packet. */
#define TSCH_CONF_ALICE_UNICAST_SF_ID 1

/* ALICE re-derives the unicast schedule on a per-packet basis, so let the link
 * selector consult the per-packet timeslot/channel offset. */
#define TSCH_CONF_WITH_LINK_SELECTOR 1

/* Use a single channel offset for the (time-varying) unicast cells. The
 * timeslot still varies every slotframe (ALICE's contribution) and the radio
 * still hops in frequency via the ASN; only the per-link channel OFFSET is held
 * constant. This is required under the Cooja native mote, whose radio model
 * does not reliably receive cells whose channel offset changes every slotframe.
 * Set to 0 to use ALICE's per-link hashed channel offsets (works on emulated/
 * real radios, e.g. MSPSim/hardware). */
#define ORCHESTRA_CONF_ONE_CHANNEL_OFFSET 1

/* Length of the unicast slotframe (number of cells). With A3 enabled this is
 * the total budget shared across links; it must be a multiple of
 * A3_UNICAST_MAX_REGION. 31 is prime and works well without A3. */
#ifdef A3_MANAGEMENT
#define ORCHESTRA_CONF_UNICAST_PERIOD 32
#else
#define ORCHESTRA_CONF_UNICAST_PERIOD 31
#endif

/* Room for a small multi-hop network's neighbors and routes. */
#define NBR_TABLE_CONF_MAX_NEIGHBORS 16
#define UIP_CONF_MAX_ROUTES 16

/* Enough link slots for ALICE to keep several cells per neighbor. */
#define TSCH_SCHEDULE_CONF_MAX_LINKS 64

/* Logging: show MAC-level info so ALICE/A3 messages are visible. */
#define LOG_CONF_LEVEL_MAC LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_WARN

#endif /* PROJECT_CONF_H_ */
