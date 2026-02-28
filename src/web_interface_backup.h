#pragma once

const char WEB_INTERFACE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Bronco Controls Configurator</title>
<style>
:root {
  --bg: #0f0f12;
  --panel: #1c1c20;
  --accent: #ff9d2e;
  --accent-soft: #ffbb66;
  --text: #f4f4f6;
  --muted: #8c909c;
  --border: #2c2c33;
  --success: #3dd598;
  --danger: #ff5c5c;
  font-family: "Space Grotesk", "Segoe UI", sans-serif;
}
* {
  box-sizing: border-box;
}
body {
  margin: 0;
  background: radial-gradient(circle at top, rgba(255,153,0,0.08), transparent 55%), var(--bg);
  color: var(--text);
  min-height: 100vh;
}
header {
  padding: 32px 4vw 16px;
}
header h1 {
  margin: 0;
  font-size: 2.25rem;
  letter-spacing: 0.05em;
}
header p {
  margin: 8px 0 0;
  color: var(--muted);
}
main {
  padding: 0 4vw 48px;
  display: flex;
  flex-direction: column;
  gap: 24px;
}
.card {
  background: var(--panel);
  border: 1px solid var(--border);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 25px 60px rgba(0,0,0,0.35);
}
.card-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 16px;
  flex-wrap: wrap;
  margin-bottom: 20px;
}
.card-head h2 {
  margin: 0;
  font-size: 1.35rem;
}
.card-head .eyebrow {
  text-transform: uppercase;
  font-size: 0.75rem;
  letter-spacing: 0.2em;
  color: var(--accent-soft);
  margin-bottom: 6px;
}
.helper-text {
  color: var(--muted);
  font-size: 0.85rem;
  margin: 0;
}
.helper-text.small {
  font-size: 0.75rem;
}
.divider {
  height: 1px;
  width: 100%;
  background: var(--border);
  opacity: 0.5;
  margin: 24px 0;
}
.join-card {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.actions {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
}
button,
.label-button {
  background: var(--accent);
  border: none;
  color: #1c1208;
  font-weight: 600;
  padding: 12px 20px;
  border-radius: 12px;
  cursor: pointer;
  transition: transform 0.15s ease, box-shadow 0.15s ease;
  box-shadow: 0 10px 20px rgba(255,157,46,0.25);
}
button.ghost,
.label-button.ghost {
  background: transparent;
  color: var(--text);
  border: 1px solid var(--border);
  box-shadow: none;
}
button.full {
  width: 100%;
}
button:hover,
.label-button:hover {
  transform: translateY(-1px);
}
input,
select {
  width: 100%;
  padding: 10px 12px;
  border-radius: 10px;
  border: 1px solid var(--border);
  background: #121216;
  color: var(--text);
}
label {
  font-size: 0.85rem;
  color: var(--muted);
}
.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.grid {
  display: grid;
  gap: 16px;
}
.grid.two-col {
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
}
.page-stack {
  display: flex;
  flex-direction: column;
  gap: 24px;
}
.page-card {
  border: 1px solid var(--border);
  border-radius: 18px;
  padding: 18px;
  background: #131318;
}
.branding-grid {
  display: grid;
  gap: 16px;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
}
.page-header {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  align-items: center;
  margin-bottom: 12px;
}
.badge {
  padding: 4px 10px;
  border-radius: 999px;
  background: rgba(255,157,46,0.15);
  color: var(--accent-soft);
  font-size: 0.75rem;
  letter-spacing: 0.08em;
}
.button-grid {
  display: grid;
  gap: 14px;
}
.button-card {
  border: 1px dashed var(--border);
  border-radius: 16px;
  padding: 14px;
  background: #101014;
}
.button-card h4 {
  margin: 0 0 6px;
}
.button-card details {
  margin-top: 12px;
}
details summary {
  cursor: pointer;
  color: var(--accent-soft);
}
.switch-row {
  display: flex;
  align-items: center;
  gap: 8px;
}
.switch-row input[type="checkbox"] {
  width: auto;
}
.toast {
  position: fixed;
  bottom: 24px;
  right: 24px;
  padding: 14px 20px;
  border-radius: 14px;
  background: rgba(18,18,24,0.95);
  border: 1px solid var(--border);
  box-shadow: 0 20px 40px rgba(0,0,0,0.45);
  display: none;
  min-width: 200px;
}
.toast.show {
  display: block;
}
.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 12px;
}
.status-chip {
  padding: 10px 14px;
  border-radius: 12px;
  border: 1px solid var(--border);
  background: #111117;
}
.status-chip span {
  display: block;
  font-size: 0.75rem;
  color: var(--muted);
}
.network-scan {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.scan-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  flex-wrap: wrap;
  gap: 16px;
}
.network-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 14px;
}
.network-card {
  border: 1px solid var(--border);
  border-radius: 16px;
  padding: 16px;
  background: #101014;
  display: flex;
  flex-direction: column;
  gap: 10px;
}
.network-card h4 {
  margin: 0;
}
.network-card .meta {
  display: flex;
  justify-content: space-between;
  font-size: 0.85rem;
  color: var(--muted);
}
.network-card .meta .signal {
  color: var(--accent-soft);
  font-weight: 600;
}
.empty-hint {
  color: var(--muted);
  font-size: 0.9rem;
}
@media (max-width: 720px) {
  header h1 {
    font-size: 1.75rem;
  }
  .card-head {
    flex-direction: column;
  }
}
</style>
</head>
<body>
<header>
  <p class="eyebrow">Bronco Controls</p>
  <h1>Web Configurator</h1>
  <p>Design pages, map CAN frames, and manage WiFi without recompiling firmware.</p>
