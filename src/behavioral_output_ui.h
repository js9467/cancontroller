#pragma once

#include <Arduino.h>

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT WEB INTERFACE                                          ║
 * ║                                                                           ║
 * ║  Provides intuitive UI for:                                               ║
 * ║    - Defining outputs with physical mapping                              ║
 * ║    - Building and activating scenes                                       ║
 * ║    - Real-time testing and preview                                        ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

const char BEHAVIORAL_OUTPUT_UI[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Behavioral Output Designer</title>
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
	--warning: #ffd93d;
}

* { box-sizing: border-box; margin: 0; padding: 0; }

body {
	font-family: 'Space Grotesk', sans-serif;
	background: radial-gradient(circle at 10% 10%, rgba(255,157,46,0.12), transparent 40%),
	            radial-gradient(circle at 90% 10%, rgba(122,215,240,0.12), transparent 40%),
	            var(--bg);
	color: var(--text);
	min-height: 100vh;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* LAYOUT */
/* ═══════════════════════════════════════════════════════════════════════ */

.header {
	padding: 24px 4vw;
	border-bottom: 1px solid var(--border);
	background: rgba(18, 20, 28, 0.8);
	backdrop-filter: blur(10px);
	position: sticky;
	top: 0;
	z-index: 100;
}

.top-nav {
	margin-top: 12px;
	display: flex;
	gap: 10px;
	flex-wrap: wrap;
}

.top-nav .btn.active {
	background: var(--accent-2);
	color: #0b0c10;
}

.header h1 {
	font-size: 1.8rem;
	letter-spacing: 0.05em;
	margin-bottom: 8px;
}

.header p {
	color: var(--muted);
	font-size: 0.9rem;
}

.main-layout {
	display: grid;
	grid-template-columns: 320px 1fr;
	gap: 0;
	min-height: calc(100vh - 120px);
}

.sidebar {
	background: var(--panel);
	border-right: 1px solid var(--border);
	overflow-y: auto;
	max-height: calc(100vh - 120px);
}

.content {
	padding: 24px;
	overflow-y: auto;
	max-height: calc(100vh - 120px);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* NAVIGATION */
/* ═══════════════════════════════════════════════════════════════════════ */

.nav-section {
	padding: 16px;
	border-bottom: 1px solid var(--border);
}

.nav-section h3 {
	font-size: 0.75rem;
	text-transform: uppercase;
	letter-spacing: 0.1em;
	color: var(--muted);
	margin-bottom: 12px;
}

.nav-item {
	display: flex;
	align-items: center;
	gap: 12px;
	padding: 10px 12px;
	border-radius: 10px;
	cursor: pointer;
	transition: all 0.2s;
	margin-bottom: 4px;
}

.nav-item:hover {
	background: var(--surface);
}

.nav-item.active {
	background: var(--accent);
	color: #16110a;
	font-weight: 700;
}

.nav-icon {
	width: 20px;
	height: 20px;
	opacity: 0.7;
}

.nav-item.active .nav-icon {
	opacity: 1;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* CARDS & PANELS */
/* ═══════════════════════════════════════════════════════════════════════ */

.card {
	background: var(--panel);
	border: 1px solid var(--border);
	border-radius: 16px;
	padding: 20px;
	margin-bottom: 16px;
}

.card h2 {
	font-size: 1.4rem;
	margin-bottom: 8px;
	letter-spacing: 0.02em;
}

.card p {
	color: var(--muted);
	margin-bottom: 20px;
	line-height: 1.5;
}

.section {
	margin-bottom: 24px;
}

.section-title {
	font-size: 1.1rem;
	margin-bottom: 12px;
	color: var(--accent-2);
	display: flex;
	align-items: center;
	gap: 8px;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* FORM ELEMENTS */
/* ═══════════════════════════════════════════════════════════════════════ */

.form-grid {
	display: grid;
	gap: 16px;
	grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
}

.form-group {
	display: flex;
	flex-direction: column;
	gap: 6px;
}

label {
	font-size: 0.85rem;
	color: var(--muted);
	font-weight: 600;
	letter-spacing: 0.02em;
}

input, select, textarea {
	width: 100%;
	padding: 10px 12px;
	border-radius: 10px;
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	font-family: inherit;
	font-size: 0.95rem;
	transition: all 0.2s;
}

input:focus, select:focus, textarea:focus {
	outline: none;
	border-color: var(--accent);
	box-shadow: 0 0 0 3px rgba(255, 157, 46, 0.1);
}

input[type="range"] {
	padding: 0;
	height: 6px;
	background: var(--border);
	appearance: none;
}

input[type="range"]::-webkit-slider-thumb {
	appearance: none;
	width: 18px;
	height: 18px;
	border-radius: 50%;
	background: var(--accent);
	cursor: pointer;
	border: 2px solid var(--panel);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* BUTTONS */
/* ═══════════════════════════════════════════════════════════════════════ */

.btn {
	padding: 10px 16px;
	border-radius: 10px;
	border: 1px solid var(--border);
	background: var(--surface);
	color: var(--text);
	font-weight: 700;
	font-size: 0.9rem;
	cursor: pointer;
	transition: all 0.2s;
	display: inline-flex;
	align-items: center;
	gap: 8px;
	justify-content: center;
}

.btn:hover {
	background: var(--accent);
	color: #16110a;
	border-color: var(--accent);
	transform: translateY(-1px);
}

.btn-primary {
	background: var(--accent);
	color: #16110a;
	border-color: var(--accent);
}

.btn-primary:hover {
	background: #ffb04e;
	border-color: #ffb04e;
}

.btn-success {
	background: var(--success);
	color: #0b0c10;
	border-color: var(--success);
}

.btn-danger {
	background: var(--danger);
	color: white;
	border-color: var(--danger);
}

.btn-sm {
	padding: 6px 12px;
	font-size: 0.85rem;
}

.btn-block {
	width: 100%;
}

.btn-group {
	display: flex;
	gap: 8px;
	flex-wrap: wrap;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* OUTPUT LIST */
/* ═══════════════════════════════════════════════════════════════════════ */

.output-list {
	display: grid;
	gap: 12px;
}

.output-item {
	background: var(--surface);
	border: 1px solid var(--border);
	border-radius: 12px;
	padding: 16px;
	display: grid;
	grid-template-columns: auto 1fr auto auto;
	gap: 12px;
	align-items: center;
	transition: all 0.2s;
}

.output-item:hover {
	border-color: var(--accent);
}

.scene-edit-row {
	background: var(--surface);
	border: 1px solid var(--border);
	border-radius: 12px;
	padding: 14px;
	margin-bottom: 12px;
}

.scene-edit-row .row-actions {
	display: flex;
	justify-content: flex-end;
	gap: 8px;
	margin-top: 10px;
}

.output-indicator {
	width: 12px;
	height: 12px;
	border-radius: 50%;
	background: var(--border);
	transition: all 0.3s;
}

.output-indicator.active {
	background: var(--success);
	box-shadow: 0 0 12px var(--success);
	animation: pulse 2s infinite;
}

@keyframes pulse {
	0%, 100% { opacity: 1; }
	50% { opacity: 0.5; }
}

.output-info {
	display: flex;
	flex-direction: column;
	gap: 4px;
}

.output-name {
	font-weight: 700;
	font-size: 0.95rem;
}

.output-meta {
	font-size: 0.8rem;
	color: var(--muted);
}

.output-badge {
	padding: 4px 10px;
	border-radius: 6px;
	font-size: 0.75rem;
	font-weight: 600;
	background: var(--surface);
	border: 1px solid var(--border);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* SCENE BUILDER */
/* ═══════════════════════════════════════════════════════════════════════ */

.scene-outputs {
	display: flex;
	flex-direction: column;
	gap: 12px;
}

.scene-output {
	background: var(--surface);
	border: 1px solid var(--border);
	border-radius: 12px;
	padding: 16px;
}

.scene-output-header {
	display: flex;
	justify-content: space-between;
	align-items: center;
	margin-bottom: 12px;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* LIVE PREVIEW */
/* ═══════════════════════════════════════════════════════════════════════ */

.preview-panel {
	background: var(--panel);
	border: 2px solid var(--border);
	border-radius: 16px;
	padding: 20px;
	min-height: 200px;
}

.preview-outputs {
	display: grid;
	grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
	gap: 12px;
}

.preview-status {
	margin-top: 12px;
	font-size: 0.85rem;
	color: var(--muted);
}

.preview-scenes {
	margin-top: 6px;
	font-size: 0.85rem;
	color: var(--muted);
}

.suspension-preview-grid {
	display: grid;
	grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
	gap: 12px;
	margin-bottom: 12px;
}

.suspension-preview-label {
	font-size: 0.75rem;
	text-transform: uppercase;
	letter-spacing: 0.08em;
	color: var(--muted);
	margin-bottom: 4px;
}

.suspension-preview-values {
	font-weight: 700;
}

.preview-output {
	aspect-ratio: auto;
	border-radius: 12px;
	border: 2px solid var(--border);
	display: flex;
	flex-direction: column;
	align-items: flex-start;
	justify-content: flex-start;
	gap: 8px;
	transition: all 0.1s;
	background: var(--surface);
	padding: 12px;
	min-height: 160px;
}

.preview-output-header {
	width: 100%;
	display: flex;
	align-items: center;
	justify-content: space-between;
	gap: 8px;
}

.preview-output-indicator {
	width: 12px;
	height: 12px;
	border-radius: 999px;
	background: #22c55e;
	box-shadow: 0 0 10px rgba(34, 197, 94, 0.5);
}

.preview-output-name {
	font-size: 0.75rem;
	color: var(--muted);
}

.preview-output-value {
	font-weight: 700;
	font-size: 1rem;
}

.preview-output-meta {
	width: 100%;
}


.preview-output-meta {
	margin-top: 4px;
	font-size: 11px;
	color: var(--muted);
}

.preview-output-actions {
	margin-top: 8px;
	display: flex;
	gap: 6px;
	width: 100%;
}

.preview-output-actions .btn {
	flex: 1;
}
/* ═══════════════════════════════════════════════════════════════════════ */
/* UTILITIES */
/* ═══════════════════════════════════════════════════════════════════════ */

.hidden { display: none !important; }

.badge {
	display: inline-block;
	padding: 4px 10px;
	border-radius: 6px;
	font-size: 0.75rem;
	font-weight: 600;
}

.badge-success {
	background: rgba(61, 213, 152, 0.2);
	color: var(--success);
	border: 1px solid var(--success);
}

.badge-info {
	background: rgba(122, 215, 240, 0.2);
	color: var(--accent-2);
	border: 1px solid var(--accent-2);
}

.alert {
	padding: 12px 16px;
	border-radius: 10px;
	margin-bottom: 16px;
	border: 1px solid;
}

.alert-info {
	background: rgba(122, 215, 240, 0.1);
	border-color: var(--accent-2);
	color: var(--accent-2);
}

.alert-warning {
	background: rgba(255, 217, 61, 0.1);
	border-color: var(--warning);
	color: var(--warning);
}

.divider {
	height: 1px;
	background: var(--border);
	margin: 24px 0;
}

</style>
</head>
<body>

<div class="header">
	<h1>🎛️ Behavioral Output Designer</h1>
	<p>Intent-based output control with outputs and scenes</p>
	<div class="top-nav">
		<button class="btn" onclick="location.href='/'">🏠 Configurator</button>
		<button class="btn" onclick="location.href='/can-monitor'">📡 CAN Monitor</button>
		<button class="btn active" onclick="location.href='/behavioral'">🎛️ Behavioral Outputs</button>
	</div>
</div>

<div class="main-layout">
	<!-- ═══════════════════════════════════════════════════════════════════ -->
	<!-- SIDEBAR NAVIGATION -->
	<!-- ═══════════════════════════════════════════════════════════════════ -->
	<div class="sidebar">
		<div class="nav-section">
			<h3>Configuration</h3>
			<div class="nav-item active" data-view="outputs">
				<span class="nav-icon">📍</span>
				<span>Outputs</span>
			</div>
			<div class="nav-item" data-view="scenes">
				<span class="nav-icon">🎬</span>
				<span>Scenes</span>
			</div>
		</div>
		
		<div class="nav-section">
			<h3>Testing</h3>
			<div class="nav-item" data-view="preview">
				<span class="nav-icon">👁️</span>
				<span>Live Preview</span>
			</div>
			<div class="nav-item" data-view="simulator">
				<span class="nav-icon">🧪</span>
				<span>Simulator</span>
			</div>
		</div>
		
		<div class="nav-section">
			<h3>Advanced</h3>
			<div class="nav-item" data-view="settings">
				<span class="nav-icon">⚙️</span>
				<span>Settings</span>
			</div>
		</div>
	</div>

	<!-- ═══════════════════════════════════════════════════════════════════ -->
	<!-- MAIN CONTENT AREA -->
	<!-- ═══════════════════════════════════════════════════════════════════ -->
	<div class="content">
		
		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: OUTPUTS -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-outputs" class="view">
			<div class="card">
				<h2>Output Definitions</h2>
				<p>Define physical outputs and their hardware mapping</p>
				
				<div class="alert alert-info">
					💡 <strong>Tip:</strong> Define outputs here, then assign behaviors per button in the main configurator.
				</div>
				
				<div class="btn-group">
					<button class="btn btn-primary" onclick="showOutputEditor()">
						➕ Add Output
					</button>
					<button class="btn" onclick="loadOutputs()">
						🔄 Refresh
					</button>
				</div>
				
				<div class="divider"></div>
				
				<div id="output-list" class="output-list">
					<!-- Dynamically populated -->
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: SCENES -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-scenes" class="view hidden">
			<div class="card">
				<h2>Scene Manager</h2>
				<p>Coordinate multiple outputs with synchronized behaviors</p>
				
				<div class="alert alert-info">
					🎬 <strong>Scenes</strong> let you activate multiple outputs with predefined behaviors simultaneously.
					Perfect for complex lighting sequences, alerts, or mode changes.
				</div>
				
				<button class="btn btn-primary" onclick="createNewScene()">
					➕ Create Scene
				</button>
				
				<div class="divider"></div>
				
				<div id="scene-list">
					<!-- Scenes go here -->
				</div>
			</div>

			<div id="scene-editor" class="card hidden">
				<h2 id="scene-editor-title">Scene Editor</h2>
				<p>Build multi-action scenes including outputs, CAN frames, Infinitybox actions, and suspension settings.</p>
				<input type="hidden" id="scene-id" />

				<div class="form-grid">
					<div class="form-group">
						<label>Scene Name</label>
						<input type="text" id="scene-name" placeholder="Trail Mode" />
					</div>
					<div class="form-group">
						<label>Description</label>
						<input type="text" id="scene-description" placeholder="Activates trail lighting and suspension preset" />
					</div>
					<div class="form-group">
						<label>Duration (ms, 0 = until off)</label>
						<input type="number" id="scene-duration" value="0" min="0" />
					</div>
					<div class="form-group">
						<label>Priority</label>
						<input type="number" id="scene-priority" value="100" min="0" max="255" />
					</div>
					<div class="form-group">
						<label><input type="checkbox" id="scene-exclusive" /> Exclusive (turn off other scenes)</label>
					</div>
				</div>

				<div class="divider"></div>
				<div class="section">
					<div class="section-title">Output Actions</div>
					<div id="scene-output-list"></div>
					<button class="btn" onclick="addSceneOutput()">➕ Add Output Action</button>
				</div>

				<div class="divider"></div>
				<div class="section">
					<div class="section-title">Custom CAN Frames</div>
					<div id="scene-can-list"></div>
					<button class="btn" onclick="addSceneCanFrame()">➕ Add CAN Frame</button>
				</div>

				<div class="divider"></div>
				<div class="section">
					<div class="section-title">Predefined Infinitybox Actions</div>
					<div id="scene-ibox-list"></div>
					<button class="btn" onclick="addSceneInfinityboxAction()">➕ Add Infinitybox Action</button>
				</div>

				<div class="divider"></div>
				<div class="section">
					<div class="section-title">Suspension Settings</div>
					<div class="form-grid">
						<div class="form-group">
							<label><input type="checkbox" id="scene-suspension-enabled" /> Apply suspension preset</label>
						</div>
						<div class="form-group">
							<label>Front Left (%)</label>
							<input type="number" id="scene-suspension-front-left" min="0" max="100" value="0" />
						</div>
						<div class="form-group">
							<label>Front Right (%)</label>
							<input type="number" id="scene-suspension-front-right" min="0" max="100" value="0" />
						</div>
						<div class="form-group">
							<label>Rear Left (%)</label>
							<input type="number" id="scene-suspension-rear-left" min="0" max="100" value="0" />
						</div>
						<div class="form-group">
							<label>Rear Right (%)</label>
							<input type="number" id="scene-suspension-rear-right" min="0" max="100" value="0" />
						</div>
						<div class="form-group">
							<label><input type="checkbox" id="scene-suspension-calibrate" /> Calibration Active</label>
						</div>
					</div>
				</div>

				<div class="divider"></div>
				<div class="btn-group">
					<button class="btn btn-success" onclick="saveSceneEditor()">💾 Save Scene</button>
					<button class="btn" onclick="closeSceneEditor()">Cancel</button>
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: LIVE PREVIEW -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-preview" class="view hidden">
			<div class="card">
				<h2>Live Output Monitor</h2>
				<p>Real-time visualization of all active outputs</p>
				
				<div class="preview-panel">
					<div id="preview-outputs" class="preview-outputs">
						<!-- Preview visualizations -->
					</div>
				</div>
				
				<div class="preview-status" id="preview-status">Status: waiting for data…</div>
				<div class="preview-scenes" id="preview-scenes" title="Active scenes">○</div>
				
				<div class="divider"></div>
				
				<div class="btn-group">
					<button class="btn" onclick="togglePreviewUpdates()">
						<span id="preview-toggle-text">⏸ Pause</span>
					</button>
					<button class="btn" onclick="clearAllOutputs()">
						🔴 Stop All
					</button>
				</div>

				<div class="divider"></div>

				<div class="section">
					<div class="section-title">Suspension Status</div>
					<div class="suspension-preview-grid">
						<div>
							<div class="suspension-preview-label">Targets</div>
							<div class="suspension-preview-values" id="suspension-targets">FL 0 • FR 0 • RL 0 • RR 0</div>
						</div>
						<div>
							<div class="suspension-preview-label">Actual</div>
							<div class="suspension-preview-values" id="suspension-actual">FL 0 • FR 0 • RL 0 • RR 0</div>
						</div>
						<div>
							<div class="suspension-preview-label">Calibration</div>
							<div class="suspension-preview-values" id="suspension-calibration">Inactive</div>
						</div>
						<div>
							<div class="suspension-preview-label">Last Feedback</div>
							<div class="suspension-preview-values" id="suspension-feedback">—</div>
						</div>
					</div>

					<div class="divider"></div>

					<div class="form-grid">
						<div class="form-group">
							<label>Front Left (%)</label>
							<input type="number" id="preview-suspension-front-left" min="0" max="100" value="0">
						</div>
						<div class="form-group">
							<label>Front Right (%)</label>
							<input type="number" id="preview-suspension-front-right" min="0" max="100" value="0">
						</div>
						<div class="form-group">
							<label>Rear Left (%)</label>
							<input type="number" id="preview-suspension-rear-left" min="0" max="100" value="0">
						</div>
						<div class="form-group">
							<label>Rear Right (%)</label>
							<input type="number" id="preview-suspension-rear-right" min="0" max="100" value="0">
						</div>
						<div class="form-group">
							<label><input type="checkbox" id="preview-suspension-calibrate"> Calibration Active</label>
						</div>
					</div>
					<div class="btn-group">
						<button class="btn btn-success" onclick="applyPreviewSuspension()">Apply Suspension</button>
						<button class="btn" onclick="refreshSuspensionPreview()">Refresh</button>
					</div>
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: SIMULATOR -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-simulator" class="view hidden">
			<div class="card">
				<h2>Scene Simulator</h2>
				<p>Test any scene with live output activation</p>
				
				<div class="alert alert-info">
					🎮 <strong>How to use:</strong><br>
					1. Click any scene button below to activate or deactivate it<br>
					2. Watch the outputs activate in real-time<br>
					3. Go to "Live Preview" view to see visual feedback<br>
					4. All saved scenes are listed below
				</div>
				
				<div class="section">
					<label>Scenes</label>
					<div id="simulator-scene-list"></div>
				</div>
				
				<div class="divider"></div>
				
				<div class="alert alert-warning">
					⚡ <strong>Tip:</strong> Use the "Scenes" tab to edit or add new scenes.
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: SETTINGS -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-settings" class="view hidden">
			<div class="card">
				<h2>System Settings</h2>
				
				<div class="form-grid">
					<div class="form-group">
						<label>Engine Update Rate (ms)</label>
						<input type="number" id="setting-update-rate" value="20" min="10" max="100">
					</div>
					<div class="form-group">
						<label>Frame Transmit Rate (ms)</label>
						<input type="number" id="setting-transmit-rate" value="50" min="20" max="500">
					</div>
				</div>
				
				<div class="divider"></div>
				
				<div class="btn-group">
					<button class="btn btn-success" onclick="saveSettings()">
						💾 Save Settings
					</button>
					<button class="btn btn-danger" onclick="factoryReset()">
						⚠️ Factory Reset
					</button>
				</div>
			</div>
		</div>

	</div>
</div>

<script>
// ═══════════════════════════════════════════════════════════════════════════
// STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

let appState = {
	outputs: [],
	scenes: [],
	infinityboxFunctions: [],
	currentView: 'outputs',
	previewActive: true
};

function firstDefined() {
	for (let i = 0; i < arguments.length; i++) {
		const v = arguments[i];
		if (v !== undefined && v !== null) return v;
	}
	return undefined;
}

// ═══════════════════════════════════════════════════════════════════════════
// NAVIGATION
// ═══════════════════════════════════════════════════════════════════════════

document.querySelectorAll('.nav-item').forEach(item => {
	item.addEventListener('click', () => {
		const view = item.dataset.view;
		switchView(view);
	});
});

function switchView(viewName) {
	// Update navigation
	document.querySelectorAll('.nav-item').forEach(item => {
		item.classList.toggle('active', item.dataset.view === viewName);
	});
	
	// Update content views
	document.querySelectorAll('.view').forEach(view => {
		view.classList.add('hidden');
	});
	document.getElementById(`view-${viewName}`).classList.remove('hidden');
	
	appState.currentView = viewName;
	
	// Load view-specific data
	if (viewName === 'outputs') loadOutputs();
	if (viewName === 'scenes') loadScenes();
	if (viewName === 'simulator') startSimulatorPolling();
	if (viewName !== 'simulator') stopSimulatorPolling();
	if (viewName === 'preview') {
		startPreview();
	} else {
		stopPreview();
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// OUTPUT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

function showOutputEditor(outputId = null) {
	const name = prompt('Output Name (e.g., "Left Turn Signal"):');
	if (!name) return;
	
	const cellAddr = parseInt(prompt('Cell Address (0-254, 0=Mastercell):', '1'));
	const outputRange = (cellAddr === 0) ? '1-8' : '1-10';
	const outputNum = parseInt(prompt(`Output Number (${outputRange}):`, '1'));
	
	const output = {
		id: outputId || `out_${Date.now()}`,
		name: name,
		cellAddress: cellAddr,
		outputNumber: outputNum,
		description: ''
	};
	
	fetch('/api/outputs', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(output)
	}).then(() => loadOutputs());
}

function editOutput(id) {
	const output = appState.outputs.find(o => o.id === id);
	if (!output) return;
	
	const name = prompt('Output Name:', output.name);
	if (!name) return;
	
	const cellAddr = parseInt(prompt('Cell Address (0-254, 0=Mastercell):', output.cellAddress));
	const outputRange = (cellAddr === 0) ? '1-8' : '1-10';
	const outputNum = parseInt(prompt(`Output Number (${outputRange}):`, output.outputNumber));
	
	const updated = {
		...output,
		name: name,
		cellAddress: cellAddr,
		outputNumber: outputNum
	};
	
	fetch('/api/outputs', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(updated)
	}).then(() => loadOutputs());
}

function deleteOutput(id) {
	if (!confirm('Delete this output? This will also remove its behavior.')) return;
	
	fetch('/api/outputs/' + id, {method: 'DELETE'})
		.then(() => loadOutputs());
}

function loadOutputs() {
	fetch('/api/outputs')
		.then(r => r.json())
		.then(outputs => {
			appState.outputs = outputs;
			renderOutputList(outputs);
		});
}

function renderOutputList(outputs) {
	const list = document.getElementById('output-list');
	if (!outputs || outputs.length === 0) {
		list.innerHTML = '<p style="color: var(--muted); text-align: center; padding: 40px;">No outputs defined yet. Click "Add Output" to get started.</p>';
		return;
	}
	
	list.innerHTML = outputs.map(out => `
		<div class="output-item">
			<div class="output-indicator ${out.isActive ? 'active' : ''}"></div>
			<div class="output-info">
				<div class="output-name">${out.name}</div>
				<div class="output-meta">Cell ${out.cellAddress} • Output ${out.outputNumber}</div>
			</div>
			<div class="output-badge">${out.behavior?.type || 'None'}</div>
			<div class="btn-group">
				<button class="btn btn-sm" onclick="editOutput('${out.id}')">Edit</button>
				<button class="btn btn-sm btn-danger" onclick="deleteOutput('${out.id}')">Delete</button>
			</div>
		</div>
	`).join('');
}

// ═══════════════════════════════════════════════════════════════════════════
// SCENES
// ═══════════════════════════════════════════════════════════════════════════

let sceneDraft = null;

function loadInfinityboxFunctions() {
	return fetch('/api/behavioral/options')
		.then(r => r.json())
		.then(data => {
			appState.infinityboxFunctions = data.infinitybox_functions || [];
		});
}

function normalizeSceneDraft(scene) {
	const isNew = !scene.id;
	const normalized = {
		id: scene.id || '',
		name: scene.name || 'New Scene',
		description: scene.description || '',
		duration_ms: scene.duration_ms || 0,
		priority: scene.priority || 100,
		exclusive: !!scene.exclusive,
		outputs: Array.isArray(scene.outputs) ? scene.outputs : [],
		can_frames: Array.isArray(scene.can_frames) ? scene.can_frames : [],
		infinitybox_actions: Array.isArray(scene.infinitybox_actions) ? scene.infinitybox_actions : [],
		suspension: scene.suspension || { enabled: false, front_left: 0, front_right: 0, rear_left: 0, rear_right: 0, calibration_active: false },
		isNew
	};

	normalized.outputs = normalized.outputs.map(out => ({
		output_id: out.output_id || '',
		action: out.action || out.output_action || 'behavior',
		behavior_type: out.behavior_type || 'STEADY',
		target_value: firstDefined(out.target_value, 255),
		period_ms: firstDefined(out.period_ms, 1000),
		duty_cycle: firstDefined(out.duty_cycle, 50),
		fade_time_ms: firstDefined(out.fade_time_ms, 500),
		on_time_ms: firstDefined(out.on_time_ms, 500),
		off_time_ms: firstDefined(out.off_time_ms, 500),
		duration_ms: firstDefined(out.duration_ms, 0),
		priority: firstDefined(out.priority, 100),
		soft_start: !!out.soft_start,
		auto_off: firstDefined(out.auto_off, true)
	}));

	normalized.can_frames = normalized.can_frames.map(frame => ({
		enabled: frame.enabled !== false,
		pgn: firstDefined(frame.pgn, 0x00FF00),
		priority: firstDefined(frame.priority, 6),
		source: firstDefined(frame.source, frame.source_address, 0xF9),
		destination: firstDefined(frame.destination, frame.destination_address, 0xFF),
		data: Array.isArray(frame.data) ? frame.data : [],
		length: firstDefined(frame.length, Array.isArray(frame.data) ? frame.data.length : 0)
	}));

	normalized.infinitybox_actions = normalized.infinitybox_actions.map(action => ({
		function: action.function || action.function_name || '',
		behavior: action.behavior || 'on',
		level: firstDefined(action.level, 100),
		on_ms: firstDefined(action.on_ms, 500),
		off_ms: firstDefined(action.off_ms, 500),
		duration_ms: firstDefined(action.duration_ms, 0),
		release_on_deactivate: action.release_on_deactivate !== false
	}));

	normalized.suspension = {
		enabled: normalized.suspension.enabled || false,
		front_left: firstDefined(normalized.suspension.front_left, 0),
		front_right: firstDefined(normalized.suspension.front_right, 0),
		rear_left: firstDefined(normalized.suspension.rear_left, 0),
		rear_right: firstDefined(normalized.suspension.rear_right, 0),
		calibration_active: normalized.suspension.calibration_active || false
	};

	return normalized;
}

function openSceneEditor(scene) {
	sceneDraft = normalizeSceneDraft(scene || {});

	document.getElementById('scene-editor-title').textContent = sceneDraft.isNew ? 'New Scene' : 'Edit Scene';
	document.getElementById('scene-id').value = sceneDraft.id || '';
	document.getElementById('scene-name').value = sceneDraft.name;
	document.getElementById('scene-description').value = sceneDraft.description;
	document.getElementById('scene-duration').value = sceneDraft.duration_ms;
	document.getElementById('scene-priority').value = sceneDraft.priority;
	document.getElementById('scene-exclusive').checked = sceneDraft.exclusive;

	document.getElementById('scene-suspension-enabled').checked = sceneDraft.suspension.enabled;
	document.getElementById('scene-suspension-front-left').value = sceneDraft.suspension.front_left;
	document.getElementById('scene-suspension-front-right').value = sceneDraft.suspension.front_right;
	document.getElementById('scene-suspension-rear-left').value = sceneDraft.suspension.rear_left;
	document.getElementById('scene-suspension-rear-right').value = sceneDraft.suspension.rear_right;
	document.getElementById('scene-suspension-calibrate').checked = sceneDraft.suspension.calibration_active;

	renderSceneOutputs();
	renderSceneCanFrames();
	renderSceneInfinityboxActions();

	document.getElementById('scene-editor').classList.remove('hidden');
}

function closeSceneEditor() {
	sceneDraft = null;
	document.getElementById('scene-id').value = '';
	document.getElementById('scene-editor').classList.add('hidden');
}

function renderSceneList(scenes) {
	const list = document.getElementById('scene-list');
	if (!scenes || scenes.length === 0) {
		list.innerHTML = '<div class="alert alert-info">No scenes created. Build your first scene!</div>';
		return;
	}
	
	list.innerHTML = scenes.map(s => {
		const meta = [];
		if (s.outputCount) meta.push(`${s.outputCount} outputs`);
		if (s.canCount) meta.push(`${s.canCount} CAN frames`);
		if (s.infinityboxCount) meta.push(`${s.infinityboxCount} Infinitybox`);
		if (s.suspensionEnabled) meta.push('Suspension');
		const metaText = meta.length ? ` • ${meta.join(' • ')}` : '';
		return `
		<div class="output-item">
			<div>
				<strong>${s.name}</strong>
				<div style="font-size: 0.85em; color: var(--muted)">
					${s.description || 'No description'}${metaText}
				</div>
			</div>
			<div class="btn-group">
				<button class="btn btn-sm btn-success" onclick="activateScene('${s.id}')">Activate</button>
				<button class="btn btn-sm" onclick="editScene('${s.id}')">Edit</button>
				<button class="btn btn-sm btn-danger" onclick="deleteScene('${s.id}')">Delete</button>
			</div>
		</div>
		`;
	}).join('');
}

function createNewScene() {
	openSceneEditor({
		id: '',
		name: '',
		description: '',
		duration_ms: 0,
		priority: 100,
		exclusive: false,
		outputs: [],
		can_frames: [],
		infinitybox_actions: [],
		suspension: { enabled: false, front_left: 0, front_right: 0, rear_left: 0, rear_right: 0, calibration_active: false }
	});
}

function editScene(id) {
	const cached = appState.scenes.find(s => s.id === id);
	if (cached) {
		openSceneEditor(cached);
	}

	const applyScene = (scene) => {
		if (!scene || !scene.id) return;
		if (cached) {
			if (!Array.isArray(scene.outputs) || scene.outputs.length === 0) {
				scene.outputs = cached.outputs || [];
			}
			if (!Array.isArray(scene.can_frames) || scene.can_frames.length === 0) {
				scene.can_frames = cached.can_frames || [];
			}
			if (!Array.isArray(scene.infinitybox_actions) || scene.infinitybox_actions.length === 0) {
				scene.infinitybox_actions = cached.infinitybox_actions || [];
			}
			if (!scene.suspension && cached.suspension) {
				scene.suspension = cached.suspension;
			}
		}
		openSceneEditor(scene);
	};

	fetch('/api/scenes/' + id)
		.then(r => r.json())
		.then(scene => {
			applyScene(scene);
		})
		.catch(() => {
			if (!cached) {
				openSceneEditor({ id, name: 'Scene', description: '' });
			}
		});
}

function saveSceneEditor() {
	if (!sceneDraft) return;

	const existingId = document.getElementById('scene-id').value.trim();
	sceneDraft.id = existingId || `scene_${Date.now()}`;
	sceneDraft.name = document.getElementById('scene-name').value.trim() || 'Untitled Scene';
	sceneDraft.description = document.getElementById('scene-description').value.trim();
	sceneDraft.duration_ms = parseInt(document.getElementById('scene-duration').value || 0);
	sceneDraft.priority = parseInt(document.getElementById('scene-priority').value || 100);
	sceneDraft.exclusive = document.getElementById('scene-exclusive').checked;

	sceneDraft.suspension = {
		enabled: document.getElementById('scene-suspension-enabled').checked,
		front_left: parseInt(document.getElementById('scene-suspension-front-left').value || 0),
		front_right: parseInt(document.getElementById('scene-suspension-front-right').value || 0),
		rear_left: parseInt(document.getElementById('scene-suspension-rear-left').value || 0),
		rear_right: parseInt(document.getElementById('scene-suspension-rear-right').value || 0),
		calibration_active: document.getElementById('scene-suspension-calibrate').checked
	};

	sceneDraft.can_frames = sceneDraft.can_frames.map(frame => ({
		...frame,
		length: Array.isArray(frame.data) ? frame.data.length : 0
	}));

	const payload = { ...sceneDraft };
	delete payload.isNew;

	fetch('/api/scenes', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(payload)
	}).then(() => {
		closeSceneEditor();
		loadScenes();
	});
}

function deleteScene(id) {
	if (!confirm('Delete this scene?')) return;
	
	fetch('/api/scenes/' + id, {method: 'DELETE'})
		.then(() => loadScenes());
}

function renderSceneOutputs() {
	const container = document.getElementById('scene-output-list');
	if (!sceneDraft) return;
	if (sceneDraft.outputs.length === 0) {
		container.innerHTML = '<div class="alert alert-info">No output behaviors yet.</div>';
		return;
	}

	container.innerHTML = sceneDraft.outputs.map((out, idx) => `
		<div class="scene-edit-row">
			<div class="form-grid">
				<div class="form-group">
					<label>Output</label>
					<select onchange="updateSceneOutput(${idx}, 'output_id', this.value)">
						${renderOutputOptions(out.output_id)}
					</select>
				</div>
				<div class="form-group">
					<label>Action</label>
					<select onchange="updateSceneOutput(${idx}, 'action', this.value)">
						${renderSceneActionOptions(out.action)}
					</select>
				</div>
				<div class="form-group">
					<label>Behavior</label>
					<select onchange="updateSceneOutput(${idx}, 'behavior_type', this.value)">
						${renderBehaviorOptions(out.behavior_type)}
					</select>
				</div>
				<div class="form-group">
					<label>Level (0-255)</label>
					<input type="number" min="0" max="255" value="${out.target_value}" oninput="updateSceneOutput(${idx}, 'target_value', this.value)" />
				</div>
				<div class="form-group">
					<label>Period (ms)</label>
					<input type="number" min="50" value="${out.period_ms}" oninput="updateSceneOutput(${idx}, 'period_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Duty Cycle (%)</label>
					<input type="number" min="0" max="100" value="${out.duty_cycle}" oninput="updateSceneOutput(${idx}, 'duty_cycle', this.value)" />
				</div>
				<div class="form-group">
					<label>Fade Time (ms)</label>
					<input type="number" min="0" value="${out.fade_time_ms}" oninput="updateSceneOutput(${idx}, 'fade_time_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>On Time (ms)</label>
					<input type="number" min="0" value="${out.on_time_ms}" oninput="updateSceneOutput(${idx}, 'on_time_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Off Time (ms)</label>
					<input type="number" min="0" value="${out.off_time_ms}" oninput="updateSceneOutput(${idx}, 'off_time_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Duration (ms)</label>
					<input type="number" min="0" value="${out.duration_ms}" oninput="updateSceneOutput(${idx}, 'duration_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Priority</label>
					<input type="number" min="0" max="255" value="${out.priority}" oninput="updateSceneOutput(${idx}, 'priority', this.value)" />
				</div>
				<div class="form-group">
					<label><input type="checkbox" ${out.soft_start ? 'checked' : ''} onchange="updateSceneOutput(${idx}, 'soft_start', this.checked)" /> Soft Start</label>
				</div>
				<div class="form-group">
					<label><input type="checkbox" ${out.auto_off ? 'checked' : ''} onchange="updateSceneOutput(${idx}, 'auto_off', this.checked)" /> Auto Off</label>
				</div>
			</div>
			<div class="row-actions">
				<button class="btn btn-danger btn-sm" onclick="removeSceneOutput(${idx})">Remove</button>
			</div>
		</div>
	`).join('');
}

function addSceneOutput() {
	if (!sceneDraft) return;
	sceneDraft.outputs.push({
		output_id: '',
		action: 'behavior',
		behavior_type: 'STEADY',
		target_value: 255,
		period_ms: 1000,
		duty_cycle: 50,
		fade_time_ms: 500,
		on_time_ms: 500,
		off_time_ms: 500,
		duration_ms: 0,
		priority: 100,
		soft_start: false,
		auto_off: true
	});
	renderSceneOutputs();
}

function updateSceneOutput(index, field, value) {
	if (!sceneDraft || !sceneDraft.outputs[index]) return;
	if (field === 'soft_start' || field === 'auto_off') {
		sceneDraft.outputs[index][field] = value;
		return;
	}
	if (field === 'output_id' || field === 'behavior_type' || field === 'action') {
		sceneDraft.outputs[index][field] = value;
		return;
	}
	const parsed = parseInt(value);
	sceneDraft.outputs[index][field] = Number.isNaN(parsed) ? 0 : parsed;
}

function removeSceneOutput(index) {
	if (!sceneDraft) return;
	sceneDraft.outputs.splice(index, 1);
	renderSceneOutputs();
}

function renderSceneCanFrames() {
	const container = document.getElementById('scene-can-list');
	if (!sceneDraft) return;
	if (sceneDraft.can_frames.length === 0) {
		container.innerHTML = '<div class="alert alert-info">No CAN frames configured.</div>';
		return;
	}

	container.innerHTML = sceneDraft.can_frames.map((frame, idx) => `
		<div class="scene-edit-row">
			<div class="form-grid">
				<div class="form-group">
					<label>PGN (hex)</label>
					<input type="text" value="${toHex(frame.pgn, 4)}" oninput="updateSceneCanFrame(${idx}, 'pgn', this.value)" />
				</div>
				<div class="form-group">
					<label>Priority</label>
					<input type="number" min="0" max="7" value="${frame.priority}" oninput="updateSceneCanFrame(${idx}, 'priority', this.value)" />
				</div>
				<div class="form-group">
					<label>Source (hex)</label>
					<input type="text" value="${toHex(frame.source, 2)}" oninput="updateSceneCanFrame(${idx}, 'source', this.value)" />
				</div>
				<div class="form-group">
					<label>Destination (hex)</label>
					<input type="text" value="${toHex(frame.destination, 2)}" oninput="updateSceneCanFrame(${idx}, 'destination', this.value)" />
				</div>
				<div class="form-group" style="grid-column: 1 / -1;">
					<label>Data (hex bytes)</label>
					<input type="text" value="${bytesToHex(frame.data)}" oninput="updateSceneCanFrame(${idx}, 'data', this.value)" />
				</div>
				<div class="form-group">
					<label><input type="checkbox" ${frame.enabled ? 'checked' : ''} onchange="updateSceneCanFrame(${idx}, 'enabled', this.checked)" /> Enabled</label>
				</div>
			</div>
			<div class="row-actions">
				<button class="btn btn-danger btn-sm" onclick="removeSceneCanFrame(${idx})">Remove</button>
			</div>
		</div>
	`).join('');
}

function addSceneCanFrame() {
	if (!sceneDraft) return;
	sceneDraft.can_frames.push({
		enabled: true,
		pgn: 0x00FF00,
		priority: 6,
		source: 0xF9,
		destination: 0xFF,
		data: [],
		length: 0
	});
	renderSceneCanFrames();
}

function updateSceneCanFrame(index, field, value) {
	if (!sceneDraft || !sceneDraft.can_frames[index]) return;
	if (field === 'enabled') {
		sceneDraft.can_frames[index].enabled = value;
		return;
	}
	if (field === 'data') {
		const bytes = parseHexBytes(value);
		sceneDraft.can_frames[index].data = bytes;
		sceneDraft.can_frames[index].length = bytes.length;
		return;
	}
	const parsed = field === 'pgn' || field === 'source' || field === 'destination'
		? parseInt(value, 16)
		: parseInt(value);
	sceneDraft.can_frames[index][field] = Number.isNaN(parsed) ? 0 : parsed;
}

function removeSceneCanFrame(index) {
	if (!sceneDraft) return;
	sceneDraft.can_frames.splice(index, 1);
	renderSceneCanFrames();
}

function renderSceneInfinityboxActions() {
	const container = document.getElementById('scene-ibox-list');
	if (!sceneDraft) return;
	if (sceneDraft.infinitybox_actions.length === 0) {
		container.innerHTML = '<div class="alert alert-info">No Infinitybox actions configured.</div>';
		return;
	}

	container.innerHTML = sceneDraft.infinitybox_actions.map((action, idx) => `
		<div class="scene-edit-row">
			<div class="form-grid">
				<div class="form-group">
					<label>Function</label>
					<select onchange="updateSceneInfinityboxAction(${idx}, 'function', this.value)">
						${renderInfinityboxFunctionOptions(action.function)}
					</select>
				</div>
				<div class="form-group">
					<label>Behavior</label>
					<select onchange="updateSceneInfinityboxAction(${idx}, 'behavior', this.value)">
						${renderInfinityboxBehaviorOptions(action.behavior)}
					</select>
				</div>
				<div class="form-group">
					<label>Level (0-100)</label>
					<input type="number" min="0" max="100" value="${action.level}" oninput="updateSceneInfinityboxAction(${idx}, 'level', this.value)" />
				</div>
				<div class="form-group">
					<label>On Time (ms)</label>
					<input type="number" min="0" value="${action.on_ms}" oninput="updateSceneInfinityboxAction(${idx}, 'on_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Off Time (ms)</label>
					<input type="number" min="0" value="${action.off_ms}" oninput="updateSceneInfinityboxAction(${idx}, 'off_ms', this.value)" />
				</div>
				<div class="form-group">
					<label>Duration (ms)</label>
					<input type="number" min="0" value="${action.duration_ms}" oninput="updateSceneInfinityboxAction(${idx}, 'duration_ms', this.value)" />
				</div>
				<div class="form-group">
					<label><input type="checkbox" ${action.release_on_deactivate ? 'checked' : ''} onchange="updateSceneInfinityboxAction(${idx}, 'release_on_deactivate', this.checked)" /> Release on deactivate</label>
				</div>
			</div>
			<div class="row-actions">
				<button class="btn btn-danger btn-sm" onclick="removeSceneInfinityboxAction(${idx})">Remove</button>
			</div>
		</div>
	`).join('');
}

function addSceneInfinityboxAction() {
	if (!sceneDraft) return;
	sceneDraft.infinitybox_actions.push({
		function: '',
		behavior: 'on',
		level: 100,
		on_ms: 500,
		off_ms: 500,
		duration_ms: 0,
		release_on_deactivate: true
	});
	renderSceneInfinityboxActions();
}

function updateSceneInfinityboxAction(index, field, value) {
	if (!sceneDraft || !sceneDraft.infinitybox_actions[index]) return;
	if (field === 'function' || field === 'behavior') {
		sceneDraft.infinitybox_actions[index][field] = value;
		return;
	}
	if (field === 'release_on_deactivate') {
		sceneDraft.infinitybox_actions[index][field] = value;
		return;
	}
	const parsed = parseInt(value);
	sceneDraft.infinitybox_actions[index][field] = Number.isNaN(parsed) ? 0 : parsed;
}

function removeSceneInfinityboxAction(index) {
	if (!sceneDraft) return;
	sceneDraft.infinitybox_actions.splice(index, 1);
	renderSceneInfinityboxActions();
}

function renderOutputOptions(selectedId) {
	const options = ['<option value="">Select output...</option>'];
	appState.outputs.forEach(out => {
		const selected = out.id === selectedId ? 'selected' : '';
		options.push(`<option value="${out.id}" ${selected}>${out.name}</option>`);
	});
	return options.join('');
}

function renderBehaviorOptions(selected) {
	const behaviors = [
		{ value: 'STEADY', label: 'Steady' },
		{ value: 'FLASH', label: 'Flash' },
		{ value: 'PULSE', label: 'Pulse' },
		{ value: 'FADE_IN', label: 'Fade In' },
		{ value: 'FADE_OUT', label: 'Fade Out' },
		{ value: 'STROBE', label: 'Strobe' },
		{ value: 'HOLD_TIMED', label: 'Hold Timed' },
		{ value: 'RAMP', label: 'Ramp' }
	];
	return behaviors.map(b => `<option value="${b.value}" ${b.value === selected ? 'selected' : ''}>${b.label}</option>`).join('');
}

function renderSceneActionOptions(selected) {
	const actions = [
		{ value: 'behavior', label: 'Apply Behavior' },
		{ value: 'on', label: 'Force On' },
		{ value: 'off', label: 'Force Off' },
		{ value: 'dim', label: 'Dim (steady level)' }
	];
	return actions.map(a => `<option value="${a.value}" ${a.value === selected ? 'selected' : ''}>${a.label}</option>`).join('');
}

function renderInfinityboxFunctionOptions(selected) {
	const options = ['<option value="">Select function...</option>'];
	appState.infinityboxFunctions.forEach(func => {
		const selectedAttr = func.name === selected ? 'selected' : '';
		options.push(`<option value="${func.name}" ${selectedAttr}>${func.name}</option>`);
	});
	return options.join('');
}

function renderInfinityboxBehaviorOptions(selected) {
	const behaviors = [
		{ value: 'on', label: 'On' },
		{ value: 'off', label: 'Off' },
		{ value: 'toggle', label: 'Toggle' },
		{ value: 'flash', label: 'Flash' },
		{ value: 'fade', label: 'Fade' },
		{ value: 'timed', label: 'Timed' },
		{ value: 'one_shot', label: 'One Shot' }
	];
	return behaviors.map(b => `<option value="${b.value}" ${b.value === selected ? 'selected' : ''}>${b.label}</option>`).join('');
}

function toHex(value, width) {
	const hex = Number(value || 0).toString(16).toUpperCase();
	return width ? hex.padStart(width, '0') : hex;
}

function bytesToHex(bytes) {
	if (!Array.isArray(bytes)) return '';
	return bytes.map(b => Number(b).toString(16).toUpperCase().padStart(2, '0')).join(' ');
}

function parseHexBytes(text) {
	if (!text) return [];
	return text.split(/[^0-9a-fA-F]+/)
		.filter(Boolean)
		.map(token => parseInt(token, 16))
		.filter(n => !Number.isNaN(n))
		.slice(0, 8);
}

function loadScenes() {
	fetch('/api/scenes')
		.then(r => r.json())
		.then(scenes => {
			appState.scenes = scenes;
			renderSceneList(scenes);
			renderSimulatorSceneList(scenes);
		})
		.catch(() => {
			const list = document.getElementById('scene-list');
			if (list) list.innerHTML = '<div class="alert alert-warning">Unable to load scenes. Check device connection.</div>';
			const simList = document.getElementById('simulator-scene-list');
			if (simList) simList.innerHTML = '<div class="alert alert-warning">Unable to load scenes. Check device connection.</div>';
		});
}

function activateScene(id) {
	fetch('/api/scene/activate/' + id)
		.then(() => loadScenes())
		.catch(() => loadScenes());
}

function deactivateScene(id) {
	fetch('/api/scene/deactivate/' + id)
		.then(() => loadScenes())
		.catch(() => loadScenes());
}

function deactivateAllScenes() {
	const active = (appState.scenes || []).filter(s => s.isActive);
	if (active.length === 0) return;
	Promise.all(active.map(scene => fetch('/api/scene/deactivate/' + scene.id)))
		.then(() => loadScenes())
		.catch(() => loadScenes());
}

function renderSimulatorSceneList(scenes) {
	const list = document.getElementById('simulator-scene-list');
	if (!list) return;
	if (!scenes || scenes.length === 0) {
		list.innerHTML = '<div class="alert alert-info">No scenes available yet. Create one in the Scenes tab.</div>';
		return;
	}
	list.innerHTML = scenes.map(scene => {
		const meta = [];
		if (scene.outputCount) meta.push(`${scene.outputCount} outputs`);
		if (scene.canCount) meta.push(`${scene.canCount} CAN frames`);
		if (scene.infinityboxCount) meta.push(`${scene.infinityboxCount} Infinitybox`);
		if (scene.suspensionEnabled) meta.push('Suspension');
		const metaText = meta.length ? ` • ${meta.join(' • ')}` : '';
		return `
			<div class="output-item">
				<div>
					<strong>${scene.name || scene.id}</strong>
					<div style="font-size: 0.85em; color: var(--muted)">${scene.description || 'No description'}${metaText}</div>
				</div>
				<div class="btn-group">
					${scene.isActive ? `<button class="btn btn-sm btn-danger" onclick="deactivateScene('${scene.id}')">Deactivate</button>` : `<button class="btn btn-sm btn-success" onclick="activateScene('${scene.id}')">Activate</button>`}
				</div>
			</div>
		`;
	}).join('') + `
		<div class="btn-group" style="margin-top: 12px;">
			<button class="btn btn-danger" onclick="deactivateAllScenes()">Stop All Scenes</button>
			<button class="btn" onclick="loadScenes()">Refresh</button>
		</div>
	`;
}

// ═══════════════════════════════════════════════════════════════════════════
// PREVIEW
// ═══════════════════════════════════════════════════════════════════════════

let previewInterval;
let previewInFlight = false;
let suspensionInFlight = false;
let previewErrorStreak = 0;
let simulatorInterval = null;
let scenePreviewInFlight = false;
let lastScenePreview = 0;
const pendingToggles = new Map();

function fetchWithTimeout(url, options = {}, timeoutMs = 1500) {
	const controller = new AbortController();
	const timer = setTimeout(() => controller.abort(), timeoutMs);
	return fetch(url, {...options, signal: controller.signal})
		.finally(() => clearTimeout(timer));
}

function startPreview() {
	if (previewInterval) return;
	updatePreview();
	updateSuspensionPreview();
	previewInterval = setInterval(() => {
		if (!appState.previewActive) return;
		if (appState.currentView !== 'preview') return;
		updatePreview();
		updateSuspensionPreview();
		const now = Date.now();
		if (now - lastScenePreview > 1000) {
			lastScenePreview = now;
			updateScenePreview();
		}
	}, 250);
}

function stopPreview() {
	if (!previewInterval) return;
	clearInterval(previewInterval);
	previewInterval = null;
}

function startSimulatorPolling() {
	if (simulatorInterval) return;
	loadScenes();
	simulatorInterval = setInterval(() => {
		if (appState.currentView !== 'simulator') return;
		loadScenes();
	}, 1000);
}

function stopSimulatorPolling() {
	if (!simulatorInterval) return;
	clearInterval(simulatorInterval);
	simulatorInterval = null;
}

function updatePreview() {
	if (previewInFlight) return;
	previewInFlight = true;
	const outputsPromise = appState.outputs
		? Promise.resolve(appState.outputs)
		: fetchWithTimeout('/api/outputs').then(r => r.json());
	const statesPromise = fetchWithTimeout('/api/outputs/live').then(r => r.json());

	Promise.all([outputsPromise, statesPromise])
		.then(([outputs, states]) => {
			appState.outputs = outputs;
			const stateById = new Map((states || []).map(state => [state.id, state]));
			const sortedOutputs = (outputs || []).slice().sort((a, b) => {
				const cellA = Number.isFinite(a.cellAddress) ? a.cellAddress : 999;
				const cellB = Number.isFinite(b.cellAddress) ? b.cellAddress : 999;
				if (cellA !== cellB) return cellA - cellB;
				const outA = Number.isFinite(a.outputNumber) ? a.outputNumber : 999;
				const outB = Number.isFinite(b.outputNumber) ? b.outputNumber : 999;
				return outA - outB;
			});
			const merged = sortedOutputs.map(out => {
				const state = stateById.get(out.id) || {};
				return {
					...out,
					...state,
					name: out.name,
					cellAddress: out.cellAddress,
					outputNumber: out.outputNumber
				};
			});
			const seen = new Set();
			const unique = [];
			for (const item of merged) {
				if (!item || !item.id || !item.name || seen.has(item.id)) continue;
				seen.add(item.id);
				unique.push(item);
			}
			renderPreview(unique.length ? unique : states);
			previewErrorStreak = 0;
			updatePreviewStatus('Live', false);
		})
		.catch(() => {
			previewErrorStreak += 1;
			updatePreviewStatus(`No data (${previewErrorStreak})`, true);
		})
		.finally(() => {
			previewInFlight = false;
		});
}

function renderPreview(states) {
	const container = document.getElementById('preview-outputs');
	if (!states || states.length === 0) {
		container.innerHTML = '<div class="alert alert-info">No outputs are configured yet.</div>';
		return;
	}
	container.innerHTML = states.filter(state => state && state.name).map(state => {
		const brightness = Math.max(0, Math.min(255, Number(state.currentValue || 0)));
		const desiredActive = !!state.desiredActive || Number(state.desiredValue || 0) > 0;
		const canActive = (state.source === 'can') && (!!state.isActive || brightness > 0);
		const isActive = canActive;
		const glow = brightness / 255;
		const cellMeta = (state.cellAddress !== undefined && state.outputNumber !== undefined)
			? `Cell ${state.cellAddress} • Output ${state.outputNumber}`
			: '';
		const amps = (typeof state.currentAmps === 'number') ? state.currentAmps : null;
		const volts = (typeof state.cellVoltageVolts === 'number') ? state.cellVoltageVolts : null;
		const temp = (typeof state.cellTemperatureC === 'number') ? state.cellTemperatureC : null;
		const meta = [
			amps !== null ? `${amps.toFixed(2)} A` : null,
			volts !== null ? `${volts.toFixed(2)} V` : null,
			temp !== null ? `${temp} °C` : null
		].filter(Boolean).join(' • ');
		const indicatorOpacity = Math.max(0.12, (state.source === 'can' ? glow : 0.12));
		const indicatorShadow = Math.max(0.12, (state.source === 'can' ? glow : 0.12));
		const pending = pendingToggles.has(state.id);
		return `
			<div class="preview-output" style="border-color: ${desiredActive ? '#facc15' : 'var(--border)'}">
				<div class="preview-output-header">
					<div class="preview-output-name">${state.name}</div>
					<div class="preview-output-indicator" style="opacity: ${indicatorOpacity}; box-shadow: 0 0 12px rgba(34, 197, 94, ${indicatorShadow});"></div>
				</div>
				<div class="preview-output-value">${isActive ? 'ACTIVE' : 'INACTIVE'}</div>
				${cellMeta ? `<div class="preview-output-meta">${cellMeta}</div>` : ''}
				${meta ? `<div class="preview-output-meta">${meta}</div>` : ''}
				<div class="preview-output-actions">
					<button class="btn btn-sm" ${pending ? 'disabled' : ''} onclick="toggleOutput('${state.id}', ${isActive ? 'true' : 'false'})">
						${pending ? 'Working…' : (isActive ? 'Turn Off' : 'Turn On')}
					</button>
				</div>
			</div>
		`;
	}).join('');
}

function togglePreviewUpdates() {
	appState.previewActive = !appState.previewActive;
	document.getElementById('preview-toggle-text').textContent = 
		appState.previewActive ? '⏸ Pause' : '▶️ Resume';
}

function updatePreviewStatus(text, isError) {
	const el = document.getElementById('preview-status');
	if (!el) return;
	const stamp = new Date().toLocaleTimeString();
	el.textContent = `Status: ${text} • ${stamp}`;
	el.style.color = isError ? 'var(--danger)' : 'var(--muted)';
}

function toggleOutput(id, isActive) {
	if (!id) return;
	if (pendingToggles.has(id)) return;
	pendingToggles.set(id, {at: Date.now(), target: !isActive});
	updatePreview();
	const safeId = encodeURIComponent(id);
	const postWithTimeout = (url, options) => fetchWithTimeout(url, options, 2500);
	const tryDeactivate = () =>
		postWithTimeout('/api/output/deactivate/' + safeId, {method: 'POST'})
			.then(response => {
				if (!response.ok) throw new Error('primary-deactivate');
			});
	const fallbackDeactivate = () =>
		postWithTimeout('/api/outputs/' + safeId + '/deactivate', {method: 'POST'})
			.then(response => {
				if (!response.ok) throw new Error('fallback-deactivate');
			});
	const payload = {
		type: 'STEADY',
		targetValue: 255,
		period_ms: 1000,
		dutyCycle: 50,
		duration_ms: 0
	};
	const tryActivate = () =>
		postWithTimeout('/api/output/behavior/' + safeId, {
			method: 'POST',
			headers: {'Content-Type': 'application/json'},
			body: JSON.stringify(payload)
		})
			.then(response => {
				if (!response.ok) throw new Error('primary-activate');
			});
	const fallbackActivate = () =>
		postWithTimeout('/api/outputs/' + safeId + '/behavior', {
			method: 'POST',
			headers: {'Content-Type': 'application/json'},
			body: JSON.stringify(payload)
		})
			.then(response => {
				if (!response.ok) throw new Error('fallback-activate');
			});
	const done = () => {
		pendingToggles.delete(id);
		updatePreview();
	};
	if (isActive) {
		tryDeactivate()
			.catch(fallbackDeactivate)
			.finally(done);
		return;
	}
	tryActivate()
		.catch(fallbackActivate)
		.finally(done);
}

function updateScenePreview() {
	if (scenePreviewInFlight) return;
	scenePreviewInFlight = true;
	fetchWithTimeout('/api/scenes')
		.then(r => r.json())
		.then(scenes => {
			const active = (scenes || []).filter(scene => scene.isActive);
			const el = document.getElementById('preview-scenes');
			if (!el) return;
			el.textContent = active.length ? '●' : '○';
			el.title = active.length ? active.map(s => s.name || s.id).join(', ') : 'Active scenes: none';
		})
		.catch(() => {
			const el = document.getElementById('preview-scenes');
			if (el) {
				el.textContent = '○';
				el.title = 'Active scenes: unavailable';
			}
		})
		.finally(() => {
			scenePreviewInFlight = false;
		});
}

function updateSuspensionPreview() {
	if (suspensionInFlight) return;
	suspensionInFlight = true;
	fetchWithTimeout('/api/suspension/state')
		.then(r => r.json())
		.then(state => {
			const targets = `FL ${state.front_left ?? 0} • FR ${state.front_right ?? 0} • RL ${state.rear_left ?? 0} • RR ${state.rear_right ?? 0}`;
			const actual = `FL ${state.actual_fl ?? 0} • FR ${state.actual_fr ?? 0} • RL ${state.actual_rl ?? 0} • RR ${state.actual_rr ?? 0}`;
			const calibrate = state.calibration_active ? 'Active' : 'Inactive';
			const feedbackAge = (typeof state.last_feedback_ms === 'number' && state.last_feedback_ms > 0)
				? `${Math.round(state.last_feedback_ms / 1000)}s ago`
				: '—';
			const targetEl = document.getElementById('suspension-targets');
			const actualEl = document.getElementById('suspension-actual');
			const calibEl = document.getElementById('suspension-calibration');
			const feedbackEl = document.getElementById('suspension-feedback');
			if (targetEl) targetEl.textContent = targets;
			if (actualEl) actualEl.textContent = actual;
			if (calibEl) calibEl.textContent = calibrate;
			if (feedbackEl) feedbackEl.textContent = feedbackAge;

			const flInput = document.getElementById('preview-suspension-front-left');
			const frInput = document.getElementById('preview-suspension-front-right');
			const rlInput = document.getElementById('preview-suspension-rear-left');
			const rrInput = document.getElementById('preview-suspension-rear-right');
			const calInput = document.getElementById('preview-suspension-calibrate');
			if (flInput && document.activeElement !== flInput) flInput.value = state.front_left ?? 0;
			if (frInput && document.activeElement !== frInput) frInput.value = state.front_right ?? 0;
			if (rlInput && document.activeElement !== rlInput) rlInput.value = state.rear_left ?? 0;
			if (rrInput && document.activeElement !== rrInput) rrInput.value = state.rear_right ?? 0;
			if (calInput) calInput.checked = !!state.calibration_active;
		})
		.catch(() => {
			const targetEl = document.getElementById('suspension-targets');
			if (targetEl) targetEl.textContent = 'Unavailable';
		})
		.finally(() => {
			suspensionInFlight = false;
		});
}

function refreshSuspensionPreview() {
	updateSuspensionPreview();
}

function applyPreviewSuspension() {
	const payload = {
		front_left: parseInt(document.getElementById('preview-suspension-front-left')?.value || 0),
		front_right: parseInt(document.getElementById('preview-suspension-front-right')?.value || 0),
		rear_left: parseInt(document.getElementById('preview-suspension-rear-left')?.value || 0),
		rear_right: parseInt(document.getElementById('preview-suspension-rear-right')?.value || 0),
		calibration_active: !!document.getElementById('preview-suspension-calibrate')?.checked
	};
	fetchWithTimeout('/api/suspension/set', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(payload)
	})
		.then(() => updateSuspensionPreview())
		.catch(() => updateSuspensionPreview());
}

function clearAllOutputs() {
	if (!confirm('Stop all active outputs?')) return;
	
	fetch('/api/outputs/stop-all', {method: 'POST'})
		.then(() => alert('All outputs stopped'));
}

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', () => {
	loadOutputs();
	loadScenes();
	loadInfinityboxFunctions();
});

</script>

</body>
</html>
)rawliteral";
