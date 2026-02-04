#pragma once

const char WEB_INTERFACE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>CAN controls configurator</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@400;600;700&display=swap');
:root {
	--bg: #0b0c10;
	--panel: #12141c;
	--surface: #191c26;
	--accent: #ff9d2e;
	--accent-2: #7ad7f0;
	--text: #f2f4f8;
	--muted: #8d92a3;
	--border: #20232f;
	--success: #3dd598;
	--danger: #ff6b6b;
}
* { box-sizing: border-box; }
body {
	margin: 0;
	font-family: 'Space Grotesk', 'Segoe UI', sans-serif;
	background: radial-gradient(circle at 10% 10%, rgba(255,157,46,0.12), transparent 40%),
							radial-gradient(circle at 90% 10%, rgba(122,215,240,0.12), transparent 40%),
							var(--bg);
	color: var(--text);
	min-height: 100vh;
}
.hero {
	padding: 32px 4vw 16px;
	border-bottom: 1px solid var(--border);
	display: flex;
	justify-content: space-between;
	align-items: flex-start;
	gap: 12px;
	flex-wrap: wrap;
}
.hero h1 { margin: 0; letter-spacing: 0.08em; font-size: 2rem; line-height: 1.1; max-width: min(460px, 70vw); overflow-wrap: anywhere; }
.hero p { margin: 6px 0 0; color: var(--muted); }
.version { font-size: 0.75rem; color: var(--muted); opacity: 0.7; margin-top: 4px; }
.container { padding: 0 4vw 48px; }
.tabs { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 16px; }
.tab-btn {
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--muted);
	padding: 10px 14px;
	border-radius: 12px;
	cursor: pointer;
	transition: all .15s ease;
	font-weight: 600;
}
.tab-btn.active { background: var(--accent); color: #16110a; border-color: var(--accent); }
.tab-btn:hover { border-color: var(--accent); color: var(--text); }
.status-banner { display: none; padding: 12px 14px; border-radius: 12px; margin-bottom: 16px; }
.status-success { background: rgba(61,213,152,0.12); border: 1px solid var(--success); color: var(--success); }
.status-error { background: rgba(255,107,107,0.12); border: 1px solid var(--danger); color: var(--danger); }
.layout { display: grid; gap: 16px; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); }
.card {
	background: var(--panel);
	border: 1px solid var(--border);
	border-radius: 18px;
	padding: 18px 18px 14px;
	box-shadow: 0 18px 50px rgba(0,0,0,0.35);
}
.card h3 { margin: 0 0 10px; font-size: 1.05rem; display: flex; align-items: center; gap: 8px; letter-spacing: 0.03em; }
.card h4 { margin: 16px 0 8px; color: var(--muted); font-size: 0.9rem; letter-spacing: 0.02em; }
.muted { color: var(--muted); font-size: 0.9rem; }
.grid { display: grid; gap: 12px; }
.two-col { grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); }
.three-col { grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); }
label { color: var(--muted); font-size: 0.85rem; letter-spacing: 0.01em; }
input, select, textarea {
	width: 100%;
	padding: 10px 12px;
	border-radius: 12px;
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	font-size: 0.95rem;
}
input[type=color] { padding: 6px; height: 46px; }
input:focus, select:focus, textarea:focus { outline: 2px solid var(--accent); border-color: var(--accent); }
.row { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }
.row > * { flex: 1; }
.btn {
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	padding: 10px 14px;
	border-radius: 12px;
	cursor: pointer;
	font-weight: 700;
	transition: all .15s ease;
}
.btn.primary { background: var(--accent); color: #16110a; border-color: var(--accent); }
.btn.ghost { background: transparent; }
.btn.danger { background: var(--danger); border-color: var(--danger); color: #fff; }
.btn.small { padding: 8px 12px; font-size: 0.9rem; }
.pill { padding: 4px 10px; border-radius: 999px; border: 1px solid var(--border); color: var(--muted); font-size: 0.8rem; }
.pill.success { border-color: var(--success); color: var(--success); }
.pill.warn { border-color: var(--accent); color: var(--accent); }
.tab { display: none; }
.tab.active { display: block; animation: fade .25s ease; }
@keyframes fade { from { opacity: 0; transform: translateY(6px); } to { opacity: 1; transform: translateY(0); } }
.panel-split { display: grid; gap: 16px; grid-template-columns: minmax(320px, 380px) 1fr; }
.page-list { display: flex; flex-direction: column; gap: 8px; }
.page-chip { padding: 12px; border: 1px solid var(--border); border-radius: 14px; background: var(--surface); display: flex; justify-content: space-between; align-items: center; cursor: grab; }
.page-chip.active { border-color: var(--accent); box-shadow: 0 0 0 1px rgba(255,157,46,0.4); }
.page-chip .name { font-weight: 600; }
.preview-shell { border: 1px solid var(--border); border-radius: 18px; background: linear-gradient(135deg, rgba(255,157,46,0.05), rgba(122,215,240,0.05)); padding: 14px; }
.device-preview { border-radius: 16px; overflow: hidden; border: 1px solid var(--border); background: var(--bg); box-shadow: inset 0 0 0 1px rgba(255,255,255,0.02); padding-top: 8px; box-sizing: border-box; }

/* Revert to original hero header styling */
.hero {
	padding: 56px 4vw 16px; /* Increased top padding to prevent title from running off */
	border-bottom: 1px solid var(--border);
	display: flex;
	justify-content: space-between;
	align-items: flex-end; /* Ensure content is aligned to the bottom of the header area */
	gap: 12px;
	flex-wrap: wrap;
}
.hero h1 {
	margin: 0;
	letter-spacing: 0.08em;
	font-size: 2rem;
	line-height: 1.1;
	max-width: min(460px, 70vw);
	overflow-wrap: anywhere;
}
.hero p {
	margin: 6px 0 0;
	color: var(--muted);
}
.preview-header { display: flex; padding: 14px; gap: 12px; align-items: center; border-bottom: 2px solid transparent; margin-bottom: 12px; }
.preview-header.stacked { flex-direction: column; align-items: flex-start; }
.preview-header.inline-left { flex-direction: row; }
.preview-header.inline-right { flex-direction: row-reverse; }
.preview-header .title-wrap { flex: 1; display: flex; flex-direction: column; gap: 4px; min-width: 0; width: 100%; }
.preview-header .title-wrap > * { word-break: break-word; overflow-wrap: anywhere; }
.preview-logo { width: auto; height: 48px; max-width: 160px; border-radius: 10px; background: transparent; flex-shrink: 0; transition: height 0.2s ease, width 0.2s ease; border: 1px dashed var(--border); overflow: hidden; cursor: move; position: relative; resize: both; }
.preview-logo:hover { border: 2px dashed var(--accent); box-shadow: 0 0 8px rgba(255, 157, 46, 0.3); }
.preview-logo img { width: 100%; height: 100%; object-fit: contain; display: block; border-radius: inherit; image-rendering: pixelated; pointer-events: none; }
.preview-header .title-wrap { cursor: move; transition: opacity 0.2s; }
.preview-header .title-wrap:hover { opacity: 0.8; }
.row.align-center { align-items: center; gap: 12px; }
.preview-nav { display: flex; gap: 8px; padding: 10px 12px; flex-wrap: wrap; background: var(--panel); margin-bottom: 12px; }
.preview-nav .pill {
	cursor: grab;
	white-space: normal;
	word-break: break-word;
	line-height: 1.2;
	max-width: 100%;
}
.status-grid {
	display: grid;
	gap: 10px;
	margin-top: 12px;
	grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
}
.status-chip {
	padding: 10px 12px;
	border-radius: 12px;
	border: 1px solid var(--border);
	background: var(--surface);
	display: flex;
	justify-content: space-between;
	align-items: center;
	gap: 8px;
	font-size: 0.9rem;
}
.status-chip span {
	color: var(--muted);
	font-size: 0.75rem;
	text-transform: uppercase;
	letter-spacing: 0.08em;
}
.preview-body { padding: 14px; min-height: 220px; }
.preview-grid { display: grid; gap: 10px; }
.preview-btn { padding: 14px 12px; border-radius: 12px; font-weight: 700; text-align: center; border: 1px solid transparent; cursor: pointer; word-break: break-word; overflow-wrap: anywhere; }
.preview-btn.empty { border: 1px dashed var(--border); background: rgba(255,255,255,0.03); color: var(--muted); }
.quick-edit { display: flex; flex-wrap: wrap; gap: 10px; margin-bottom: 10px; align-items: center; }
.quick-edit .field { display: inline-flex; align-items: center; gap: 6px; }
.quick-edit input[type="color"] { width: 46px; padding: 0; }
.quick-edit input[type="text"] { width: 150px; }
.builder-grid { border: 1px dashed var(--border); border-radius: 14px; padding: 12px; background: rgba(255,255,255,0.02); }
.grid-cell { border: 1px dashed var(--border); border-radius: 10px; min-height: 72px; display: flex; align-items: center; justify-content: center; color: var(--muted); cursor: pointer; transition: all .12s ease; }
.grid-cell:hover { border-color: var(--accent); color: var(--accent); background: rgba(255,157,46,0.06); }
.grid-btn { width: 100%; height: 100%; border-radius: 12px; padding: 10px; border: 1px solid var(--border); display: flex; align-items: center; justify-content: center; cursor: grab; word-break: break-word; text-align: center; }
.floating-bar { position: sticky; bottom: 0; margin-top: 18px; padding: 12px; background: rgba(11,12,16,0.7); backdrop-filter: blur(8px); border: 1px solid var(--border); border-radius: 14px; display: flex; justify-content: space-between; align-items: center; gap: 12px; box-shadow: 0 20px 40px rgba(0,0,0,0.35); }
.modal { position: fixed; inset: 0; display: none; background: rgba(0,0,0,0.6); align-items: center; justify-content: center; padding: 24px; z-index: 20; }
.modal.open { display: flex; }
.modal-content { background: var(--panel); border: 1px solid var(--border); border-radius: 16px; padding: 18px; width: min(680px, 96vw); max-height: 90vh; overflow: auto; box-shadow: 0 24px 60px rgba(0,0,0,0.45); }
.modal-head { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
.wifi-list { margin-top: 10px; display: grid; gap: 8px; }
.wifi-item { padding: 12px; border: 1px solid var(--border); border-radius: 12px; background: var(--surface); display: flex; justify-content: space-between; cursor: pointer; }
.wifi-item.active { border-color: var(--accent); }
.can-card { border: 1px solid var(--border); border-radius: 12px; padding: 12px; background: var(--surface); }
</style>
</head>
<body>
<script>
// Define tab switching early so tabs work even if later scripts fail
function switchTab(tabName){
	document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));
	document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
	const btn=document.querySelector(`[data-tab="${tabName}"]`);
	const tab=document.getElementById(`tab-${tabName}`);
	if(btn) btn.classList.add('active');
	if(tab) tab.classList.add('active');
}
</script>
<div class="hero">
	<div>
		<h1>CAN controls configurator</h1>
		<p>Build, preview, and save the exact UI layout before flashing.</p>
		<div class="version">Firmware v{{VERSION}}</div>
	</div>
	<div class="pill">Live builder</div>
</div>
<div class="container">
	<div class="tabs">
		<button class="tab-btn active" data-tab="wifi" onclick="switchTab('wifi')">WiFi</button>
		<button class="tab-btn" data-tab="builder" onclick="switchTab('builder')">Interface Builder</button>
		<button class="tab-btn" data-tab="settings" onclick="switchTab('settings')">Settings</button>
	</div>
	<div id="status-banner" class="status-banner"></div>

	<section id="tab-wifi" class="tab active">
		<div class="layout">
			<div class="card">
				<h3>Access Point</h3>
				<div class="muted">Always-on local network to reach the configurator.</div>
				<div class="grid">
					<div>
						<label>SSID</label>
						<input id="ap-ssid" type="text" placeholder="CAN-Control" />
					</div>
					<div>
						<label>Password (8+ chars)</label>
						<input id="ap-password" type="password" placeholder="********" />
					</div>
					<div class="row">
						<label><input id="ap-enabled" type="checkbox" /> Enable AP</label>
					</div>
				</div>
			</div>
			<div class="card">
				<h3>Join Existing WiFi</h3>
				<div class="row">
					<button class="btn" onclick="scanWiFi()" id="scan-btn">Scan</button>
					<span class="pill warn">Choose, then save</span>
				</div>
				<div class="wifi-list" id="wifi-results"></div>
				<div class="grid">
					<div>
						<label>SSID</label>
						<input id="sta-ssid" type="text" placeholder="Your WiFi" />
					</div>
					<div>
						<label>Password</label>
						<input id="sta-password" type="password" placeholder="********" />
					</div>
					<div class="row">
						<label><input id="sta-enabled" type="checkbox" /> Connect on boot</label>
					</div>
				</div>
			</div>
		</div>
		<div class="card">
			<h3>Device Info & Updates</h3>
			<div class="muted">OTA firmware updates via .bin files from GitHub.</div>
			<div class="status-grid" id="status" data-version="{{VERSION}}">
				<div class="status-chip"><span>Firmware</span>v{{VERSION}}</div>
				<div class="status-chip"><span>Available Update</span><span id="update-available">Tap check</span></div>
				<div class="status-chip"><span>Device IP</span>—</div>
				<div class="status-chip"><span>Connected Network</span>—</div>
				<div class="status-chip"><span>AP IP</span>—</div>
				<div class="status-chip"><span>Station IP</span>—</div>
			</div>
			<div class="row" style="margin-top: 12px; gap: 8px;">
				<button class="btn" onclick="checkForUpdates()">Check Updates</button>
			</div>
			<div id="update-section" class="row" style="margin-top: 8px;">
				<!-- Version selector will appear here after checking -->
			</div>
			<div class="row" style="margin-top: 8px; gap: 8px;">
				<a class="btn ghost" href="/suspension" target="_blank" rel="noopener">Open Suspension Interface</a>
			</div>
		</div>
	</section>

	<section id="tab-builder" class="tab">
		<div class="panel-split">
			<div class="card">
				<h3>Branding & Header</h3>
				<div class="grid">
					<div class="row">
						<label>Title</label>
						<input id="header-title-input" type="text" placeholder="CAN Control" oninput="updateHeaderFromInputs()" />
					</div>
					<div class="row">
						<label>Subtitle</label>
						<input id="header-subtitle-input" type="text" placeholder="Configuration Interface" oninput="updateHeaderFromInputs()" />
					</div>
					<div class="row">
						<label>Title Font</label>
						<select id="header-title-font" onchange="updateHeaderFromInputs()"></select>
						<label>Subtitle Font</label>
						<select id="header-subtitle-font" onchange="updateHeaderFromInputs()"></select>
					</div>
					<div class="row">
						<label>Text Alignment</label>
						<select id="header-title-align" onchange="updateHeaderFromInputs()">
							<option value="left">Left</option>
							<option value="center">Center</option>
							<option value="right">Right</option>
						</select>
					</div>
					<div class="row">
						<label>Logo Placement</label>
						<select id="header-logo-position" onchange="updateHeaderFromInputs()">
							<option value="stacked">Above title</option>
							<option value="inline-left">Left of text</option>
							<option value="inline-right">Right of text</option>
						</select>
					</div>
					<div class="row align-center">
						<label>Logo Display Size</label>
						<input id="header-logo-size" type="range" min="24" max="96" value="64" oninput="handleLogoSizeInput(this.value)" />
						<span class="muted" id="header-logo-size-label">64px</span>
					</div>
					<div class="row">
						<label><input id="header-logo-keep-aspect" type="checkbox" checked onchange="updateHeaderFromInputs()" /> Maintain proportions</label>
					</div>
					<div class="row align-center">
						<label>Header ↔ Nav Gap</label>
						<input id="header-nav-spacing" type="range" min="0" max="48" value="12" oninput="handleNavSpacingInput(this.value)" />
						<span class="muted" id="header-nav-spacing-label">12px</span>
					</div>
					<!-- Logo upload moved to Image Assets section -->
				</div>
				<h4>Header Appearance</h4>
				<div class="grid two-col">
					<div><label>Header BG</label><input id="theme-surface" type="color" /></div>
					<div><label>Title Color</label><input id="theme-text-primary" type="color" /></div>
					<div><label>Subtitle Color</label><input id="theme-text-secondary" type="color" /></div>
					<div><label>Header Border Color</label><input id="theme-header-border" type="color" /></div>
					<div><label>Header Border Width</label><input id="theme-header-border-width" type="number" min="0" max="10" /></div>
				</div>
				<h4>Window Baseline (Template)</h4>
				<p class="muted">Define default styling that can be applied to windows</p>
				<div class="grid two-col">
					<div><label>Page BG</label><input id="theme-page-bg" type="color" /></div>
					<div><label>Text Color</label><input id="theme-text-color" type="color" /></div>
					<div><label>Nav Active</label><input id="theme-nav-active" type="color" /></div>
					<div><label>Nav Inactive</label><input id="theme-nav-button" type="color" /></div>
					<div><label>Nav Text</label><input id="theme-nav-text" type="color" /></div>
					<div><label>Nav Radius</label><input id="theme-nav-radius" type="number" min="0" max="50" /></div>
					<div><label>Button Fill</label><input id="theme-button-color" type="color" /></div>
					<div><label>Button Pressed</label><input id="theme-button-pressed" type="color" /></div>
					<div><label>Button Border</label><input id="theme-border" type="color" /></div>
					<div><label>Button Border Width</label><input id="theme-border-width" type="number" min="0" max="10" /></div>
					<div><label>Button Radius</label><input id="theme-radius" type="number" min="0" max="50" /></div>
				</div>
			</div>

			<div class="preview-shell">
				<div class="quick-edit">
					<div class="field"><label>Window</label><select id="quick-page-select" onchange="quickPageSelectChanged()"></select></div>
					<button class="btn small" onclick="addPage()">Add</button>
					<button class="btn small" onclick="deletePage()">Delete</button>
					<div class="field" style="min-width:140px;"><label>Grid</label>
						<select id="page-rows" onchange="updateGrid()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select>
						<span style="color:var(--muted);">x</span>
						<select id="page-cols" onchange="updateGrid()"><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select>
					</div>
					<div class="field"><label>Name</label><input id="page-name-input" type="text" oninput="updatePageMeta()" /></div>
					<div class="field"><label>Nav Text</label><input id="page-nav-text-input" type="text" oninput="updatePageMeta()" /></div>
				</div>
				<div class="quick-edit" style="border-top: 1px solid var(--border); padding-top: 10px; margin-top: 0;">
					<h4 style="grid-column: 1/-1; margin: 0 0 8px;">Window Config</h4>
					<div class="field"><label>Page BG</label><input id="page-bg-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Text</label><input id="page-text-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Nav Active</label><input id="page-nav-color" type="color" onchange="updatePageMeta()" /></div>
					<div class="field"><label>Nav Inactive</label><input id="page-nav-inactive-color" type="color" onchange="updatePageMeta()" /></div>
					<div class="field"><label>Nav Text Color</label><input id="nav-text-color" type="color" onchange="updatePageNavStyle()" /></div>
					<div class="field"><label>Nav Radius</label><input id="nav-radius" type="number" min="0" max="50" onchange="updatePageNavStyle()" /></div>
					<div class="field"><label>Button Fill</label><input id="page-btn-color" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Pressed</label><input id="page-btn-pressed" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Border</label><input id="page-btn-border" type="color" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Border W</label><input id="page-btn-border-width" type="number" min="0" max="10" onchange="updatePageStyle()" /></div>
					<div class="field"><label>Radius</label><input id="page-btn-radius" type="number" min="0" max="50" onchange="updatePageStyle()" /></div>
					<button class="btn small" onclick="applyPageStyleToButtons()">Apply to buttons</button>
					<button class="btn small ghost" onclick="capturePageAsBaseline()">Save as baseline</button>
					<button class="btn small ghost" onclick="applyBaselineToPage()">Apply baseline</button>
				</div>
				<div class="device-preview" id="live-preview">
					<div class="preview-header stacked" id="preview-header">
						<div class="preview-logo" id="preview-logo"><img id="preview-logo-img" alt="Header logo preview" /></div>
						<div class="title-wrap">
							<div id="preview-title" style="font-weight: 700; line-height: 1.2;">CAN Control</div>
							<div id="preview-subtitle" class="muted" style="font-size: 0.85rem; line-height: 1.3;">Configuration Interface</div>
						</div>
					</div>
					<div class="preview-nav" id="preview-nav"></div>
					<div class="preview-body" id="preview-body"></div>
				</div>
			</div>
		</div>

		<div class="layout" style="margin-top:16px;">
			<div class="card">
				<h3>🖼️ Image Assets</h3>
				
				<h4>Header Logo</h4>
				<div class="grid">
					<div class="row">
						<input id="header-logo-upload" type="file" accept="image/*" />
						<button class="btn small" onclick="clearImage('header')">Clear</button>
					</div>
					<div id="header-logo-preview" style="display:none; padding:8px; background: var(--surface); border: 1px solid var(--border); border-radius:8px;">
						<img id="header-logo-preview-img" style="max-height:36px; display:block;" />
						<span class="muted" style="font-size:0.75rem;" id="header-logo-size"></span>
					</div>
				</div>

				<h4>Splash Screen Logo (400x300px max)</h4>
				<div class="grid">
					<div class="row">
						<input id="splash-logo-upload" type="file" accept="image/*" />
						<button class="btn small" onclick="clearImage('splash')">Clear</button>
					</div>
					<div id="splash-logo-preview" style="display:none; padding:8px; background: var(--surface); border: 1px solid var(--border); border-radius:8px;">
						<img id="splash-logo-preview-img" style="max-height:100px; display:block;" />
						<span class="muted" style="font-size:0.75rem;" id="splash-logo-size"></span>
					</div>
				</div>

				<h4>Background Image (800x480px)</h4>
				<div class="grid">
					<div class="row">
						<input id="background-upload" type="file" accept="image/*" />
						<button class="btn small" onclick="clearImage('background')">Clear</button>
					</div>
					<div id="background-preview" style="display:none; padding:8px; background: var(--surface); border: 1px solid var(--border); border-radius:8px;">
						<img id="background-preview-img" style="max-height:120px; max-width:100%; display:block;" />
						<span class="muted" style="font-size:0.75rem;" id="background-size"></span>
					</div>
				</div>

				<h4>Sleep Overlay Logo (200x150px max)</h4>
				<div class="grid">
					<div class="row">
						<input id="sleep-logo-upload" type="file" accept="image/*" />
						<button class="btn small" onclick="clearImage('sleep')">Clear</button>
					</div>
					<div id="sleep-logo-preview" style="display:none; padding:8px; background: var(--surface); border: 1px solid var(--border); border-radius:8px;">
						<img id="sleep-logo-preview-img" style="max-height:80px; display:block;" />
						<span class="muted" style="font-size:0.75rem;" id="sleep-logo-size"></span>
					</div>
				</div>
			</div>

			<div class="card">
				<h3>Display & Sleep</h3>
				<div class="grid two-col">
					<div><label>Brightness</label><input id="display-brightness" type="range" min="0" max="100" oninput="document.getElementById('brightness-value').textContent=this.value+'%';" /></div>
					<div class="row" style="justify-content:space-between;"><span class="muted">Value</span><span id="brightness-value">100%</span></div>
					<div class="row"><label><input id="sleep-enabled" type="checkbox" /> Enable Sleep Overlay</label></div>
					<div><label>Sleep Timeout (s)</label><input id="sleep-timeout" type="number" min="5" max="3600" /></div>
				</div>
			</div>
		</div>
	</section>

	<section id="tab-settings" class="tab">
		<div class="layout">
			<div class="card">
				<h3>Diagnostics & Monitoring</h3>
				<div class="muted">Quick access to diagnostic and monitoring tools.</div>
				<div class="row" style="margin-top:10px;">
					<a class="btn primary" href="http://192.168.7.116/can-monitor" target="_blank" rel="noopener">CAN Monitor</a>
					<a class="btn" href="http://192.168.7.116/behavioral" target="_blank" rel="noopener">Behavioral Outputs</a>
				</div>
				<div class="muted" style="margin-top:8px; font-size:0.85rem;">IP shown is placeholder and will be dynamic.</div>
			</div>
		</div>
	</section>

	<div class="floating-bar">
		<div class="muted">Save mirrors what you preview. Reload pulls the current device config.</div>
		<div class="row" style="flex:0 0 auto;">
			<button class="btn" onclick="loadConfig()">Reload</button>
			<button class="btn primary" onclick="saveConfig()">Save</button>
		</div>
	</div>
</div>

<div class="modal" id="button-modal">
	<div class="modal-content">
		<div class="modal-head">
			<h3>Edit Button</h3>
			<button class="btn ghost" onclick="closeModal()">Close</button>
		</div>
		<div class="grid two-col">
			<div><label>Label</label><input id="btn-label" type="text" /></div>
			<div><label>Font Size</label><select id="btn-font-size"><option value="12">12</option><option value="14">14</option><option value="16">16</option><option value="18">18</option><option value="20">20</option><option value="22">22</option><option value="24">24</option><option value="28">28</option><option value="32">32</option></select></div>
			<div><label>Font Family</label><select id="btn-font-family"><option value="montserrat">Montserrat</option><option value="unscii">UNSCII</option></select></div>
			<div><label>Text Align</label><select id="btn-text-align"><option value="center">Center</option><option value="top-left">Top Left</option><option value="top-center">Top Center</option><option value="top-right">Top Right</option><option value="bottom-left">Bottom Left</option><option value="bottom-center">Bottom Center</option><option value="bottom-right">Bottom Right</option></select></div>
			<div><label>Fill</label><input id="btn-color" type="color" /></div>
			<div><label>Pressed</label><input id="btn-pressed-color" type="color" /></div>
			<div><label>Text Color</label><input id="btn-text-color" type="color" /></div>
			<div><label>Border</label><input id="btn-border-color" type="color" /></div>
			<div><label>Border Width</label><input id="btn-border-width" type="number" min="0" max="10" /></div>
			<div><label>Corner Radius</label><input id="btn-corner-radius" type="number" min="0" max="50" /></div>
			<div class="row"><label><input id="btn-momentary" type="checkbox" /> Momentary</label></div>
		</div>
		<h4>Button Action Mode</h4>
		<div class="grid two-col">
			<div style="grid-column:1/-1;">
				<label>Mode</label>
				<select id="btn-mode" onchange="toggleBehavioralFields()">
					<option value="can">CAN Frames (Advanced)</option>
					<option value="output">Behavioral Output</option>
					<option value="scene">Scene (Multiple Outputs)</option>
				</select>
			</div>
			<div class="muted" style="grid-column:1/-1; font-size:0.85rem;">Choose CAN for manual control, Output for single light/function, or Scene for choreographed sequences</div>
		</div>
		
		<div id="behavioral-output-config" style="display:none;">
			<h4>Output Configuration</h4>
			<div class="grid two-col">
				<div style="grid-column:1/-1;"><label>Output</label><select id="btn-output-id"><option value="">Select output...</option></select></div>
				<div style="grid-column:1/-1;"><label>Action</label><select id="btn-output-action">
					<option value="on">On (apply behavior)</option>
					<option value="off">Off</option>
					<option value="toggle">Toggle</option>
				</select></div>
				<div style="grid-column:1/-1;"><label>Behavior</label><select id="btn-behavior-type">
					<option value="steady">Steady</option>
					<option value="flash">Flash</option>
					<option value="pulse">Pulse</option>
					<option value="fade_in">Fade In</option>
					<option value="fade_out">Fade Out</option>
					<option value="strobe">Strobe</option>
					<option value="hold_timed">Hold Timed</option>
					<option value="ramp">Ramp</option>
				</select></div>
				<div><label>Target Value (0-255)</label><input id="btn-target-value" type="number" min="0" max="255" value="255" /></div>
				<div><label>Period (ms)</label><input id="btn-period-ms" type="number" min="50" max="10000" value="1000" /></div>
				<div><label>Duty Cycle (%)</label><input id="btn-duty-cycle" type="number" min="0" max="100" value="50" /></div>
				<div><label>Fade Time (ms)</label><input id="btn-fade-time-ms" type="number" min="0" max="5000" value="500" /></div>
				<div><label>Hold Duration (ms)</label><input id="btn-hold-duration-ms" type="number" min="0" max="30000" value="2000" /></div>
				<div class="row"><label><input id="btn-auto-off" type="checkbox" /> Release to OFF</label></div>
			</div>
		</div>
		
		<div id="behavioral-scene-config" style="display:none;">
			<h4>Scene Selection</h4>
			<div class="grid two-col">
				<div style="grid-column:1/-1;"><label>Scene</label><select id="btn-scene-id"><option value="">Select scene...</option></select></div>
				<div style="grid-column:1/-1;"><label>Action</label><select id="btn-scene-action">
					<option value="on">On</option>
					<option value="off">Off</option>
					<option value="toggle">Toggle</option>
				</select></div>
				<div><label>Duration (ms)</label><input id="btn-scene-duration-ms" type="number" min="0" max="60000" value="0" /></div>
				<div class="row"><label><input id="btn-scene-release-off" type="checkbox" /> Release to OFF</label></div>
				<div class="muted" style="grid-column:1/-1; font-size:0.85rem;">Scenes control multiple outputs with coordinated behaviors. Configure scenes in the Behavioral Output tab.</div>
			</div>
		</div>
		
		<div id="can-config-section" style="display:none;">
		<h4>CAN Frame</h4>
		<div class="row" style="margin-bottom:10px;"><label><input id="btn-can-enabled" type="checkbox" onchange="toggleCanFields()" /> Send CAN on press</label></div>
		<div id="can-config-wrapper" class="grid two-col" style="display:none;">
			<div><label>PGN (hex)</label><input id="btn-can-pgn" type="text" placeholder="FEF9" /></div>
			<div><label>Priority</label><input id="btn-can-priority" type="number" min="0" max="7" /></div>
			<div><label>Source (hex)</label><input id="btn-can-src" type="text" placeholder="F9" /></div>
			<div><label>Dest (hex)</label><input id="btn-can-dest" type="text" placeholder="FF" /></div>
			<div class="row" style="grid-column:1/-1;">
				<label>Data Bytes</label>
				<input id="btn-can-data" type="text" placeholder="00 00 00 00 00 00 00 00" />
			</div>
			<div class="row" style="grid-column:1/-1;">
				<label>From Library</label>
				<select id="btn-can-library-select" onchange="loadCanFromLibrary()"></select>
			</div>
		</div>
		<h4>CAN Release Frame</h4>
		<div class="row" style="margin-bottom:10px;"><label><input id="btn-can-off-enabled" type="checkbox" onchange="toggleCanFields()" /> Send CAN on release (OFF)</label></div>
		<div id="can-off-config-wrapper" class="grid two-col" style="display:none;">
			<div><label>PGN (hex)</label><input id="btn-can-off-pgn" type="text" placeholder="FEF9" /></div>
			<div><label>Priority</label><input id="btn-can-off-priority" type="number" min="0" max="7" /></div>
			<div><label>Source (hex)</label><input id="btn-can-off-src" type="text" placeholder="F9" /></div>
			<div><label>Dest (hex)</label><input id="btn-can-off-dest" type="text" placeholder="FF" /></div>
			<div class="row" style="grid-column:1/-1;">
				<label>Data Bytes</label>
				<input id="btn-can-off-data" type="text" placeholder="00 00 00 00 00 00 00 00" />
			</div>
			<div class="row" style="grid-column:1/-1;">
				<label>From Library</label>
				<select id="btn-can-off-library-select" onchange="loadCanOffFromLibrary()"></select>
			</div>
		</div>
		</div><!-- end can-config-section -->
		
		<div class="row" style="margin-top:12px; justify-content:flex-end; gap:8px;">
			<button class="btn danger" onclick="deleteButtonFromModal()">Delete</button>
			<button class="btn primary" onclick="saveButtonFromModal()">Save</button>
		</div>
	</div>
</div>

<script>
let config = {};
let activePageIndex = 0;
let editingButton = { row: -1, col: -1 };
let wifiNetworks = [];

function firstDefined() {
	for (let i = 0; i < arguments.length; i++) {
		const v = arguments[i];
		if (v !== undefined && v !== null) {
			return v;
		}
	}
	return undefined;
}

function clampLogoSize(value){
	const parsed = parseInt(value, 10);
	if (Number.isNaN(parsed)) return 64;
	return Math.max(24, Math.min(96, parsed));
}

function setLogoSizeLabel(value){
	const label = document.getElementById('header-logo-size-label');
	if(label) label.textContent = `${value}px`;
}

function handleLogoSizeInput(value){
	const clamped = clampLogoSize(value);
	setLogoSizeLabel(clamped);
	updateHeaderFromInputs();
}

function clampNavSpacing(value){
	const parsed = parseInt(value, 10);
	if (Number.isNaN(parsed)) return 12;
	return Math.max(0, Math.min(48, parsed));
}

function setNavSpacingLabel(value){
	const label = document.getElementById('header-nav-spacing-label');
	if(label) label.textContent = `${value}px`;
}

function handleNavSpacingInput(value){
	const clamped = clampNavSpacing(value);
	setNavSpacingLabel(clamped);
	updateHeaderFromInputs();
}

function getHeaderLogoDimensions(value){
	if (!value) return null;
	if (value.startsWith('lvimg:')) {
		const parsed = parseLvimgPayload(value);
		if (parsed) {
			return { width: parsed.width, height: parsed.height };
		}
	}
	return null;
}

window.switchTab = function(tabName){
	document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));
	document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
	document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
	document.getElementById(`tab-${tabName}`).classList.add('active');
}