</header>

<main>
  <section class="card">
    <div class="card-head">
      <div>
        <p class="eyebrow">Network</p>
        <h2>WiFi & Access Point</h2>
      </div>
      <div class="actions">
        <button onclick="saveConfig()">Save Config</button>
        <button class="ghost" onclick="exportConfig()">Export JSON</button>
        <label class="label-button ghost" for="import">Import JSON
          <input id="import" type="file" accept="application/json" onchange="importConfig(event)" hidden />
        </label>
      </div>
    </div>
    <div class="grid two-col">
      <div class="field">
        <label>AP Enabled</label>
        <div class="switch-row">
          <input type="checkbox" id="ap-enabled" onchange="updateWifi('ap','enabled', this.checked)" />
          <span>Broadcast BroncoControls network</span>
        </div>
      </div>
      <div class="field">
        <label>AP SSID</label>
        <input id="ap-ssid" type="text" maxlength="32" placeholder="BroncoControls"
               oninput="updateWifi('ap','ssid', this.value)" />
      </div>
      <div class="field">
        <label>AP Password (8+ chars)</label>
        <input id="ap-password" type="text" maxlength="64" placeholder="bronco123"
               oninput="updateWifi('ap','password', this.value)" />
      </div>
      <div class="field">
        <label>Station Mode</label>
        <div class="switch-row">
          <input type="checkbox" id="sta-enabled" onchange="updateWifi('sta','enabled', this.checked)" />
          <span>Also join my garage WiFi</span>
        </div>
      </div>
      <div class="field">
        <label>Station SSID</label>
        <input id="sta-ssid" type="text" maxlength="32" oninput="updateWifi('sta','ssid', this.value)" />
      </div>
      <div class="field">
        <label>Station Password</label>
        <input id="sta-password" type="password" maxlength="64" oninput="updateWifi('sta','password', this.value)" />
      </div>
    </div>
    <div class="divider"></div>
    <div class="join-card">
      <div>
        <p class="eyebrow" style="margin-bottom:4px;">Home WiFi</p>
        <h3 style="margin:0;">Join Existing Network</h3>
        <p class="helper-text">Bridge the configurator onto your house network while keeping the AP online.</p>
      </div>
      <div class="grid two-col">
        <div class="field">
          <label>Home SSID</label>
          <input id="quick-ssid" type="text" maxlength="32" placeholder="MyHomeWiFi" />
        </div>
        <div class="field">
          <label>Home Password</label>
          <input id="quick-password" type="password" maxlength="64" placeholder="" />
        </div>
      </div>
      <div class="actions">
        <button onclick="joinHomeWifi()">Join Home WiFi</button>
        <button class="ghost" onclick="forgetSta()">Forget STA Profile</button>
      </div>
    </div>
    <div class="divider"></div>
    <div class="network-scan">
      <div class="scan-header">
        <div>
          <p class="eyebrow" style="margin-bottom:4px;">Nearby Networks</p>
          <h3 style="margin:0;">Scan &amp; Discover</h3>
          <p class="helper-text">Scan for SSIDs, copy them into the join form, then push credentials with one tap.</p>
        </div>
        <div class="actions">
          <button onclick="scanNetworks()">Scan Networks</button>
        </div>
      </div>
      <div id="scan-results" class="network-grid empty-hint">Tap "Scan Networks" to populate this list.</div>
    </div>
    <div class="status-grid" id="status"></div>
  </section>

  <section class="card">
    <div class="card-head">
      <div>
        <p class="eyebrow">Branding</p>
        <h2>Header &amp; Logo</h2>
      </div>
    </div>
    <div class="branding-grid">
      <div class="field">
        <label>Header Title</label>
        <input id="header-title" type="text" maxlength="48" placeholder="Bronco Controls"
               oninput="updateHeaderField('title', this.value)" />
      </div>
      <div class="field">
        <label>Header Subtitle</label>
        <input id="header-subtitle" type="text" maxlength="64" placeholder="Web Configurator"
               oninput="updateHeaderField('subtitle', this.value)" />
      </div>
      <div class="field">
        <label>Show Logo</label>
        <div class="switch-row">
          <input type="checkbox" id="header-show-logo" onchange="toggleHeaderField('show_logo', this.checked)" />
          <span>Display a logo next to the header text</span>
        </div>
      </div>
      <div class="field">
        <label>Logo Variant</label>
        <select id="header-logo" onchange="updateHeaderField('logo_variant', this.value)"></select>
        <p class="helper-text small">Pick from the built-in Bronco icons or hide it entirely.</p>
      </div>
    </div>
  </section>

  <section class="card">
    <div class="card-head">
      <div>
        <p class="eyebrow">Pages</p>
        <h2>Screen Builder</h2>
      </div>
      <div class="actions">
        <button class="ghost" onclick="addPage()">Add Page</button>
      </div>
    </div>
    <div class="page-stack" id="pages"></div>
  </section>
