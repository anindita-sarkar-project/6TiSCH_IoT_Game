CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI = ../..

# EASE uses floating-point (CUSUM/game) and larger tables; use the cooja native
# mote (msp430 motes are too small).
PLATFORMS_EXCLUDE = sky z1 native

# TSCH MAC + RPL classic in storing mode (EASE needs knowledge of children)
MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra

# Enable EASE in the TSCH core and select the EASE rule. Rule order matters:
# Orchestra claims a packet with the FIRST matching rule and default_common
# matches everything, so the EASE rule must come before it.
# Slotframe handles: eb=0, ease=1, default_common=2.
CFLAGS += -DTSCH_CONF_WITH_EASE=1
CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_per_time_source,&unicast_per_neighbor_ease,&default_common}"
CFLAGS += -DRPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST

# EASE management: CUSUM traffic prediction (Phase C) and the game-theoretic
# token-bucket allocation (Phase D). On by default; set MAKE_WITH_EASE_MGMT=0
# for the plain two-zone baseline (fixed one dedicated cell per neighbour).
MAKE_WITH_EASE_MGMT ?= 1
ifeq ($(MAKE_WITH_EASE_MGMT),1)
  CFLAGS += -DEASE_MANAGEMENT=1
endif

# Client UDP send period in ms.
# Paper (Section III-C): low load = 1 pkt/slotframe = 1010 ms; high load =
# 10 pkt/slotframe = 101 ms (SF=101, 10 ms/slot).  Default: low-load baseline.
APP_SEND_INTERVAL_MS ?= 1010
CFLAGS += -DAPP_SEND_INTERVAL_MS=$(APP_SEND_INTERVAL_MS)

include $(CONTIKI)/Makefile.include