function showBanner(msg,type='success'){
	const el = document.getElementById('status-banner');
	el.className = `status-banner ${type==='success'?'status-success':'status-error'}`;
	el.textContent = msg;
	el.style.display = 'block';
	setTimeout(()=>{ el.style.display='none'; }, 3500);
}

function ensurePages(){
	console.log('[WEB] ensurePages() - current pages:', config.pages);
	if(!config.pages || config.pages.length===0){
		console.log('[WEB] No pages found, creating default Home page');
		config.pages = [{ id:'page_0', name:'Home', rows:2, cols:2, buttons:[] }];
		activePageIndex = 0;
	}
	console.log('[WEB] ensurePages() - final pages:', config.pages);
}

function renderPageSelect(){
	const sel = document.getElementById('quick-page-select');
	if(!sel) return;
	sel.innerHTML = '';
	config.pages.forEach((page, idx)=>{
		const opt = document.createElement('option');
		opt.value = `${idx}`;
		opt.textContent = page.name || `Window ${idx+1}`;
		sel.appendChild(opt);
	});
	sel.value = `${activePageIndex}`;
}

function renderPageList(){
	ensurePages();
	renderPageSelect();
	const list = document.getElementById('page-list');
	if(!list) return;
	list.innerHTML = '';
	config.pages.forEach((page,idx)=>{
		const item = document.createElement('div');
		item.className = 'page-chip'+(idx===activePageIndex?' active':'');
		item.draggable = true;
		item.dataset.index = idx;
		item.innerHTML = `<span class="name">${page.name||'Page '+(idx+1)}</span><span class="muted">${page.rows}x${page.cols}</span>`;
		item.onclick = ()=>setActivePage(idx);
		item.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', idx); };
		item.ondragover = (e)=>{ e.preventDefault(); };
		item.ondrop = (e)=>{
			e.preventDefault();
			const from = parseInt(e.dataTransfer.getData('text/plain'));
			if(isNaN(from) || from===idx) return;
			const moved = config.pages.splice(from,1)[0];
			config.pages.splice(idx,0,moved);
			activePageIndex = idx;
			renderPageList();
			renderNav();
			renderGrid();
			renderPreview();
		};
		list.appendChild(item);
	});
}