</main>

<div class="toast" id="toast"></div>

<script>
const ICON_OPTIONS = [
  { id: '', label: 'None (color only)' },
  { id: 'bronco', label: 'Bronco Logo' },
  { id: 'home', label: 'Home Icon' },
  { id: 'windows', label: 'Windows Icon' },
  { id: 'locks', label: 'Locks Icon' },
  { id: 'runningboards', label: 'Running Boards Icon' }
];

const HEADER_LOGO_OPTIONS = [
  { id: 'bronco', label: 'Bronco Logo' },
  { id: 'home', label: 'Home Icon' },
  { id: 'windows', label: 'Windows Icon' },
  { id: 'locks', label: 'Locks Icon' },
  { id: 'runningboards', label: 'Running Boards Icon' },
  { id: 'none', label: 'Hide Logo' }
];

const DEFAULT_HEADER = {
  title: 'Bronco Controls',
  subtitle: 'Web Configurator',
  show_logo: true,
  logo_variant: 'bronco'
};

const PAGE_LAYOUT_FIELDS = new Set(['rows', 'cols']);
const BUTTON_LAYOUT_FIELDS = new Set(['row', 'col', 'row_span', 'col_span']);

let config = null;
let scanResults = [];

async function loadConfig() {
  try {
    const res = await fetch('/api/config');
    config = await res.json();
    ensureHeader();
    renderWifi();
    renderHeaderSettings();
    renderPages();
    showToast('Configuration loaded');
  } catch (err) {
    showToast('Failed to load config', 'error');
    console.error(err);
  }
}

