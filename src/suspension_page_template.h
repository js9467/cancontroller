#pragma once

// Prefabricated suspension control page template
// Customize the CAN frame configurations and button labels as needed
const char SUSPENSION_PAGE_HTML[] PROGMEM = R"rawliteral(
<div class="suspension-control-panel">
	<div class="panel-section">
		<h2 class="panel-title">Suspension Controls</h2>
		<p class="panel-subtitle">Front Damper Settings</p>
		
		<div class="damper-grid">
			<!-- Front Left Damper -->
			<div class="damper-card">
				<div class="damper-header">Front Left</div>
				<div class="damper-controls">
					<button class="damper-btn decrease" onclick="adjustDamper('front_left', -1)" 
						data-pgn="737h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-decrease="00 00 00 00 00 00 00 00"
						title="Decrease damping">−</button>
					
					<div class="damper-display">
						<span class="damper-value" id="fl-value">0</span>
						<span class="damper-unit">%</span>
					</div>
					
					<button class="damper-btn increase" onclick="adjustDamper('front_left', +1)"
						data-pgn="737h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-increase="01 00 00 00 00 00 00 00"
						title="Increase damping">+</button>
				</div>
				<div class="damper-status">
					<span class="status-indicator" id="fl-status">●</span>
					<span class="status-text">Ready</span>
				</div>
			</div>
			
			<!-- Front Right Damper -->
			<div class="damper-card">
				<div class="damper-header">Front Right</div>
				<div class="damper-controls">
					<button class="damper-btn decrease" onclick="adjustDamper('front_right', -1)"
						data-pgn="737h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-decrease="00 00 00 00 00 00 00 00">−</button>
					
					<div class="damper-display">
						<span class="damper-value" id="fr-value">0</span>
						<span class="damper-unit">%</span>
					</div>
					
					<button class="damper-btn increase" onclick="adjustDamper('front_right', +1)"
						data-pgn="737h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-increase="02 00 00 00 00 00 00 00">+</button>
				</div>
				<div class="damper-status">
					<span class="status-indicator" id="fr-status">●</span>
					<span class="status-text">Ready</span>
				</div>
			</div>
		</div>
	</div>
	
	<div class="panel-section">
		<p class="panel-subtitle">Rear Damper Settings</p>
		
		<div class="damper-grid">
			<!-- Rear Left Damper -->
			<div class="damper-card">
				<div class="damper-header">Rear Left</div>
				<div class="damper-controls">
					<button class="damper-btn decrease" onclick="adjustDamper('rear_left', -1)"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-decrease="00 00 00 00 00 00 00 00">−</button>
					
					<div class="damper-display">
						<span class="damper-value" id="rl-value">0</span>
						<span class="damper-unit">%</span>
					</div>
					
					<button class="damper-btn increase" onclick="adjustDamper('rear_left', +1)"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-increase="03 00 00 00 00 00 00 00">+</button>
				</div>
				<div class="damper-status">
					<span class="status-indicator" id="rl-status">●</span>
					<span class="status-text">Ready</span>
				</div>
			</div>
			
			<!-- Rear Right Damper -->
			<div class="damper-card">
				<div class="damper-header">Rear Right</div>
				<div class="damper-controls">
					<button class="damper-btn decrease" onclick="adjustDamper('rear_right', -1)"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-decrease="00 00 00 00 00 00 00 00">−</button>
					
					<div class="damper-display">
						<span class="damper-value" id="rr-value">0</span>
						<span class="damper-unit">%</span>
					</div>
					
					<button class="damper-btn increase" onclick="adjustDamper('rear_right', +1)"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data-increase="04 00 00 00 00 00 00 00">+</button>
				</div>
				<div class="damper-status">
					<span class="status-indicator" id="rr-status">●</span>
					<span class="status-text">Ready</span>
				</div>
			</div>
		</div>
	</div>
	
	<div class="panel-section">
		<p class="panel-subtitle">Anti-Roll & Pitch Control</p>
		
		<div class="control-grid">
			<!-- Anti-Roll Damper -->
			<div class="control-card">
				<div class="control-header">Anti-Roll Damper</div>
				<div class="control-buttons">
					<button class="control-btn" onclick="sendCanCommand('anti_roll_decrease')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="05 00 00 00 00 00 00 00">
						Decrease
					</button>
					<button class="control-btn primary" onclick="sendCanCommand('anti_roll_neutral')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="00 00 00 00 00 00 00 00">
						Neutral
					</button>
					<button class="control-btn" onclick="sendCanCommand('anti_roll_increase')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="06 00 00 00 00 00 00 00">
						Increase
					</button>
				</div>
			</div>
			
			<!-- Anti-Pitch Damper -->
			<div class="control-card">
				<div class="control-header">Anti-Pitch Damper</div>
				<div class="control-buttons">
					<button class="control-btn" onclick="sendCanCommand('anti_pitch_decrease')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="07 00 00 00 00 00 00 00">
						Decrease
					</button>
					<button class="control-btn primary" onclick="sendCanCommand('anti_pitch_neutral')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="00 00 00 00 00 00 00 00">
						Neutral
					</button>
					<button class="control-btn" onclick="sendCanCommand('anti_pitch_increase')"
						data-pgn="738h" data-priority="6" data-src="F9" data-dest="FF"
						data-data="08 00 00 00 00 00 00 00">
						Increase
					</button>
				</div>
			</div>
		</div>
	</div>
	
	<div class="panel-section info">
		<div class="info-box">
			<h3>Configuration Notes</h3>
			<ul>
				<li><strong>Bitrate:</strong> 250 kbps (J1939)</li>
				<li><strong>Protocol:</strong> J1939 CAN</li>
				<li><strong>Default Priority:</strong> 6</li>
				<li><strong>Source Address:</strong> 0xF9 (TCU)</li>
				<li><strong>Destination Address:</strong> 0xFF (All)</li>
			</ul>
		</div>
		<div class="info-box">
			<h3>Frame Reference</h3>
			<ul>
				<li><strong>Front Damper:</strong> PGN 0x0737 (CAN ID 0x0737)</li>
				<li><strong>Rear Damper:</strong> PGN 0x0738 (CAN ID 0x0738)</li>
				<li><strong>Reserved:</strong> Bytes 1-8 for command data</li>
				<li><strong>Format:</strong> Hex bytes (00-FF)</li>
			</ul>
		</div>
	</div>
