#pragma once

#include <Arduino.h>

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT WEB INTERFACE                                          ║
 * ║                                                                           ║
 * ║  Provides intuitive UI for:                                               ║
 * ║    - Defining outputs with physical mapping                              ║
 * ║    - Configuring behaviors (flash, fade, pulse, etc.)                    ║
 * ║    - Creating custom patterns                                             ║
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
/* BEHAVIOR TYPES */
/* ═══════════════════════════════════════════════════════════════════════ */

.behavior-types {
	display: grid;
	grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
	gap: 10px;
	margin-bottom: 20px;
}

.behavior-type {
	padding: 12px;
	border: 2px solid var(--border);
	border-radius: 12px;
	text-align: center;
	cursor: pointer;
	transition: all 0.2s;
	background: var(--surface);
}

.behavior-type:hover {
	border-color: var(--accent);
	transform: translateY(-2px);
}

.behavior-type.selected {
	border-color: var(--accent);
	background: var(--accent);
	color: #16110a;
}

.behavior-type-icon {
	font-size: 1.5rem;
	margin-bottom: 6px;
}

.behavior-type-name {
	font-size: 0.85rem;
	font-weight: 700;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PATTERN EDITOR */
/* ═══════════════════════════════════════════════════════════════════════ */

.pattern-steps {
	display: flex;
	flex-direction: column;
	gap: 8px;
	margin-bottom: 12px;
}

.pattern-step {
	background: var(--surface);
	border: 1px solid var(--border);
	border-radius: 10px;
	padding: 12px;
	display: grid;
	grid-template-columns: 40px 1fr 1fr 100px auto;
	gap: 12px;
	align-items: center;
}

.step-number {
	width: 32px;
	height: 32px;
	border-radius: 50%;
	background: var(--accent);
	color: #16110a;
	display: flex;
	align-items: center;
	justify-content: center;
	font-weight: 700;
	font-size: 0.9rem;
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

.preview-output {
	aspect-ratio: 1;
	border-radius: 12px;
	border: 2px solid var(--border);
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;
	gap: 8px;
	transition: all 0.1s;
	background: var(--surface);
}

.preview-output-name {
	font-size: 0.75rem;
	color: var(--muted);
}

.preview-output-value {
	font-weight: 700;
	font-size: 1rem;
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
	<p>Intent-based output control with behaviors, patterns, and scenes</p>
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
			<div class="nav-item" data-view="behaviors">
				<span class="nav-icon">⚡</span>
				<span>Behaviors</span>
			</div>
			<div class="nav-item" data-view="patterns">
				<span class="nav-icon">🎨</span>
				<span>Patterns</span>
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
			<div class="nav-item" data-view="can-direct">
				<span class="nav-icon">🔧</span>
				<span>Direct CAN</span>
			</div>
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
					💡 <strong>Tip:</strong> Start by defining your physical outputs (lights, relays, etc.), then assign behaviors to them.
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
		<!-- VIEW: BEHAVIORS -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-behaviors" class="view hidden">
			<div class="card">
				<h2>Behavior Designer</h2>
				<p>Configure how outputs should behave (flash, fade, pulse, etc.)</p>
				
				<div class="section">
					<div class="section-title">🎯 Select Output</div>
					<select id="behavior-output-select" class="form-control" onchange="loadBehaviorForOutput()">
						<option value="">-- Select an output --</option>
					</select>
				</div>
				
				<div id="behavior-config-panel" class="hidden">
					<div class="section">
						<div class="section-title">⚡ Behavior Type</div>
						<div class="behavior-types" id="behavior-types">
							<div class="behavior-type" data-type="STEADY">
								<div class="behavior-type-icon">💡</div>
								<div class="behavior-type-name">Steady</div>
							</div>
							<div class="behavior-type" data-type="FLASH">
								<div class="behavior-type-icon">⚡</div>
								<div class="behavior-type-name">Flash</div>
							</div>
							<div class="behavior-type" data-type="PULSE">
								<div class="behavior-type-icon">💓</div>
								<div class="behavior-type-name">Pulse</div>
							</div>
							<div class="behavior-type" data-type="FADE_IN">
								<div class="behavior-type-icon">🌅</div>
								<div class="behavior-type-name">Fade In</div>
							</div>
							<div class="behavior-type" data-type="FADE_OUT">
								<div class="behavior-type-icon">🌇</div>
								<div class="behavior-type-name">Fade Out</div>
							</div>
							<div class="behavior-type" data-type="STROBE">
								<div class="behavior-type-icon">⚠️</div>
								<div class="behavior-type-name">Strobe</div>
							</div>
							<div class="behavior-type" data-type="PATTERN">
								<div class="behavior-type-icon">🎨</div>
								<div class="behavior-type-name">Pattern</div>
							</div>
							<div class="behavior-type" data-type="HOLD_TIMED">
								<div class="behavior-type-icon">⏱️</div>
								<div class="behavior-type-name">Timed Hold</div>
							</div>
						</div>
					</div>
					
					<div class="section">
						<div class="section-title">⚙️ Parameters</div>
						<div class="form-grid" id="behavior-params">
							<!-- Dynamically populated based on selected behavior type -->
						</div>
					</div>
					
					<div class="btn-group">
						<button class="btn btn-success" onclick="applyBehavior()">
							✓ Apply Behavior
						</button>
						<button class="btn btn-danger" onclick="stopBehavior()">
							⏹ Stop Output
						</button>
						<button class="btn" onclick="testBehavior()">
							▶️ Test (5s)
						</button>
					</div>
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: PATTERNS -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-patterns" class="view hidden">
			<div class="card">
				<h2>Pattern Library</h2>
				<p>Create custom timing patterns for complex behaviors</p>
				
				<div class="alert alert-warning">
					⚠️ <strong>Advanced:</strong> Patterns let you define exact sequences of states and transitions.
				</div>
				
				<button class="btn btn-primary" onclick="createNewPattern()">
					➕ New Pattern
				</button>
				
				<div class="divider"></div>
				
				<div id="pattern-list">
					<!-- Pattern list goes here -->
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
				
				<div class="divider"></div>
				
				<div class="btn-group">
					<button class="btn" onclick="togglePreviewUpdates()">
						<span id="preview-toggle-text">⏸ Pause</span>
					</button>
					<button class="btn" onclick="clearAllOutputs()">
						🔴 Stop All
					</button>
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: SIMULATOR -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-simulator" class="view hidden">
			<div class="card">
				<h2>Behavior Simulator</h2>
				<p>Test predefined scenes with live output activation</p>
				
				<div class="alert alert-info">
					🎮 <strong>How to use:</strong><br>
					1. Click any scene button below to activate it<br>
					2. Watch the outputs activate in real-time<br>
					3. Go to "Live Preview" view to see visual feedback<br>
					4. These scenes control the predefined turn signals, hazards, and beacon
				</div>
				
				<div class="section">
					<label>Quick Scene Tests</label>
					<div class="btn-group">
						<button class="btn" onclick="simulateLeftTurn()">◀️ Left Turn</button>
						<button class="btn" onclick="simulateRightTurn()">▶️ Right Turn</button>
						<button class="btn" onclick="simulate4Way()">⚠️ 4-Way</button>
						<button class="btn" onclick="simulateBeacon()">🚨 Beacon</button>
					</div>
				</div>
				
				<div class="divider"></div>
				
				<div class="alert alert-warning">
					⚡ <strong>Note:</strong> These buttons activate scenes that are pre-configured in the system.
					Create your own scenes in the "Scenes" tab to add more options here.
				</div>
			</div>
		</div>

		<!-- ───────────────────────────────────────────────────────────── -->
		<!-- VIEW: DIRECT CAN -->
		<!-- ───────────────────────────────────────────────────────────── -->
		<div id="view-can-direct" class="view hidden">
			<div class="card">
				<h2>Direct CAN Control</h2>
				<p>For non-POWERCELL devices: map outputs to custom CAN frames</p>
				
				<div class="alert alert-info">
					🔧 This allows behavioral control of arbitrary CAN devices by defining
					custom frame mappings for each output.
				</div>
				
				<!-- Direct CAN interface -->
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
	patterns: [],
	scenes: [],
	currentView: 'outputs',
	selectedOutput: null,
	previewActive: true
};

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
	if (viewName === 'behaviors') loadBehaviorOutputSelect();
	if (viewName === 'patterns') loadPatterns();
	if (viewName === 'scenes') loadScenes();
	if (viewName === 'preview') startPreview();
}

// ═══════════════════════════════════════════════════════════════════════════
// OUTPUT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

function showOutputEditor(outputId = null) {
	const name = prompt('Output Name (e.g., "Left Turn Signal"):');
	if (!name) return;
	
	const cellAddr = parseInt(prompt('Cell Address (1-254):', '1'));
	const outputNum = parseInt(prompt('Output Number (1-10):', '1'));
	
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
	
	const cellAddr = parseInt(prompt('Cell Address (1-254):', output.cellAddress));
	const outputNum = parseInt(prompt('Output Number (1-10):', output.outputNumber));
	
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
// BEHAVIOR CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

function loadBehaviorOutputSelect() {
	const select = document.getElementById('behavior-output-select');
	select.innerHTML = '<option value="">-- Select an output --</option>';
	appState.outputs.forEach(out => {
		select.innerHTML += `<option value="${out.id}">${out.name}</option>`;
	});
}

function loadBehaviorForOutput() {
	const outputId = document.getElementById('behavior-output-select').value;
	if (!outputId) {
		document.getElementById('behavior-config-panel').classList.add('hidden');
		return;
	}
	
	appState.selectedOutput = outputId;
	document.getElementById('behavior-config-panel').classList.remove('hidden');
	
	// Load current behavior if exists
	fetch(`/api/outputs/${outputId}/behavior`)
		.then(r => r.json())
		.then(behavior => {
			if (behavior) selectBehaviorType(behavior.type);
		});
}

document.querySelectorAll('.behavior-type').forEach(typeEl => {
	typeEl.addEventListener('click', () => {
		const type = typeEl.dataset.type;
		selectBehaviorType(type);
	});
});

function selectBehaviorType(type) {
	document.querySelectorAll('.behavior-type').forEach(el => {
		el.classList.toggle('selected', el.dataset.type === type);
	});
	
	renderBehaviorParams(type);
}

function renderBehaviorParams(type) {
	const paramsDiv = document.getElementById('behavior-params');
	
	const commonParams = `
		<div class="form-group">
			<label>Target Brightness (0-255)</label>
			<input type="range" id="param-target" min="0" max="255" value="255" oninput="this.nextElementSibling.textContent=this.value">
			<span>255</span>
		</div>
		<div class="form-group">
			<label>Duration (ms, 0=infinite)</label>
			<input type="number" id="param-duration" value="0" min="0">
		</div>
		<div class="form-group">
			<label>Priority (0-255)</label>
			<input type="number" id="param-priority" value="100" min="0" max="255">
		</div>
	`;
	
	let specificParams = '';
	
	switch (type) {
		case 'FLASH':
			specificParams = `
				<div class="form-group">
					<label>Period (ms)</label>
					<input type="number" id="param-period" value="1000" min="50">
				</div>
				<div class="form-group">
					<label>Duty Cycle (%)</label>
					<input type="range" id="param-duty" min="0" max="100" value="50" oninput="this.nextElementSibling.textContent=this.value+'%'">
					<span>50%</span>
				</div>
			`;
			break;
		case 'PULSE':
			specificParams = `
				<div class="form-group">
					<label>Period (ms)</label>
					<input type="number" id="param-period" value="2000" min="100">
				</div>
			`;
			break;
		case 'FADE_IN':
		case 'FADE_OUT':
			specificParams = `
				<div class="form-group">
					<label>Fade Time (ms)</label>
					<input type="number" id="param-fade-time" value="1000" min="50">
				</div>
			`;
			break;
		case 'STROBE':
			specificParams = `
				<div class="form-group">
					<label>On Time (ms)</label>
					<input type="number" id="param-on-time" value="50" min="10">
				</div>
				<div class="form-group">
					<label>Off Time (ms)</label>
					<input type="number" id="param-off-time" value="50" min="10">
				</div>
			`;
			break;
	}
	
	paramsDiv.innerHTML = commonParams + specificParams;
}

function applyBehavior() {
	if (!appState.selectedOutput) return;
	
	const type = document.querySelector('.behavior-type.selected')?.dataset.type;
	if (!type) {
		alert('Please select a behavior type');
		return;
	}
	
	const behavior = {
		type: type,
		targetValue: parseInt(document.getElementById('param-target')?.value || 255),
		duration_ms: parseInt(document.getElementById('param-duration')?.value || 0),
		priority: parseInt(document.getElementById('param-priority')?.value || 100),
		period_ms: parseInt(document.getElementById('param-period')?.value || 1000),
		dutyCycle: parseInt(document.getElementById('param-duty')?.value || 50),
		fadeTime_ms: parseInt(document.getElementById('param-fade-time')?.value || 1000),
		onTime_ms: parseInt(document.getElementById('param-on-time')?.value || 50),
		offTime_ms: parseInt(document.getElementById('param-off-time')?.value || 50)
	};
	
	fetch(`/api/outputs/${appState.selectedOutput}/behavior`, {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(behavior)
	}).then(() => {
		alert('Behavior applied!');
	});
}

function stopBehavior() {
	if (!appState.selectedOutput) return;
	
	fetch(`/api/outputs/${appState.selectedOutput}/deactivate`, {
		method: 'POST'
	}).then(() => {
		alert('Output stopped');
	});
}

function testBehavior() {
	applyBehavior();
	setTimeout(() => stopBehavior(), 5000);
}

// ═══════════════════════════════════════════════════════════════════════════
// PATTERNS
// ═══════════════════════════════════════════════════════════════════════════

function loadPatterns() {
	fetch('/api/patterns')
		.then(r => r.json())
		.then(patterns => {
			appState.patterns = patterns;
			renderPatternList(patterns);
		});
}

function renderPatternList(patterns) {
	const list = document.getElementById('pattern-list');
	if (!patterns || patterns.length === 0) {
		list.innerHTML = '<div class="alert alert-info">No patterns defined. Create your first pattern!</div>';
		return;
	}
	
	list.innerHTML = patterns.map(p => `
		<div class="output-item">
			<div>
				<strong>${p.name}</strong>
				<div style="font-size: 0.85em; color: var(--muted)">${p.steps.length} steps</div>
			</div>
			<div class="btn-group">
				<button class="btn btn-sm" onclick="editPattern('${p.id}')">Edit</button>
				<button class="btn btn-sm btn-danger" onclick="deletePattern('${p.id}')">Delete</button>
			</div>
		</div>
	`).join('');
}

function createNewPattern() {
	const name = prompt('Pattern Name:');
	if (!name) return;
	
	const pattern = {
		id: 'pattern_' + Date.now(),
		name: name,
		steps: [
			{value: 255, duration_ms: 500},
			{value: 0, duration_ms: 500}
		]
	};
	
	fetch('/api/patterns', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(pattern)
	})
	.then(r => r.json())
	.then(() => {
		alert('Pattern created! Use it in behaviors by selecting PATTERN type.');
		loadPatterns();
	});
}

function editPattern(id) {
	alert('Pattern editor coming soon! For now, use the API to update patterns.');
}

function deletePattern(id) {
	if (!confirm('Delete this pattern?')) return;
	
	fetch('/api/patterns/' + id, {method: 'DELETE'})
		.then(() => loadPatterns());
}

// ═══════════════════════════════════════════════════════════════════════════
// SCENES
// ═══════════════════════════════════════════════════════════════════════════

function loadScenes() {
	fetch('/api/scenes')
		.then(r => r.json())
		.then(scenes => {
			appState.scenes = scenes;
			renderSceneList(scenes);
		});
}

function renderSceneList(scenes) {
	const list = document.getElementById('scene-list');
	if (!scenes || scenes.length === 0) {
		list.innerHTML = '<div class="alert alert-info">No scenes created. Build your first scene!</div>';
		return;
	}
	
	list.innerHTML = scenes.map(s => `
		<div class="output-item">
			<div>
				<strong>${s.name}</strong>
				<div style="font-size: 0.85em; color: var(--muted)">
					${s.description || 'No description'}
				</div>
			</div>
			<div class="btn-group">
				<button class="btn btn-sm btn-success" onclick="activateScene('${s.id}')">Activate</button>
				<button class="btn btn-sm" onclick="editScene('${s.id}')">Edit</button>
				<button class="btn btn-sm btn-danger" onclick="deleteScene('${s.id}')">Delete</button>
			</div>
		</div>
	`).join('');
}

function createNewScene() {
	const name = prompt('Scene Name:');
	if (!name) return;
	
	const description = prompt('Description (optional):') || '';
	
	const scene = {
		id: 'scene_' + Date.now(),
		name: name,
		description: description,
		duration_ms: 0,
		priority: 100,
		exclusive: false,
		behaviors: []
	};
	
	fetch('/api/scenes', {
		method: 'POST',
		headers: {'Content-Type': 'application/json'},
		body: JSON.stringify(scene)
	})
	.then(r => r.json())
	.then(() => {
		alert('Scene created! Now add behaviors to outputs and save.');
		loadScenes();
	});
}

function activateScene(id) {
	fetch('/api/scenes/activate/' + id, {method: 'POST'})
		.then(() => console.log('Scene activated: ' + id));
}

function editScene(id) {
	alert('Scene editor coming soon! For now, scenes can be activated from the Simulator view.');
}

function deleteScene(id) {
	if (!confirm('Delete this scene?')) return;
	
	fetch('/api/scenes/' + id, {method: 'DELETE'})
		.then(() => loadScenes());
}

// ═══════════════════════════════════════════════════════════════════════════
// PREVIEW
// ═══════════════════════════════════════════════════════════════════════════

let previewInterval;

function startPreview() {
	if (previewInterval) return;
	
	previewInterval = setInterval(() => {
		if (!appState.previewActive) return;
		updatePreview();
	}, 100);
}

function updatePreview() {
	fetch('/api/outputs/state')
		.then(r => r.json())
		.then(states => {
			renderPreview(states);
		});
}

function renderPreview(states) {
	const container = document.getElementById('preview-outputs');
	container.innerHTML = states.map(state => {
		const brightness = state.currentValue;
		const bgColor = `rgba(255, 157, 46, ${brightness / 255})`;
		return `
			<div class="preview-output" style="background: ${bgColor}; border-color: ${brightness > 0 ? 'var(--accent)' : 'var(--border)'}">
				<div class="preview-output-name">${state.name}</div>
				<div class="preview-output-value">${brightness}</div>
			</div>
		`;
	}).join('');
}

function togglePreviewUpdates() {
	appState.previewActive = !appState.previewActive;
	document.getElementById('preview-toggle-text').textContent = 
		appState.previewActive ? '⏸ Pause' : '▶️ Resume';
}

function clearAllOutputs() {
	if (!confirm('Stop all active outputs?')) return;
	
	fetch('/api/outputs/stop-all', {method: 'POST'})
		.then(() => alert('All outputs stopped'));
}

// ═══════════════════════════════════════════════════════════════════════════
// SIMULATOR
// ═══════════════════════════════════════════════════════════════════════════

function simulateLeftTurn() {
	fetch('/api/scenes/activate/left_turn', {method: 'POST'});
}

function simulateRightTurn() {
	fetch('/api/scenes/activate/right_turn', {method: 'POST'});
}

function simulate4Way() {
	fetch('/api/scenes/activate/four_way', {method: 'POST'});
}

function simulateBeacon() {
	fetch('/api/scenes/activate/beacon', {method: 'POST'});
}

// ═══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

document.addEventListener('DOMContentLoaded', () => {
	loadOutputs();
	loadScenes();
	loadPatterns();
	startPreview();
});

</script>

</body>
</html>
)rawliteral";