function renderWifi() {
  if (!config) return;
  const { ap, sta } = config.wifi;
  document.getElementById('ap-enabled').checked = ap.enabled;
  document.getElementById('ap-ssid').value = ap.ssid || '';
  document.getElementById('ap-password').value = ap.password || '';
  document.getElementById('sta-enabled').checked = sta.enabled;
  document.getElementById('sta-ssid').value = sta.ssid || '';
  document.getElementById('sta-password').value = sta.password || '';
  const quickSsid = document.getElementById('quick-ssid');
  const quickPass = document.getElementById('quick-password');
  if (quickSsid) quickSsid.value = sta.ssid || '';
  if (quickPass) quickPass.value = sta.password || '';
}

function renderPages() {
  if (!config) return;
  const container = document.getElementById('pages');
  if (!config.pages) config.pages = [];
  if (!container) return;
  if (config.pages.length === 0) {
    container.innerHTML = '<p>No pages yet. Tap "Add Page" to get started.</p>';
    return;
  }
  container.innerHTML = config.pages.map((page, pageIndex) => renderPageCard(page, pageIndex)).join('');
}

function ensureHeader() {
  if (!config) return;
  config.header = { ...DEFAULT_HEADER, ...(config.header || {}) };
}

function renderHeaderSettings() {
  if (!config) return;
  ensureHeader();
  const titleInput = document.getElementById('header-title');
  if (!titleInput) return;
  titleInput.value = config.header.title || '';
  const subtitleInput = document.getElementById('header-subtitle');
  if (subtitleInput) subtitleInput.value = config.header.subtitle || '';
  const showLogo = document.getElementById('header-show-logo');
  if (showLogo) showLogo.checked = config.header.show_logo !== false;
  const logoSelect = document.getElementById('header-logo');
  if (logoSelect) {
    if (!logoSelect.dataset.initialized) {
      logoSelect.innerHTML = renderLogoOptionsMarkup();
      logoSelect.dataset.initialized = 'true';
    }
    logoSelect.value = config.header.logo_variant || 'bronco';
  }
}

function renderScanResults() {
  const container = document.getElementById('scan-results');
  if (!container) return;
  if (!scanResults.length) {
    container.innerHTML = '<p class="helper-text">No networks discovered yet. Tap "Scan Networks" to refresh.</p>';
    return;
  }
  container.innerHTML = scanResults.map((net, index) => {
    const ssid = net.ssid && net.ssid.length ? net.ssid : '(hidden)';
    const security = net.secure ? (net.auth ? net.auth.toUpperCase() : 'SECURED') : 'OPEN';
    const rssi = typeof net.rssi === 'number' ? `${net.rssi} dBm` : '—';
    return `
      <div class="network-card">
        <div>
          <h4>${escapeHtml(ssid)}</h4>
          <div class="meta">
            <span>${escapeHtml(security)}</span>
            <span class="signal">${escapeHtml(rssi)}</span>
          </div>
        </div>
        <div class="actions">
          <button class="ghost" onclick="prefillFromScan(${index})">Use SSID</button>
        </div>
      </div>
    `;
  }).join('');
}