function quickPageSelectChanged(){
	const sel = document.getElementById('quick-page-select');
	if(!sel) return;
	const idx = parseInt(sel.value);
	if(!isNaN(idx)) setActivePage(idx);
}

function setActivePage(idx){
	activePageIndex = idx;
	hydratePageFields();
	renderPageList();
	renderGrid();
	renderPreview();
}

function addPage(){
	ensurePages();
	const id = 'page_'+config.pages.length;
	config.pages.push({ id, name:'Page '+(config.pages.length+1), rows:2, cols:2, buttons:[] });
	activePageIndex = config.pages.length-1;
	renderPageList();
	hydratePageFields();
	renderGrid();
	renderPreview();
}

function addSuspensionPageTemplate(){
	ensurePages();
	const id = 'page_suspension_'+Date.now();
	const buttonSpecs = [
		{ label:'Power On', pgn:0x737, data:[0,0,0,0,0,0,0,0x30] },
		{ label:'Power Off', pgn:0x737, data:[0,0,0,0,0,0,0,0x00] },
		{ label:'Calibrate', pgn:0x738, data:[0,0,0,0,0,0,0,0x01] },
		{ label:'Info', pgn:0x737, data:[0,0,0,0,0,0,0,0] },
		{ label:'Mode 1', pgn:0x737, data:[0,0,0,0x01,0x01,0x01,0x01,0] },
		{ label:'Mode 2', pgn:0x737, data:[0,0,0,0x02,0x02,0x02,0x02,0] },
		{ label:'Mode 3', pgn:0x737, data:[0,0,0,0x03,0x03,0x03,0x03,0] },
		{ label:'Mode 4', pgn:0x737, data:[0,0,0,0x04,0x04,0x04,0x04,0] },
		{ label:'Mode 5', pgn:0x737, data:[0,0,0,0x05,0x05,0x05,0x05,0] },
		{ label:'Front Soft', pgn:0x737, data:[0,0,0,0,0,0,0x01,0] },
		{ label:'Front Medium', pgn:0x737, data:[0,0,0,0,0,0,0x03,0] },
		{ label:'Front Firm', pgn:0x737, data:[0,0,0,0,0,0,0x05,0] },
		{ label:'Rear Soft', pgn:0x737, data:[0,0,0,0,0,0x01,0,0] },
		{ label:'Rear Medium', pgn:0x737, data:[0,0,0,0,0,0x03,0,0] },
		{ label:'Rear Firm', pgn:0x737, data:[0,0,0,0,0,0x05,0,0] },
		{ label:'Pitch Soft', pgn:0x737, data:[0,0,0,0x01,0,0,0,0] },
		{ label:'Pitch Medium', pgn:0x737, data:[0,0,0,0x03,0,0,0,0] },
		{ label:'Pitch Firm', pgn:0x737, data:[0,0,0,0x05,0,0,0,0] },
		{ label:'Roll Soft', pgn:0x737, data:[0,0,0,0,0x01,0,0,0] },
		{ label:'Roll Medium', pgn:0x737, data:[0,0,0,0,0x03,0,0,0] },
		{ label:'Roll Firm', pgn:0x737, data:[0,0,0,0,0x05,0,0,0] }
	];

	const buttons = buttonSpecs.map((spec, i) => {
		const row = Math.floor(i / 4);
		const col = i % 4;
		return {
			id: `${spec.label.toLowerCase().replace(/[^a-z0-9]+/g,'_')}_${i}`,
			label: spec.label,
			row,
			col,
			row_span: 1,
			col_span: 1,
			momentary: false,
			font_size: 20,
			corner_radius: 12,
			can: {
				enabled: true,
				pgn: spec.pgn,
				priority: 6,
				source_address: 0xF9,
				destination_address: 0xFF,
				data: spec.data
			}
		};
	});

	const page = {
		id,
		name: 'Suspension',
		rows: 6,
		cols: 4,
		buttons
	};
	config.pages.push(page);
	activePageIndex = config.pages.length - 1;
	renderPageList();
	hydratePageFields();
	renderGrid();
	renderPreview();
	showBanner('Suspension page added', 'success');
}

function deletePage(){
	ensurePages();
	if(config.pages.length<=1){ showBanner('At least one page is required','error'); return; }
	config.pages.splice(activePageIndex,1);
	if(activePageIndex>=config.pages.length) activePageIndex = config.pages.length-1;
	renderPageList();
	hydratePageFields();
	renderGrid();
	renderPreview();
}

function hydratePageFields(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const nameInput = document.getElementById('page-name-input');
	if(nameInput) nameInput.value = page.name || '';
	const navLabelInput = document.getElementById('page-nav-text-input');
	if (navLabelInput) navLabelInput.value = page.nav_text || '';
	const pageNavColor = document.getElementById('page-nav-color');
	if (pageNavColor) pageNavColor.value = page.nav_color || theme.nav_button_active_color || '#ff9d2e';
	const pageNavInactive = document.getElementById('page-nav-inactive-color');
	if (pageNavInactive) pageNavInactive.value = page.nav_inactive_color || theme.nav_button_color || '#3a3a3a';
	const navTextInput = document.getElementById('nav-text-color');
	if (navTextInput) navTextInput.value = page.nav_text_color || theme.nav_button_text_color || theme.text_primary || '#f2f4f8';
	const navRadiusInput = document.getElementById('nav-radius');
	if (navRadiusInput) {
		const fallbackRadius = (typeof theme.nav_button_radius === 'number')
			? theme.nav_button_radius
			: ((typeof theme.button_radius === 'number') ? theme.button_radius : 20);
		const hasPageRadius = typeof page.nav_button_radius === 'number' && !Number.isNaN(page.nav_button_radius);
		navRadiusInput.value = hasPageRadius ? page.nav_button_radius : fallbackRadius;
	}
	const pageBgColor = document.getElementById('page-bg-color');
	if (pageBgColor) pageBgColor.value = page.bg_color || theme.page_bg_color || '#0f0f0f';
	const pageTextColor = document.getElementById('page-text-color');
	if (pageTextColor) pageTextColor.value = page.text_color || theme.text_primary || '#ffffff';
	const pageBtnColor = document.getElementById('page-btn-color');
	if (pageBtnColor) pageBtnColor.value = page.button_color || theme.accent_color || '#ff9d2e';
	const pageBtnPressed = document.getElementById('page-btn-pressed');
	if (pageBtnPressed) pageBtnPressed.value = page.button_pressed_color || theme.button_pressed_color || '#ff7a1a';
	const pageBtnBorder = document.getElementById('page-btn-border');
	if (pageBtnBorder) pageBtnBorder.value = page.button_border_color || theme.border_color || '#3a3a3a';
	const pageBtnBorderWidth = document.getElementById('page-btn-border-width');
	if (pageBtnBorderWidth) pageBtnBorderWidth.value = page.button_border_width || 0;
	const pageBtnRadius = document.getElementById('page-btn-radius');
	if (pageBtnRadius) pageBtnRadius.value = page.button_radius || theme.button_radius || 12;
	const pageRows = document.getElementById('page-rows');
	if (pageRows) pageRows.value = page.rows || 2;
	const pageCols = document.getElementById('page-cols');
	if (pageCols) pageCols.value = page.cols || 2;
	const sel = document.getElementById('quick-page-select');
	if(sel) sel.value = `${activePageIndex}`;
}

function updatePageMeta(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const nameInput = document.getElementById('page-name-input');
	if (nameInput) {
		const trimmedName = nameInput.value ? nameInput.value.trim() : '';
		if (trimmedName.length) {
			page.name = trimmedName;
		}
	}
	const navLabelInput = document.getElementById('page-nav-text-input');
	if (navLabelInput) {
		page.nav_text = navLabelInput.value ? navLabelInput.value.trim() : '';
	}
	page.nav_color = document.getElementById('page-nav-color').value;
	page.nav_inactive_color = document.getElementById('page-nav-inactive-color').value;
	renderPageList();
	renderNav();
	renderPreview();
}

function updatePageNavStyle(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const navTextInput = document.getElementById('nav-text-color');
	if (navTextInput) {
		page.nav_text_color = navTextInput.value || '';
	}
	const navRadiusInput = document.getElementById('nav-radius');
	if (navRadiusInput) {
		let radius = parseInt(navRadiusInput.value);
		const fallbackRadius = (typeof theme.nav_button_radius === 'number')
			? theme.nav_button_radius
			: ((typeof theme.button_radius === 'number') ? theme.button_radius : 20);
		if (Number.isNaN(radius)) {
			delete page.nav_button_radius;
			navRadiusInput.value = fallbackRadius;
		} else {
			radius = Math.max(0, Math.min(50, radius));
			page.nav_button_radius = radius;
			navRadiusInput.value = radius;
		}
	}
	renderNav();
	renderPreview();
}

function updatePageStyle(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.bg_color = document.getElementById('page-bg-color').value;
	page.text_color = document.getElementById('page-text-color').value;
	page.button_color = document.getElementById('page-btn-color').value;
	page.button_pressed_color = document.getElementById('page-btn-pressed').value;
	page.button_border_color = document.getElementById('page-btn-border').value;
	page.button_border_width = parseInt(document.getElementById('page-btn-border-width').value)||0;
	page.button_radius = parseInt(document.getElementById('page-btn-radius').value)||0;
	renderGrid();
	renderPreview();
}

function applyPageStyleToButtons(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};

	const pageBg = document.getElementById('page-bg-color');
	const pageText = document.getElementById('page-text-color');
	const pageNavActive = document.getElementById('page-nav-color');
	const pageNavInactive = document.getElementById('page-nav-inactive-color');
	const pageBtnColor = document.getElementById('page-btn-color');
	const pageBtnPressed = document.getElementById('page-btn-pressed');
	const pageBtnBorder = document.getElementById('page-btn-border');
	const pageBtnBorderWidth = document.getElementById('page-btn-border-width');
	const pageBtnRadius = document.getElementById('page-btn-radius');

	// Normalize page-level fields to whatever the user currently sees in the UI
	page.bg_color = pageBg ? pageBg.value : (page.bg_color || theme.page_bg_color || '#0f0f0f');
	page.text_color = pageText ? pageText.value : (page.text_color || theme.text_primary || '#f2f4f8');
	page.nav_color = pageNavActive ? pageNavActive.value : (page.nav_color || theme.nav_button_active_color || '#ff9d2e');
	page.nav_inactive_color = pageNavInactive ? pageNavInactive.value : (page.nav_inactive_color || theme.nav_button_color || '#3a3a3a');
	page.button_color = pageBtnColor ? pageBtnColor.value : (page.button_color || theme.accent_color || '#ff9d2e');
	page.button_pressed_color = pageBtnPressed ? pageBtnPressed.value : (page.button_pressed_color || theme.button_pressed_color || page.button_color);
	page.button_border_color = pageBtnBorder ? pageBtnBorder.value : (page.button_border_color || theme.border_color || '#20232f');
	page.button_border_width = pageBtnBorderWidth ? parseInt(pageBtnBorderWidth.value)||0 : (Number.isFinite(page.button_border_width) ? page.button_border_width : (theme.border_width||0));
	page.button_radius = pageBtnRadius ? parseInt(pageBtnRadius.value)||0 : (Number.isFinite(page.button_radius) ? page.button_radius : (theme.button_radius||12));
	page.buttons = (page.buttons||[]).map(btn=>({
		...btn,
		color: page.button_color || btn.color,
		pressed_color: page.button_pressed_color || btn.pressed_color || page.button_color || btn.color,
		border_color: page.button_border_color || btn.border_color,
		border_width: page.button_border_width !== undefined ? page.button_border_width : btn.border_width,
		corner_radius: page.button_radius !== undefined ? page.button_radius : btn.corner_radius,
		text_color: page.text_color || btn.text_color || theme.text_primary || '#FFFFFF'
	}));
	renderGrid();
	renderPreview();
	showBanner('Applied current window styling to all buttons on this page','success');
}

function capturePageAsBaseline(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const firstBtn = (page.buttons||[])[0] || {};
	const buttonFontFamily = firstDefined(firstBtn.font_family, theme.button_font_family, 'montserrat');
	const buttonFontSize = firstDefined(firstBtn.font_size, theme.button_font_size, 24);
	const navTextInput = document.getElementById('nav-text-color');
	const navRadiusInput = document.getElementById('nav-radius');
	let navTextColor = page.nav_text_color || theme.nav_button_text_color || theme.text_primary || '#f2f4f8';
	if (navTextInput && navTextInput.value) {
		navTextColor = navTextInput.value;
	}
	let navRadius = (typeof page.nav_button_radius === 'number' && !Number.isNaN(page.nav_button_radius)) ? page.nav_button_radius : undefined;
	if (navRadiusInput) {
		const parsedRadius = parseInt(navRadiusInput.value);
		if (!Number.isNaN(parsedRadius)) navRadius = parsedRadius;
	}
	if (!Number.isFinite(navRadius)) {
		navRadius = (typeof theme.nav_button_radius === 'number' && !Number.isNaN(theme.nav_button_radius))
			? theme.nav_button_radius
			: ((typeof theme.button_radius === 'number' && !Number.isNaN(theme.button_radius)) ? theme.button_radius : 20);
	}
	
	config.theme = {
		...theme,
		page_bg_color: page.bg_color || theme.page_bg_color || '#0f0f0f',
		text_primary: page.text_color || theme.text_primary || '#f2f4f8',
		nav_button_active_color: page.nav_color || theme.nav_button_active_color || '#ff9d2e',
		nav_button_color: page.nav_inactive_color || theme.nav_button_color || '#3a3a3a',
		nav_button_text_color: navTextColor,
		nav_button_radius: Math.max(0, Math.min(50, navRadius)),
		accent_color: page.button_color || theme.accent_color || '#ff9d2e',
		button_pressed_color: page.button_pressed_color || theme.button_pressed_color || '#ff7a1a',
		border_color: page.button_border_color || theme.border_color || '#20232f',
		border_width: page.button_border_width || theme.border_width || 0,
		button_radius: page.button_radius || theme.button_radius || 12,
		button_font_family: buttonFontFamily,
		button_font_size: buttonFontSize
	};
	hydrateThemeFields();
	// Don't change active page - just update preview
	showBanner('Saved this page as the baseline theme','success');
}

function applyBaselineToPage(){
	ensurePages();
	const theme = config.theme || {};
	const page = config.pages[activePageIndex];
	
	page.bg_color = theme.page_bg_color || page.bg_color || '#0f0f0f';
	page.text_color = theme.text_primary || page.text_color || '#f2f4f8';
	page.nav_color = theme.nav_button_active_color || page.nav_color || '#ff9d2e';
	page.nav_inactive_color = theme.nav_button_color || page.nav_inactive_color || '#3a3a3a';
	page.nav_text_color = theme.nav_button_text_color || page.nav_text_color || '';
	page.button_color = theme.accent_color || page.button_color || '#ff9d2e';
	page.button_pressed_color = firstDefined(theme.button_pressed_color, page.button_pressed_color, '#ff7a1a');
	page.button_border_color = theme.border_color || page.button_border_color || '#20232f';
	page.button_border_width = theme.border_width !== undefined ? theme.border_width : (page.button_border_width || 0);
	page.button_radius = theme.button_radius !== undefined ? theme.button_radius : (page.button_radius || 12);
	if (typeof theme.nav_button_radius === 'number' && !Number.isNaN(theme.nav_button_radius)) {
		page.nav_button_radius = theme.nav_button_radius;
	} else {
		delete page.nav_button_radius;
	}
	
	// push updated fills/borders into existing buttons
	page.buttons = (page.buttons||[]).map(btn=>({
		...btn,
		color: theme.accent_color || page.button_color || btn.color,
		pressed_color: firstDefined(theme.button_pressed_color, page.button_pressed_color, btn.pressed_color, theme.accent_color, '#ff7a1a'),
		border_color: theme.border_color || page.button_border_color || btn.border_color,
		border_width: theme.border_width !== undefined ? theme.border_width : (page.button_border_width || btn.border_width),
		corner_radius: theme.button_radius !== undefined ? theme.button_radius : (page.button_radius || btn.corner_radius),
		text_color: theme.text_primary || page.text_color || btn.text_color || '#f2f4f8',
		font_family: theme.button_font_family || btn.font_family || 'montserrat',
		font_size: theme.button_font_size || btn.font_size || 24
	}));
	hydratePageFields();
	renderGrid();
	renderPreview();
	showBanner('Applied baseline to this page','success');
}

