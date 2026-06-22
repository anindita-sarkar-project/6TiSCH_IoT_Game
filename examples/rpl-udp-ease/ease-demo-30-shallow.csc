<?xml version="1.0" encoding="UTF-8"?>
<!-- EASE 30-node SHALLOW demo: root (id1) centred, 20 nodes 1 hop away, 9 nodes
     2 hops. Tests the same EASE code without a deep convergecast funnel. -->
<simconf>
  <simulation>
    <title>EASE 30-node shallow</title>
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
      <commands>rm -f build/cooja/obj/node.o build/cooja/node.cooja; $(MAKE) -j$(CPUS) node.cooja TARGET=cooja MAKE_WITH_EASE_MGMT=1 APP_SEND_INTERVAL_MS=1010</commands>
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
      <interface_config>org.contikios.cooja.interfaces.Position<x>40.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>38.0</x> <y>12.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>32.4</x> <y>23.5</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>4</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>23.5</x> <y>32.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>5</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>12.4</x> <y>38.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>6</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>40.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>7</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-12.4</x> <y>38.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>8</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-23.5</x> <y>32.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>9</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-32.4</x> <y>23.5</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>10</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-38.0</x> <y>12.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>11</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-40.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>12</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-38.0</x> <y>-12.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>13</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-32.4</x> <y>-23.5</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>14</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-23.5</x> <y>-32.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>15</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-12.4</x> <y>-38.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>16</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-0.0</x> <y>-40.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>17</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>12.4</x> <y>-38.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>18</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>23.5</x> <y>-32.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>19</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>32.4</x> <y>-23.5</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>20</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>38.0</x> <y>-12.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>21</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>74.5</x> <y>23.1</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>22</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>42.3</x> <y>65.6</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>23</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-9.8</x> <y>77.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>24</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-57.2</x> <y>53.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>25</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-77.9</x> <y>3.8</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>26</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-62.1</x> <y>-47.1</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>27</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-17.3</x> <y>-76.1</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>28</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>35.6</x> <y>-69.4</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>29</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>71.9</x> <y>-30.2</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>30</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
sent = 0; src = {};
TIMEOUT(600000);
while(true) {
  YIELD();
  log.log(time/1000 + " " + id + " " + msg + "\n");
  if(msg.contains("Received request")) {
    sent++;
    var p = msg.indexOf("from fd00::20");
    if(p >= 0) { src[msg.substring(p)] = true; }
  }
  var nsrc = 0; for(var k in src) { nsrc++; }
  if(nsrc >= 20 &amp;&amp; sent >= 400) {
    log.log("EASE shallow OK: distinct_senders=" + nsrc + " delivered=" + sent + "\n");
    log.testOK();
  }
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