function renderPageCard(page, index) {
  const buttons = page.buttons || [];
  const buttonHtml = buttons.map((btn, btnIndex) => renderButtonCard(page, index, btn, btnIndex)).join('');
  return `
    <div class="page-card" data-page-index="${index}">
      <div class="page-header">
        <div>
          <div class="badge" data-role="page-id">${escapeHtml(page.id || ('page_' + index))}</div>
          <h3 data-role="page-title">${escapeHtml(page.name || 'Untitled Page')}</h3>
        </div>
        <div class="actions">
          <button class="ghost" onclick="removePage(${index})">Remove</button>
        </div>
      </div>
      <div class="grid two-col">
        <div class="field">
          <label>Title</label>
          <input type="text" value="${escapeAttr(page.name)}" oninput="updatePageField(${index}, 'name', this.value)" />
        </div>
        <div class="field">
          <label>Internal ID</label>
          <input type="text" value="${escapeAttr(page.id)}" oninput="updatePageField(${index}, 'id', this.value)" />
        </div>
        <div class="field">
          <label>Grid Rows</label>
          <input type="number" min="1" max="4" value="${page.rows || 2}" oninput="updatePageField(${index}, 'rows', parseInt(this.value) || 1)" />
        </div>
        <div class="field">
          <label>Grid Columns</label>
          <input type="number" min="1" max="4" value="${page.cols || 2}" oninput="updatePageField(${index}, 'cols', parseInt(this.value) || 1)" />
        </div>
      </div>
      <div class="button-grid">
        ${buttonHtml || '<div class="badge">No buttons on this page yet.</div>'}
      </div>
      <button class="ghost full" onclick="addButton(${index})">Add Button</button>
    </div>
  `;
}

function renderLogoOptionsMarkup() {
  return HEADER_LOGO_OPTIONS.map(({ id, label }) => `<option value="${id}">${label}</option>`).join('');
}

function renderIconOptions(selected) {
  return ICON_OPTIONS.map(({ id, label }) => `<option value="${id}" ${selected === id ? 'selected' : ''}>${label}</option>`).join('');
}

function updateHeaderField(field, value) {
  if (!config) return;
  ensureHeader();
  config.header[field] = value;
}

function toggleHeaderField(field, checked) {
  if (!config) return;
  ensureHeader();
  config.header[field] = checked;
}

function renderButtonCard(page, pageIndex, button, btnIndex) {
  const can = button.can || { enabled: false, pgn: 0, priority: 3, source_address: 0x80, destination_address: 0xFF, data: new Array(8).fill(0) };
  const dataFields = (can.data || new Array(8).fill(0)).map((value, i) => `
      <input type="number" min="0" max="255" value="${value}" oninput="updateCanData(${pageIndex}, ${btnIndex}, ${i}, this.value)" />
  `).join('');
  return `
    <div class="button-card" data-page-index="${pageIndex}" data-button-index="${btnIndex}">
      <h4 data-role="button-title">${escapeHtml(button.label || 'Button')}</h4>
      <div class="grid two-col">
        <div class="field">
          <label>Label</label>
          <input type="text" value="${escapeAttr(button.label)}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'label', this.value)" />
        </div>
        <div class="field">
          <label>Color (#RRGGBB)</label>
          <input type="text" value="${escapeAttr(button.color || '#FFA500')}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'color', this.value)" />
        </div>
        <div class="field">
          <label>Icon Overlay</label>
          <select onchange="updateButtonField(${pageIndex}, ${btnIndex}, 'icon', this.value)">
            ${renderIconOptions(button.icon || '')}
          </select>
          <p class="helper-text small">Optional graphic that sits atop the button color.</p>
        </div>
        <div class="field">
          <label>Row</label>
          <input type="number" min="0" max="${Math.max(0, (page.rows || 2) - 1)}" value="${button.row || 0}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'row', parseInt(this.value) || 0)" />
        </div>
        <div class="field">
          <label>Column</label>
          <input type="number" min="0" max="${Math.max(0, (page.cols || 2) - 1)}" value="${button.col || 0}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'col', parseInt(this.value) || 0)" />
        </div>
        <div class="field">
          <label>Row Span</label>
          <input type="number" min="1" max="${page.rows || 2}" value="${button.row_span || 1}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'row_span', parseInt(this.value) || 1)" />
        </div>
        <div class="field">
          <label>Col Span</label>
          <input type="number" min="1" max="${page.cols || 2}" value="${button.col_span || 1}" oninput="updateButtonField(${pageIndex}, ${btnIndex}, 'col_span', parseInt(this.value) || 1)" />
        </div>
      </div>
      <div class="switch-row">
        <input type="checkbox" ${button.momentary ? 'checked' : ''} onchange="updateButtonField(${pageIndex}, ${btnIndex}, 'momentary', this.checked)" />
        <span>Momentary (hold to send)</span>
      </div>
      <details>
        <summary>CAN Frame</summary>
        <div class="grid two-col" style="margin-top:12px;">
          <div class="field">
            <label>Enable CAN</label>
            <div class="switch-row">
              <input type="checkbox" ${can.enabled ? 'checked' : ''} onchange="toggleCan(${pageIndex}, ${btnIndex}, this.checked)" />
              <span>Send frame when tapped</span>
            </div>
          </div>
          <div class="field">
            <label>PGN</label>
            <input type="number" min="0" max="0x3FFFF" value="${can.pgn || 0}" oninput="updateCanField(${pageIndex}, ${btnIndex}, 'pgn', parseInt(this.value) || 0)" />
          </div>
          <div class="field">
            <label>Priority</label>
            <input type="number" min="0" max="7" value="${can.priority || 3}" oninput="updateCanField(${pageIndex}, ${btnIndex}, 'priority', parseInt(this.value) || 0)" />
          </div>
          <div class="field">
            <label>Source Address</label>
            <input type="number" min="0" max="255" value="${can.source_address || 0}" oninput="updateCanField(${pageIndex}, ${btnIndex}, 'source_address', parseInt(this.value) || 0)" />
          </div>
          <div class="field">
            <label>Destination Address</label>
            <input type="number" min="0" max="255" value="${can.destination_address || 0}" oninput="updateCanField(${pageIndex}, ${btnIndex}, 'destination_address', parseInt(this.value) || 0)" />
          </div>
          <div class="field">
            <label>Data Bytes</label>
            <div class="grid two-col" style="grid-template-columns: repeat(8, minmax(50px,1fr));">
              ${dataFields}
            </div>
          </div>
        </div>
      </details>
      <button class="ghost full" onclick="removeButton(${pageIndex}, ${btnIndex})">Remove Button</button>
    </div>
  `;
}

