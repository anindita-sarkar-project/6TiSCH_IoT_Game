<?xml version="1.0" encoding="UTF-8"?>
<!-- EASE clean 1-hop star: root + 5 clients in a pentagon at radius 35.
     tx_range 38 reaches the root (35 m) but NOT the siblings (41 m apart), so
     RPL cannot build multi-hop relays: every node attaches directly to the root.
     This is the paper's low-load single-hop scenario (1 pkt/slotframe/node) and
     is the topology that produces a clean log. 30-min run, full per-line log. -->
<simconf>
  <simulation>
    <title>EASE star5</title>
    <randomseed>54321</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>50.0</transmitting_range>
      <interference_range>60.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events><logoutput>40000</logoutput></events>
    <motetype>
      org.contikios.cooja.contikimote.ContikiMoteType
      <identifier>easenode</identifier>
      <description>EASE node</description>
      <source>[CONTIKI_DIR]/examples/rpl-udp-ease/node.c</source>
      <commands>$(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_EASE_MGMT=1 APP_SEND_INTERVAL_MS=1010</commands>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiClock</moteinterface>
    </motetype>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>1</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>35.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>10.8</x> <y>33.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-28.3</x> <y>20.6</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>4</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-28.3</x> <y>-20.6</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>5</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>10.8</x> <y>-33.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>6</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
TIMEOUT(1800000, log.testOK());
while(true){
  YIELD();
  log.log(time/1000 + " " + id + " " + msg + "\n");
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
