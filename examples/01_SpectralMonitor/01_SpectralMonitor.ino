#include <WiFi.h>
#include <WebServer.h>
#include <AS7343_7Semi.h>

AS7343_7Semi sensor;
WebServer server(80);

// ============================================================
// Wi-Fi Access Point
// ============================================================

const char* AP_SSID     = "AS7343-Spectral";
const char* AP_PASSWORD = "12345678";

// ============================================================
// Spectral channels
// ============================================================

struct SpectralChannel {
    const char* name;
    uint16_t wavelength;
    const char* region;
    uint16_t value;
};

SpectralChannel channels[12] = {
    {"F1",  405, "Violet",             0},
    {"F2",  425, "Violet-Blue",      0},
    {"FZ",  450, "Blue",               0},
    {"F3",  475, "Blue-Cyan",          0},
    {"F4",  515, "Green",              0},
    {"F5",  550, "Green-Yellow",       0},
    {"FY",  555, "Yellow-Green",       0},
    {"FXL", 600, "Orange",             0},
    {"F6",  640, "Red",                0},
    {"F7",  690, "Deep Red",           0},
    {"F8",  745, "Red Near Infrared",      0},
    {"NIR", 855, "Near Infrared",      0}
};

bool sensorConnected = false;
bool ledEnabled = true;

unsigned long lastSensorRead = 0;
unsigned long lastPresenceCheck = 0;

const unsigned long SENSOR_INTERVAL  = 200;
const unsigned long PRESENCE_INTERVAL = 1000;

// ============================================================
// Read AS7343
// ============================================================

void updateChannelValues() {
    channels[0].value  = sensor.violet_F1();
    channels[1].value  = sensor.violetBlue_F2();
    channels[2].value  = sensor.blue_FZ();
    channels[3].value  = sensor.blueCyan_F3();

    channels[4].value  = sensor.green_F4();
    channels[5].value  = sensor.greenYellow_F5();
    channels[6].value  = sensor.yellowGreen_FY();

    channels[7].value  = sensor.orange_FXL();

    channels[8].value  = sensor.red_F6();
    channels[9].value  = sensor.deepRed_F7();
    channels[10].value = sensor.redNearInfrared_F8();
    channels[11].value = sensor.nearInfrared_NIR();
}

void readSpectralData() {
    if (!sensor.read()) {
        return;
    }
    sensorConnected = true;
    updateChannelValues();
}

// ============================================================
// Professional HTML UI
// ============================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>AS7343 Spectral Analyzer</title>

<style>
* {
    box-sizing: border-box;
}

body {
    margin: 0;
    background:
        linear-gradient(135deg, #f7f9fc 0%, #eef2f7 100%);
    color: #172033;
    font-family:
        Inter,
        -apple-system,
        BlinkMacSystemFont,
        "Segoe UI",
        Arial,
        sans-serif;
}

.container {
    max-width: 1250px;
    margin: auto;
    padding: 28px 20px 55px;
}

/* ------------------------------------------------------------
   Header
------------------------------------------------------------ */

.header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 20px;
    margin-bottom: 22px;
}

.title {
    font-size: 29px;
    font-weight: 750;
    letter-spacing: -0.6px;
}

.subtitle {
    margin-top: 5px;
    color: #667085;
    font-size: 14px;
}

.status {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 13px;
    border-radius: 22px;
    background: #ecfdf3;
    color: #027a48;
    font-size: 13px;
    font-weight: 650;
    white-space: nowrap;
}

.status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #12b76a;
}

.status.offline {
    background: #fef3f2;
    color: #b42318;
}

.status.offline .status-dot {
    background: #f04438;
}

/* ------------------------------------------------------------
   Cards
------------------------------------------------------------ */

.card,
.stat {
    background: #ffffff;
    border: 1px solid #e7eaf0;
    box-shadow: 0 4px 18px rgba(16, 24, 40, 0.055);
}

.card {
    border-radius: 16px;
    margin-bottom: 20px;
}

.card-header {
    padding: 18px 22px;
    border-bottom: 1px solid #eef0f4;
    display: flex;
    align-items: center;
    justify-content: space-between;
}