async function scanNetworks() {
  const container = document.getElementById('scan-results');
  if (container) {
    container.innerHTML = '<p class="helper-text">Scanning nearby networks...</p>';
  }
  try {
    const res = await fetch('/api/wifi/scan');
    const payload = await res.json();
    if (!res.ok) throw new Error(payload.message || 'Scan failed');
    scanResults = payload.networks || [];
    renderScanResults();
    const count = scanResults.length;
    showToast(`Found ${count} network${count === 1 ? '' : 's'}`);
  } catch (err) {
    showToast(err.message || 'Scan failed', 'error');
  }
}

function prefillFromScan(index) {
  const net = scanResults[index];
  if (!net) return;
  const ssid = net.ssid || '';
  const staEnabled = document.getElementById('sta-enabled');
  if (staEnabled) staEnabled.checked = true;
  updateWifi('sta', 'enabled', true);
  updateWifi('sta', 'ssid', ssid);
  const staSsidInput = document.getElementById('sta-ssid');
  if (staSsidInput) staSsidInput.value = ssid;
  const quickSsid = document.getElementById('quick-ssid');
  if (quickSsid) quickSsid.value = ssid;
  const staPassInput = document.getElementById('sta-password');
  const quickPass = document.getElementById('quick-password');
  if (!net.secure) {
    if (staPassInput) staPassInput.value = '';
    if (quickPass) quickPass.value = '';
  }
  showToast(`Ready to join ${ssid || 'hidden SSID'}. Enter password if needed, then tap "Join Home WiFi".`);
}

function updateWifi(section, field, value) {
  if (!config) return;
  config.wifi = config.wifi || { ap: {}, sta: {} };
  config.wifi[section] = config.wifi[section] || {};
  config.wifi[section][field] = value;
}

function updatePageField(index, field, value) {
  if (!config || !config.pages[index]) return;
  config.pages[index][field] = value;
  if (PAGE_LAYOUT_FIELDS.has(field)) {
    renderPages();
    return;
  }
  updatePagePreviewDom(index, field, value);
}