function updateGrid(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.rows = parseInt(document.getElementById('page-rows').value)||2;
	page.cols = parseInt(document.getElementById('page-cols').value)||2;
	renderGrid();
	renderPreview();
}

function renderGrid(){
	ensurePages();
	const grid = document.getElementById('layout-grid');
	if(!grid) return;
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const fallbackText = page.text_color || theme.text_primary || '#f2f4f8';
	const fallbackFill = page.button_color || theme.accent_color || '#ff9d2e';
	const fallbackBorderColor = page.button_border_color || theme.border_color || '#20232f';
	const fallbackBorderWidth = Number.isFinite(page.button_border_width) ? page.button_border_width : (theme.border_width || 0);
	const fallbackRadius = Number.isFinite(page.button_radius) && page.button_radius > 0 ? page.button_radius : (theme.button_radius || 12);
	grid.style.gridTemplateColumns = `repeat(${page.cols}, minmax(120px, 1fr))`;
	grid.innerHTML = '';
	for(let r=0;r<page.rows;r++){
		for(let c=0;c<page.cols;c++){
			const btn = (page.buttons||[]).find(b=>b.row===r && b.col===c);
			if(btn){
				const cell = document.createElement('div');
				cell.className = 'grid-cell';
				const inner = document.createElement('div');
				inner.className = 'grid-btn';
				const displayFill = btn.color || fallbackFill;
				const displayText = btn.text_color || fallbackText;
				const displayBorderColor = btn.border_color || fallbackBorderColor;
				const displayBorderWidth = (btn.border_width !== undefined ? btn.border_width : fallbackBorderWidth) || 0;
				const displayRadius = (btn.corner_radius !== undefined && btn.corner_radius !== null && btn.corner_radius > 0) ? btn.corner_radius : fallbackRadius;
				inner.style.background = displayFill;
				inner.style.color = displayText;
				inner.style.border = `${displayBorderWidth}px solid ${displayBorderColor}`;
				inner.style.borderRadius = `${displayRadius}px`;
				inner.textContent = btn.label || 'Button';
				inner.draggable = true;
				inner.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', `${r},${c}`); };
				inner.ondrop = (e)=>{
					e.preventDefault();
					const [fr,fc] = e.dataTransfer.getData('text/plain').split(',').map(n=>parseInt(n));
					moveButton(fr,fc,r,c);
				};
				inner.ondragover = (e)=>e.preventDefault();
				inner.onclick = ()=>openButtonModal(r,c);
				cell.appendChild(inner);
				grid.appendChild(cell);
			} else {
				const empty = document.createElement('div');
				empty.className = 'grid-cell';
				empty.textContent = '+';
				empty.onclick = ()=>openButtonModal(r,c);
				grid.appendChild(empty);
			}
		}
	}
}

function moveButton(fr,fc,tr,tc){
	const page = config.pages[activePageIndex];
	const idx = (page.buttons||[]).findIndex(b=>b.row===fr && b.col===fc);
	if(idx<0) return;
	page.buttons[idx].row = tr;
	page.buttons[idx].col = tc;
	page.buttons[idx].id = `btn_${tr}_${tc}`;
	renderGrid();
	renderPreview();
}

function openButtonModal(row,col){
	ensurePages();
	editingButton = {row,col};
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const btn = (page.buttons||[]).find(b=>b.row===row && b.col===col);
	const defaults = {
		label:`Button ${row}${col}`,
		color: page.button_color || theme.button_color || theme.accent_color || '#ff9d2e',
		pressed_color: firstDefined(page.button_pressed_color, theme.button_pressed_color, '#ff7a1a'),
		border_color: page.button_border_color || theme.border_color || '#3a3a3a',
		border_width: page.button_border_width || 0,
		corner_radius: page.button_radius || theme.button_radius || 12,
		font_size: firstDefined(theme.button_font_size, 24),
		font_family: firstDefined(theme.button_font_family, 'montserrat'),
		text_align: 'center',
		momentary: false,
		mode: 'can',
		output_behavior: {
			output_id: '',
			action: 'on',
			behavior_type: 'steady',
			target_value: 255,
			period_ms: 1000,
			duty_cycle: 50,
			fade_time_ms: 500,
			hold_duration_ms: 2000,
			on_time_ms: 0,
			off_time_ms: 0,
			auto_off: true
		},
		scene_id: '',
		scene_action: 'on',
		scene_duration_ms: 0,
		scene_release_off: false,
		can:{enabled:false,pgn:0,priority:6,source_address:0xF9,destination_address:0xFF,data:[0,0,0,0,0,0,0,0]},
		can_off:{enabled:false,pgn:0,priority:6,source_address:0xF9,destination_address:0xFF,data:[0,0,0,0,0,0,0,0]}
	};
	const data = btn || defaults;
	
	// Load basic fields first
	document.getElementById('btn-label').value = data.label || '';
	document.getElementById('btn-color').value = data.color;
	document.getElementById('btn-pressed-color').value = firstDefined(data.pressed_color, defaults.pressed_color);
	document.getElementById('btn-text-color').value = data.text_color || theme.text_primary || '#FFFFFF';
	document.getElementById('btn-border-color').value = firstDefined(data.border_color, defaults.border_color);
	document.getElementById('btn-border-width').value = firstDefined(data.border_width, defaults.border_width);
	document.getElementById('btn-corner-radius').value = firstDefined(data.corner_radius, defaults.corner_radius);
	document.getElementById('btn-font-size').value = data.font_size || 24;
	document.getElementById('btn-font-family').value = data.font_family || 'montserrat';
	document.getElementById('btn-text-align').value = data.text_align || 'center';
	document.getElementById('btn-momentary').checked = data.momentary || false;
	
	// Mode and behavioral fields (set non-dropdown fields first)
	document.getElementById('btn-mode').value = data.mode || 'can';
	const ob = data.output_behavior || defaults.output_behavior;
	document.getElementById('btn-output-action').value = ob.action || 'on';
	document.getElementById('btn-behavior-type').value = ob.behavior_type || 'steady';
	document.getElementById('btn-target-value').value = ob.target_value || 255;
	document.getElementById('btn-period-ms').value = ob.period_ms || 1000;
	document.getElementById('btn-duty-cycle').value = ob.duty_cycle || 50;
	document.getElementById('btn-fade-time-ms').value = ob.fade_time_ms || 500;
	document.getElementById('btn-hold-duration-ms').value = ob.hold_duration_ms || 2000;
	document.getElementById('btn-auto-off').checked = ob.auto_off !== undefined ? ob.auto_off : true;
	
	// Load behavioral options from server, then set dropdown values
	loadBehavioralOptions().then(() => {
		// Set dropdown values AFTER options are loaded
		document.getElementById('btn-output-id').value = ob.output_id || '';
		document.getElementById('btn-scene-id').value = data.scene_id || '';
	}).catch(err => {
		console.error('Failed to load behavioral options:', err);
		// Set values anyway even if loading failed
		document.getElementById('btn-output-id').value = ob.output_id || '';
		document.getElementById('btn-scene-id').value = data.scene_id || '';
	});
	document.getElementById('btn-scene-action').value = data.scene_action || 'on';
	document.getElementById('btn-scene-duration-ms').value = data.scene_duration_ms || 0;
	document.getElementById('btn-scene-release-off').checked = data.scene_release_off || false;
	const canCfg = data.can || {};
	document.getElementById('btn-can-enabled').checked = canCfg.enabled || false;
	document.getElementById('btn-can-pgn').value = (canCfg.pgn || 0).toString(16).toUpperCase();
	document.getElementById('btn-can-priority').value = firstDefined(canCfg.priority, 6);
	document.getElementById('btn-can-src').value = (firstDefined(canCfg.source_address, 0xF9)).toString(16).toUpperCase();
	document.getElementById('btn-can-dest').value = (firstDefined(canCfg.destination_address, 0xFF)).toString(16).toUpperCase();
	const canData = (canCfg.data && canCfg.data.length) ? canCfg.data : defaults.can.data;
	document.getElementById('btn-can-data').value = canData.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	const canOffCfg = data.can_off || {};
	document.getElementById('btn-can-off-enabled').checked = canOffCfg.enabled || false;
	document.getElementById('btn-can-off-pgn').value = (canOffCfg.pgn || 0).toString(16).toUpperCase();
	document.getElementById('btn-can-off-priority').value = firstDefined(canOffCfg.priority, 6);
	document.getElementById('btn-can-off-src').value = (firstDefined(canOffCfg.source_address, 0xF9)).toString(16).toUpperCase();
	document.getElementById('btn-can-off-dest').value = (firstDefined(canOffCfg.destination_address, 0xFF)).toString(16).toUpperCase();
	const canOffData = (canOffCfg.data && canOffCfg.data.length) ? canOffCfg.data : defaults.can_off.data;
	document.getElementById('btn-can-off-data').value = canOffData.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	populateCanLibraryDropdown();
	toggleBehavioralFields();
	document.getElementById('button-modal').classList.add('open');
}

function closeModal(){ document.getElementById('button-modal').classList.remove('open'); }

function saveButtonFromModal(){
	ensurePages();
	const page = config.pages[activePageIndex];
	if(!page.buttons) page.buttons = [];
	const idx = page.buttons.findIndex(b=>b.row===editingButton.row && b.col===editingButton.col);
	const canEnabled = document.getElementById('btn-can-enabled').checked;
	const canData = (document.getElementById('btn-can-data').value||'').split(' ').map(v=>parseInt(v,16)||0).slice(0,8);
	while(canData.length<8) canData.push(0);
	const canOffEnabled = document.getElementById('btn-can-off-enabled').checked;
	const canOffData = (document.getElementById('btn-can-off-data').value||'').split(' ').map(v=>parseInt(v,16)||0).slice(0,8);
	while(canOffData.length<8) canOffData.push(0);
	const button = {
		id:`btn_${editingButton.row}_${editingButton.col}`,
		row: editingButton.row,
		col: editingButton.col,
		row_span: 1,
		col_span: 1,
		label: document.getElementById('btn-label').value || 'Button',
		color: document.getElementById('btn-color').value,
		pressed_color: document.getElementById('btn-pressed-color').value,
		text_color: document.getElementById('btn-text-color').value,
		border_color: document.getElementById('btn-border-color').value,
		border_width: parseInt(document.getElementById('btn-border-width').value)||0,
		corner_radius: parseInt(document.getElementById('btn-corner-radius').value)||0,
		font_size: parseInt(document.getElementById('btn-font-size').value)||24,
		font_family: document.getElementById('btn-font-family').value,
		font_weight: '400',
		font_name: document.getElementById('btn-font-family').value+'_16',
		text_align: document.getElementById('btn-text-align').value,
		momentary: document.getElementById('btn-momentary').checked,
		mode: document.getElementById('btn-mode').value,
		output_behavior: {
			output_id: document.getElementById('btn-output-id').value || '',
			action: document.getElementById('btn-output-action').value || 'on',
			behavior_type: document.getElementById('btn-behavior-type').value,
			target_value: parseInt(document.getElementById('btn-target-value').value) || 255,
			period_ms: parseInt(document.getElementById('btn-period-ms').value) || 1000,
			duty_cycle: parseInt(document.getElementById('btn-duty-cycle').value) || 50,
			fade_time_ms: parseInt(document.getElementById('btn-fade-time-ms').value) || 500,
			hold_duration_ms: parseInt(document.getElementById('btn-hold-duration-ms').value) || 2000,
			on_time_ms: 0,
			off_time_ms: 0,
			auto_off: document.getElementById('btn-auto-off').checked
		},
		scene_id: document.getElementById('btn-scene-id').value || '',
		scene_action: document.getElementById('btn-scene-action').value || 'on',
		scene_duration_ms: parseInt(document.getElementById('btn-scene-duration-ms').value) || 0,
		scene_release_off: document.getElementById('btn-scene-release-off').checked,
		can: {
			enabled: canEnabled,
			pgn: canEnabled ? parseInt(document.getElementById('btn-can-pgn').value,16)||0 : 0,
			priority: canEnabled ? parseInt(document.getElementById('btn-can-priority').value)||6 : 6,
			source_address: canEnabled ? parseInt(document.getElementById('btn-can-src').value,16)||0xF9 : 0xF9,
			destination_address: canEnabled ? parseInt(document.getElementById('btn-can-dest').value,16)||0xFF : 0xFF,
			data: canData
		},
		can_off: {
			enabled: canOffEnabled,
			pgn: canOffEnabled ? parseInt(document.getElementById('btn-can-off-pgn').value,16)||0 : 0,
			priority: canOffEnabled ? parseInt(document.getElementById('btn-can-off-priority').value)||6 : 6,
			source_address: canOffEnabled ? parseInt(document.getElementById('btn-can-off-src').value,16)||0xF9 : 0xF9,
			destination_address: canOffEnabled ? parseInt(document.getElementById('btn-can-off-dest').value,16)||0xFF : 0xFF,
			data: canOffData
		}
	};
	if(idx>=0) page.buttons[idx] = button; else page.buttons.push(button);
	closeModal();
	renderGrid();
	renderPreview();
}

function deleteButtonFromModal(){
	ensurePages();
	const page = config.pages[activePageIndex];
	page.buttons = (page.buttons||[]).filter(b=>!(b.row===editingButton.row && b.col===editingButton.col));
	closeModal();
	renderGrid();
	renderPreview();
}

function toggleBehavioralFields(){
	const mode = document.getElementById('btn-mode').value;
	document.getElementById('behavioral-output-config').style.display = mode === 'output' ? 'block' : 'none';
	document.getElementById('behavioral-scene-config').style.display = mode === 'scene' ? 'block' : 'none';
	document.getElementById('can-config-section').style.display = mode === 'can' ? 'block' : 'none';
	const momentary = document.getElementById('btn-momentary');
	if (momentary) {
		momentary.disabled = mode !== 'can';
		if (mode !== 'can') {
			momentary.checked = false;
		}
	}
	if(mode === 'can') toggleCanFields(); // Update CAN sub-sections
}

function loadBehavioralOptions(){
	return fetch('/api/behavioral/options')
		.then(r => r.json())
		.then(data => {
			const outputSel = document.getElementById('btn-output-id');
			outputSel.innerHTML = '<option value="">Select output...</option>';
			(data.outputs || []).forEach(out => {
				const opt = document.createElement('option');
				opt.value = out.id;
				opt.textContent = out.name;
				outputSel.appendChild(opt);
			});
			
			const sceneSel = document.getElementById('btn-scene-id');
			sceneSel.innerHTML = '<option value="">Select scene...</option>';
			(data.scenes || []).forEach(scene => {
				const opt = document.createElement('option');
				opt.value = scene.id;
				opt.textContent = scene.name;
				sceneSel.appendChild(opt);
			});
		});
}

function toggleCanFields(){
	const showOn = document.getElementById('btn-can-enabled').checked;
	document.getElementById('can-config-wrapper').style.display = showOn ? 'grid' : 'none';
	const showOff = document.getElementById('btn-can-off-enabled').checked;
	document.getElementById('can-off-config-wrapper').style.display = showOff ? 'grid' : 'none';
}

