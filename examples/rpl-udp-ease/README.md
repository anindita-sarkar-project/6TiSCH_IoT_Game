# EASE: Energy-Aware Autonomous Scheduling for 6TiSCH

This example runs a RPL + TSCH network scheduled with **EASE**, an autonomous
6TiSCH scheduler that combines a two-zone autonomous slotframe, a CUSUM traffic
predictor, and a game-theoretic, duty-cycle-bounded slot allocation enforced by
a token bucket. It implements the scheme in `../../6TiSCH_Game_IoT_Anindita.pdf`.

Node 1 is the DAG root / UDP server; the other nodes are UDP clients that send
periodically to the root.

## The four EASE components and where they live

1. **Two-zone autonomous slotframe** — `os/services/orchestra/orchestra-rule-ease.c`.
   The unicast slotframe SF is split into Zone 1 (shared cells, `[0, SF/2)`) and
   Zone 2 (dedicated cells, `[SF/2, SF)`):
   - Shared cell towards parent `P`: `hash(P + ASFN) mod SF_s`. The parent
     listens there and all its children contend (CSMA) — used for the first
     packet.
   - Dedicated cell for a `(P,C)` pair, packet `i`:
     `SF_s + hash(α·HASH2(P,C) + i + ASFN) mod SF_x` — for extra packets.
   The schedule is time-varying (rebuilt every absolute slotframe number, ASFN).

2. **CUSUM traffic predictor** (paper eqs 2–5) — **inline in
   `os/net/mac/tsch/tsch-schedule.c`** (the same place the A3 reference keeps its
   EWMA). Each ASFN: `z=D−μ`, `S=max(0,S+z)`, `μ=(S>H)?D:ρμ+(1−ρ)D`,
   `c=round(μ)`. `c` is the number of dedicated cells per link. CUSUM detects
   upward traffic shifts faster than EWMA. Per-child state
   (`ease_demand,ease_s,ease_mu,ease_cells`) is in `os/net/ipv6/uip-ds6-nbr.h`;
   demand `D` is counted in `os/net/mac/tsch/tsch-slot-operation.c`.

3. **Game-theoretic allocation** (paper eqs 6–17) — **inline in
   `os/net/mac/tsch/tsch-packet.c`**. The parent has an RX budget `B = round(δ·SF)`
   (δ = `EASE_RDC_PERCENT`). Each child `Ci` has a priority weight `wi`; the
   non-cooperative game has the unique Nash equilibrium `ki* = wi/λ − 1` with
   `λ* = Σwi/(B+M)`. Quotas are rounded/bounded to integers `qi` summing to `B`.

4. **Token-bucket / ACK-withholding** (paper eq 17) — **inline in
   `tsch-packet.c`** (seeding) and `tsch-slot-operation.c` (enforcement). Each
   slotframe `tokensᵢ = max(qi, Pi)` (`Pi` = CUSUM `ease_cells`). On a packet
   from child `i`, the parent ACKs and decrements if `tokensᵢ>0`, else withholds
   the ACK to signal congestion.

Everything is gated behind `TSCH_WITH_EASE` (scheduling) and `EASE_MANAGEMENT`
(CUSUM + game), so default Contiki-NG builds are unaffected. EASE requires RPL
storing mode (it needs to know its children).

## Build

```bash
# Full EASE (two zones + CUSUM + game)
make TARGET=cooja node.cooja
# Plain two-zone baseline (fixed 1 dedicated cell/neighbour, no CUSUM/game)
make TARGET=cooja MAKE_WITH_EASE_MGMT=0 node.cooja
# Load the links (shorter send period) to exercise CUSUM/game
make TARGET=cooja APP_SEND_INTERVAL_MS=40 node.cooja
```

## Run in Cooja (headless)

```bash
cd ../../tools/cooja
CSC=$(pwd)/../../examples/rpl-udp-ease/ease-demo.csc
./gradlew --no-daemon run --args="--no-gui --autostart --contiki=$(pwd)/../.. $CSC"
```

## What to look for in the mote log
- `EASE init: SF 16 (shared 8 + dedicated 8) ...` at startup.
- `EASE CUSUM: child N dedicated cells x->y (mu ...)` — the predictor growing/
  shrinking dedicated cells as offered load changes.
- `EASE game: child N weight w quota q (budget B, M ...)` — the budget shared by
  priority (higher weight → larger quota; `Σqi = B`).
- `EASE withhold ack to N` — congestion signal (fires only when a child's
  offered load exceeds its predicted+quota token budget; quiet when the
  CUSUM/game allocation already matches the load).

## Notes / status
- Validated in Cooja (native mote): network forms, UDP delivered, CUSUM tracks
  load (e.g. `cells 0->1->3->4`), and the game shares the budget by weight
  (e.g. `weight 3 -> quota 2`, `weight 2 -> quota 1`, `Σ = B`). Sender and
  receiver derive matching cell counts.
- The per-link **channel offset** is held constant
  (`ORCHESTRA_CONF_ONE_CHANNEL_OFFSET 1`) because the Cooja native radio does not
  reliably receive cells whose channel offset changes every slotframe; the
  timeslots still vary every ASFN (EASE's contribution).
- EASE uses floating-point and larger tables (designed for M3-class nodes), so
  it does not fit the msp430 emulated motes (sky/z1); use the cooja native mote.
- Not validated against the paper's quantitative PDR/latency results, which need
  a multi-hop testbed-scale evaluation on a faithful radio.