function updateButtonField(pageIndex, btnIndex, field, value) {
  if (!config || !config.pages[pageIndex]) return;
  config.pages[pageIndex].buttons = config.pages[pageIndex].buttons || [];
  if (!config.pages[pageIndex].buttons[btnIndex]) return;
  config.pages[pageIndex].buttons[btnIndex][field] = value;
  if (BUTTON_LAYOUT_FIELDS.has(field)) {
    renderPages();
    return;
  }
  updateButtonPreviewDom(pageIndex, btnIndex, field, value);
}

function updatePagePreviewDom(pageIndex, field, value) {
  const card = document.querySelector(`.page-card[data-page-index="${pageIndex}"]`);
  if (!card) return;
  if (field === 'name') {
    const pageTitle = card.querySelector('[data-role="page-title"]');
    if (pageTitle) pageTitle.textContent = value || 'Untitled Page';
  } else if (field === 'id') {
    const pageId = card.querySelector('[data-role="page-id"]');
    if (pageId) pageId.textContent = value || `page_${pageIndex}`;
  }
}

function updateButtonPreviewDom(pageIndex, btnIndex, field, value) {
  const selector = `.button-card[data-page-index="${pageIndex}"][data-button-index="${btnIndex}"]`;
  const card = document.querySelector(selector);
  if (!card) return;
  if (field === 'label') {
    const buttonTitle = card.querySelector('[data-role="button-title"]');
    if (buttonTitle) buttonTitle.textContent = value || 'Button';
  }
}

function toggleCan(pageIndex, btnIndex, enabled) {
  if (!config || !config.pages[pageIndex]) return;
  const button = config.pages[pageIndex].buttons[btnIndex];
  button.can = button.can || { data: new Array(8).fill(0) };
  button.can.enabled = enabled;
}

function updateCanField(pageIndex, btnIndex, field, value) {
  if (!config || !config.pages[pageIndex]) return;
  const button = config.pages[pageIndex].buttons[btnIndex];
  button.can = button.can || { data: new Array(8).fill(0) };
  button.can[field] = value;
}

function updateCanData(pageIndex, btnIndex, byteIndex, value) {
  if (!config || !config.pages[pageIndex]) return;
  const button = config.pages[pageIndex].buttons[btnIndex];
  button.can = button.can || { data: new Array(8).fill(0) };
  button.can.data = button.can.data || new Array(8).fill(0);
  button.can.data[byteIndex] = Math.min(255, Math.max(0, parseInt(value) || 0));
}

function addPage() {
  if (!config) return;
  config.pages = config.pages || [];
  if (config.pages.length >= 20) {
    return showToast('Reached page limit (20)', 'error');
  }
  config.pages.push({
    id: `page_${config.pages.length}`,
    name: 'New Page',
    rows: 2,
    cols: 2,
    buttons: []
  });
  renderPages();
}

function removePage(index) {
  if (!config || !config.pages[index]) return;
  config.pages.splice(index, 1);
  renderPages();
}

function addButton(pageIndex) {
  if (!config || !config.pages[pageIndex]) return;
  const page = config.pages[pageIndex];
  page.buttons = page.buttons || [];
  if (page.buttons.length >= 12) {
    return showToast('Max 12 buttons per page', 'error');
  }
  page.buttons.push({
    id: `btn_${page.buttons.length}`,
    label: 'New Button',
    color: '#FFA500',
    row: 0,
    col: 0,
    row_span: 1,
    col_span: 1,
    momentary: false,
    can: { enabled: false, pgn: 0, priority: 3, source_address: 0x80, destination_address: 0xFF, data: new Array(8).fill(0) }
  });
  renderPages();
}

function removeButton(pageIndex, btnIndex) {
  if (!config || !config.pages[pageIndex]) return;
  config.pages[pageIndex].buttons.splice(btnIndex, 1);
  renderPages();
}

async function saveConfig() {
  if (!config) return;
  try {
    const res = await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(config)
    });
    const payload = await parseJsonSafe(res);
    if (!res.ok) throw new Error(payload.message || `Validation failed (${res.status})`);
    showToast(payload.message || 'Saved! UI will refresh.');
  } catch (err) {
    showToast(err.message || 'Save failed', 'error');
    console.error(err);
  }
}