.card-title {
    font-size: 17px;
    font-weight: 700;
}

.live {
    color: #667085;
    font-size: 12px;
}

.chart-controls {
    display: flex;
    align-items: center;
    gap: 14px;
}

.led-toggle {
    border: 0;
    border-radius: 8px;
    padding: 9px 13px;
    background: #15803d;
    color: #ffffff;
    font: inherit;
    font-size: 13px;
    font-weight: 650;
    cursor: pointer;
}

.led-toggle.off {
    background: #475467;
}

.led-toggle:disabled {
    cursor: wait;
    opacity: 0.65;
}

/* ------------------------------------------------------------
   Statistics
------------------------------------------------------------ */

.stats {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 15px;
    margin-bottom: 20px;
}

.stat {
    border-radius: 14px;
    padding: 17px 20px;
}

.stat-label {
    color: #667085;
    font-size: 11px;
    font-weight: 650;
    margin-bottom: 7px;
    text-transform: uppercase;
    letter-spacing: .55px;
}

.stat-value {
    font-size: 25px;
    font-weight: 750;
}

.stat-unit {
    color: #667085;
    font-size: 13px;
    margin-left: 3px;
}

/* ------------------------------------------------------------
   Chart
------------------------------------------------------------ */

.chart-wrap {
    padding: 8px 14px 22px;
    overflow-x: auto;
}

svg {
    width: 100%;
    min-width: 850px;
    height: 500px;
    display: block;
}

.grid {
    stroke: #e5e7eb;
    stroke-width: 1;
}

.axis {
    stroke: #98a2b3;
    stroke-width: 1.2;
}

