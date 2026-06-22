CONTIKI_PROJECT = node
all: $(CONTIKI_PROJECT)

CONTIKI = ../..

# A3 uses floating-point EWMA and larger tables (designed for M3-class nodes),
# so it does not fit the msp430 motes (sky/z1, ~8-10 KB RAM). Use the cooja
# native mote.
PLATFORMS_EXCLUDE = sky z1 native

# TSCH MAC + RPL classic in storing mode (ALICE needs knowledge of children)
MAKE_MAC = MAKE_MAC_TSCH
MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/orchestra

# Enable ALICE time-varying scheduling in the TSCH core, and select the ALICE
# unicast rule. Order matters: Orchestra claims a packet with the FIRST matching
# rule, and default_common matches everything, so the ALICE unicast rule must
# come before it. Slotframe handles: eb=0, alice=1, default_common=2.
CFLAGS += -DTSCH_CONF_WITH_ALICE=1
CFLAGS += -DORCHESTRA_CONF_RULES="{&eb_per_time_source,&unicast_per_neighbor_link_based_alice,&default_common}"
CFLAGS += -DRPL_CONF_MOP=RPL_MOP_STORING_NO_MULTICAST

# Enable the A3 adaptive slot-allocation layer with: make MAKE_WITH_A3=1
MAKE_WITH_A3 ?= 0
# Maximum number of cells A3 may allocate per link (power of two: 2, 4 or 8)
A3_UNICAST_MAX_REGION ?= 4
ifeq ($(MAKE_WITH_A3),1)
  CFLAGS += -DA3_MANAGEMENT=1
  CFLAGS += -DA3_UNICAST_MAX_REGION=$(A3_UNICAST_MAX_REGION)
endif

# Client UDP send period in ms (lower it to load the links and exercise A3)
APP_SEND_INTERVAL_MS ?= 10000
CFLAGS += -DAPP_SEND_INTERVAL_MS=$(APP_SEND_INTERVAL_MS)

# Optional: freeze the schedule (no time-varying timeslots). The time-varying
# mode already works in Cooja via the single channel offset set in
# project-conf.h; this is only a fallback for debugging.
ifeq ($(ALICE_STATIC_ASFN),1)
  CFLAGS += -DALICE_STATIC_ASFN
endif

include $(CONTIKI)/Makefile.include
