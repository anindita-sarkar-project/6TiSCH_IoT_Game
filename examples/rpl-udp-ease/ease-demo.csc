<?xml version="1.0" encoding="UTF-8"?>
<!-- EASE demo: 1 root + 2 loaded children. The mote log shows the CUSUM
     predictor (EASE CUSUM) growing dedicated cells under load and the
     game-theoretic allocation (EASE game) sharing the RDC budget by weight.
     Run headless from tools/cooja via the gradle run task (see README). -->
<simconf>
  <simulation>
    <title>EASE demo</title>
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
      <identifier>easenode</identifier>
      <description>EASE node</description>
      <source>[CONTIKI_DIR]/examples/rpl-udp-ease/node.c</source>
      <commands>$(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_EASE_MGMT=1 APP_SEND_INTERVAL_MS=101</commands>
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
      <interface_config>org.contikios.cooja.interfaces.Position<x>30.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>30.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
/* Pass once we have seen both the CUSUM predictor and the game allocation act;
 * fail on timeout (4 min sim) if EASE never adapts. */
cusum = 0; game = 0;
TIMEOUT(360000);
while(true) {
  YIELD();
  if(msg.contains("EASE CUSUM")) { cusum++; log.log(time + " " + id + " " + msg + "\n"); }
  if(msg.contains("EASE game"))  { game++;  log.log(time + " " + id + " " + msg + "\n"); }
  if(cusum >= 3 &amp;&amp; game >= 2) {
    log.log("EASE working: CUSUM=" + cusum + " game=" + game + "\n");
    log.testOK();
  }
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