function exportConfig() {
  if (!config) return;
  const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
  const link = document.createElement('a');
  link.href = URL.createObjectURL(blob);
  link.download = 'bronco_config.json';
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

function importConfig(event) {
  const file = event.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = () => {
    try {
      config = JSON.parse(reader.result);
      renderWifi();
      renderHeaderSettings();
      renderPages();
      showToast('Imported draft config. Save to apply.');
    } catch (err) {
      showToast('Invalid JSON file', 'error');
    }
  };
  reader.readAsText(file);
}

async function refreshStatus() {
  try {
    const res = await fetch('/api/status');
    const status = await res.json();
    const statusContainer = document.getElementById('status');
    statusContainer.innerHTML = `
      <div class="status-chip"><span>AP IP</span>${status.ap_ip || 'N/A'}</div>
      <div class="status-chip"><span>Station IP</span>${status.sta_ip || '—'}</div>
      <div class="status-chip"><span>Station</span>${status.sta_connected ? 'Connected' : 'Idle'}</div>
      <div class="status-chip"><span>Uptime</span>${Math.floor((status.uptime_ms || 0) / 1000)}s</div>
      <div class="status-chip"><span>Heap</span>${status.heap || 0} bytes</div>
    `;
  } catch (_) {
    // ignore
  }
}

async function joinHomeWifi() {
  if (!config) return;
  const ssid = document.getElementById('quick-ssid').value.trim();
  const password = document.getElementById('quick-password').value;
  if (!ssid) {
    showToast('Enter an SSID to join', 'error');
    return;
  }
  try {
    const res = await fetch('/api/wifi/connect', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password, persist: true })
    });
    const payload = await res.json();
    if (!res.ok) throw new Error(payload.message || 'Join failed');
    updateWifi('sta', 'enabled', true);
    updateWifi('sta', 'ssid', ssid);
    updateWifi('sta', 'password', password);
    document.getElementById('sta-enabled').checked = true;
    document.getElementById('sta-ssid').value = ssid;
    document.getElementById('sta-password').value = password;
    showToast(`Connecting to ${ssid}...`);
    await loadConfig();
    await refreshStatus();
  } catch (err) {
    showToast(err.message || 'Join failed', 'error');
  }
}

async function forgetSta() {
  if (!config) return;
  updateWifi('sta', 'enabled', false);
  updateWifi('sta', 'ssid', '');
  updateWifi('sta', 'password', '');
  document.getElementById('sta-enabled').checked = false;
  document.getElementById('sta-ssid').value = '';
  document.getElementById('sta-password').value = '';
  const quickSsid = document.getElementById('quick-ssid');
  const quickPass = document.getElementById('quick-password');
  if (quickSsid) quickSsid.value = '';
  if (quickPass) quickPass.value = '';
  await saveConfig();
  await refreshStatus();
  showToast('Station profile cleared');
}

function showToast(message, tone = 'info') {
  const toast = document.getElementById('toast');
  toast.textContent = message;
  const color = tone === 'error' ? getCssVar('--danger') : getCssVar('--border');
  toast.style.borderColor = color.trim() || '#ffffff';
  toast.classList.add('show');
  setTimeout(() => toast.classList.remove('show'), 3200);
}

function getCssVar(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name) || '';
}

async function parseJsonSafe(res) {
  const text = await res.text();
  if (!text) {
    return {};
  }
  try {
    return JSON.parse(text);
  } catch (err) {
    console.warn('Invalid JSON response', err, text);
    return {};
  }
}

function escapeHtml(value) {
  if (value == null) return '';
  return value.toString().replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

function escapeAttr(value) {
  return escapeHtml(value || '');
}

document.addEventListener('DOMContentLoaded', () => {
  renderScanResults();
  loadConfig();
  refreshStatus();
  setInterval(refreshStatus, 5000);
});
</script>
</body>
</html>
)rawliteral";
