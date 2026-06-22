<?xml version="1.0" encoding="UTF-8"?>
<!-- A3 adaptation demo: 3 nodes (1 root + 2 senders) at a high send rate so the
     links are loaded. The mote log prints "A3 adapt: ... cells tx 1 to 2" as A3
     grows/shrinks cell counts. Run headless from tools/cooja via the gradle
     "run" task with the no-gui and autostart args and this .csc (see README). -->
<simconf>
  <simulation>
    <title>A3 adaptation demo</title>
    <randomseed>123456</randomseed>
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
      <identifier>a3node</identifier>
      <description>ALICE+A3 node</description>
      <source>[CONTIKI_DIR]/examples/rpl-udp-alice/node.c</source>
      <commands>$(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_A3=1 APP_SEND_INTERVAL_MS=40</commands>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiClock</moteinterface>
    </motetype>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>1</id></interface_config>
      <motetype_identifier>a3node</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>30.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>a3node</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>30.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>a3node</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
/* Pass once A3 has adapted cell counts a few times under load; fail on timeout
 * (4 min sim time) if it never adapts. */
adapts = 0;
TIMEOUT(240000);
while(true) {
  YIELD();
  if(msg.contains("A3 adapt")) {
    adapts++;
    log.log(time + " mote " + id + " " + msg + "\n");
    if(adapts == 5) {
      log.log("A3 adapted " + adapts + " times under load\n");
      log.testOK();
    }
  }
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
