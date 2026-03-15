#pragma once

// Suspension control page -- arc gauge layout matching the LVGL display.
// Served at /suspension  (full standalone HTML page)
const char SUSPENSION_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Suspension - Bronco Controls</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f0f0f;color:#f2f4f8;font-family:system-ui,-apple-system,sans-serif;min-height:100vh;padding:12px}
h1{text-align:center;color:#D4A017;font-size:1.3rem;letter-spacing:2px;margin-bottom:12px;text-transform:uppercase}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;max-width:700px;margin:0 auto 10px}
.panel{background:#10131b;border:1px solid #2a2a3a;border-radius:12px;padding:10px;display:flex;flex-direction:column;align-items:center;gap:6px}
.panel-title{color:#D4A017;font-size:.75rem;font-weight:700;letter-spacing:2px;text-transform:uppercase}
.arc-wrap{position:relative;width:130px;height:130px}
.arc-wrap svg{width:130px;height:130px}
.arc-bg{fill:none;stroke:#2a2a3a;stroke-width:13;stroke-linecap:round}
.arc-ind{fill:none;stroke:#D4A017;stroke-width:13;stroke-linecap:round;transition:stroke-dashoffset .4s ease}
.arc-center{position:absolute;top:50%;left:50%;transform:translate(-50%,-56%);font-size:2.2rem;font-weight:700;color:#fff;text-align:center;line-height:1}
.btn-row{display:flex;gap:6px;width:100%}
.btn{flex:1;height:28px;border-radius:8px;border:1px solid #3a3a4a;background:#1a1d28;color:#fff;font-size:1rem;font-weight:700;cursor:pointer;transition:background .15s}
.btn.dec:hover,.btn.dec:active{background:#7a2a2a}
.btn.inc:hover,.btn.inc:active{background:#2a7a2a}
.preset-bar{max-width:700px;margin:0 auto 10px;display:flex;gap:8px}
.preset{flex:1;height:38px;border-radius:10px;border:2px solid #3a3a4a;background:#1a1d28;color:#ccc;font-size:1rem;font-weight:700;cursor:pointer;transition:all .15s}
.preset.active{background:#D4A017;border-color:#D4A017;color:#0f0f0f}
.preset:hover:not(.active){background:#2a3a4a}
.footer{max-width:700px;margin:0 auto;display:flex;gap:8px;align-items:center}
.calibrate-btn{flex:0 0 auto;padding:8px 16px;border-radius:8px;border:1px solid #3a3a4a;background:#1a2a3a;color:#fff;font-size:.8rem;cursor:pointer;transition:background .2s}
.calibrate-btn.active{background:#D4A017;color:#0f0f0f}
.status-bar{flex:1;font-size:.72rem;color:#8d92a3;text-align:right}
.err{color:#ff6b6b}
.ok{color:#3dd598}
</style>
</head>
<body>
<h1>Suspension Control</h1>
<div class="grid" id="panels"></div>
<div class="preset-bar" id="preset-bar"></div>
<div class="footer">
  <button class="calibrate-btn" id="cal-btn" onclick="toggleCalibrate()">Calibrate</button>
  <span class="status-bar" id="status">Connecting...</span>
</div>
<script>
const R=50,CX=65,CY=65,CIRC=2*Math.PI*R,ARC_LEN=CIRC*0.75;
const AXES=["front","rear","roll","pitch"];
const LABELS={front:"FRONT",rear:"REAR",roll:"ROLL",pitch:"PITCH"};
let state={front:1,rear:1,roll:1,pitch:1,calibration_active:false,power_on:false};

function makeSVG(ax){
  return `<svg viewBox="0 0 130 130">
    <circle class="arc-bg" cx="${CX}" cy="${CY}" r="${R}"
      stroke-dasharray="${ARC_LEN} ${CIRC}"
      transform="rotate(-225 ${CX} ${CY})"/>
    <circle class="arc-ind" id="ind-${ax}" cx="${CX}" cy="${CY}" r="${R}"
      stroke-dasharray="0 ${CIRC}"
      transform="rotate(-225 ${CX} ${CY})"/>
  </svg>`;
}

function buildPanels(){
  const g=document.getElementById("panels");
  g.innerHTML=AXES.map(ax=>`
    <div class="panel">
      <div class="panel-title">${LABELS[ax]}</div>
      <div class="arc-wrap">${makeSVG(ax)}
        <div class="arc-center"><span id="val-${ax}">-</span></div>
      </div>
      <div class="btn-row">
        <button class="btn dec" onclick="adjust('${ax}',-1)">-</button>
        <button class="btn inc" onclick="adjust('${ax}',+1)">+</button>
      </div>
    </div>`).join("");

  // Universal preset row: 5 presets that set ALL axes simultaneously
  const pb=document.getElementById("preset-bar");
  pb.innerHTML=[1,2,3,4,5].map(n=>
    `<button class="preset" id="pre-${n}" onclick="setPreset(${n})">${n}</button>`
  ).join("");
}

function setArc(ax,v){
  const el=document.getElementById("ind-"+ax);
  if(!el)return;
  el.setAttribute("stroke-dasharray",((v-1)/4*ARC_LEN)+" "+CIRC);
}

function render(){
  AXES.forEach(ax=>{
    const v=state[ax]||1;
    document.getElementById("val-"+ax).textContent=v;
    setArc(ax,v);
  });
  // Universal preset: highlight if all 4 axes share the same value
  const allSame=(state.front===state.rear&&state.rear===state.roll&&state.roll===state.pitch)?state.front:0;
  for(let n=1;n<=5;n++){
    const b=document.getElementById("pre-"+n);
    if(b) b.className="preset"+(n===allSame?" active":"");
  }
  const cb=document.getElementById("cal-btn");
  if(cb) cb.className="calibrate-btn"+(state.calibration_active?" active":"");
}

async function fetchState(){
  try{
    const r=await fetch("/api/suspension/state");
    if(!r.ok)throw new Error(r.status);
    const d=await r.json();
    state.front=d.front||1;
    state.rear=d.rear||1;
    state.roll=d.roll||1;
    state.pitch=d.pitch||1;
    state.calibration_active=d.calibration_active||false;
    state.power_on=d.power_on||false;
    const errs=[];
    if(d.error_fr)errs.push("ErrFR");
    if(d.error_fl)errs.push("ErrFL");
    if(d.error_rr)errs.push("ErrRR");
    if(d.error_rl)errs.push("ErrRL");
    const sb=document.getElementById("status");
    if(errs.length)sb.innerHTML=`<span class="err">${errs.join(" ")}</span>`;
    else if(d.power_on)sb.innerHTML=`<span class="ok">Active</span> - ${d.tx_count} TX`;
    else sb.innerHTML="Standby";
    render();
  }catch(e){document.getElementById("status").textContent="Error: "+e;}
}

async function sendSet(patch){
  try{
    const r=await fetch("/api/suspension/set",{
      method:"POST",headers:{"Content-Type":"application/json"},
      body:JSON.stringify(patch)
    });
    if(!r.ok)throw new Error(r.status);
    const d=await r.json();
    if(d.front!=null)state.front=d.front;
    if(d.rear!=null) state.rear=d.rear;
    if(d.roll!=null) state.roll=d.roll;
    if(d.pitch!=null)state.pitch=d.pitch;
    state.calibration_active=d.calibration_active||false;
    render();
  }catch(e){document.getElementById("status").textContent="Send error: "+e;}
}

function adjust(ax,delta){
  const v=Math.max(1,Math.min(5,(state[ax]||1)+delta));
  state[ax]=v; render(); sendSet({[ax]:v});
}

// Universal preset: set all 4 axes simultaneously
function setPreset(v){
  state.front=state.rear=state.roll=state.pitch=v;
  render();
  sendSet({front:v,rear:v,roll:v,pitch:v});
}

function toggleCalibrate(){
  state.calibration_active=!state.calibration_active;
  render(); sendSet({calibration_active:state.calibration_active});
}

buildPanels();
fetchState();
setInterval(fetchState,500);
</script>
</body>
</html>
)rawliteral";
