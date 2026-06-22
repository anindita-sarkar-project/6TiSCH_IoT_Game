/*
 * A RPL+TSCH node running the ALICE (and optionally A3) autonomous scheduler.
 * Node 1 acts as the DAG root and UDP server (echoes requests); all other
 * nodes are UDP clients periodically sending to the root.
 */

#include "contiki.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "random.h"

#include <inttypes.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

/* Client send period in milliseconds. Override from the Makefile (e.g.
 * -DAPP_SEND_INTERVAL_MS=200) to load the links and exercise A3 adaptation. */
#ifndef APP_SEND_INTERVAL_MS
#define APP_SEND_INTERVAL_MS 10000
#endif
#define SEND_INTERVAL ((APP_SEND_INTERVAL_MS * CLOCK_SECOND) / 1000)

static struct simple_udp_connection udp_conn;
static uint32_t app_rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "ALICE RPL node");
AUTOSTART_PROCESSES(&node_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                const uint8_t *data, uint16_t datalen)
{
  app_rx_count++;
  if(node_id == 1) {
    /* Server: log and echo back to the sender. */
    LOG_INFO("Received request '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
  } else {
    /* Client: a reply from the server. */
    LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  static uint32_t tx_count;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  if(node_id == 1) {
    /* Coordinator / DAG root / UDP server. */
    NETSTACK_ROUTING.root_start();
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                        UDP_CLIENT_PORT, udp_rx_callback);
    LOG_INFO("Started as ALICE DAG root / UDP server\n");
  } else {
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                        UDP_SERVER_PORT, udp_rx_callback);
    LOG_INFO("Started as ALICE UDP client\n");
  }

  NETSTACK_MAC.on();

  /* Client loop: send to the DAG root periodically. The root just serves. */
  if(node_id != 1) {
    etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

      if(NETSTACK_ROUTING.node_is_reachable()
         && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        if(tx_count % 10 == 0) {
          LOG_INFO("Tx/Rx: %" PRIu32 "/%" PRIu32 "\n", tx_count, app_rx_count);
        }
        snprintf(str, sizeof(str), "hello %" PRIu32, tx_count);
        LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");
        simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
        tx_count++;
      } else {
        LOG_INFO("Not reachable yet\n");
      }

      /* Re-arm with +/-25% jitter, kept non-negative for sub-second intervals. */
      etimer_set(&periodic_timer,
                 (SEND_INTERVAL - SEND_INTERVAL / 4)
                 + (random_rand() % (SEND_INTERVAL / 2 + 1)));
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
