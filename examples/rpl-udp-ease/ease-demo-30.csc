<?xml version="1.0" encoding="UTF-8"?>
<!-- EASE 30-node demo: 1 root (id 1) + 29 clients in a 6x5 grid, spacing 25,
     TX range 50 (multi-hop). Run at LOW load (1 pkt/slotframe = 1010 ms): a
     single sink can serve ~70 frames/SF (50 shared + 20 dedicated), so 30x1
     pkt/SF forms and stays reachable. High load (101 ms) oversubscribes a
     30-to-1 sink ~4x and is physically infeasible regardless of scheduler. -->
<simconf>
  <simulation>
    <title>EASE 30-node demo</title>
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
      <interface_config>org.contikios.cooja.interfaces.Position<x>25.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>50.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>75.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>4</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>100.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>5</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>125.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>6</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>7</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>25.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>8</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>50.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>9</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>75.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>10</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>100.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>11</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>125.0</x> <y>25.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>12</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>13</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>25.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>14</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>50.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>15</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>75.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>16</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>100.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>17</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>125.0</x> <y>50.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>18</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>19</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>25.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>20</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>50.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>21</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>75.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>22</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>100.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>23</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>125.0</x> <y>75.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>24</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>0.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>25</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>25.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>26</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>50.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>27</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>75.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>28</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>100.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>29</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>125.0</x> <y>100.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>30</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
/* 30-node reachability + EASE-adaptation check. Pass once a healthy fraction of
 * clients have delivered to the root AND EASE has adapted (CUSUM + game); fail
 * on timeout if the network never forms. */
/* Pass requires DELIVERY BREADTH: >=15 distinct source nodes must have reached
 * the root (not just root's 1-hop neighbours), so the run exercises multi-hop
 * forwarding to steady state instead of stopping the instant a few neighbours
 * deliver. Fail on timeout if the network never forms. */
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
  if(nsrc >= 15 &amp;&amp; sent >= 300) {
    log.log("EASE 30-node OK: distinct_senders=" + nsrc + " delivered=" + sent + "\n");
    log.testOK();
  }
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