function applyInfinityboxTemplate(){
	const funcName = document.getElementById('btn-infinitybox-function').value;
	if(!funcName) return;
	
	// POWERCELL NGX J1939 function mapping
	// Format: {cell: address, output: 1-10, label: display name, softStart: boolean}
	const templates = {
		left_turn: {cell:1, output:1, label:'Left Turn', softStart:false},
		right_turn: {cell:1, output:2, label:'Right Turn', softStart:false},
		four_way: {cell:1, output:1, label:'4-Ways', softStart:false, dual:2}, // Controls outputs 1 & 2
		horn: {cell:1, output:6, label:'Horn', softStart:false},
		high_beams: {cell:1, output:7, label:'High Beams', softStart:false},
		headlights: {cell:1, output:5, label:'Headlights', softStart:true},
		parking_lights: {cell:1, output:8, label:'Parking Lights', softStart:true},
		backup_lights: {cell:2, output:1, label:'Backup Lights', softStart:false},
		brake_lights: {cell:2, output:2, label:'Brake Lights', softStart:false},
		interior_lights: {cell:1, output:9, label:'Interior Lights', softStart:true},
		gauge_illumination: {cell:1, output:10, label:'Gauge Illum', softStart:true},
		door_lock: {cell:2, output:3, label:'Door Lock', softStart:false},
		door_unlock: {cell:2, output:4, label:'Door Unlock', softStart:false},
		window_up_driver: {cell:2, output:5, label:'Win Up Drv', softStart:false},
		window_down_driver: {cell:2, output:6, label:'Win Dn Drv', softStart:false},
		window_up_passenger: {cell:2, output:7, label:'Win Up Pax', softStart:false},
		window_down_passenger: {cell:2, output:8, label:'Win Dn Pax', softStart:false},
		ignition: {cell:1, output:3, label:'Ignition', softStart:false},
		starter: {cell:1, output:4, label:'Starter', softStart:false},
		fuel_pump: {cell:2, output:9, label:'Fuel Pump', softStart:false},
		cooling_fan: {cell:2, output:10, label:'Cooling Fan', softStart:false},
		aux_1: {cell:3, output:1, label:'AUX 1', softStart:false},
		aux_2: {cell:3, output:2, label:'AUX 2', softStart:false},
		security_arm: {cell:3, output:3, label:'Sec Arm', softStart:false},
		security_disarm: {cell:3, output:4, label:'Sec Disarm', softStart:false}
	};
	
	const tmpl = templates[funcName];
	if(!tmpl) return;
	
	// Auto-populate label if still default
	const currentLabel = document.getElementById('btn-label').value;
	if(!currentLabel || currentLabel.match(/^Button \d+$/)){
		document.getElementById('btn-label').value = tmpl.label;
	}
	
	// POWERCELL NGX uses address-specific PGNs: FF01-FF10
	const pgn = 0xFF00 + tmpl.cell;
	
	// Build POWERCELL NGX format:
	// Byte 1 bits 7-0: Track outputs 1-8
	// Byte 2 bits 7-6: Track outputs 9-10
	// Byte 2 bits 5-0 + Byte 3 bits 7-6: Soft-Start outputs 1-10
	// Byte 3 bits 5-0 + Byte 4 bits 7-6: PWM enable outputs 1-8
	// Bytes 5-8: PWM duty cycle (4 bits per output)
	
	function buildPowercellFrame(output, state, softStart, dualOutput) {
		const data = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
		
		if(state) {
			// Set the appropriate bit(s) based on output number
			if(softStart) {
				// Soft-Start: Byte 2 bits 5-0 + Byte 3 bits 7-6
				if(output <= 6) {
					data[1] |= (1 << (5 - (output - 1))); // Outputs 1-6 in byte 2
				} else if(output <= 10) {
					data[2] |= (1 << (13 - output)); // Outputs 7-10 in byte 3
				}
				if(dualOutput) {
					if(dualOutput <= 6) {
						data[1] |= (1 << (5 - (dualOutput - 1)));
					} else if(dualOutput <= 10) {
						data[2] |= (1 << (13 - dualOutput));
					}
				}
			} else {
				// Track: Byte 1 bits 7-0 (outputs 1-8), Byte 2 bits 7-6 (outputs 9-10)
				if(output <= 8) {
					data[0] |= (1 << (8 - output)); // Outputs 1-8 in byte 1
				} else if(output === 9) {
					data[1] |= 0x80; // Output 9 = bit 7 of byte 2
				} else if(output === 10) {
					data[1] |= 0x40; // Output 10 = bit 6 of byte 2
				}
				if(dualOutput) {
					if(dualOutput <= 8) {
						data[0] |= (1 << (8 - dualOutput));
					} else if(dualOutput === 9) {
						data[1] |= 0x80;
					} else if(dualOutput === 10) {
						data[1] |= 0x40;
					}
				}
			}
		}
		// OFF state = all zeros (already initialized)
		
		return data;
	}
	
	const dataOn = buildPowercellFrame(tmpl.output, true, tmpl.softStart, tmpl.dual);
	const dataOff = buildPowercellFrame(tmpl.output, false, tmpl.softStart, tmpl.dual);
	
	// Populate CAN ON frame
	document.getElementById('btn-can-enabled').checked = true;
	document.getElementById('btn-can-pgn').value = pgn.toString(16).toUpperCase();
	document.getElementById('btn-can-priority').value = '6';
	document.getElementById('btn-can-src').value = '1E'; // Default controller source address
	document.getElementById('btn-can-dest').value = 'FF'; // Broadcast
	document.getElementById('btn-can-data').value = dataOn.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	
	// Populate CAN OFF frame
	document.getElementById('btn-can-off-enabled').checked = true;
	document.getElementById('btn-can-off-pgn').value = pgn.toString(16).toUpperCase();
	document.getElementById('btn-can-off-priority').value = '6';
	document.getElementById('btn-can-off-src').value = '1E';
	document.getElementById('btn-can-off-dest').value = 'FF';
	document.getElementById('btn-can-off-data').value = dataOff.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	
	toggleCanFields();
}

function renderPreview(){
	ensurePages();
	const page = config.pages[activePageIndex];
	const theme = config.theme || {};
	const headerCfg = config.header || {};
	const header = document.getElementById('preview-header');
	header.style.background = theme.surface_color || '#12141c';
	header.style.borderBottom = `${firstDefined(theme.header_border_width, 0)}px solid ${firstDefined(theme.header_border_color, theme.accent_color, '#ff9d2e')}`;
	const navSpacingValue = clampNavSpacing(firstDefined(headerCfg.nav_spacing, 12));
	header.style.marginBottom = `${navSpacingValue}px`;
	const previewNav = document.getElementById('preview-nav');
	if (previewNav) {
		previewNav.style.marginBottom = '12px';
	}
	const placement = headerCfg.logo_position || 'stacked';
	const inlineLayout = placement === 'inline-left' || placement === 'inline-right';
	header.classList.toggle('stacked', placement === 'stacked');
	header.classList.toggle('inline-left', placement === 'inline-left');
	header.classList.toggle('inline-right', placement === 'inline-right');
	const previewLogo = document.getElementById('preview-logo');
	const previewLogoImg = document.getElementById('preview-logo-img');
	const headerLogoValue = (config.images && config.images.header_logo) || '';
	const previewLogoSrc = getImagePreviewSrc(headerLogoValue);
	const hasLogo = !!previewLogoSrc;
	if (previewLogo && previewLogoImg) {
		const shouldShowLogo = headerCfg.show_logo !== false && hasLogo;
		if (!shouldShowLogo) {
			previewLogo.style.display = 'none';
			previewLogoImg.src = '';
			previewLogo.style.removeProperty('width');
			previewLogo.style.removeProperty('min-width');
			previewLogo.style.removeProperty('height');
			previewLogo.style.removeProperty('max-height');
			previewLogoImg.style.objectFit = 'contain';
		} else {
			previewLogo.style.display = 'flex';
			previewLogo.style.alignItems = 'center';
			previewLogo.style.justifyContent = 'center';
			previewLogoImg.src = previewLogoSrc;
			previewLogoImg.alt = 'Header logo preview';
			const targetHeight = clampLogoSize(headerCfg.logo_target_height || 64);
			const dims = getHeaderLogoDimensions(headerLogoValue);
			let aspectRatio = 1;
			if (dims && dims.height) {
				aspectRatio = Math.max(0.2, Math.min(5, dims.width / Math.max(1, dims.height)));
			}
			const preserveAspect = headerCfg.logo_preserve_aspect !== false;
			const maxWidth = inlineLayout ? targetHeight * 2.5 : targetHeight * 3;
			let displayWidth = targetHeight * aspectRatio;
			if (!Number.isFinite(displayWidth) || displayWidth <= 0) {
				displayWidth = targetHeight;
			}
			if (!preserveAspect) {
				displayWidth = targetHeight;
				previewLogoImg.style.objectFit = 'fill';
				previewLogoImg.style.imageRendering = 'pixelated';
			} else {
				displayWidth = Math.max(targetHeight, Math.min(maxWidth, displayWidth));
				previewLogoImg.style.objectFit = 'contain';
				previewLogoImg.style.imageRendering = 'pixelated';
			}
			previewLogo.style.width = `${displayWidth}px`;
			previewLogo.style.minWidth = `${Math.max(targetHeight, Math.min(displayWidth, maxWidth))}px`;
			previewLogo.style.height = `${targetHeight}px`;
			previewLogo.style.maxHeight = `${targetHeight}px`;
		}
	}
	
	const titleEl = document.getElementById('preview-title');
	const subtitleEl = document.getElementById('preview-subtitle');
	titleEl.textContent = headerCfg.title || 'CAN Control';
	subtitleEl.textContent = headerCfg.subtitle || '';
	titleEl.style.color = theme.text_primary || '#fff';
	subtitleEl.style.color = theme.text_secondary || '#8d92a3';
	
	// Apply font styling to match what will be shown on device
	const titleFont = headerCfg.title_font || 'montserrat_24';
	const subtitleFont = headerCfg.subtitle_font || 'montserrat_12';
	const titleSize = parseInt(titleFont.split('_')[1]) || 24;
	const subtitleSize = parseInt(subtitleFont.split('_')[1]) || 12;
	const titleFamily = titleFont.startsWith('unscii') ? 'monospace' : 'Space Grotesk, sans-serif';
	const subtitleFamily = subtitleFont.startsWith('unscii') ? 'monospace' : 'Space Grotesk, sans-serif';
	titleEl.style.fontSize = `${Math.min(titleSize, 28)}px`;
	titleEl.style.fontFamily = titleFamily;
	subtitleEl.style.fontSize = `${Math.min(subtitleSize, 16)}px`;
	subtitleEl.style.fontFamily = subtitleFamily;
	
	// Apply text alignment
	const titleAlign = headerCfg.title_align || 'center';
	titleEl.style.textAlign = titleAlign;
	subtitleEl.style.textAlign = titleAlign;

	renderNav();

	const body = document.getElementById('preview-body');
	body.style.background = page.bg_color || theme.page_bg_color || '#0f0f0f';
	body.innerHTML = '';
	const grid = document.createElement('div');
	grid.className = 'preview-grid';
	grid.style.gridTemplateColumns = `repeat(${page.cols||2}, minmax(80px,1fr))`;
	for(let r=0;r<page.rows;r++){
		for(let c=0;c<page.cols;c++){
			const btn = (page.buttons||[]).find(b=>b.row===r && b.col===c);
			const el = document.createElement('div');
			el.className = 'preview-btn'+(btn ? '' : ' empty');
			const fill = firstDefined(btn && btn.color, theme.button_color, theme.accent_color, '#ff9d2e');
			el.style.background = fill;
			el.style.borderColor = firstDefined(btn && btn.border_color, theme.border_color, 'transparent');
			el.style.borderWidth = `${firstDefined(btn && btn.border_width, theme.border_width, 0)}px`;
			const textColor = firstDefined(btn && btn.text_color, theme.text_primary, '#FFFFFF');
			el.style.color = textColor;
			el.style.borderRadius = `${firstDefined(btn && btn.corner_radius, theme.button_radius, 12)}px`;
			el.textContent = (btn && btn.label) || '+';
			// Apply text alignment if button has specific alignment
			if (btn && btn.text_align) {
				const alignMap = {
					'top-left': 'flex-start', 'top-center': 'center', 'top-right': 'flex-end',
					'center': 'center',
					'bottom-left': 'flex-start', 'bottom-center': 'center', 'bottom-right': 'flex-end'
				};
				const vAlignMap = {
					'top-left': 'flex-start', 'top-center': 'flex-start', 'top-right': 'flex-start',
					'center': 'center',
					'bottom-left': 'flex-end', 'bottom-center': 'flex-end', 'bottom-right': 'flex-end'
				};
				el.style.display = 'flex';
				el.style.alignItems = vAlignMap[btn.text_align] || 'center';
				el.style.justifyContent = alignMap[btn.text_align] || 'center';
			}
			el.dataset.row = r;
			el.dataset.col = c;
			el.draggable = !!btn;
			el.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', `${r},${c}`); };
			el.ondragover = (e)=>e.preventDefault();
			el.ondrop = (e)=>{
				e.preventDefault();
				const data = e.dataTransfer.getData('text/plain');
				if(!data) return;
				const [fr,fc] = data.split(',').map(n=>parseInt(n));
				if(isNaN(fr) || isNaN(fc)) return;
				moveButton(fr,fc,r,c);
			};
			el.onclick = ()=>openButtonModal(r,c);
			grid.appendChild(el);
		}
	}
	body.appendChild(grid);
}

function renderNav(){
	console.log('[WEB] renderNav() called');
	ensurePages();
	const nav = document.getElementById('preview-nav');
	console.log('[WEB] preview-nav element:', nav);
	console.log('[WEB] Rendering', config.pages.length, 'navigation pills');
	const theme = config.theme || {};
	nav.innerHTML = '';
	nav.style.background = firstDefined(theme.surface_color, '#12141c');
	nav.style.borderBottom = `1px solid ${firstDefined(theme.border_color, '#20232f')}`;
	const fallbackNavRadius = (typeof theme.nav_button_radius === 'number' && !Number.isNaN(theme.nav_button_radius))
		? theme.nav_button_radius
		: ((typeof theme.button_radius === 'number' && !Number.isNaN(theme.button_radius)) ? theme.button_radius : 20);
	config.pages.forEach((p,idx)=>{
		const chip = document.createElement('div');
		chip.className = 'pill';
		chip.textContent = p.nav_text || p.name || 'Page '+(idx+1);
		const active = idx===activePageIndex;
		chip.style.background = active 
			? (p.nav_color || theme.nav_button_active_color || theme.accent_color || '#ff9d2e') 
			: (p.nav_inactive_color || theme.nav_button_color || '#2a2a2a');
		const navTextColor = p.nav_text_color || theme.nav_button_text_color || theme.text_primary || '#f2f4f8';
		chip.style.color = navTextColor;
		chip.style.opacity = active ? 1 : 0.75;
		const chipRadius = (typeof p.nav_button_radius === 'number' && !Number.isNaN(p.nav_button_radius))
			? p.nav_button_radius
			: fallbackNavRadius;
		chip.style.borderRadius = `${chipRadius}px`;
		chip.draggable = true;
		chip.ondragstart = (e)=>{ e.dataTransfer.setData('text/plain', idx); };
		chip.ondragover = (e)=>e.preventDefault();
		chip.ondrop = (e)=>{
			e.preventDefault();
			const from = parseInt(e.dataTransfer.getData('text/plain'));
			if(isNaN(from)||from===idx) return;
			const moved = config.pages.splice(from,1)[0];
			config.pages.splice(idx,0,moved);
			activePageIndex = idx;
			renderNav();
			renderPageList();
			renderPreview();
		};
		chip.onclick = ()=>setActivePage(idx);
		nav.appendChild(chip);
		console.log('[WEB] Added nav pill:', p.name || 'Page '+(idx+1));
	});
	console.log('[WEB] renderNav() complete - nav children:', nav.children.length);
}

function updateHeaderFromInputs(){
	config.header = config.header || {};
	config.header.title = document.getElementById('header-title-input').value || 'CAN Control';
	config.header.subtitle = document.getElementById('header-subtitle-input').value || '';
	config.header.title_font = document.getElementById('header-title-font').value || 'montserrat_24';
	config.header.subtitle_font = document.getElementById('header-subtitle-font').value || 'montserrat_12';
	config.header.title_align = document.getElementById('header-title-align').value || 'center';
	const logoPlacement = document.getElementById('header-logo-position');
	config.header.logo_position = logoPlacement ? (logoPlacement.value || 'stacked') : 'stacked';
	const sizeInput = document.getElementById('header-logo-size');
	const keepAspect = document.getElementById('header-logo-keep-aspect');
	const logoHeight = clampLogoSize(sizeInput ? sizeInput.value : 64);
	setLogoSizeLabel(logoHeight);
	config.header.logo_target_height = logoHeight;
	config.header.logo_preserve_aspect = keepAspect ? keepAspect.checked : true;
	const navSpacingInput = document.getElementById('header-nav-spacing');
	const navSpacing = clampNavSpacing(navSpacingInput ? navSpacingInput.value : 12);
	setNavSpacingLabel(navSpacing);
	config.header.nav_spacing = navSpacing;
	
	// show_logo now controlled by image upload - always true if image exists
	config.header.show_logo = !!(config.images && config.images.header_logo);
	renderPreview();
}