.spectral-line {
    fill: none;
    stroke: url(#spectralLineGradient);
    stroke-width: 1.8;
    stroke-linejoin: round;
    stroke-linecap: round;
}

.point {
    stroke: white;
    stroke-width: 1.2;
}

.point-label {
    font-size: 11px;
    font-weight: 700;
    fill: #111827;
}

.point,
.spectral-line {
    vector-effect: non-scaling-stroke;
}

.channel-label {
    font-size: 10px;
    font-weight: 650;
    fill: #344054;
}

.axis-label {
    font-size: 12px;
    fill: #667085;
}

.region-label {
    font-size: 12px;
    font-weight: 650;
    fill: #667085;
}

/* ------------------------------------------------------------
   Table
------------------------------------------------------------ */

.table-wrap {
    overflow-x: auto;
}

table {
    width: 100%;
    border-collapse: collapse;
}

th {
    background: #f8fafc;
    padding: 12px 18px;
    text-align: left;
    color: #475467;
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: .45px;
}

td {
    padding: 13px 18px;
    border-top: 1px solid #eef0f4;
    font-size: 14px;
}

.channel-name {
    font-weight: 700;
}

.value {
    font-family: "SFMono-Regular", Consolas, monospace;
    font-weight: 650;
}

/* ------------------------------------------------------------
   Responsive
------------------------------------------------------------ */

@media(max-width: 700px) {
    .container {
        padding: 20px 12px 40px;
    }

    .header {
        flex-direction: column;
    }

    .stats {
        grid-template-columns: 1fr;
    }

    .title {
        font-size: 24px;
    }
}
</style>
</head>

<body>

<div class="container">

    <div class="header">
        <div>
            <div class="title">AS7343 Spectral Analyzer</div>
            <div class="subtitle">
                12-channel real-time spectral measurement
            </div>
        </div>

        <div id="connectionStatus" class="status">
            <span class="status-dot"></span>
            <span id="statusText">Connecting...</span>
        </div>
    </div>

    <!-- Statistics -->
    <div class="stats">

        <div class="stat">
            <div class="stat-label">Peak Intensity</div>
            <span id="peakValue" class="stat-value">--</span>
        </div>

        <div class="stat">
            <div class="stat-label">Peak Wavelength</div>
            <span id="peakWavelength" class="stat-value">--</span>
            <span class="stat-unit">nm</span>
        </div>

        <div class="stat">
            <div class="stat-label">Peak Channel</div>
            <span id="peakChannel" class="stat-value">--</span>
        </div>

    </div>

    <!-- Spectral chart -->
    <div class="card">

        <div class="card-header">
            <div class="card-title">Spectral Response</div>
            <div class="chart-controls">
                <div class="live">LIVE • 200 ms</div>
                <button id="ledToggle"
                        class="led-toggle"
                        type="button"
                        aria-pressed="true"
                        onclick="toggleLed()">LED: ON</button>
            </div>
        </div>

        <div class="chart-wrap">

            <svg viewBox="0 0 1100 500"
                 preserveAspectRatio="xMidYMid meet">

                <defs>

                    <!-- Visible spectrum -->
                    <linearGradient id="visibleSpectrum"
                                    x1="0%" y1="0%"
                                    x2="100%" y2="0%">
                        <stop offset="0%" stop-color="#7c3aed"/>
                        <stop offset="6.78%" stop-color="#5b21b6"/>
                        <stop offset="15.25%" stop-color="#2563eb"/>
                        <stop offset="23.73%" stop-color="#0891b2"/>
                        <stop offset="37.29%" stop-color="#16a34a"/>
                        <stop offset="49.15%" stop-color="#84cc16"/>
                        <stop offset="50.85%" stop-color="#eab308"/>
                        <stop offset="66.10%" stop-color="#f97316"/>
                        <stop offset="79.66%" stop-color="#ef4444"/>
                        <stop offset="96.61%" stop-color="#b91c1c"/>
                        <stop offset="100%" stop-color="#991b1b"/>
                    </linearGradient>

                    <!-- NIR -->
                    <linearGradient id="nirSpectrum"
                                    x1="0%" y1="0%"
                                    x2="100%" y2="0%">
                        <stop offset="0%" stop-color="#b91c1c"/>
                        <stop offset="100%" stop-color="#5b21b6"/>
                    </linearGradient>

                    <linearGradient id="spectralLineGradient"
                                    gradientUnits="userSpaceOnUse"
                                    x1="85" y1="0"
                                    x2="1015" y2="0">
                        <stop offset="0%" stop-color="#6d28d9"/>
                        <stop offset="4.44%" stop-color="#7c3aed"/>
                        <stop offset="10%" stop-color="#2563eb"/>
                        <stop offset="15.56%" stop-color="#0891b2"/>
                        <stop offset="24.44%" stop-color="#16a34a"/>
                        <stop offset="32.22%" stop-color="#84cc16"/>
                        <stop offset="33.33%" stop-color="#eab308"/>
                        <stop offset="43.33%" stop-color="#f97316"/>
                        <stop offset="52.22%" stop-color="#ef4444"/>
                        <stop offset="63.33%" stop-color="#b91c1c"/>
                        <stop offset="75.56%" stop-color="#7e22ce"/>
                        <stop offset="100%" stop-color="#4c1d95"/>
                    </linearGradient>

                </defs>

                <!-- Visible region: 405–700 nm -->
                <rect x="85" y="55"
                      width="609.67"
                      height="350"
                      fill="url(#visibleSpectrum)"
                      opacity="0.30"/>

                <!-- Near-infrared region: 700–855 nm -->
                <rect x="694.67" y="55"
                      width="320.33"
                      height="350"
                      fill="url(#nirSpectrum)"
                      opacity="0.24"/>

                <text x="389.84" y="39"
                      text-anchor="middle"
                      class="region-label">
                    Visible Spectrum
                </text>

                <text x="855" y="39"
                      text-anchor="middle"
                      class="region-label">
                    Near Infrared
                </text>

                <g id="grid"></g>

                <polyline id="spectralLine"
                          class="spectral-line"
                          points=""></polyline>

                <g id="points"></g>

                <!-- Axes -->
                <line x1="85" y1="405"
                      x2="1015" y2="405"
                      class="axis"/>

                <line x1="85" y1="55"
                      x2="85" y2="405"
                      class="axis"/>

                <text x="550" y="475"
                      text-anchor="middle"
                      class="axis-label">
                    Wavelength (nm)
                </text>

                <text x="25" y="230"
                      transform="rotate(-90 25 230)"
                      text-anchor="middle"
                      class="axis-label">
                    Raw Intensity
                </text>

            </svg>

        </div>
    </div>

    <!-- Table -->
    <div class="card">

        <div class="card-header">
            <div class="card-title">Channel Measurements</div>
        </div>

        <div class="table-wrap">

            <table>
                <thead>
                    <tr>
                        <th>Channel</th>
                        <th>Wavelength</th>
                        <th>Region</th>
                        <th>Raw Value</th>
                    </tr>
                </thead>

                <tbody id="channelTable"></tbody>
            </table>

        </div>
    </div>

</div>

<script>

// ============================================================
// Channel metadata
// ============================================================

const channels = [
    {name:"F1",  wavelength:405, region:"Violet", color:"#6d28d9"},
    {name:"F2",  wavelength:425, region:"Violet-Blue", color:"#7c3aed"},
    {name:"FZ",  wavelength:450, region:"Blue", color:"#2563eb"},
    {name:"F3",  wavelength:475, region:"Blue-Cyan", color:"#0891b2"},
    {name:"F4",  wavelength:515, region:"Green", color:"#16a34a"},
    {name:"F5",  wavelength:550, region:"Green-Yellow", color:"#84cc16"},
    {name:"FY",  wavelength:555, region:"Yellow-Green", color:"#eab308"},
    {name:"FXL", wavelength:600, region:"Orange", color:"#f97316"},
    {name:"F6",  wavelength:640, region:"Red", color:"#ef4444"},
    {name:"F7",  wavelength:690, region:"Deep Red", color:"#b91c1c"},
    {name:"F8",  wavelength:745, region:"Red Near Infrared", color:"#7e22ce"},
    {name:"NIR", wavelength:855, region:"Near Infrared", color:"#4c1d95"}
];
let ledOn = true;

const graph = {
    left: 85,
    right: 1015,
    top: 55,
    bottom: 405,
    minWavelength: 405,
    maxWavelength: 855
};

// ============================================================
// Wavelength -> X
// ============================================================

function wavelengthX(wavelength) {
    return graph.left +
        ((wavelength - graph.minWavelength) /
        (graph.maxWavelength - graph.minWavelength)) *
        (graph.right - graph.left);
}

// ============================================================
// Nice Y-axis maximum
// ============================================================

function niceMaximum(value) {

    if (value <= 0) return 100;

    const magnitude =
        Math.pow(10, Math.floor(Math.log10(value)));

    const normalized = value / magnitude;

    let nice;

    if (normalized <= 1) nice = 1;
    else if (normalized <= 2) nice = 2;
    else if (normalized <= 5) nice = 5;
    else nice = 10;

    return nice * magnitude;
}

// ============================================================
// Draw chart
// ============================================================

function drawChart(values) {

    const actualMaximum = Math.max(...values);

    // Add only a small amount of headroom.
    const yMaximum = niceMaximum(actualMaximum * 1.10);

    const grid = document.getElementById("grid");
    grid.innerHTML = "";

    const ySteps = 5;

    // Horizontal grid
    for (let i = 0; i <= ySteps; i++) {

        const fraction = i / ySteps;

        const y =
            graph.bottom -
            fraction * (graph.bottom - graph.top);

        const value =
            Math.round(yMaximum * fraction);

        grid.innerHTML += `
            <line
                x1="${graph.left}"
                y1="${y}"
                x2="${graph.right}"
                y2="${y}"
                class="grid">
            </line>

            <text
                x="${graph.left - 12}"
                y="${y + 4}"
                text-anchor="end"
                class="axis-label">
                ${value}
            </text>
        `;
    }

    // X-axis grid
    const wavelengthTicks =
        [405,450,500,550,600,650,700,750,800,855];

    wavelengthTicks.forEach(wavelength => {

        const x = wavelengthX(wavelength);

        grid.innerHTML += `
            <line
                x1="${x}"
                y1="${graph.top}"
                x2="${x}"
                y2="${graph.bottom}"
                class="grid">
            </line>

            <text
                x="${x}"
                y="${graph.bottom + 22}"
                text-anchor="middle"
                class="axis-label">
                ${wavelength}
            </text>
        `;
    });

    // Spectral line
    let polyline = "";

    channels.forEach((channel, index) => {

        const x = wavelengthX(channel.wavelength);

        const y =
            graph.bottom -
            (values[index] / yMaximum) *
            (graph.bottom - graph.top);

        polyline += `${x},${y} `;
    });

    document.getElementById("spectralLine")
        .setAttribute("points", polyline);

    // Points and labels
    const pointGroup =
        document.getElementById("points");

    pointGroup.innerHTML = "";

    channels.forEach((channel, index) => {

        const x = wavelengthX(channel.wavelength);

        const y =
            graph.bottom -
            (values[index] / yMaximum) *
            (graph.bottom - graph.top);

        pointGroup.innerHTML += `
            <circle
                cx="${x}"
                cy="${y}"
                r="3.5"
                class="point"
                fill="${channel.color}">
            </circle>

            <text
                x="${x}"
                y="${y - 9}"
                text-anchor="middle"
                class="point-label">
                ${values[index]}
            </text>

            <text
                x="${x}"
                y="${graph.top - 8}"
                text-anchor="middle"
                class="channel-label">
                ${channel.name}
            </text>
        `;
    });
}

// ============================================================
// Update table
// ============================================================

function updateTable(values) {

    const table =
        document.getElementById("channelTable");

    let html = "";

    channels.forEach((channel, index) => {

        html += `
            <tr>
                <td class="channel-name">
                    ${channel.name}
                </td>

                <td>
                    ${channel.wavelength} nm
                </td>

                <td>
                    ${channel.region}
                </td>

                <td class="value">
                    ${values[index]}
                </td>
            </tr>
        `;
    });

    table.innerHTML = html;
}

// ============================================================
// Statistics
// ============================================================

function updateStatistics(values) {

    let maxIndex = 0;

    for (let i = 1; i < values.length; i++) {
        if (values[i] > values[maxIndex]) {
            maxIndex = i;
        }
    }

    document.getElementById("peakValue").textContent =
        values[maxIndex];

    document.getElementById("peakWavelength").textContent =
        channels[maxIndex].wavelength;

    document.getElementById("peakChannel").textContent =
        channels[maxIndex].name;
}

// ============================================================
// Connection status
// ============================================================

function setConnection(connected, sensorOK) {

    const status =
        document.getElementById("connectionStatus");

    const text =
        document.getElementById("statusText");

    if (connected && sensorOK) {

        status.className = "status";
        text.textContent = "Sensor connected";

    } else {

        status.className = "status offline";

        text.textContent =
            sensorOK === false
            ? "Sensor disconnected"
            : "Connection lost";
    }
}

function renderLedButton(enabled) {
    const button = document.getElementById("ledToggle");
    button.textContent = enabled ? "LED: ON" : "LED: OFF";
    button.classList.toggle("off", !enabled);
    button.setAttribute("aria-pressed", enabled ? "true" : "false");
}

async function toggleLed() {
    const button = document.getElementById("ledToggle");
    const requestedState = ledOn ? "off" : "on";

    button.disabled = true;

    try {
        const response = await fetch("/api/led", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: "state=" + requestedState
        });

        if (!response.ok) {
            throw new Error("LED request failed: HTTP " + response.status);
        }

        const data = await response.json();
        if (typeof data.led !== "boolean") {
            throw new Error("Invalid LED response");
        }

        ledOn = data.led;
        renderLedButton(ledOn);
    } catch (error) {
        console.error(error);
    } finally {
        button.disabled = false;
    }
}

