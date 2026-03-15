#pragma once

// Suspension control page â€” arc gauge layout matching the LVGL display.
// Served at /suspension  (full standalone HTML page)
const char SUSPENSION_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Suspension â€” Bronco Controls</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0f0f0f;color:#f2f4f8;font-family:system-ui,-apple-system,sans-serif;min-height:100vh;padding:12px}
h1{text-align:center;color:#D4A017;font-size:1.3rem;letter-spacing:2px;margin-bottom:12px;text-transform:uppercase}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;max-width:700px;margin:0 auto 12px}
.panel{background:#10131b;border:1px solid #2a2a3a;border-radius:12px;padding:10px;display:flex;flex-direction:column;align-items:center;gap:6px}
.panel-title{color:#D4A017;font-size:.75rem;font-weight:700;letter-spacing:2px;text-transform:uppercase}
.arc-wrap{position:relative;width:130px;height:130px}
.arc-wrap svg{width:130px;height:130px}
.arc-bg{fill:none;stroke:#2a2a3a;stroke-width:13;stroke-linecap:round}
.arc-ind{fill:none;stroke:#D4A017;stroke-width:13;stroke-linecap:round;transition:stroke-dashoffset .4s ease}
.arc-center{position:absolute;top:50%;left:50%;transform:translate(-50%,-56%);font-size:2.2rem;font-weight:700;color:#fff;text-align:center;line-height:1}
.arc-unit{font-size:.65rem;color:#8d92a3;display:block;margin-top:2px;text-align:center}
.btn-row{display:flex;gap:6px;width:100%}
.btn{flex:1;height:28px;border-radius:8px;border:1px solid #3a3a4a;background:#1a1d28;color:#fff;font-size:1rem;font-weight:700;cursor:pointer;transition:background .15s}
.btn.dec:hover,.btn.dec:active{background:#7a2a2a}
.btn.inc:hover,.btn.inc:active{background:#2a7a2a}
.presets{display:flex;gap:4px;width:100%}
.preset{flex:1;height:21px;border-radius:6px;border:1px solid #3a3a4a;background:#1a1d28;color:#ccc;font-size:.7rem;font-weight:600;cursor:pointer;transition:all .15s}
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
<div class="footer">
  <button class="calibrate-btn" id="cal-btn" onclick="toggleCalibrate()">Calibrate</button>
  <span class="status-bar" id="status">Connectingâ€¦</span>
</div>
<script>
// Arc geometry: 270Â° sweep, gap at top.
// SVG: r=50, cx=cy=65, stroke-width=13.
// Full circle circ = 2*PI*50 = 314.16; 270Â° fraction = 235.62; gap = 78.54
const R=50, CX=65, CY=65, CIRC=2*Math.PI*R;
const ARC_LEN=CIRC*0.75, GAP_LEN=CIRC*0.25;

const AXES=['front','rear','roll','pitch'];
const LABELS={front:'FRONT',rear:'REAR',roll:'ROLL',pitch:'PITCH'};
let state={front:1,rear:1,roll:1,pitch:1,calibration_active:false,power_on:false,messaging_enabled:true};

function makeSVG(){
  // circle starts at 3-o'clock (East); rotate -225Â° = start at 135Â° CW from East = lower-left
  return `<svg viewBox="0 0 130 130">
    <circle class="arc-bg" cx="${CX}" cy="${CY}" r="${R}"
      stroke-dasharray="${ARC_LEN} ${CIRC}"
      transform="rotate(-225 ${CX} ${CY})"/>
    <circle class="arc-ind" id="ind-AXIS" cx="${CX}" cy="${CY}" r="${R}"
      stroke-dasharray="0 ${CIRC}"
      transform="rotate(-225 ${CX} ${CY})"/>
  </svg>`;
}

function buildPanels(){
  const g=document.getElementById('panels');
  g.innerHTML='';
  AXES.forEach(ax=>{
    const svg=makeSVG().replace(/AXIS/g,ax);
    const presets=[1,2,3,4,5].map(n=>
      `<button class="preset" id="pre-${ax}-${n}" onclick="setPreset('${ax}',${n})">${n}</button>`
    ).join('');
    g.innerHTML+=`
    <div class="panel">
      <div class="panel-title">${LABELS[ax]}</div>
      <div class="arc-wrap">
        ${svg}
        <div class="arc-center">
          <span id="val-${ax}">-</span>
          <span class="arc-unit" id="unit-${ax}"></span>
        </div>
      </div>
      <div class="btn-row">
        <button class="btn dec" onclick="adjust('${ax}',-1)">-</button>
        <button class="btn inc" onclick="adjust('${ax}',+1)">+</button>
      </div>
      <div class="presets">${presets}</div>
    </div>`;
  });
}

function setArc(ax, v){
  const el=document.getElementById('ind-'+ax);
  if(!el)return;
  const frac=(v-1)/4;
  const filled=frac*ARC_LEN;
  el.setAttribute('stroke-dasharray',filled+' '+CIRC);
}

function render(){
  AXES.forEach(ax=>{
    const v=state[ax]||1;
    document.getElementById('val-'+ax).textContent=v;
    setArc(ax,v);
    for(let n=1;n<=5;n++){
      const b=document.getElementById(`pre-${ax}-${n}`);
      if(b) b.className='preset'+(n===v?' active':'');
    }
  });
  const cb=document.getElementById('cal-btn');
  if(cb) cb.className='calibrate-btn'+(state.calibration_active?' active':'');
}

async function fetchState(){
  try{
    const r=await fetch('/api/suspension/state');
    if(!r.ok)throw new Error(r.status);
    const d=await r.json();
    state.front=d.front||d.actual_front+1||1;
    state.rear=d.rear||d.actual_rear+1||1;
    state.roll=d.roll||d.actual_roll+1||1;
    state.pitch=d.pitch||d.actual_pitch+1||1;
    state.calibration_active=d.calibration_active||false;
    state.power_on=d.power_on||false;
    state.messaging_enabled=d.messaging_enabled!==false;

    // Prefer actual (feedback) values when fresh
    const now=Date.now(), ms=d.last_feedback_ms||0;
    if(ms>0 && (now/1000-ms/1000)<2){
      if(d.actual_front!==undefined) state.front=d.actual_front+1;
      if(d.actual_rear!==undefined)  state.rear=d.actual_rear+1;
      if(d.actual_roll!==undefined)  state.roll=d.actual_roll+1;
      if(d.actual_pitch!==undefined) state.pitch=d.actual_pitch+1;
    }

    const errs=[];
    if(d.error_fr) errs.push('ErrFR');
    if(d.error_fl) errs.push('ErrFL');
    if(d.error_rr) errs.push('ErrRR');
    if(d.error_rl) errs.push('ErrRL');
    const sb=document.getElementById('status');
    if(errs.length) sb.innerHTML='<span class="err">'+errs.join(' ')+'</span>';
    else if(d.power_on) sb.innerHTML='<span class="ok">Active</span> Â· '+d.tx_count+' TX';
    else if(!d.messaging_enabled) sb.innerHTML='<span class="err">Messaging disabled</span>';
    else sb.innerHTML='Standby';
    render();
  }catch(e){
    document.getElementById('status').textContent='Error: '+e;
  }
}

async function sendSet(patch){
  try{
    const r=await fetch('/api/suspension/set',{
      method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify(patch)
    });
    if(!r.ok)throw new Error(r.status);
    const d=await r.json();
    state.front=d.front||state.front;
    state.rear=d.rear||state.rear;
    state.roll=d.roll||state.roll;
    state.pitch=d.pitch||state.pitch;
    state.calibration_active=d.calibration_active||false;
    render();
  }catch(e){document.getElementById('status').textContent='Send error: '+e;}
}

function adjust(ax, delta){
  const v=Math.max(1,Math.min(5,(state[ax]||1)+delta));
  const patch={[ax]:v};
  state[ax]=v;
  render();
  sendSet(patch);
}

function setPreset(ax, v){
  const patch={[ax]:v};
  state[ax]=v;
  render();
  sendSet(patch);
}

function toggleCalibrate(){
  const next=!state.calibration_active;
  state.calibration_active=next;
  render();
  sendSet({calibration_active:next});
}

buildPanels();
fetchState();
setInterval(fetchState,500);
</script>
</body>
</html>
)rawliteral";