function hydrateThemeFields(){
	const theme = config.theme || {};
	// Header appearance
	const themeSurface = document.getElementById('theme-surface');
	if (themeSurface) themeSurface.value = theme.surface_color || '#12141c';
	const themeTextPrimary = document.getElementById('theme-text-primary');
	if (themeTextPrimary) themeTextPrimary.value = theme.text_primary || '#f2f4f8';
	const themeTextSecondary = document.getElementById('theme-text-secondary');
	if (themeTextSecondary) themeTextSecondary.value = theme.text_secondary || '#8d92a3';
	const themeHeaderBorder = document.getElementById('theme-header-border');
	if (themeHeaderBorder) themeHeaderBorder.value = theme.header_border_color || '#ff9d2e';
	const themeHeaderBorderWidth = document.getElementById('theme-header-border-width');
	if (themeHeaderBorderWidth) themeHeaderBorderWidth.value = theme.header_border_width || 0;
	
	// Window baseline
	const themePageBg = document.getElementById('theme-page-bg');
	if (themePageBg) themePageBg.value = theme.page_bg_color || '#0f0f0f';
	const themeTextColor = document.getElementById('theme-text-color');
	if (themeTextColor) themeTextColor.value = theme.text_primary || '#f2f4f8';
	const themeNavActive = document.getElementById('theme-nav-active');
	if (themeNavActive) themeNavActive.value = theme.nav_button_active_color || '#ff9d2e';
	const themeNavButton = document.getElementById('theme-nav-button');
	if (themeNavButton) themeNavButton.value = theme.nav_button_color || '#2a2a2a';
	const themeNavText = document.getElementById('theme-nav-text');
	if (themeNavText) themeNavText.value = theme.nav_button_text_color || theme.text_primary || '#f2f4f8';
	const themeNavRadius = document.getElementById('theme-nav-radius');
	if (themeNavRadius) themeNavRadius.value = (typeof theme.nav_button_radius === 'number') ? theme.nav_button_radius : (theme.button_radius || 20);
	const themeButtonColor = document.getElementById('theme-button-color');
	if (themeButtonColor) themeButtonColor.value = theme.accent_color || '#ff9d2e';
	const themeButtonPressed = document.getElementById('theme-button-pressed');
	if (themeButtonPressed) themeButtonPressed.value = theme.button_pressed_color || '#ff7a1a';
	const themeBorder = document.getElementById('theme-border');
	if (themeBorder) themeBorder.value = theme.border_color || '#20232f';
	const themeBorderWidth = document.getElementById('theme-border-width');
	if (themeBorderWidth) themeBorderWidth.value = theme.border_width || 0;
	const themeRadius = document.getElementById('theme-radius');
	if (themeRadius) themeRadius.value = theme.button_radius || 12;
}

function collectTheme(){
	const existing = config.theme || {};
	config.theme = {
		// Header appearance
		surface_color: document.getElementById('theme-surface').value,
		text_primary: document.getElementById('theme-text-primary').value,
		text_secondary: document.getElementById('theme-text-secondary').value,
		header_border_color: document.getElementById('theme-header-border').value,
		header_border_width: parseInt(document.getElementById('theme-header-border-width').value)||0,
		
		// Window baseline
		page_bg_color: document.getElementById('theme-page-bg').value,
		accent_color: document.getElementById('theme-button-color').value,
		button_pressed_color: document.getElementById('theme-button-pressed').value,
		border_color: document.getElementById('theme-border').value,
		nav_button_color: document.getElementById('theme-nav-button').value,
		nav_button_active_color: document.getElementById('theme-nav-active').value,
		nav_button_text_color: document.getElementById('theme-nav-text').value,
		nav_button_radius: Math.max(0, Math.min(50, parseInt(document.getElementById('theme-nav-radius').value)||20)),
		button_radius: parseInt(document.getElementById('theme-radius').value)||12,
		border_width: parseInt(document.getElementById('theme-border-width').value)||0,
		
		// Preserve additional fields
		button_font_family: existing.button_font_family || 'montserrat',
		button_font_size: existing.button_font_size || 24
		};

		const activePage = (config.pages || [])[activePageIndex];
		const quickNavText = document.getElementById('nav-text-color');
		if (quickNavText && (!activePage || !activePage.nav_text_color)) {
			quickNavText.value = config.theme.nav_button_text_color || config.theme.text_primary || '#f2f4f8';
		}
		const quickNavRadius = document.getElementById('nav-radius');
		if (quickNavRadius && (!activePage || typeof activePage.nav_button_radius !== 'number')) {
			quickNavRadius.value = (typeof config.theme.nav_button_radius === 'number')
				? config.theme.nav_button_radius
				: (config.theme.button_radius || 20);
		}
}

function wireThemeInputs(){
	['theme-surface','theme-text-primary','theme-text-secondary','theme-header-border','theme-header-border-width',
	 'theme-page-bg','theme-text-color','theme-nav-active','theme-nav-button','theme-nav-text','theme-nav-radius',
	 'theme-button-color','theme-button-pressed','theme-border','theme-border-width','theme-radius'].forEach(id=>{
		const el = document.getElementById(id);
		if(!el) return;
		el.addEventListener('input', ()=>{ collectTheme(); renderPreview(); });
		el.addEventListener('change', ()=>{ collectTheme(); renderPreview(); });
	});
}

function updatePageConfig(){
	if(!config.theme) config.theme = {};
	config.theme.page_bg_color = document.getElementById('page-bg').value;
	config.theme.nav_button_active_color = document.getElementById('nav-active').value;
	config.theme.nav_button_color = document.getElementById('nav-inactive').value;
	config.theme.accent_color = document.getElementById('button-active').value;
	config.theme.button_pressed_color = document.getElementById('button-pressed').value;
	config.theme.button_radius = parseInt(document.getElementById('button-radius').value)||8;
	config.theme.border_width = parseInt(document.getElementById('button-border-width').value)||1;
	renderPreview();
}

function applyBaseline(){
	const pageBg = document.getElementById('baseline-page-bg').value;
	const navActive = document.getElementById('baseline-nav-active').value;
	const navInactive = document.getElementById('baseline-nav-inactive').value;
	const buttonActive = document.getElementById('baseline-button-active').value;
	const buttonPressed = document.getElementById('baseline-button-pressed').value;
	const buttonRadius = parseInt(document.getElementById('baseline-button-radius').value)||8;
	const buttonBorderWidth = parseInt(document.getElementById('baseline-button-border-width').value)||1;
	
	document.getElementById('page-bg').value = pageBg;
	document.getElementById('nav-active').value = navActive;
	document.getElementById('nav-inactive').value = navInactive;
	document.getElementById('button-active').value = buttonActive;
	document.getElementById('button-pressed').value = buttonPressed;
	document.getElementById('button-radius').value = buttonRadius;
	document.getElementById('button-border-width').value = buttonBorderWidth;
	
	updatePageConfig();
	showBanner('Baseline applied to page configuration','success');
}

function saveAsBaseline(){
	const pageBg = document.getElementById('page-bg').value;
	const navActive = document.getElementById('nav-active').value;
	const navInactive = document.getElementById('nav-inactive').value;
	const buttonActive = document.getElementById('button-active').value;
	const buttonPressed = document.getElementById('button-pressed').value;
	const buttonRadius = parseInt(document.getElementById('button-radius').value)||8;
	const buttonBorderWidth = parseInt(document.getElementById('button-border-width').value)||1;
	
	document.getElementById('baseline-page-bg').value = pageBg;
	document.getElementById('baseline-nav-active').value = navActive;
	document.getElementById('baseline-nav-inactive').value = navInactive;
	document.getElementById('baseline-button-active').value = buttonActive;
	document.getElementById('baseline-button-pressed').value = buttonPressed;
	document.getElementById('baseline-button-radius').value = buttonRadius;
	document.getElementById('baseline-button-border-width').value = buttonBorderWidth;
	
	if(!config.theme) config.theme = {};
	config.theme.baseline_page_bg = pageBg;
	config.theme.baseline_nav_active = navActive;
	config.theme.baseline_nav_inactive = navInactive;
	config.theme.baseline_button_active = buttonActive;
	config.theme.baseline_button_pressed = buttonPressed;
	config.theme.baseline_button_radius = buttonRadius;
	config.theme.baseline_button_border_width = buttonBorderWidth;
	
	showBanner('Current page configuration saved as baseline','success');
}

function hydrateHeaderFields(){
	const header = config.header || {};
	const titleInput = document.getElementById('header-title-input');
	if (titleInput) titleInput.value = header.title || 'CAN Control';
	const subtitleInput = document.getElementById('header-subtitle-input');
	if (subtitleInput) subtitleInput.value = header.subtitle || '';
	const titleAlign = document.getElementById('header-title-align');
	if (titleAlign) titleAlign.value = header.title_align || 'center';
	const logoPlacement = document.getElementById('header-logo-position');
	if (logoPlacement) logoPlacement.value = header.logo_position || 'stacked';
	
	// Set full font names in selects
	const titleFont = document.getElementById('header-title-font');
	if (titleFont) titleFont.value = header.title_font || 'montserrat_24';
	
	const subFont = document.getElementById('header-subtitle-font');
	if (subFont) subFont.value = header.subtitle_font || 'montserrat_12';
	const sizeSlider = document.getElementById('header-logo-size');
	const sliderValue = clampLogoSize(header.logo_target_height || 64);
	if (sizeSlider) sizeSlider.value = sliderValue;
	setLogoSizeLabel(sliderValue);
	const keepAspect = document.getElementById('header-logo-keep-aspect');
	if (keepAspect) keepAspect.checked = header.logo_preserve_aspect !== false;
	const navSpacingSlider = document.getElementById('header-nav-spacing');
	const navSpacingValue = clampNavSpacing(header.nav_spacing ?? 12);
	if (navSpacingSlider) navSpacingSlider.value = navSpacingValue;
	setNavSpacingLabel(navSpacingValue);
	// show_logo checkbox removed - logo display controlled by Image Assets upload
}

function populateFontSelects(){
	const fonts = config.available_fonts || [];
	
	// Populate title and subtitle font selects with full font names
	const titleSel = document.getElementById('header-title-font');
	const subSel = document.getElementById('header-subtitle-font');
	
	if (!titleSel || !subSel) return;  // Elements don't exist yet
	
	[titleSel, subSel].forEach(sel => {
		sel.innerHTML = '';
		fonts.forEach(font => {
			const opt = document.createElement('option');
			opt.value = font.name;
			opt.textContent = font.display_name || font.name;
			sel.appendChild(opt);
		});
	});
}

function renderCanLibrary(){
	const list = document.getElementById('can-library-list');
	const items = config.can_library || [];
	if(items.length===0){ list.innerHTML = '<div class="muted">No messages yet.</div>'; return; }
	list.innerHTML = '';
	items.forEach((msg,idx)=>{
		const card = document.createElement('div');
		card.className = 'can-card';
		card.innerHTML = `<div style="display:flex;justify-content:space-between;align-items:center;">
			<div><strong>${msg.name}</strong><div class="muted">PGN 0x${msg.pgn.toString(16).toUpperCase()}</div></div>
			<div class="row" style="flex:0 0 auto;">
				<button class="btn small" onclick="editCanMessage(${idx})">Edit</button>
				<button class="btn small danger" onclick="deleteCanMessage(${idx})">Delete</button>
			</div>
		</div>
		<div class="muted" style="margin-top:6px;">${msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ')}</div>`;
		list.appendChild(card);
	});
}

function addCanMessage(){
	const name = prompt('Message name','New Message');
	if(!name) return;
	const pgn = parseInt(prompt('PGN (hex)','FEF6'),16);
	if(isNaN(pgn)) return;
	if(!config.can_library) config.can_library = [];
	config.can_library.push({ id:'msg_'+Date.now(), name, pgn, priority:6, source_address:0xF9, destination_address:0xFF, data:[0,0,0,0,0,0,0,0], description:'' });
	renderCanLibrary();
}

function editCanMessage(idx){
	const msg = config.can_library[idx];
	if(!msg) return;
	const name = prompt('Message name', msg.name);
	if(!name) return;
	const pgn = parseInt(prompt('PGN (hex)', msg.pgn.toString(16)),16);
	if(isNaN(pgn)) return;
	const dataStr = prompt('8 bytes (space separated)', msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' '));
	const data = dataStr.split(' ').map(b=>parseInt(b,16)||0).slice(0,8);
	while(data.length<8) data.push(0);
	config.can_library[idx] = {...msg, name, pgn, data};
	renderCanLibrary();
}

function deleteCanMessage(idx){
	if(!confirm('Delete this message?')) return;
	config.can_library.splice(idx,1);
	renderCanLibrary();
}

function importCanMessage(type){
	const templates = {
		windows:{name:'Windows',pgn:0xFEF6,data:[255,255,255,255,255,255,255,255]},
		locks:{name:'Locks',pgn:0xFECA,data:[0,0,0,0,255,255,255,255]},
		boards:{name:'Running Boards',pgn:0xFE00,data:[1,0,0,0,255,255,255,255]}
	};
	const t = templates[type];
	if(!t) return;
	if(!config.can_library) config.can_library = [];
	config.can_library.push({ id:'msg_'+Date.now(), name:t.name, pgn:t.pgn, priority:firstDefined(t.priority,6), source_address:firstDefined(t.source_address,0xF9), destination_address:firstDefined(t.destination_address,0xFF), data:t.data, description:'Quick import' });
	renderCanLibrary();
}

function populateCanLibraryDropdown(){
	const sels = [
		document.getElementById('btn-can-library-select'),
		document.getElementById('btn-can-off-library-select')
	].filter(Boolean);
	if(!sels.length) return;
	sels.forEach(sel=>{ sel.innerHTML = '<option value="">-- Select --</option>'; });
	(config.can_library||[]).forEach((msg,idx)=>{
		const label = `${msg.name} (PGN 0x${msg.pgn.toString(16).toUpperCase()})`;
		sels.forEach(sel=>{
			const opt = document.createElement('option');
			opt.value = idx;
			opt.textContent = label;
			sel.appendChild(opt);
		});
	});
}

function loadCanFromLibrary(){
	const sel = document.getElementById('btn-can-library-select');
	const idx = parseInt(sel.value);
	if(isNaN(idx)) return;
	const msg = config.can_library[idx];
	if(!msg) return;
	document.getElementById('btn-can-enabled').checked = true;
	document.getElementById('btn-can-pgn').value = msg.pgn.toString(16).toUpperCase();
	document.getElementById('btn-can-priority').value = msg.priority;
	document.getElementById('btn-can-src').value = msg.source_address.toString(16).toUpperCase();
	document.getElementById('btn-can-dest').value = msg.destination_address.toString(16).toUpperCase();
	document.getElementById('btn-can-data').value = msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	toggleCanFields();
}

function loadCanOffFromLibrary(){
	const sel = document.getElementById('btn-can-off-library-select');
	const idx = parseInt(sel.value);
	if(isNaN(idx)) return;
	const msg = config.can_library[idx];
	if(!msg) return;
	document.getElementById('btn-can-off-enabled').checked = true;
	document.getElementById('btn-can-off-pgn').value = msg.pgn.toString(16).toUpperCase();
	document.getElementById('btn-can-off-priority').value = msg.priority;
	document.getElementById('btn-can-off-src').value = msg.source_address.toString(16).toUpperCase();
	document.getElementById('btn-can-off-dest').value = msg.destination_address.toString(16).toUpperCase();
	document.getElementById('btn-can-off-data').value = msg.data.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
	toggleCanFields();
}

async function scanWiFi(){
	const btn = document.getElementById('scan-btn');
	btn.textContent = 'Scanning...';
	btn.disabled = true;
	try{
		const res = await fetch('/api/wifi/scan');
		const data = await res.json();
		wifiNetworks = data.networks || [];
		renderWifiList();
		showBanner(`Found ${wifiNetworks.length} networks`,`success`);
	}catch(err){ showBanner('Scan failed: '+err.message,'error'); }
	btn.textContent = 'Scan';
	btn.disabled = false;
}

function renderWifiList(){
	const list = document.getElementById('wifi-results');
	if(!wifiNetworks.length){ list.innerHTML = '<div class="muted">No networks yet.</div>'; return; }
	list.innerHTML = '';
	wifiNetworks.forEach((net,idx)=>{
		const item = document.createElement('div');
		item.className = 'wifi-item';
		item.innerHTML = `<div><strong>${net.ssid}</strong><div class="muted">Channel ${net.channel||'?'} · RSSI ${net.rssi||''}</div></div><div>${net.secure?'Locked':'Open'}</div>`;
		item.onclick = ()=>{
			document.getElementById('sta-ssid').value = net.ssid;
			document.getElementById('sta-password').focus();
			document.querySelectorAll('.wifi-item').forEach(el=>el.classList.remove('active'));
			item.classList.add('active');
		};
		list.appendChild(item);
	});
}

async function refreshStatus(){
	const statusContainer = document.getElementById('status');
	if(!statusContainer) return;
	try{
		const res = await fetch('/api/status');
		const status = await res.json();
		const firmwareVersion = status.firmware_version || statusContainer.dataset.version || '—';
		const deviceIp = status.device_ip || status.sta_ip || status.ap_ip || '—';
		const connectedNetwork = status.connected_network || (status.sta_connected ? 'Hidden network' : '—');
		statusContainer.innerHTML = `
			<div class="status-chip"><span>Firmware</span>v${firmwareVersion}</div>
			<div class="status-chip"><span>Device IP</span>${deviceIp || '—'}</div>
			<div class="status-chip"><span>Connected Network</span>${connectedNetwork || '—'}</div>
			<div class="status-chip"><span>AP IP</span>${status.ap_ip || 'N/A'}</div>
			<div class="status-chip"><span>Station IP</span>${status.sta_ip || '—'}</div>
		`;
	}catch(err){
		const firmwareVersion = statusContainer.dataset.version || '—';
		statusContainer.innerHTML = `
			<div class="status-chip"><span>Firmware</span>v${firmwareVersion}</div>
			<div class="status-chip"><span>Device IP</span>Unavailable</div>
			<div class="status-chip"><span>Connected Network</span>Unavailable</div>
			<div class="status-chip"><span>AP IP</span>Unavailable</div>
			<div class="status-chip"><span>Station IP</span>Unavailable</div>
		`;
	}
}