</div>

<style>
.suspension-control-panel {
	background: linear-gradient(135deg, #0f0f0f 0%, #1a1a1f 100%);
	padding: 20px;
	border-radius: 16px;
	font-family: 'Space Grotesk', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
	color: #f2f4f8;
}

.panel-section {
	margin-bottom: 24px;
	padding: 0;
}

.panel-section:last-child {
	margin-bottom: 0;
}

.panel-section.info {
	background: rgba(255, 157, 46, 0.05);
	padding: 16px;
	border: 1px solid rgba(255, 157, 46, 0.2);
	border-radius: 12px;
	margin-top: 16px;
}

.panel-title {
	font-size: 24px;
	font-weight: 700;
	margin: 0 0 4px;
	letter-spacing: 0.5px;
	color: #ff9d2e;
}

.panel-subtitle {
	font-size: 13px;
	color: #8d92a3;
	margin: 12px 0 12px;
	text-transform: uppercase;
	letter-spacing: 1px;
	font-weight: 600;
}

.damper-grid {
	display: grid;
	grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
	gap: 12px;
	margin-bottom: 16px;
}

.damper-card {
	background: #12141c;
	border: 1px solid #20232f;
	border-radius: 14px;
	padding: 14px;
	display: flex;
	flex-direction: column;
	align-items: center;
	transition: all 0.2s ease;
}

.damper-card:hover {
	border-color: #ff9d2e;
	background: #16192a;
	box-shadow: 0 8px 24px rgba(255, 157, 46, 0.1);
}

.damper-header {
	font-size: 12px;
	font-weight: 700;
	color: #7ad7f0;
	margin-bottom: 10px;
	text-transform: uppercase;
	letter-spacing: 0.5px;
}

.damper-controls {
	display: flex;
	align-items: center;
	justify-content: center;
	gap: 8px;
	margin-bottom: 10px;
}

.damper-btn {
	width: 36px;
	height: 36px;
	border-radius: 8px;
	border: 1px solid #3a3a4a;
	background: #1a1d28;
	color: #f2f4f8;
	font-size: 18px;
	font-weight: 700;
	cursor: pointer;
	transition: all 0.15s ease;
	display: flex;
	align-items: center;
	justify-content: center;
	padding: 0;
}

.damper-btn:hover {
	background: #ff9d2e;
	color: #16110a;
	border-color: #ff9d2e;
	transform: scale(1.05);
}

.damper-btn:active {
	transform: scale(0.95);
}

.damper-btn.decrease:hover {
	background: #ff6b6b;
	border-color: #ff6b6b;
}

.damper-btn.increase:hover {
	background: #3dd598;
	border-color: #3dd598;
}

.damper-display {
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;
	min-width: 50px;
}

.damper-value {
	font-size: 20px;
	font-weight: 700;
	color: #ff9d2e;
}

.damper-unit {
	font-size: 10px;
	color: #8d92a3;
	margin-top: 2px;
}

.damper-status {
	display: flex;
	align-items: center;
	gap: 6px;
	font-size: 11px;
	color: #8d92a3;
}

.status-indicator {
	color: #3dd598;
	font-size: 12px;
}

.control-grid {
	display: grid;
	grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
	gap: 12px;
	margin-bottom: 16px;
}

.control-card {
	background: #12141c;
	border: 1px solid #20232f;
	border-radius: 14px;
	padding: 14px;
	transition: all 0.2s ease;
}

.control-card:hover {
	border-color: #7ad7f0;
	background: #16192a;
}

.control-header {
	font-size: 13px;
	font-weight: 700;
	color: #7ad7f0;
	margin-bottom: 12px;
	text-transform: uppercase;
	letter-spacing: 0.5px;
}

.control-buttons {
	display: grid;
	grid-template-columns: 1fr 1fr 1fr;
	gap: 8px;
}

.control-btn {
	padding: 10px 8px;
	border-radius: 8px;
	border: 1px solid #3a3a4a;
	background: #1a1d28;
	color: #f2f4f8;
	font-size: 11px;
	font-weight: 700;
	cursor: pointer;
	transition: all 0.15s ease;
	text-transform: uppercase;
	letter-spacing: 0.5px;
	white-space: nowrap;
}

.control-btn:hover {
	background: #ff9d2e;
	color: #16110a;
	border-color: #ff9d2e;
	transform: translateY(-2px);
}

.control-btn.primary {
	background: #3dd598;
	color: #0a0f0a;
	border-color: #3dd598;
}

.control-btn.primary:hover {
	background: #2eb882;
	border-color: #2eb882;
}

.info-box {
	margin-bottom: 12px;
}

.info-box:last-child {
	margin-bottom: 0;
}

.info-box h3 {
	font-size: 12px;
	color: #ff9d2e;
	margin: 0 0 8px;
	text-transform: uppercase;
	letter-spacing: 0.5px;
	font-weight: 700;
}

.info-box ul {
	margin: 0;
	padding-left: 16px;
	list-style: none;
}

.info-box li {
	font-size: 12px;
	color: #8d92a3;
	margin-bottom: 6px;
	line-height: 1.4;
}

.info-box strong {
	color: #f2f4f8;
}

@media (max-width: 480px) {
	.damper-grid {
		grid-template-columns: repeat(2, 1fr);
	}
	
	.control-grid {
		grid-template-columns: 1fr;
	}
	
	.control-buttons {
		grid-template-columns: repeat(3, 1fr);
	}
	
	.panel-title {
		font-size: 20px;
	}
}
</style>

<script>
// Suspension control system state
let suspensionState = {
	front_left: 0,
	front_right: 0,
	rear_left: 0,
	rear_right: 0
};

// Function to adjust damper values
function adjustDamper(location, direction) {
	const currentValue = suspensionState[location] || 0;
	const newValue = Math.max(0, Math.min(100, currentValue + (direction * 5)));
	suspensionState[location] = newValue;
	
	// Update display
	const displayId = location.split('_')[0][0] + location.split('_')[1][0];
	const valueElement = document.getElementById(displayId + '-value');
	if (valueElement) {
		valueElement.textContent = newValue;
	}
	
	// Send CAN frame
	sendSuspensionCommand(location, newValue);
}

// Function to send CAN commands
function sendCanCommand(command) {
	// Command structure:
	// anti_roll_decrease, anti_roll_neutral, anti_roll_increase
	// anti_pitch_decrease, anti_pitch_neutral, anti_pitch_increase
	const map = {
		anti_roll_decrease: [0x05, 0, 0, 0, 0, 0, 0, 0],
		anti_roll_neutral:  [0x00, 0, 0, 0, 0, 0, 0, 0],
		anti_roll_increase: [0x06, 0, 0, 0, 0, 0, 0, 0],
		anti_pitch_decrease:[0x07, 0, 0, 0, 0, 0, 0, 0],
		anti_pitch_neutral: [0x00, 0, 0, 0, 0, 0, 0, 0],
		anti_pitch_increase:[0x08, 0, 0, 0, 0, 0, 0, 0]
	};

	const data = map[command];
	if (!data) return;

	fetch('/api/can/send_std', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ id: 0x738, data })
	}).then(() => updateStatus(command, true))
	  .catch(() => updateStatus(command, false));
}

