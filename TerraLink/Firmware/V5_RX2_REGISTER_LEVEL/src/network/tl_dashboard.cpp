#include <Arduino.h>
#include "tl_dashboard.h"

const char *tl_dashboard_html(void) {
    return R"rawliteral(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TerraLink Rescue Command Center</title>
<style>
body{font-family:Arial;background:#101318;color:#eee;margin:20px}
.card{background:#1b2028;padding:14px;margin:8px 0;border-radius:10px}
button{padding:10px;margin:4px;background:#2d3542;color:white;border:0;border-radius:7px}
table{width:100%;border-collapse:collapse}td,th{padding:7px;border-bottom:1px solid #333}
.sos{font-weight:bold}
</style></head><body>
<h1>TerraLink Rescue Command Center</h1>
<div class="card" id="status">Loading...</div>
<div class="card"><h2>Nodes</h2><table><thead><tr><th>ID</th><th>SOS</th><th>Lat</th><th>Lon</th><th>Hop</th><th>RSSI</th></tr></thead><tbody id="nodes"></tbody></table></div>
<div class="card"><h2>Commands</h2>
<input id="node" type="number" placeholder="Node ID">
<button onclick="cmd('MSG','STAY CALM')">STAY CALM</button>
<button onclick="cmd('MSG','RESCUE TEAM IS ON THE WAY')">RESCUE</button>
<button onclick="cmd('MSG','DO NOT MOVE')">DO NOT MOVE</button>
<button onclick="cmd('MSG','HELP IS APPROACHING')">HELP</button>
<button onclick="cmd('GPS_REQUEST','')">GPS REQUEST</button>
<button onclick="cmd('LOC_AGAIN','')">LOCATION AGAIN</button>
<button onclick="cmd('EMERGENCY','')">EMERGENCY</button>
<button onclick="resolveNode()">RESOLVE</button>
</div>
<div class="card"><h2>Messages</h2><pre id="messages"></pre></div>
<script>
async function load(){
 let s=await fetch('/api/status').then(r=>r.json());
 document.getElementById('status').textContent=
 'RX1 '+(s.rx1_online?'ONLINE':'OFFLINE')+' | Nodes '+s.nodes+
 ' | Active SOS '+s.active_sos+' | LoRa RX '+s.rx+' | TX '+s.tx;
 let n=await fetch('/api/nodes').then(r=>r.json());
 document.getElementById('nodes').innerHTML=n.map(x=>'<tr><td>'+x.node+
 '</td><td class="sos">'+(x.sos?'ACTIVE':'-')+'</td><td>'+x.lat+
 '</td><td>'+x.lon+'</td><td>'+x.hop+'</td><td>'+x.rssi+'</td></tr>').join('');
 let m=await fetch('/api/messages').then(r=>r.text());
 document.getElementById('messages').textContent=m;
}
async function cmd(type,msg){
 let node=document.getElementById('node').value;
 if(!node)return alert('Enter node ID');
 await fetch('/cmd?type='+encodeURIComponent(type)+'&node='+node+'&msg='+encodeURIComponent(msg));
 load();
}
async function resolveNode(){
 let node=document.getElementById('node').value;
 if(node) await fetch('/resolve?node='+node); load();
}
load();setInterval(load,3000);
</script></body></html>)rawliteral";
}
