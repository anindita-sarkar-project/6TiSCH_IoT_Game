

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

#define EASE_SF              ORCHESTRA_UNICAST_PERIOD
#define EASE_SHARED_LEN      (EASE_SF / 2)               /* SF_s, zone 1 */
#define EASE_DEDICATED_LEN   (EASE_SF - EASE_SHARED_LEN) /* SF_x, zone 2 */

#define EASE_BASELINE_DEDICATED 1

static uint16_t asfn_schedule = 0;

static uint16_t slotframe_handle = 0;
static struct tsch_slotframe *sf_unicast;

static const uint8_t link_option_rx = LINK_OPTION_RX;
static const uint8_t link_option_tx = LINK_OPTION_TX | LINK_OPTION_SHARED;

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

ease_num_dedicated(const linkaddr_t *neighbor)
{
#ifdef EASE_MANAGEMENT
  if(linkaddr_cmp(neighbor, &orchestra_parent_linkaddr)) {
    return ease_p_cells > 2 ? ease_p_cells : 2;
  }
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
static void
ease_schedule_unicast_slotframe(void)
{
  uint8_t i;
  struct tsch_link *l = list_head(sf_unicast->links_list);
  while(l != NULL) {
    tsch_schedule_remove_link_alice(sf_unicast, l);
    l = list_head(sf_unicast->links_list);
  }

  if(is_root() != 1) {
    const linkaddr_t *parent = &orchestra_parent_linkaddr;
    tsch_schedule_add_link_alice(sf_unicast, link_option_tx, LINK_TYPE_NORMAL,
                                 &tsch_broadcast_address, parent,
                                 ease_shared_timeslot(parent), ease_shared_choff(parent));
    for(i = 0; i < ease_num_dedicated(parent); i++) {
      tsch_schedule_add_link_alice(sf_unicast, link_option_tx, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, parent,
                                   ease_dedicated_timeslot(parent, &linkaddr_node_addr, i),
                                   ease_dedicated_choff(parent, &linkaddr_node_addr, i));
    }
    for(i = 0; i < EASE_DOWNLINK_CELLS; i++) {
      tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                   &tsch_broadcast_address, parent,
                                   ease_dedicated_timeslot(&linkaddr_node_addr, parent, i),
                                   ease_dedicated_choff(&linkaddr_node_addr, parent, i));
    }
  }

  if(nbr_table_head(nbr_routes) != NULL) {
    tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                 &tsch_broadcast_address, &linkaddr_node_addr,
                                 ease_shared_timeslot(&linkaddr_node_addr),
                                 ease_shared_choff(&linkaddr_node_addr));
  }
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *child = nbr_table_get_lladdr(nbr_routes, item);
    if(child != NULL) {
      for(i = 0; i < ease_num_dedicated(child); i++) {
        tsch_schedule_add_link_alice(sf_unicast, link_option_rx, LINK_TYPE_NORMAL,
                                     &tsch_broadcast_address, child,
                                     ease_dedicated_timeslot(&linkaddr_node_addr, child, i),
                                     ease_dedicated_choff(&linkaddr_node_addr, child, i));
      }
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
      *ts = ease_shared_timeslot(rx_lladdr);
      *choff = ease_shared_choff(rx_lladdr);
    }
    return 1;
  }

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