// Function to send suspension-specific CAN command
function sendSuspensionCommand(location, value) {
	const payload = {
		front_left: suspensionState.front_left || 0,
		front_right: suspensionState.front_right || 0,
		rear_left: suspensionState.rear_left || 0,
		rear_right: suspensionState.rear_right || 0
	};

	fetch('/api/suspension/set', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	}).then(() => {
		const locKey = location.split('_').map(x => x[0]).join('').toUpperCase();
		const statusEl = document.getElementById(locKey + '-status');
		if (statusEl) {
			statusEl.style.color = '#7ad7f0';
			statusEl.textContent = '◐';
			setTimeout(() => {
				statusEl.style.color = '#3dd598';
				statusEl.textContent = '●';
			}, 300);
		}
	});
}

// Function to update status indicator
function updateStatus(location, success) {
	const statusEl = document.querySelector(`[onclick*="${location}"]`);
	if (statusEl) {
		statusEl.style.opacity = success ? '1' : '0.5';
	}
}

// Initialize on load
document.addEventListener('DOMContentLoaded', function() {
	console.log('[Suspension] Control panel initialized');
	// Set initial display values
	Object.keys(suspensionState).forEach(key => {
		const displayId = key.split('_').map(x => x[0]).join('').toLowerCase();
		const element = document.getElementById(displayId + '-value');
		if (element) {
			element.textContent = suspensionState[key];
		}
	});
});
</script>
)rawliteral";