// ============================================================
// Fetch ESP32 data
// ============================================================

let requestRunning = false;

async function fetchSpectrum() {

    if (requestRunning) return;

    requestRunning = true;

    try {

        const response =
            await fetch("/api/spectrum?t=" + Date.now(), {
                cache: "no-store"
            });

        if (!response.ok) {
            throw new Error("HTTP " + response.status);
        }

        const data = await response.json();

        if (!Array.isArray(data.values) ||
            data.values.length !== 12) {
            throw new Error("Invalid spectrum");
        }

        const values =
            data.values.map(value => Number(value));

        drawChart(values);
        updateTable(values);
        updateStatistics(values);
        if (typeof data.led === "boolean") {
            ledOn = data.led;
            renderLedButton(ledOn);
        }

        setConnection(true, data.sensor);

    } catch (error) {

        console.log(error);
        setConnection(false, null);

    } finally {

        requestRunning = false;
    }
}

fetchSpectrum();

setInterval(fetchSpectrum, 500);

</script>

</body>
</html>
)rawliteral";

// ============================================================
// Web server handlers
// ============================================================

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleSpectrum() {

    char json[280];

    snprintf(
        json,
        sizeof(json),
        "{\"sensor\":%s,\"led\":%s,\"values\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}",
        sensorConnected ? "true" : "false",
        ledEnabled ? "true" : "false",
        channels[0].value,
        channels[1].value,
        channels[2].value,
        channels[3].value,
        channels[4].value,
        channels[5].value,
        channels[6].value,
        channels[7].value,
        channels[8].value,
        channels[9].value,
        channels[10].value,
        channels[11].value
    );

    server.sendHeader(
        "Cache-Control",
        "no-store, no-cache, must-revalidate"
    );

    server.send(
        200,
        "application/json",
        json
    );
}

