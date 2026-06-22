<?xml version="1.0" encoding="UTF-8"?>
<!-- EASE single-hop star: root (id1) centred, 15 clients all 1 hop away. No
     relaying -> no funnel -> nodes become reachable and stay delivering. Low
     load (APP_SEND_INTERVAL_MS=1010 = 1 pkt/slotframe). -->
<simconf>
  <simulation>
    <title>EASE star</title>
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
      <interface_config>org.contikios.cooja.interfaces.Position<x>35.0</x> <y>0.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>2</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>32.0</x> <y>14.2</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>3</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>23.4</x> <y>26.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>4</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>10.8</x> <y>33.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>5</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-3.7</x> <y>34.8</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>6</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-17.5</x> <y>30.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>7</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-28.3</x> <y>20.6</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>8</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-34.2</x> <y>7.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>9</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-34.2</x> <y>-7.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>10</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-28.3</x> <y>-20.6</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>11</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-17.5</x> <y>-30.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>12</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>-3.7</x> <y>-34.8</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>13</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>10.8</x> <y>-33.3</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>14</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>23.4</x> <y>-26.0</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>15</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
    <mote>
      <interface_config>org.contikios.cooja.interfaces.Position<x>32.0</x> <y>-14.2</y> <z>0.0</z></interface_config>
      <interface_config>org.contikios.cooja.contikimote.interfaces.ContikiMoteID<id>16</id></interface_config>
      <motetype_identifier>easenode</motetype_identifier>
    </mote>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <script>
sent=0; src={};
TIMEOUT(600000);
while(true){
  YIELD();
  if(msg.contains("Received request")){
    sent++;
    var p=msg.indexOf("from fd00::20");
    if(p>=0){ src[msg.substring(p)]=true; }
  }
  var n=0; for(var k in src){ n++; }
  if(n>=14 &amp;&amp; sent>=400){
    log.log("EASE star OK: distinct_senders="+n+" delivered="+sent+"\n");
    log.testOK();
  }
}
      </script>
      <active>true</active>
    </plugin_config>
  </plugin>
</simconf>