// Legacy logo upload function removed - use Image Assets section instead

function handleSleepIconUpload(evt){
	const file = evt.target.files[0];
	if(!file) return;
	if(file.size>512000){ showBanner('Sleep icon too large (500KB)','error'); evt.target.value=''; return; }
	const reader = new FileReader();
	reader.onload = (e)=>{
		if(!config.display) config.display={};
		config.display.sleep_icon_base64 = e.target.result;
		document.getElementById('sleep-icon-preview-img').src = e.target.result;
		document.getElementById('sleep-icon-preview').style.display='block';
	};
	reader.readAsDataURL(file);
}

// clearSleepIcon removed - use Image Assets clear button instead

// Image upload configurations - reduced sizes for memory safety
const IMAGE_CONFIGS = {
	header: {
		maxSize: [240, 96],
		maxBytes: 80 * 1024,  // Allow higher-res logos to match on-device sizing
		format: 'PNG',
		hasAlpha: true,
		configPath: 'images.header_logo'
	},
	splash: {
		maxSize: [300, 225],  // Reduced from 400x300
		maxBytes: 40 * 1024,  // 40KB max
		format: 'JPEG',  // Changed to JPEG for better compression
		hasAlpha: false,
		configPath: 'images.splash_logo'
	},
	background: {
		maxSize: [400, 240],  // Reduced from 800x480 for memory
		maxBytes: 50 * 1024,  // 50KB max
		format: 'JPEG',
		hasAlpha: false,
		configPath: 'images.background_image'
	},
	sleep: {
		maxSize: [150, 113],  // Reduced from 200x150
		maxBytes: 60 * 1024,  // Allow larger raw lvimg payload
		format: 'PNG',
		hasAlpha: true,
		configPath: 'images.sleep_logo'
	}
};

const LVGL_IMAGE_TYPES = new Set(['header','sleep']);
const lvimgPreviewCache = new Map();

function needsLvglPayload(imageType) {
	return LVGL_IMAGE_TYPES.has(imageType);
}

function getHeaderUploadSettings(){
	const sizeInput = document.getElementById('header-logo-size');
	const placement = document.getElementById('header-logo-position');
	const keepAspectInput = document.getElementById('header-logo-keep-aspect');
	const clamped = clampLogoSize(sizeInput ? sizeInput.value : 64);
	const inlineLayout = placement && (placement.value === 'inline-left' || placement.value === 'inline-right');
	const widthBudget = inlineLayout ? Math.max(80, Math.round(clamped * 2.5)) : Math.max(80, Math.round(clamped * 1.8));
	return {
		width: widthBudget,
		height: clamped,
		keepAspect: keepAspectInput ? keepAspectInput.checked : true
	};
}

function blobToDataUrl(blob) {
	return new Promise((resolve, reject) => {
		const reader = new FileReader();
		reader.onload = () => resolve(reader.result);
		reader.onerror = () => reject(new Error('Failed to read optimized image'));
		reader.readAsDataURL(blob);
	});
}

function bufferToBase64(buffer) {
	let binary = '';
	const chunkSize = 0x8000;
	for (let i = 0; i < buffer.length; i += chunkSize) {
		const chunk = buffer.subarray(i, i + chunkSize);
		let str = '';
		for (let j = 0; j < chunk.length; j++) {
			str += String.fromCharCode(chunk[j]);
		}
		binary += str;
	}
	return btoa(binary);
}

function rgbaToRgb565a(rgbaPixels) {
	const pixelCount = rgbaPixels.length / 4;
	const buffer = new Uint8Array(pixelCount * 3);
	for (let i = 0, src = 0; i < pixelCount; i++, src += 4) {
		const r = rgbaPixels[src];
		const g = rgbaPixels[src + 1];
		const b = rgbaPixels[src + 2];
		const a = rgbaPixels[src + 3];
		const rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
		const dst = i * 3;
		buffer[dst] = rgb565 & 0xFF;
		buffer[dst + 1] = (rgb565 >> 8) & 0xFF;
		buffer[dst + 2] = a;
	}
	return buffer;
}

function stripNearWhiteBackground(imageData, tolerance = 20) {
	if (!imageData) return 0;
	const clampedTolerance = Math.max(1, Math.min(80, tolerance));
	const threshold = 255 - clampedTolerance;
	const data = imageData.data;
	let modified = 0;
	for (let i = 0; i < data.length; i += 4) {
		const r = data[i];
		const g = data[i + 1];
		const b = data[i + 2];
		const a = data[i + 3];
		if (a === 0) continue;
		const maxChannel = Math.max(r, g, b);
		const minChannel = Math.min(r, g, b);
		if (maxChannel >= threshold && (maxChannel - minChannel) <= clampedTolerance) {
			data[i + 3] = 0;
			modified++;
		}
	}
	return modified;
}

async function drawBlobToCanvas(blob) {
	return new Promise((resolve, reject) => {
		const img = new Image();
		const url = URL.createObjectURL(blob);
		img.onload = () => {
			const canvas = document.createElement('canvas');
			canvas.width = img.naturalWidth || img.width;
			canvas.height = img.naturalHeight || img.height;
			const ctx = canvas.getContext('2d');
			if (!ctx) {
				URL.revokeObjectURL(url);
				reject(new Error('Canvas context unavailable'));
				return;
			}
			ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
			const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
			URL.revokeObjectURL(url);
			resolve({ canvas, imageData, ctx });
		};
		img.onerror = () => {
			URL.revokeObjectURL(url);
			reject(new Error('Failed to decode optimized image'));
		};
		img.src = url;
	});
}

async function blobToLvglPayload(blob, options = {}) {
	const { canvas, imageData, ctx } = await drawBlobToCanvas(blob);
	if (options.stripWhiteBg && imageData && ctx) {
		const removed = stripNearWhiteBackground(imageData, options.bgTolerance ?? 20);
		if (removed > 0) {
			ctx.putImageData(imageData, 0, 0);
			console.log(`Removed ${removed.toLocaleString()} near-white pixels for transparency`);
		}
	}
	const { width, height, data } = imageData;
	const rgb565a = rgbaToRgb565a(data);
	return {
		payload: `lvimg:rgb565a:${width}x${height}:${bufferToBase64(rgb565a)}`,
		previewDataUrl: canvas.toDataURL('image/png'),
		rawBytes: rgb565a.length,
		width,
		height
	};
}

function base64ToUint8Array(base64) {
	try {
		const binary = atob(base64);
		const len = binary.length;
		const bytes = new Uint8Array(len);
		for (let i = 0; i < len; i++) {
			bytes[i] = binary.charCodeAt(i);
		}
		return bytes;
	} catch (err) {
		console.error('lvimg decode failed', err);
		return null;
	}
}

function parseLvimgPayload(payload) {
	if (!payload || payload.indexOf('lvimg:') !== 0) {
		return null;
	}
	const fmtSep = payload.indexOf(':', 6);
	if (fmtSep === -1) return null;
	const sizeSep = payload.indexOf(':', fmtSep + 1);
	if (sizeSep === -1) return null;
	const sizePart = payload.slice(fmtSep + 1, sizeSep);
	const [wStr, hStr] = sizePart.split('x');
	const width = parseInt(wStr, 10);
	const height = parseInt(hStr, 10);
	if (!width || !height) return null;
	const base64 = payload.slice(sizeSep + 1);
	const buffer = base64ToUint8Array(base64);
	if (!buffer) return null;
	return { format: payload.slice(6, fmtSep), width, height, buffer };
}

function lvimgToDataUrl(payload) {
	const parsed = parseLvimgPayload(payload);
	if (!parsed) return null;
	const { format, width, height, buffer } = parsed;
	const bytesPerPixel = format === 'rgb565a' ? 3 : (format === 'rgb565' ? 2 : 0);
	if (!bytesPerPixel) return null;
	const expected = width * height * bytesPerPixel;
	if (buffer.length !== expected) return null;
	const rgba = new Uint8ClampedArray(width * height * 4);
	for (let i = 0, src = 0, dst = 0; i < width * height; i++, dst += 4, src += bytesPerPixel) {
		const color = buffer[src] | (buffer[src + 1] << 8);
		let r = (color >> 11) & 0x1F;
		let g = (color >> 5) & 0x3F;
		let b = color & 0x1F;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		rgba[dst] = r;
		rgba[dst + 1] = g;
		rgba[dst + 2] = b;
		rgba[dst + 3] = (format === 'rgb565a') ? buffer[src + 2] : 255;
	}
	const canvas = document.createElement('canvas');
	canvas.width = width;
	canvas.height = height;
	const ctx = canvas.getContext('2d');
	if (!ctx) return null;
	ctx.putImageData(new ImageData(rgba, width, height), 0, 0);
	return canvas.toDataURL('image/png');
}

function getImagePreviewSrc(value) {
	if (!value) return null;
	if (value.startsWith('lvimg:')) {
		if (lvimgPreviewCache.has(value)) {
			return lvimgPreviewCache.get(value);
		}
		const dataUrl = lvimgToDataUrl(value);
		if (dataUrl) {
			lvimgPreviewCache.set(value, dataUrl);
		}
		return dataUrl;
	}
	return value;
}

// Image processing utility - resize image using canvas
function resizeImage(file, imageType) {
	return new Promise((resolve, reject) => {
		const imgConfig = IMAGE_CONFIGS[imageType];
		if (!imgConfig) {
			reject(new Error('Unknown image type'));
			return;
		}

		const reader = new FileReader();
		reader.onload = (e) => {
			const img = new Image();
			img.onload = () => {
				const canvas = document.createElement('canvas');
				const ctx = canvas.getContext('2d');

				// Calculate scaling to fit within max dimensions
				const [maxWidth, maxHeight] = imgConfig.maxSize;
				let { width, height } = img;
				const ratio = Math.min(maxWidth / width, maxHeight / height, 1);
				
				width = Math.floor(width * ratio);
				height = Math.floor(height * ratio);

				canvas.width = width;
				canvas.height = height;

				// Draw resized image
				ctx.drawImage(img, 0, 0, width, height);

				// Progressive compression to hit size target
				const mimeType = imgConfig.format === 'PNG' ? 'image/png' : 'image/jpeg';
				let qualityLevels = imgConfig.format === 'PNG' 
					? [0.9, 0.8, 0.7, 0.6, 0.5]  // PNG quality levels
					: [0.8, 0.7, 0.6, 0.5, 0.4, 0.3];  // JPEG quality levels
				
				let qualityIndex = 0;
				
				const tryCompress = () => {
					if (qualityIndex >= qualityLevels.length) {
						reject(new Error(`Cannot compress image below ${(imgConfig.maxBytes / 1024).toFixed(1)}KB. Try a simpler image or remove details.`));
						return;
					}
					
					const quality = qualityLevels[qualityIndex];
					canvas.toBlob((blob) => {
						if (!blob) {
							reject(new Error('Failed to process image'));
							return;
						}

						console.log(`${imageType}: Trying quality ${quality.toFixed(2)}, size: ${(blob.size / 1024).toFixed(1)}KB`);
						
						if (blob.size > imgConfig.maxBytes) {
							qualityIndex++;
							tryCompress();  // Try next quality level
						} else {
							// Success - convert to base64
							const reader2 = new FileReader();
							reader2.onload = () => resolve({ 
								dataUrl: reader2.result, 
								size: blob.size, 
								width, 
								height,
								quality: quality
							});
							reader2.readAsDataURL(blob);
						}
					}, mimeType, quality);
				};
				
				tryCompress();
			};
			img.onerror = () => reject(new Error('Failed to load image'));
			img.src = e.target.result;
		};
		reader.onerror = () => reject(new Error('Failed to read file'));
		reader.readAsDataURL(file);
	});
}

// Handle image upload for any type with Fly optimization
async function handleImageUpload(evt, imageType) {
	const file = evt.target.files[0];
	if (!file) return;

	// Check file size (20MB max for upload to Fly)
	if (file.size > 20 * 1024 * 1024) {
		showBanner(`Image too large (${(file.size/(1024*1024)).toFixed(1)}MB). Maximum 20MB.`, 'error');
		evt.target.value = '';
		return;
	}

	try {
		showBanner(`Optimizing ${imageType} image via Fly.io...`, 'info');
		const imgConfig = IMAGE_CONFIGS[imageType];
		if (!imgConfig) {
			throw new Error('Unsupported image type');
		}
		
		let targetWidth = imgConfig.maxSize[0];
		let targetHeight = imgConfig.maxSize[1];
		let fitMode = 'contain';
		if (imageType === 'header') {
			const sizing = getHeaderUploadSettings();
			targetWidth = sizing.width;
			targetHeight = sizing.height;
			fitMode = sizing.keepAspect ? 'contain' : 'fill';
			config.header = config.header || {};
			config.header.logo_target_height = sizing.height;
			config.header.logo_preserve_aspect = sizing.keepAspect;
		}
		const format = imgConfig.format.toLowerCase();
		
		const formData = new FormData();
		formData.append('file', file);
		
		const flyUrl = 'https://image-optimizer-still-flower-1282.fly.dev/optimize';
		const params = new URLSearchParams({
			w: targetWidth.toString(),
			h: targetHeight.toString(),
			fit: fitMode,
			fmt: format === 'png' ? 'png' : 'jpeg',
			q: '80',
			bg: '000000',
			rotate: '1',
			alpha: imgConfig.hasAlpha ? '1' : '0'
		});
		if (imageType === 'header') {
			params.set('strip', 'white');
			params.set('strip_tol', '32');
		}
		
		console.log(`Optimizing ${imageType} via Fly: ${flyUrl}?${params}`);
		
		const optimizeResponse = await fetch(`${flyUrl}?${params}`, {
			method: 'POST',
			body: formData
		});
		
		if (!optimizeResponse.ok) {
			throw new Error(`Fly optimization failed: ${optimizeResponse.status} ${optimizeResponse.statusText}`);
		}
		
		const optimizedBlob = await optimizeResponse.blob();
		console.log(`Optimized ${imageType}: ${(optimizedBlob.size / 1024).toFixed(1)}KB via Fly`);

		const needsLvgl = needsLvglPayload(imageType);
		let storagePayload;
		let previewDataUrl;
		let payloadBytes = optimizedBlob.size;
		if (needsLvgl) {
			const lvglOptions = (imageType === 'header') ? { stripWhiteBg: true, bgTolerance: 24 } : {};
			const lvglPayload = await blobToLvglPayload(optimizedBlob, lvglOptions);
			storagePayload = lvglPayload.payload;
			previewDataUrl = lvglPayload.previewDataUrl;
			payloadBytes = lvglPayload.rawBytes;
			lvimgPreviewCache.set(storagePayload, previewDataUrl);
		} else {
			previewDataUrl = await blobToDataUrl(optimizedBlob);
			storagePayload = previewDataUrl;
		}

		if (payloadBytes > imgConfig.maxBytes) {
			showBanner(`Optimized image still too large (${(payloadBytes/1024).toFixed(1)}KB raw). Try a simpler image.`, 'error');
			evt.target.value = '';
			return;
		}
		
		// Upload to ESP32
		showBanner(`Uploading optimized ${imageType} to device...`, 'info');
		const response = await fetch('/api/image/upload', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({
				type: imageType,
				data: storagePayload
			})
		});
		
		if (!response.ok) {
			throw new Error(`ESP32 upload failed: ${response.statusText}`);
		}
		
		// Store in local config for preview
		if (!config.images) config.images = {};
		const configPath = IMAGE_CONFIGS[imageType].configPath.split('.');
		if (configPath[0] === 'images') {
			config.images[configPath[1]] = storagePayload;
		}
		if (imageType === 'header') {
			config.header = config.header || {};
			config.header.show_logo = true;
		}

		// Update preview
		const previewImg = document.getElementById(`${imageType}-logo-preview-img`);
		const previewDiv = document.getElementById(`${imageType}-logo-preview`);
		const sizeSpan = document.getElementById(`${imageType}-logo-size`);
		
		if (previewImg && previewDiv && sizeSpan) {
			previewImg.src = previewDataUrl;
			previewDiv.style.display = 'block';
			const img = new Image();
			img.onload = () => {
				sizeSpan.textContent = `${img.width}x${img.height}, ${(optimizedBlob.size / 1024).toFixed(1)}KB (optimized)`;
			};
			img.src = previewDataUrl;
		}

		renderPreview();
		showBanner(`✅ ${imageType} image optimized & uploaded! Check device display.`, 'success');
		evt.target.value = '';
	} catch (error) {
		console.error('Image upload error:', error);
		showBanner(`Error: ${error.message}`, 'error');
		evt.target.value = '';
	}
}