void handleLed() {
    if (!server.hasArg("state")) {
        server.send(400, "application/json", "{\"error\":\"Missing state\"}");
        return;
    }

    if (!sensor.sensorPresent()) {
        sensorConnected = false;
        server.send(503, "application/json", "{\"error\":\"Sensor disconnected\"}");
        return;
    }

    const String requestedState = server.arg("state");
    if (requestedState == "on") {
        sensor.ledOn();
        ledEnabled = true;
    } else if (requestedState == "off") {
        sensor.ledOff();
        ledEnabled = false;
    } else {
        server.send(400, "application/json", "{\"error\":\"State must be on or off\"}");
        return;
    }

    server.send(
        200,
        "application/json",
        ledEnabled ? "{\"led\":true}" : "{\"led\":false}"
    );
}

// ============================================================
// Setup
// ============================================================

void setup() {

    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("AS7343 Spectral Analyzer");
    Serial.println("==============================");

    // --------------------------------------------------------
    // Sensor
    // --------------------------------------------------------

    if (!sensor.begin()) {

        Serial.println("AS7343_7Semi begin failed");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("AS7343 initialized");

    // These are the library settings used by the spectral
    // channel getters.
    sensor.powerOn();
    sensor.configureAutoSMUX();
    sensor.setIntegration();
    sensor.setGain(AS7343_7Semi::Gain::X32);
    sensor.ledOn();
    ledEnabled = true;

    // First measurement
    readSpectralData();

    // --------------------------------------------------------
    // Wi-Fi Access Point
    // --------------------------------------------------------

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        AP_SSID,
        AP_PASSWORD
    );

    Serial.println();
    Serial.println("Wi-Fi Access Point started");

    Serial.print("SSID: ");
    Serial.println(AP_SSID);

    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);

    Serial.print("Open browser: http://");
    Serial.println(WiFi.softAPIP());

    // --------------------------------------------------------
    // HTTP routes
    // --------------------------------------------------------

    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );

    server.on(
        "/api/spectrum",
        HTTP_GET,
        handleSpectrum
    );

    server.on(
        "/api/led",
        HTTP_POST,
        handleLed
    );

    server.begin();

    Serial.println("Web server started");
}

// ============================================================
// Loop
// ============================================================

void loop() {

    server.handleClient();

    const unsigned long now = millis();

    // --------------------------------------------------------
    // Sensor presence check
    // --------------------------------------------------------

    if (now - lastPresenceCheck >= PRESENCE_INTERVAL) {

        lastPresenceCheck = now;

        sensorConnected = sensor.sensorPresent();
    }

    // --------------------------------------------------------
    // Sensor acquisition
    // --------------------------------------------------------

    if (sensorConnected &&
        now - lastSensorRead >= SENSOR_INTERVAL) {

        lastSensorRead = now;

        readSpectralData();
    }
}
