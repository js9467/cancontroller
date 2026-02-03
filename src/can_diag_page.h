#ifndef CAN_DIAG_PAGE_H
#define CAN_DIAG_PAGE_H

const char CAN_DIAG_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CAN Diagnostics - Bronco Controls</title>
        <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
        <meta http-equiv="Pragma" content="no-cache" />
        <meta http-equiv="Expires" content="0" />
        <!-- CAN_DIAG_VERSION: 2026-02-01-01 -->
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #1a1a1a; color: #e0e0e0; padding: 20px; }
        .container { max-width: 1400px; margin: 0 auto; }
        h1 { color: #4a9eff; margin-bottom: 10px; }
        .subtitle { color: #888; margin-bottom: 30px; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(400px, 1fr)); gap: 20px; margin-bottom: 20px; }
        .panel { background: #2a2a2a; border: 1px solid #3a3a3a; border-radius: 8px; padding: 20px; }
        .panel h2 { color: #4a9eff; font-size: 18px; margin-bottom: 15px; border-bottom: 1px solid #3a3a3a; padding-bottom: 10px; }
        .status { display: flex; justify-content: space-between; margin: 8px 0; padding: 8px; background: #1f1f1f; border-radius: 4px; }
        .status-label { color: #aaa; }
        .status-value { font-weight: bold; }
        .status-value.good { color: #4caf50; }
        .status-value.bad { color: #f44336; }
        .status-value.warn { color: #ff9800; }
        button { background: #4a9eff; color: white; border: none; padding: 10px 20px; border-radius: 4px; cursor: pointer; margin: 5px; font-size: 14px; transition: background 0.3s; }
        button:hover { background: #3a8eef; }
        button.danger { background: #f44336; }
        button.danger:hover { background: #d32f2f; }
        button.success { background: #4caf50; }
        button.success:hover { background: #45a049; }
        input, select { background: #1f1f1f; color: #e0e0e0; border: 1px solid #3a3a3a; padding: 8px; border-radius: 4px; width: 100%; margin: 5px 0; }
        .form-group { margin: 15px 0; }
        .form-group label { display: block; color: #aaa; margin-bottom: 5px; font-size: 14px; }
        .monitor { background: #000; color: #0f0; font-family: 'Courier New', monospace; padding: 15px; border-radius: 4px; height: 400px; overflow-y: auto; font-size: 12px; }
        .monitor-line { margin: 2px 0; white-space: pre; }
        .i2c-device { background: #1f1f1f; padding: 10px; margin: 5px 0; border-radius: 4px; border-left: 3px solid #4a9eff; }
        .i2c-addr { color: #4a9eff; font-weight: bold; }
        .btn-group { display: flex; gap: 10px; flex-wrap: wrap; }
        .inline-group { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .alert { padding: 15px; border-radius: 4px; margin: 15px 0; }
        .alert-info { background: #1e4976; border-left: 4px solid #4a9eff; }
        .alert-warning { background: #6b4700; border-left: 4px solid #ff9800; }
        .alert-danger { background: #6b1b1b; border-left: 4px solid #f44336; }
        .metric { text-align: center; padding: 15px; background: #1f1f1f; border-radius: 4px; }
        .metric-value { font-size: 32px; font-weight: bold; color: #4a9eff; }
        .metric-label { color: #888; font-size: 12px; margin-top: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 CAN Bus Diagnostics & Configuration</h1>
        <p class="subtitle">Real-time monitoring and hardware troubleshooting</p>
            <div style="color:#666;font-size:12px;margin-top:4px;">CAN_DIAG_VERSION: 2026-02-01-01</div>

        <div class="grid">
            <!-- CAN Status Panel -->
            <div class="panel">
                <h2>📊 CAN Status</h2>
                <div id="can-status">Loading...</div>
                <div class="btn-group" style="margin-top: 15px;">
                    <button onclick="refreshStatus()" class="success">🔄 Refresh</button>
                    <button onclick="restartCAN()" class="danger">🔁 Restart CAN</button>
                </div>
            </div>

            <!-- I2C Scanner Panel -->
            <div class="panel">
                <h2>🔍 I2C Bus Scanner</h2>
                <div id="i2c-devices">Click Scan to detect devices...</div>
                <button onclick="scanI2C()" style="margin-top: 15px;">🔍 Scan I2C Bus</button>
            </div>

            <!-- Quick Metrics -->
            <div class="panel">
                <h2>📈 Live Metrics</h2>
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 10px;">
                    <div class="metric">
                        <div class="metric-value" id="rx-count">0</div>
                        <div class="metric-label">RX Frames</div>
                    </div>
                    <div class="metric">
                        <div class="metric-value" id="tx-count">0</div>
                        <div class="metric-label">TX Frames</div>
                    </div>
                </div>
                <button onclick="resetCounters()" style="margin-top: 15px; width: 100%;">Reset Counters</button>
            </div>
        </div>

        <!-- Configuration Panel -->
        <div class="panel" style="margin-bottom: 20px;">
            <h2>⚙️ CAN Configuration</h2>
            <div class="alert alert-info">
                <strong>⚠️ Advanced Settings:</strong> Changing these values may break CAN communication. Use with caution.
            </div>
            
            <div class="inline-group">
                <div class="form-group">
                    <label>CAN TX Pin (GPIO)</label>
                    <input type="number" id="cfg-tx-pin" min="0" max="48">
                </div>
                <div class="form-group">
                    <label>CAN RX Pin (GPIO)</label>
                    <input type="number" id="cfg-rx-pin" min="0" max="48">
                </div>
            </div>

            <div class="inline-group">
                <div class="form-group">
                    <label>I2C Chip Address (Hex)</label>
                    <input type="text" id="cfg-i2c-chip" placeholder="0x24">
                </div>
                <div class="form-group">
                    <label>I2C Output Register (Hex)</label>
                    <input type="text" id="cfg-i2c-output" placeholder="0x38">
                </div>
            </div>

            <div class="inline-group">
                <div class="form-group">
                    <label>Gate Enable Value (Hex)</label>
                    <select id="cfg-gate-value">
                        <option value="0x2A">0x2A (Primary)</option>
                        <option value="0x43">0x43 (Alt 1)</option>
                        <option value="0x07">0x07 (Alt 2)</option>
                        <option value="custom">Custom...</option>
                    </select>
                    <input type="text" id="cfg-gate-custom" style="display: none; margin-top: 5px;" placeholder="0x00">
                </div>
                <div class="form-group">
                    <label>CAN Bitrate</label>
                    <select id="cfg-bitrate">
                        <option value="125000">125 kbps</option>
                        <option value="250000" selected>250 kbps</option>
                        <option value="500000">500 kbps</option>
                        <option value="1000000">1000 kbps</option>
                    </select>
                </div>
            </div>

            <div class="inline-group">
                <div class="form-group">
                    <label>Loopback Mode (Self-Test)</label>
                    <div style="display:flex;align-items:center;gap:10px;">
                        <input type="checkbox" id="cfg-loopback" style="width:auto;">
                        <span style="color:#aaa;font-size:13px;">Enable TWAI loopback</span>
                    </div>
                </div>
                <div class="form-group"></div>
            </div>

            <div class="btn-group">
                <button onclick="applyConfig()" class="success">💾 Apply Configuration</button>
                <button onclick="testGate(true)">🟢 Enable Transceiver</button>
                <button onclick="testGate(false)" class="danger">🔴 Disable Transceiver</button>
            </div>
        </div>

        <!-- CAN Frame Monitor -->
        <div class="panel">
            <h2>📡 CAN Frame Monitor</h2>
            <div class="btn-group">
                <button id="monitor-btn" onclick="toggleMonitor()" class="success">▶️ Start Monitor</button>
                <button onclick="clearMonitor()">🗑️ Clear</button>
                <button onclick="sendTestFrame()">📤 Send Test Frame</button>
            </div>
            <div id="monitor" class="monitor" style="margin-top: 15px;">
Monitor stopped. Click "Start Monitor" to begin receiving frames...
            </div>
        </div>

        <!-- Test Tools -->
        <div class="panel">
            <h2>🧪 Test Tools</h2>
            <div class="btn-group">
                <button onclick="runDiagnostic()">🔍 Run Full Diagnostic</button>
                <button onclick="testReceive()">📥 Test Receive (5s)</button>
                <button onclick="dumpHardware()">💾 Dump Hardware Status</button>
            </div>
            <div id="test-output" style="margin-top: 15px; background: #1f1f1f; padding: 10px; border-radius: 4px; font-family: monospace; font-size: 12px; max-height: 200px; overflow-y: auto;">
Test results will appear here...
            </div>
        </div>
    </div>

    <script>
        let monitorActive = false;
        let rxCount = 0;
        let txCount = 0;
        let formInitialized = false;

        // Delayed event listener setup
        window.addEventListener('DOMContentLoaded', function() {
            const gateSelect = document.getElementById('cfg-gate-value');
            if (gateSelect) {
                gateSelect.addEventListener('change', function() {
                    const customInput = document.getElementById('cfg-gate-custom');
                    if (customInput) customInput.style.display = this.value === 'custom' ? 'block' : 'none';
                });
            }
        });

        async function refreshStatus() {
            try {
                const resp = await fetch('/api/can/status');
                const data = await resp.json();
                console.log('CAN Status:', data);
                
                // Initialize form on first load
                if (!formInitialized && data.config) {
                    formInitialized = true;
                    const cfg = data.config;
                    const e = (id, val) => { const el = document.getElementById(id); if (el) el.value = val; };
                    e('cfg-tx-pin', cfg.tx_pin || 20);
                    e('cfg-rx-pin', cfg.rx_pin || 19);
                    e('cfg-i2c-chip', cfg.i2c_chip_addr || '0x24');
                    e('cfg-i2c-output', cfg.i2c_output_addr || '0x38');
                    e('cfg-bitrate', cfg.bitrate || 250000);
                    const lb = document.getElementById('cfg-loopback');
                    if (lb) lb.checked = !!cfg.loopback;
                }
                
                let html = '';
                html += `<div class="status"><span class="status-label">Ready:</span><span class="status-value ${data.ready ? 'good' : 'bad'}">${data.ready ? 'YES' : 'NO'}</span></div>`;
                html += `<div class="status"><span class="status-label">State:</span><span class="status-value">${data.bus_state || 'INIT'}</span></div>`;
                html += `<div class="status"><span class="status-label">TX Pin:</span><span class="status-value">GPIO${data.tx_pin || 20}</span></div>`;
                html += `<div class="status"><span class="status-label">RX Pin:</span><span class="status-value">GPIO${data.rx_pin || 19}</span></div>`;
                html += `<div class="status"><span class="status-label">Bitrate:</span><span class="status-value">${data.bitrate || 250000} bps</span></div>`;
                html += `<div class="status"><span class="status-label">Mode:</span><span class="status-value ${data.loopback ? 'warn' : 'good'}">${data.loopback ? 'LOOPBACK' : 'NORMAL'}</span></div>`;
                html += `<div class="status"><span class="status-label">RX High:</span><span class="status-value ${(data.rx_pin_ones || 0) > 5 ? 'good' : 'bad'}">${(data.rx_pin_ones || 0)}/20</span></div>`;
                html += `<div class="status"><span class="status-label">Diagnosis:</span><span class="status-value">${data.diagnosis || 'N/A'}</span></div>`;
                
                const el = document.getElementById('can-status');
                if (el) el.innerHTML = html;
            } catch(e) {
                console.error('Error:', e);
                const el = document.getElementById('can-status');
                if (el) el.innerHTML = 'Error loading status';
            }
        }

        async function scanI2C() {
            const el = document.getElementById('i2c-devices');
            if (!el) return;
            el.innerHTML = 'Scanning...';
            try {
                const resp = await fetch('/api/i2c/scan');
                const data = await resp.json();
                let html = '';
                if (!data.found_devices || data.found_devices.length === 0) {
                    html = 'No I2C devices found';
                } else {
                    data.found_devices.forEach(d => {
                        html += `<div class="i2c-device"><span class="i2c-addr">${d.address}</span> - ${d.possible_device}</div>`;
                    });
                }
                el.innerHTML = html;
            } catch(e) {
                el.innerHTML = 'Scan error';
            }
        }

        async function toggleMonitor() {
            monitorActive = !monitorActive;
            const btn = document.getElementById('monitor-btn');
            if (!btn) return;
            if (monitorActive) {
                btn.textContent = '⏸️ Stop';
                btn.className = 'danger';
                monitorLoop();
            } else {
                btn.textContent = '▶️ Start';
                btn.className = 'success';
            }
        }

        async function monitorLoop() {
            if (!monitorActive) return;
            try {
                const resp = await fetch('/api/can/frames');
                const data = await resp.json();
                if (data.frames && data.frames.length > 0) {
                    const mon = document.getElementById('monitor');
                    if (mon) {
                        data.frames.forEach(f => {
                            const div = document.createElement('div');
                            div.className = 'monitor-line';
                            div.textContent = `ID:0x${f.id.toString(16).padStart(8,'0')} DLC:${f.dlc}`;
                            mon.appendChild(div);
                            rxCount++;
                        });
                        const rc = document.getElementById('rx-count');
                        if (rc) rc.textContent = rxCount;
                    }
                }
            } catch(e) {}
            setTimeout(monitorLoop, 100);
        }

        function clearMonitor() {
            const el = document.getElementById('monitor');
            if (el) el.innerHTML = '';
        }

        function resetCounters() {
            rxCount = txCount = 0;
            const rx = document.getElementById('rx-count');
            const tx = document.getElementById('tx-count');
            if (rx) rx.textContent = '0';
            if (tx) tx.textContent = '0';
        }

        async function testGate(en) {
            try {
                const resp = await fetch('/api/can/gate', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({enable: en})
                });
                const data = await resp.json();
                alert(data.message || 'Done');
                setTimeout(refreshStatus, 500);
            } catch(e) {
                alert('Error');
            }
        }

        async function applyConfig() {
            const cfg = {
                tx_pin: parseInt(document.getElementById('cfg-tx-pin').value || 20),
                rx_pin: parseInt(document.getElementById('cfg-rx-pin').value || 19),
                i2c_chip_addr: document.getElementById('cfg-i2c-chip').value || '0x24',
                i2c_output_addr: document.getElementById('cfg-i2c-output').value || '0x38',
                gate_enable_value: document.getElementById('cfg-gate-value').value,
                bitrate: parseInt(document.getElementById('cfg-bitrate').value || 250000),
                loopback: document.getElementById('cfg-loopback')?.checked || false
            };
            if (!confirm('Apply config?')) return;
            try {
                const resp = await fetch('/api/can/config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(cfg)
                });
                const data = await resp.json();
                alert(data.success ? 'Applied' : 'Failed');
                setTimeout(refreshStatus, 1000);
            } catch(e) {
                alert('Error: ' + e.message);
            }
        }

        async function restartCAN() {
            if (!confirm('Restart CAN?')) return;
            try {
                await fetch('/api/can/restart', {method: 'POST'});
                alert('Restarted');
                setTimeout(refreshStatus, 500);
            } catch(e) {
                alert('Error');
            }
        }

        async function sendTestFrame() {
            try {
                const resp = await fetch('/api/can/send_test', {method: 'POST'});
                const data = await resp.json();
                txCount++;
                const tx = document.getElementById('tx-count');
                if (tx) tx.textContent = txCount;
                alert(data.message || 'Sent');
            } catch(e) {
                alert('Error');
            }
        }

        async function testReceive() {
            const out = document.getElementById('test-output');
            if (out) out.innerHTML = 'Listening for 5 seconds...';
            
            try {
                for (let i = 0; i < 10; i++) {
                    const resp = await fetch('/api/can/frames');
                    const data = await resp.json();
                    if (data.frames && data.frames.length > 0) {
                        let html = `<div>Received ${data.frames.length} frame(s):</div>`;
                        data.frames.forEach(f => {
                            html += `<div>ID: 0x${f.id.toString(16).padStart(8,'0')} DLC: ${f.dlc} Data: ${f.data}</div>`;
                            rxCount++;
                        });
                        if (out) out.innerHTML = html;
                        const rc = document.getElementById('rx-count');
                        if (rc) rc.textContent = rxCount;
                        return;
                    }
                    await new Promise(r => setTimeout(r, 500));
                }
                if (out) out.innerHTML = 'No frames received in 5 seconds';
            } catch(e) {
                if (out) out.innerHTML = `Error: ${e.message}`;
            }
        }

        async function runDiagnostic() {
            const out = document.getElementById('test-output');
            if (out) out.innerHTML = 'Running diagnostic...';
            
            try {
                const resp = await fetch('/api/can/status');
                const data = await resp.json();
                let html = '<div>Diagnostic Results:</div>';
                html += `<div>CAN Ready: ${data.ready ? 'YES' : 'NO'}</div>`;
                html += `<div>Bus State: ${data.bus_state}</div>`;
                html += `<div>Bitrate: ${data.bitrate} bps</div>`;
                html += `<div>TX Errors: ${data.tx_errors}</div>`;
                html += `<div>RX Errors: ${data.rx_errors}</div>`;
                html += `<div>RX Pin State: HIGH ${data.rx_pin_ones}/20 (${data.diagnosis})</div>`;
                if (out) out.innerHTML = html;
            } catch(e) {
                if (out) out.innerHTML = `Error: ${e.message}`;
            }
        }

        async function dumpHardware() {
            const out = document.getElementById('test-output');
            if (out) out.innerHTML = 'Dumping hardware config...';
            
            try {
                const resp = await fetch('/api/can/status');
                const data = await resp.json();
                if (data.config) {
                    let html = '<div>Hardware Configuration:</div>';
                    Object.keys(data.config).forEach(k => {
                        html += `<div>${k}: ${data.config[k]}</div>`;
                    });
                    html += `<div>TX Pin: GPIO${data.tx_pin}</div>`;
                    html += `<div>RX Pin: GPIO${data.rx_pin}</div>`;
                    if (out) out.innerHTML = html;
                }
            } catch(e) {
                if (out) out.innerHTML = `Error: ${e.message}`;
            }
        }

        // Start auto-refresh
        console.log('Page initialized');
        refreshStatus();
        setInterval(refreshStatus, 2000);
    </script>
</body>
</html>
)rawliteral";

#endif // CAN_DIAG_PAGE_H
