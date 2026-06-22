/*
 * Project configuration for the EASE example.
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* EASE unicast slotframe handle. Must match the index of the EASE rule in
 * ORCHESTRA_CONF_RULES (eb=0, ease=1, default_common=2). */
#define TSCH_CONF_EASE_SF_ID 1

/* Time-varying schedule: let the link selector re-derive the per-ASFN cell. */
#define TSCH_CONF_WITH_LINK_SELECTOR 1

/* Slotframe length SF (split into shared zone SF/2 + dedicated zone SF/2).
 * Paper (Section H): SF = L = 101, delta = 0.2 -> B = delta*SF = 20 RX cells.
 * Two equal zones: shared [0,50) and dedicated [50,101). */
#define ORCHESTRA_CONF_UNICAST_PERIOD 101

/* Hold the per-link channel OFFSET constant (still full ASN frequency hopping).
 * The Cooja native radio does not reliably receive cells whose channel offset
 * changes every slotframe; the timeslots still vary (EASE's contribution). */
#define ORCHESTRA_CONF_ONE_CHANNEL_OFFSET 1

/* Room for a multi-hop network. With SF=101 each node can hold up to
 * EASE_MAX_DEDICATED=20 dedicated links per neighbor; 128 covers ~5 neighbors. */
#define NBR_TABLE_CONF_MAX_NEIGHBORS 20
#define UIP_CONF_MAX_ROUTES 20
#define TSCH_SCHEDULE_CONF_MAX_LINKS 128

#define LOG_CONF_LEVEL_MAC LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_WARN

#endif /* PROJECT_CONF_H_ */
