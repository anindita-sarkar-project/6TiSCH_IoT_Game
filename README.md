<img src="https://github.com/contiki-ng/contiki-ng.github.io/blob/master/images/logo/Contiki_logo_2RGB.png" alt="Logo" width="256">

# Contiki-NG: The OS for Next Generation IoT Devices

[![Github Actions](https://github.com/contiki-ng/contiki-ng/workflows/CI/badge.svg?branch=develop)](https://github.com/contiki-ng/contiki-ng/actions)
[![Documentation Status](https://readthedocs.org/projects/contiki-ng/badge/?version=master)](https://contiki-ng.readthedocs.io/en/master/?badge=master)
[![license](https://img.shields.io/badge/license-3--clause%20bsd-brightgreen.svg)](https://github.com/contiki-ng/contiki-ng/blob/master/LICENSE.md)
[![Latest release](https://img.shields.io/github/release/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/releases/latest)
[![GitHub Release Date](https://img.shields.io/github/release-date/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/releases/latest)
[![Last commit](https://img.shields.io/github/last-commit/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/commit/HEAD)

[![Stack Overflow Tag](https://img.shields.io/badge/Stack%20Overflow%20tag-Contiki--NG-blue?logo=stackoverflow)](https://stackoverflow.com/questions/tagged/contiki-ng)
[![Gitter](https://img.shields.io/badge/Gitter-Contiki--NG-blue?logo=gitter)](https://gitter.im/contiki-ng)
[![Twitter](https://img.shields.io/badge/Twitter-%40contiki__ng-blue?logo=twitter)](https://twitter.com/contiki_ng)

Contiki-NG is an open-source, cross-platform operating system for Next-Generation IoT devices. It focuses on dependable (secure and reliable) low-power communication and standard protocols, such as IPv6/6LoWPAN, 6TiSCH, RPL, and CoAP. Contiki-NG comes with extensive documentation, tutorials, a roadmap, release cycle, and well-defined development flow for smooth integration of community contributions.

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
