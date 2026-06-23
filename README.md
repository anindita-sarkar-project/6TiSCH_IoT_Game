<img src="https://github.com/contiki-ng/contiki-ng.github.io/blob/master/images/logo/Contiki_logo_2RGB.png" alt="Logo" width="256">

# Contiki-NG: The OS for Next Generation IoT Devices

Find out more:

* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://docs.contiki-ng.org/
* List of releases and changes: https://github.com/contiki-ng/contiki-ng/releases
* Web site: http://contiki-ng.org

Paper → file mapping (every component accounted for)

1. Zone 1  and 2 hash - os/services/orchestra/orchestra-rule-ease.c 
2. CUSUM predictor - os/net/mac/tsch/tsch-schedule.c
3. Game-theoretic allocation - os/net/mac/tsch/tsch-packet.c (ease_game_recompute) 
4. Token bucket - tsch-packet.c (ease_token_consume) + tsch-slot-operation.c (NACK signaling)
5. Demand counting Dₜ - os/net/mac/tsch/tsch-slot-operation.c
6. Algorithm 1 - tsch-schedule.c (lines 707–719)                                            
7. Per-neighbor CUSUM/game state - os/net/ipv6/uip-ds6-nbr.h / .c (ease_demand/s/mu/cells/quota/tokens)       
8. Per-link neighbor binding for time-varying reschedule  - os/net/mac/tsch/tsch-types.h (shared with ALICE)                         
9. Downward traffic (root→device dedicated cells)  - orchestra-rule-ease.c (EASE_DOWNLINK_CELLS, fixed this session)     

Example file present in - examples/rpl-udp-ease