// Clear image
function clearImage(imageType) {
	if (!config.images) config.images = {};
	const configPath = IMAGE_CONFIGS[imageType].configPath.split('.');
	if (configPath[0] === 'images') {
		config.images[configPath[1]] = '';
	}
	if (imageType === 'header') {
		config.header = config.header || {};
		config.header.show_logo = false;
	}
	
	// Upload empty image to server
	fetch('/api/image/upload', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({
			type: imageType,
			data: ''
		})
	}).then(response => {
		if (response.ok) {
			const previewDiv = document.getElementById(`${imageType}-logo-preview`);
			const uploadInput = document.getElementById(`${imageType}-logo-upload`);
			lvimgPreviewCache.clear();
			
			if (previewDiv) previewDiv.style.display = 'none';
			if (uploadInput) uploadInput.value = '';
			renderPreview();
			
			showBanner(`${imageType} image cleared`, 'success');
		}
	}).catch(err => {
		showBanner(`Error clearing image: ${err.message}`, 'error');
	});
}

function hydrateDisplay(){
	const display = config.display || {};
	const brightnessEl = document.getElementById('display-brightness');
	if (brightnessEl) brightnessEl.value = display.brightness ?? 100;
	const brightnessValue = document.getElementById('brightness-value');
	if (brightnessValue) brightnessValue.textContent = `${display.brightness ?? 100}%`;
	const sleepEnabled = document.getElementById('sleep-enabled');
	if (sleepEnabled) sleepEnabled.checked = display.sleep_enabled || false;
	const sleepTimeout = document.getElementById('sleep-timeout');
	if (sleepTimeout) sleepTimeout.value = display.sleep_timeout_seconds ?? 60;

	// Hydrate image assets
	const images = config.images || {};
	
	const headerPreviewSrc = getImagePreviewSrc(images.header_logo);
	if (headerPreviewSrc) {
		const headerPreviewImg = document.getElementById('header-logo-preview-img');
		if (headerPreviewImg) headerPreviewImg.src = headerPreviewSrc;
		const headerPreview = document.getElementById('header-logo-preview');
		if (headerPreview) headerPreview.style.display = 'block';
	}

	const splashPreviewSrc = getImagePreviewSrc(images.splash_logo);
	if (splashPreviewSrc) {
		const splashPreviewImg = document.getElementById('splash-logo-preview-img');
		if (splashPreviewImg) splashPreviewImg.src = splashPreviewSrc;
		const splashPreview = document.getElementById('splash-logo-preview');
		if (splashPreview) splashPreview.style.display = 'block';
	}

	const bgPreviewSrc = getImagePreviewSrc(images.background_image);
	if (bgPreviewSrc) {
		const bgPreviewImg = document.getElementById('background-preview-img');
		if (bgPreviewImg) bgPreviewImg.src = bgPreviewSrc;
		const bgPreview = document.getElementById('background-preview');
		if (bgPreview) bgPreview.style.display = 'block';
	}

	const sleepPreviewSrc = getImagePreviewSrc(images.sleep_logo);
	if (sleepPreviewSrc) {
		const sleepImg = document.getElementById('sleep-logo-preview-img');
		const sleepPreview = document.getElementById('sleep-logo-preview');
		if (sleepImg) sleepImg.src = sleepPreviewSrc;
		if (sleepPreview) sleepPreview.style.display = 'block';
	}

	// Legacy sleep icon support
	if(display.sleep_icon_base64){
		if (!config.images) config.images = {};
		if (!config.images.sleep_logo) {
			config.images.sleep_logo = display.sleep_icon_base64;
		}
	}
}

async function loadConfig(){
	console.log('[WEB] loadConfig() called');
	try{
		const res = await fetch('/api/config');
		config = await res.json();
		console.log('[WEB] Config loaded:', config);
		console.log('[WEB] Pages before ensurePages:', config.pages);
		lvimgPreviewCache.clear();
		ensurePages();
		console.log('[WEB] Pages after ensurePages:', config.pages);
		populateFontSelects();  // Must populate fonts BEFORE hydrating header fields
		hydrateThemeFields();
		hydrateHeaderFields();
		hydratePageFields();
		hydrateDisplay();
		const wifiCfg = config.wifi || {};
		const apCfg = wifiCfg.ap || {};
		const staCfg = wifiCfg.sta || {};
		const apSsid = document.getElementById('ap-ssid');
		if (apSsid) apSsid.value = apCfg.ssid || 'CAN-Control';
		const apPassword = document.getElementById('ap-password');
		if (apPassword) apPassword.value = apCfg.password || '';
		const apEnabled = document.getElementById('ap-enabled');
		if (apEnabled) apEnabled.checked = apCfg.enabled !== false;
		const staSsid = document.getElementById('sta-ssid');
		if (staSsid) staSsid.value = staCfg.ssid || '';
		const staPassword = document.getElementById('sta-password');
		if (staPassword) staPassword.value = staCfg.password || '';
		const staEnabled = document.getElementById('sta-enabled');
		if (staEnabled) staEnabled.checked = staCfg.enabled || false;
		renderPageList();
		renderGrid();
		renderPreview();
		renderCanLibrary();
		refreshStatus();
		showBanner('Config loaded','success');
	}catch(err){ showBanner('Load failed: '+err.message,'error'); }
}

async function saveConfig(){
	ensurePages();
	collectTheme();
	config.header = config.header || {};
	const titleInput = document.getElementById('header-title-input');
	if (titleInput) config.header.title = titleInput.value || 'CAN Control';
	const subtitleInput = document.getElementById('header-subtitle-input');
	if (subtitleInput) config.header.subtitle = subtitleInput.value || '';
	const titleFont = document.getElementById('header-title-font');
	if (titleFont) config.header.title_font = titleFont.value || 'montserrat_24';
	const subtitleFont = document.getElementById('header-subtitle-font');
	if (subtitleFont) config.header.subtitle_font = subtitleFont.value || 'montserrat_12';
	const titleAlign = document.getElementById('header-title-align');
	if (titleAlign) config.header.title_align = titleAlign.value || 'center';
	// show_logo controlled by whether image exists
	config.header.show_logo = !!(config.images && config.images.header_logo);

	const apEnabled = document.getElementById('ap-enabled');
	const apSsid = document.getElementById('ap-ssid');
	const apPassword = document.getElementById('ap-password');
	const staEnabled = document.getElementById('sta-enabled');
	const staSsid = document.getElementById('sta-ssid');
	const staPassword = document.getElementById('sta-password');
	config.wifi = {
		ap:{ enabled: apEnabled ? apEnabled.checked : true, ssid: apSsid ? apSsid.value : 'CAN-Control', password: apPassword ? apPassword.value : '' },
		sta:{ enabled: staEnabled ? staEnabled.checked : false, ssid: staSsid ? staSsid.value : '', password: staPassword ? staPassword.value : '' }
	};

	const existingDisplay = config.display || {};
	config.display = {
		brightness: parseInt(document.getElementById('display-brightness').value)||100,
		sleep_enabled: document.getElementById('sleep-enabled').checked,
		sleep_timeout_seconds: parseInt(document.getElementById('sleep-timeout').value)||60,
		sleep_icon_base64: existingDisplay.sleep_icon_base64 || ''
	};
	
	// Remove images from config - they're uploaded separately via /api/image/upload
	const configToSave = JSON.parse(JSON.stringify(config));
	delete configToSave.images;
	if (configToSave.header) {
		delete configToSave.header.logo_base64;
	}
	if (configToSave.display) {
		delete configToSave.display.sleep_icon_base64;
	}

	try{
		const res = await fetch('/api/config',{ method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(configToSave) });
		if(!res.ok){ const text = await res.text(); throw new Error(text); }
		showBanner('Configuration saved. Reboot device to apply display changes.','success');
	}catch(err){ showBanner('Save failed: '+err.message,'error'); }
}

async function checkForUpdates(){
	const btn = document.querySelector('button[onclick="checkForUpdates()"]');
	if (btn) btn.disabled = true;
	const updateChip = document.getElementById('update-available');
	if (updateChip) updateChip.textContent = 'Checking...';
	try{
		const res = await fetch('/api/ota/github/versions');
		const data = await res.json();
		if (data.status === 'ok' && data.versions && data.versions.length > 0) {
			const currentVersion = data.current;
			
			// Create version selector dropdown
			const updateSection = document.getElementById('update-section');
			if (updateSection) {
				updateSection.innerHTML = `
					<div style="display:flex;flex-direction:column;gap:10px;width:100%;">
						<div style="display:flex;gap:10px;align-items:center;flex-wrap:wrap;">
							<label for="version-select" style="color:var(--text-secondary);">Select Version:</label>
							<select id="version-select" style="padding:8px;border-radius:4px;background:#2a2a2a;color:#ffffff;border:1px solid #444;flex:1;min-width:150px;">
								${data.versions.map(v => `<option value="${v}" style="background:#2a2a2a;color:#ffffff;" ${v === currentVersion ? 'selected' : ''}>${v}${v === currentVersion ? ' (current)' : ''}</option>`).join('')}
							</select>
						</div>
						<button id="update-btn" onclick="triggerOTAUpdate()" style="padding:10px 20px;background:var(--accent);color:#000;border:none;border-radius:4px;cursor:pointer;font-weight:600;">Install Selected Version</button>
						<div style="font-size:0.85em;color:var(--text-secondary);line-height:1.4;">
							<strong>Current:</strong> ${currentVersion} • <strong>Available:</strong> ${data.versions.length} version(s)<br/>
							<em>OTA updates use .bin firmware files for fast wireless installation</em>
						</div>
					</div>
				`;
			}
			
			if (updateChip) updateChip.innerHTML = `<span style="color: var(--success);">${data.versions.length} available</span>`;
			showBanner(`Found ${data.versions.length} OTA-capable version(s)`, 'success');
		} else {
			if (updateChip) updateChip.textContent = 'No versions found';
			showBanner('No GitHub versions available', 'error');
		}
	}catch(err){
		if (updateChip) updateChip.textContent = 'Error';
		showBanner('Check failed: '+err.message, 'error');
	}finally{
		if (btn) btn.disabled = false;
	}
}

async function triggerOTAUpdate(){
	const select = document.getElementById('version-select');
	const version = select ? select.value : null;
	
	if (!version) {
		showBanner('No version selected', 'error');
		return;
	}
	
	if (!confirm(`Install version ${version}? The device will reboot.`)) return;
	
	const btn = document.getElementById('update-btn');
	if (btn) {
		btn.disabled = true;
		btn.textContent = 'Starting...';
	}
	
	// Create progress indicator
	const updateSection = document.getElementById('update-section');
	if (updateSection) {
		updateSection.innerHTML = `
			<div style="display:flex;flex-direction:column;gap:15px;width:100%;padding:20px;background:#1a1a1a;border-radius:8px;">
				<div style="text-align:center;">
					<h3 style="margin:0;color:var(--accent);">Installing Version ${version}</h3>
					<p id="ota-status-text" style="color:var(--text-secondary);margin:10px 0;">Initializing...</p>
				</div>
				<div style="width:100%;height:30px;background:#333;border-radius:15px;overflow:hidden;position:relative;">
					<div id="ota-progress-bar" style="width:0%;height:100%;background:linear-gradient(90deg, var(--accent) 0%, #ffa500 100%);transition:width 0.3s ease;"></div>
					<div id="ota-progress-text" style="position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);color:#fff;font-weight:600;">0%</div>
				</div>
				<div id="ota-messages" style="font-size:0.85em;color:var(--text-secondary);max-height:150px;overflow-y:auto;"></div>
			</div>
		`;
	}
	
	try{
		const res = await fetch('/api/ota/github/install', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify({ version: version })
		});
		const data = await res.json();
		if (data.status === 'ok') {
			// Start polling for status
			pollOTAStatus();
		} else {
			showBanner(data.message || 'Update failed', 'error');
			setTimeout(() => checkForUpdates(), 2000);
		}
	}catch(err){
		showBanner('Update request failed: '+err.message, 'error');
		setTimeout(() => checkForUpdates(), 2000);
	}
}

let otaPolling = null;
async function pollOTAStatus() {
	if (otaPolling) clearInterval(otaPolling);
	
	let lastProgress = 0;
	let consecutiveErrors = 0;
	
	otaPolling = setInterval(async () => {
		try {
			const res = await fetch('/api/ota/status');
			if (!res.ok) {
				consecutiveErrors++;
				if (consecutiveErrors > 5) {
					clearInterval(otaPolling);
					showBanner('Lost connection to device. It may be rebooting...', 'info');
					setTimeout(() => {
						showBanner('Attempting to reconnect...', 'info');
						window.location.reload();
					}, 10000);
				}
				return;
			}
			
			consecutiveErrors = 0;
			const data = await res.json();
			
			// Update progress bar
			const progressBar = document.getElementById('ota-progress-bar');
			const progressText = document.getElementById('ota-progress-text');
			const statusText = document.getElementById('ota-status-text');
			
			if (progressBar && data.progress !== undefined) {
				progressBar.style.width = data.progress + '%';
				if (progressText) progressText.textContent = data.progress + '%';
			}
			
			if (statusText && data.message) {
				statusText.textContent = data.message;
			}
			
			// Add status messages
			const messages = document.getElementById('ota-messages');
			if (messages && data.message && data.progress > lastProgress) {
				const msg = document.createElement('div');
				msg.textContent = `[${new Date().toLocaleTimeString()}] ${data.message} (${data.progress}%)`;
				messages.appendChild(msg);
				messages.scrollTop = messages.scrollHeight;
				lastProgress = data.progress;
			}
			
			// Check if update completed
			if (data.progress >= 100 || data.message.includes('Success') || data.message.includes('Rebooting')) {
				clearInterval(otaPolling);
				showBanner('Update successful! Device is rebooting...', 'success');
				setTimeout(() => {
					showBanner('Waiting for device to come back online...', 'info');
					window.location.reload();
				}, 5000);
			}
			
		} catch (err) {
			consecutiveErrors++;
			console.error('OTA status poll error:', err);
			if (consecutiveErrors > 5) {
				clearInterval(otaPolling);
				showBanner('Device is rebooting. Please wait...', 'info');
				setTimeout(() => window.location.reload(), 15000);
			}
		}
	}, 500); // Poll every 500ms for smooth updates
}

document.addEventListener('DOMContentLoaded',()=>{
	loadConfig();
	refreshStatus();
	
	// Image upload handlers for new Image Assets section
	const headerUpload = document.getElementById('header-logo-upload');
	if (headerUpload) headerUpload.addEventListener('change', (e) => handleImageUpload(e, 'header'));
	
	const splashUpload = document.getElementById('splash-logo-upload');
	if (splashUpload) splashUpload.addEventListener('change', (e) => handleImageUpload(e, 'splash'));
	
	const bgUpload = document.getElementById('background-upload');
	if (bgUpload) bgUpload.addEventListener('change', (e) => handleImageUpload(e, 'background'));
	
	const sleepUpload = document.getElementById('sleep-logo-upload');
	if (sleepUpload) sleepUpload.addEventListener('change', (e) => handleImageUpload(e, 'sleep'));
	
	// Enable drag-and-drop for logo and header text positioning
	const headerEl = document.getElementById('preview-header');
	const logoEl = headerEl?.querySelector('.preview-logo');
	const titleWrap = headerEl?.querySelector('.title-wrap');
	
	let draggedElement = null;
	let dragStartX = 0;
	let dragStartY = 0;
	
	if (logoEl) {
		// Make logo draggable for repositioning
		logoEl.setAttribute('draggable', 'true');
		logoEl.addEventListener('dragstart', (e) => {
			draggedElement = logoEl;
			logoEl.style.opacity = '0.5';
			e.dataTransfer.effectAllowed = 'move';
		});
		logoEl.addEventListener('dragend', (e) => {
			logoEl.style.opacity = '1';
			draggedElement = null;
		});
		
		// Allow resizing by dragging corners (using CSS resize property)
		// When user resizes, update the config
		let resizeObserver = new ResizeObserver(() => {
			const newHeight = logoEl.offsetHeight;
			if (config.header && Math.abs(newHeight - (config.header.logo_size || 48)) > 2) {
				config.header.logo_size = newHeight;
				document.getElementById('header-logo-size').value = newHeight;
			}
		});
		resizeObserver.observe(logoEl);
	}
	
	if (titleWrap) {
		// Make title draggable
		titleWrap.setAttribute('draggable', 'true');
		titleWrap.addEventListener('dragstart', (e) => {
			draggedElement = titleWrap;
			titleWrap.style.opacity = '0.5';
			e.dataTransfer.effectAllowed = 'move';
		});
		titleWrap.addEventListener('dragend', (e) => {
			titleWrap.style.opacity = '1';
			draggedElement = null;
		});
	}
	
	if (headerEl) {
		headerEl.addEventListener('dragover', (e) => {
			e.preventDefault();
			e.dataTransfer.dropEffect = 'move';
		});
		
		headerEl.addEventListener('drop', (e) => {
			e.preventDefault();
			if (!draggedElement) return;
			
			// Swap positions of logo and title
			if (draggedElement === logoEl) {
				const placement = config.header?.logo_position || 'stacked';
				if (placement === 'inline-left') {
					config.header.logo_position = 'inline-right';
				} else if (placement === 'inline-right') {
					config.header.logo_position = 'inline-left';
				} else {
					// Toggle between inline layouts
					config.header.logo_position = 'inline-right';
				}
				document.getElementById('header-logo-position').value = config.header.logo_position;
				updateHeaderPreview();
			}
		});
	}
	
	wireThemeInputs();
});
</script>
</body>
</html>
)rawliteral";
