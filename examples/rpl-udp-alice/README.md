# ALICE + A3 autonomous TSCH scheduling (Contiki-NG)

This example runs a RPL + TSCH network scheduled with **ALICE** (Autonomous
Link-based Cell scheduling) and, optionally, **A3** (adaptive autonomous
allocation of TSCH slots) on top of it. Both are ported to Contiki-NG from the
A3 codebase (https://github.com/skimskimskim/A3); see `../../A3.pdf` for the
paper.

* **ALICE** re-derives each link's unicast cell (timeslot + channel offset) from
  `hash(link, absolute_slotframe_number)`, so the schedule varies over time and
  avoids persistent collisions. It is a new Orchestra rule
  (`unicast_per_neighbor_link_based_alice`).
* **A3** estimates the traffic load on each link from the Tx/Rx outcomes it
  observes in its own cells (no control messages) and doubles/halves the number
  of cells per link accordingly.

Node 1 is the DAG root and UDP server; all other nodes are UDP clients that
periodically send a request to the root, which echoes it back.

## How it is wired in

ALICE/A3 are gated behind compile-time flags, so default Contiki-NG builds are
unaffected:

* `TSCH_CONF_WITH_ALICE=1` enables the ALICE hooks in the TSCH core
  (`os/net/mac/tsch/tsch-alice.h`, additions to `tsch-schedule.c`,
  `tsch-queue.c`, `tsch-slot-operation.c`, `tsch-types.h`).
* Selecting `&unicast_per_neighbor_link_based_alice` in `ORCHESTRA_CONF_RULES`
  activates the rule (slotframe handle 2 here).
* `A3_MANAGEMENT=1` additionally compiles in the load-estimation layer
  (`os/services/orchestra/a3-management.c`, per-neighbor counters in
  `struct uip_ds6_nbr`).

ALICE requires RPL **storing** mode (it needs to know the node's children); the
Makefile selects `MAKE_ROUTING_RPL_CLASSIC` with
`RPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST`.

## Build

```bash
# ALICE only
make TARGET=cooja node.cooja

# ALICE + A3 (adaptive), max 4 cells per link
make TARGET=cooja MAKE_WITH_A3=1 node.cooja
# or change the cap:
make TARGET=cooja MAKE_WITH_A3=1 A3_UNICAST_MAX_REGION=8 node.cooja
```

## Run in Cooja (headless)

Cooja's mote firmware is built by the `.csc` make command and picks up
`MAKE_WITH_A3` from the environment. Launch through Cooja's gradle `run` task so
the required JVM flags (`--enable-preview`) are set:

```bash
cd ../../tools/cooja
CSC=$(pwd)/../../examples/rpl-udp-alice/rpl-udp-alice.csc

# ALICE only
MAKE_WITH_A3=0 ./gradlew --no-daemon run \
    --args="--no-gui --autostart --contiki=$(pwd)/../.. $CSC"

# ALICE + A3 (adaptive)
MAKE_WITH_A3=1 ./gradlew --no-daemon run \
    --args="--no-gui --autostart --contiki=$(pwd)/../.. $CSC"
```

`--no-daemon` matters: a reused gradle daemon keeps its original environment, so
without it the `MAKE_WITH_A3` change would not reach Cooja's firmware build. To
switch modes you can also just rebuild the firmware first
(`make TARGET=cooja MAKE_WITH_A3=1 node.cooja`) and run the simulation.

(If you run a prebuilt `cooja.jar` directly instead, pass
`java --enable-preview --enable-native-access=ALL-UNNAMED -jar ...`.)

The embedded test script ends successfully ("ALICE network operational") once
the root has received 5 requests, confirming TSCH association, RPL DAG
formation, and end-to-end UDP.

## What to look for

* `ALICE unicast period ... limit ASFN ...` at startup (rule init).
* With A3: `A3 enabled: max region ... CID period ...`, and `A3 adapt: ... cells
  tx X->Y rx ...` lines whenever a link's cell count changes.

To load the links, lower the client send period, e.g.
`make TARGET=cooja MAKE_WITH_A3=1 APP_SEND_INTERVAL_MS=200 node.cooja`.

### Validation status (verified against the A3 paper)

Checked against "A3: Adaptive Autonomous Allocation of TSCH Slots" (IPSN'21,
`../../A3.pdf`):

* **Load-estimation algorithm and parameters: faithfully implemented.** The
  EWMA update, the increase/decrease thresholds (Tx tau_H=0.75 / tau_L=0.36, Rx
  tau'_H=0.65 / tau'_L=0.29), the "halve when collision ratio > 50%" rule
  (a <= 2*a_succ), the N_z=4 zones and the SHIFT/zone hashing all match the
  paper (see `a3-management.c` and `get_node_timeslot()` in the ALICE rule).

* **A3 adaptation is demonstrated to work** (with `ALICE_STATIC_ASFN=1`, see
  below): under load the per-link Tx attempt EWMA rises past the 0.75 threshold
  and A3 grows the cell count (`A3 adapt: parent cells tx 1->2`), shrinking it
  again when the load drops — exactly the intended behaviour. It does *not*
  over-allocate when idle.

Two real bugs were found and fixed along the way:

1. **Orchestra rule ordering.** `default_common` matches every packet and the
   dispatcher takes the first match, so the ALICE rule must precede it
   (`{eb, alice, default_common}`); otherwise ALICE/A3 cells are never used at
   all. Fixed in this example's `Makefile`/`project-conf.h`.
2. **Zone/offset hash correlation.** The A3 cell position is `zone*CID_PERIOD +
   offset`, but the upstream code derived both `zone` (`hash % N_z`) and `offset`
   (`hash % CID_PERIOD`) from the *same* hash value, so `zone == offset % N_z`
   and every cell collapsed onto only `CID_PERIOD` of the slotframe's timeslots,
   making a link's up/down cells collide ~1 in 8. Decorrelated with a salt in
   `get_node_timeslot()` (matches the paper's intent that zone and intra-zone
   offset be independent); this spread the cells over the whole slotframe and
   removed the up/down collisions.

### Time-varying schedule and the channel-offset caveat

ALICE re-derives every cell's **timeslot** each slotframe from
`hash(link, ASFN)` — its core contribution — and this works: with the default
config, running `MAKE_WITH_A3=1 APP_SEND_INTERVAL_MS=40` produces a stream of
`A3 adapt: ... cells tx 1->2` events on parent and child links as load varies.

The one caveat is the per-link **channel offset**. ALICE normally also hashes a
per-link channel offset that changes every slotframe; under the **Cooja native
mote**, the radio model does not reliably receive cells whose channel offset
changes that fast (even though sender and receiver compute the same offset), so
A3 never sees the load. Holding the channel offset constant
(`ORCHESTRA_CONF_ONE_CHANNEL_OFFSET 1`, set in `project-conf.h`) fixes this while
keeping the time-varying timeslots and full ASN-based frequency hopping — and A3
then adapts correctly. This is a Cooja-native-radio limitation, not an ALICE/A3
logic error (the radio still hops; only the per-link offset is fixed). Set it
back to `0` to use ALICE's hashed channel offsets on emulated/real radios.

To watch A3 grow cells under load:

```bash
make TARGET=cooja MAKE_WITH_A3=1 APP_SEND_INTERVAL_MS=40 node.cooja
# ...then run a multi-node .csc and grep the mote log for "A3 adapt"
```

`ALICE_STATIC_ASFN=1` additionally freezes the timeslots (non-time-varying); it
is only a debugging fallback.

### Not yet validated against the paper's PDR results

The paper's headline numbers (2-6x PDR, lower latency) come from a 62-node M3
testbed (FIT IoT-LAB) with real radios, multi-hop forwarding and dynamic
topology. This example demonstrates that **the A3 control loop runs and adapts
cell counts to load**, but it does **not** reproduce the paper's PDR trends:
the test here is a 2-3 node, near-lossless Cooja network where there is little
multi-hop load differentiation for A3 to exploit, and A3's floating-point EWMA
does not fit the only emulated motes Cooja offers (msp430). Reproducing the
paper would require a multi-hop scenario (tens of nodes), a slotframe-length /
traffic sweep, and an A3-on vs A3-off PDR comparison on a faithful radio
(M3-class hardware or emulation).
