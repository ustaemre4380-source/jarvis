#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

// WiFi bilgileri
const char* ssid     = "H3XTOR_LAB_2.4GHz";
const char* password = "17455558";

// Statik IP ayarları
IPAddress local_IP(192,168,1,106);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(8,8,4,4);

// Röle pinleri
#define RELAY1  32
#define RELAY2  33
#define RELAY3  26
#define RELAY4  27
#define PC_POWER 25   // bilgisayar açma için röle

// 🔔 Buzzer pini
#define BUZZER 23

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3 * 3600;  // Türkiye GMT+3
const int   daylightOffset_sec = 0;

// Web sunucu
WebServer server(80);

// LCD kontrol için değişkenler
unsigned long lastLcdUpdate = 0;
bool showIpAfterDelay = false;

// Fonksiyon: saat ve tarih al
String getDateTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "Saat alinmadi";
  }
  char buffer[50];
  sprintf(buffer, "Saat:%02d:%02d<br>Tarih:%02d/%02d/%04d",
          timeinfo.tm_hour, timeinfo.tm_min,
          timeinfo.tm_mday, timeinfo.tm_mon+1, 1900+timeinfo.tm_year);
  return String(buffer);
}

// HTML sayfa üretici
String generatePage() {
  String dt = getDateTime();
  String html = R"rawhtml(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>J.A.R.V.I.S. — Kontrol Paneli</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@300;400;600&display=swap');

  :root {
    --cyan: #00d4ff;
    --cyan-dim: #007a99;
    --cyan-glow: rgba(0,212,255,0.15);
    --red: #ff3c3c;
    --red-glow: rgba(255,60,60,0.2);
    --bg: #020c12;
    --panel: rgba(0,20,35,0.85);
    --border: rgba(0,212,255,0.25);
    --text: #c8eaf5;
    --dim: #4a7a8a;
  }

  * { margin:0; padding:0; box-sizing:border-box; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Rajdhani', sans-serif;
    min-height: 100vh;
    overflow-x: hidden;
    position: relative;
  }

  /* Animated grid background */
  body::before {
    content: '';
    position: fixed; inset: 0;
    background-image:
      linear-gradient(rgba(0,212,255,0.04) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0,212,255,0.04) 1px, transparent 1px);
    background-size: 40px 40px;
    animation: gridScroll 20s linear infinite;
    pointer-events: none;
    z-index: 0;
  }

  /* Radial glow center */
  body::after {
    content: '';
    position: fixed;
    top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    width: 800px; height: 800px;
    background: radial-gradient(circle, rgba(0,212,255,0.06) 0%, transparent 70%);
    pointer-events: none;
    z-index: 0;
  }

  @keyframes gridScroll {
    0% { background-position: 0 0; }
    100% { background-position: 40px 40px; }
  }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.5; }
  }

  @keyframes scanline {
    0% { transform: translateY(-100%); }
    100% { transform: translateY(100vh); }
  }

  @keyframes flicker {
    0%,19%,21%,23%,25%,54%,56%,100% { opacity: 1; }
    20%,22%,24%,55% { opacity: 0.6; }
  }

  .scanline {
    position: fixed; top: 0; left: 0; right: 0;
    height: 3px;
    background: linear-gradient(transparent, rgba(0,212,255,0.3), transparent);
    animation: scanline 6s linear infinite;
    pointer-events: none;
    z-index: 10;
  }

  .container {
    position: relative; z-index: 1;
    max-width: 680px;
    margin: 0 auto;
    padding: 24px 16px 40px;
  }

  /* Header */
  .header {
    text-align: center;
    margin-bottom: 32px;
    padding-bottom: 20px;
    border-bottom: 1px solid var(--border);
    position: relative;
  }

  .header::after {
    content: '';
    position: absolute; bottom: -1px; left: 50%;
    transform: translateX(-50%);
    width: 60px; height: 2px;
    background: var(--cyan);
    box-shadow: 0 0 12px var(--cyan);
  }

  .logo {
    font-family: 'Orbitron', monospace;
    font-size: 2.2rem;
    font-weight: 900;
    letter-spacing: 0.3em;
    color: var(--cyan);
    text-shadow: 0 0 20px var(--cyan), 0 0 40px rgba(0,212,255,0.4);
    animation: flicker 8s infinite;
  }

  .logo span { color: #fff; }

  .subtitle {
    font-family: 'Rajdhani', sans-serif;
    font-size: 0.75rem;
    letter-spacing: 0.5em;
    color: var(--dim);
    margin-top: 4px;
    text-transform: uppercase;
  }

  /* Status bar */
  .status-bar {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 12px 20px;
    margin-bottom: 24px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    backdrop-filter: blur(8px);
  }

  .status-item {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 0.85rem;
    color: var(--dim);
    letter-spacing: 0.05em;
  }

  .status-item .val {
    color: var(--cyan);
    font-family: 'Orbitron', monospace;
    font-size: 0.8rem;
  }

  .dot {
    width: 6px; height: 6px;
    border-radius: 50%;
    background: #00ff88;
    box-shadow: 0 0 6px #00ff88;
    animation: pulse 2s ease-in-out infinite;
  }

  /* Section */
  .section {
    margin-bottom: 20px;
  }

  .section-label {
    font-family: 'Orbitron', monospace;
    font-size: 0.6rem;
    letter-spacing: 0.4em;
    color: var(--cyan-dim);
    text-transform: uppercase;
    margin-bottom: 10px;
    padding-left: 4px;
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .section-label::after {
    content: '';
    flex: 1;
    height: 1px;
    background: var(--border);
  }

  /* Relay grid */
  .relay-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
  }

  .relay-card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 16px;
    backdrop-filter: blur(8px);
    position: relative;
    overflow: hidden;
    transition: border-color 0.2s, box-shadow 0.2s;
  }

  .relay-card:hover {
    border-color: rgba(0,212,255,0.5);
    box-shadow: 0 0 16px rgba(0,212,255,0.08);
  }

  .relay-card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, var(--cyan), transparent);
    opacity: 0.4;
  }

  .relay-name {
    font-family: 'Orbitron', monospace;
    font-size: 0.65rem;
    letter-spacing: 0.2em;
    color: var(--dim);
    margin-bottom: 12px;
    text-transform: uppercase;
  }

  .relay-btns {
    display: flex;
    gap: 8px;
  }

  .btn {
    flex: 1;
    padding: 8px 0;
    border: none;
    border-radius: 4px;
    font-family: 'Orbitron', monospace;
    font-size: 0.6rem;
    font-weight: 700;
    letter-spacing: 0.15em;
    cursor: pointer;
    text-transform: uppercase;
    text-decoration: none;
    display: block;
    text-align: center;
    transition: all 0.15s ease;
    position: relative;
    overflow: hidden;
  }

  .btn-on {
    background: transparent;
    border: 1px solid var(--cyan);
    color: var(--cyan);
    text-shadow: 0 0 8px var(--cyan);
  }

  .btn-on:hover {
    background: var(--cyan-glow);
    box-shadow: 0 0 16px rgba(0,212,255,0.3), inset 0 0 8px rgba(0,212,255,0.1);
  }

  .btn-off {
    background: transparent;
    border: 1px solid var(--dim);
    color: var(--dim);
  }

  .btn-off:hover {
    border-color: var(--red);
    color: var(--red);
    text-shadow: 0 0 8px var(--red);
    box-shadow: 0 0 16px var(--red-glow);
  }

  /* PC Button */
  .pc-card {
    background: var(--panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 20px;
    backdrop-filter: blur(8px);
    display: flex;
    align-items: center;
    justify-content: space-between;
    position: relative;
    overflow: hidden;
    transition: all 0.2s;
  }

  .pc-card:hover {
    border-color: rgba(0,212,255,0.5);
    box-shadow: 0 0 24px rgba(0,212,255,0.1);
  }

  .pc-card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, var(--cyan), transparent);
    opacity: 0.4;
  }

  .pc-info { display: flex; flex-direction: column; gap: 4px; }

  .pc-title {
    font-family: 'Orbitron', monospace;
    font-size: 0.75rem;
    letter-spacing: 0.2em;
    color: var(--text);
    text-transform: uppercase;
  }

  .pc-desc {
    font-size: 0.75rem;
    color: var(--dim);
    letter-spacing: 0.05em;
  }

  .btn-pc {
    padding: 10px 22px;
    background: transparent;
    border: 1px solid var(--cyan);
    border-radius: 4px;
    color: var(--cyan);
    font-family: 'Orbitron', monospace;
    font-size: 0.65rem;
    font-weight: 700;
    letter-spacing: 0.2em;
    cursor: pointer;
    text-decoration: none;
    text-transform: uppercase;
    transition: all 0.15s;
    text-shadow: 0 0 8px var(--cyan);
    white-space: nowrap;
  }

  .btn-pc:hover {
    background: var(--cyan-glow);
    box-shadow: 0 0 20px rgba(0,212,255,0.4), inset 0 0 10px rgba(0,212,255,0.1);
  }

  /* Corner decorations */
  .corner-box {
    position: relative;
  }
  .corner-box::before, .corner-box::after {
    content: '';
    position: absolute;
    width: 10px; height: 10px;
    border-color: var(--cyan);
    border-style: solid;
    opacity: 0.6;
  }
  .corner-box::before {
    top: -1px; left: -1px;
    border-width: 2px 0 0 2px;
  }
  .corner-box::after {
    bottom: -1px; right: -1px;
    border-width: 0 2px 2px 0;
  }

  /* Footer */
  .footer {
    text-align: center;
    margin-top: 32px;
    padding-top: 16px;
    border-top: 1px solid var(--border);
    font-size: 0.7rem;
    color: var(--dim);
    letter-spacing: 0.2em;
    text-transform: uppercase;
  }
</style>
</head>
<body>
<div class="scanline"></div>
<div class="container">

  <div class="header">
    <div class="logo">J<span>.</span>A<span>.</span>R<span>.</span>V<span>.</span>I<span>.</span>S</div>
    <div class="subtitle">Just A Rather Very Intelligent System &nbsp;·&nbsp; J4</div>
  </div>

  <div class="status-bar">
    <div class="status-item">
      <div class="dot"></div>
      <span>SİSTEM</span>
      <span class="val">ONLINE</span>
    </div>
    <div class="status-item">
      <span>SAAT</span>
      <span class="val" id="clk">--:--</span>
    </div>
    <div class="status-item">
      <span>TARİH</span>
      <span class="val" id="dt">--/--/----</span>
    </div>
  </div>

  <div class="section">
    <div class="section-label">Röle Kontrol Modülü</div>
    <div class="relay-grid">

      <div class="relay-card corner-box">
        <div class="relay-name">Röle &nbsp;01</div>
        <div class="relay-btns">
          <a class="btn btn-on" href="/relay1/on">AÇ</a>
          <a class="btn btn-off" href="/relay1/off">KAPAT</a>
        </div>
      </div>

      <div class="relay-card corner-box">
        <div class="relay-name">Röle &nbsp;02</div>
        <div class="relay-btns">
          <a class="btn btn-on" href="/relay2/on">AÇ</a>
          <a class="btn btn-off" href="/relay2/off">KAPAT</a>
        </div>
      </div>

      <div class="relay-card corner-box">
        <div class="relay-name">Röle &nbsp;03</div>
        <div class="relay-btns">
          <a class="btn btn-on" href="/relay3/on">AÇ</a>
          <a class="btn btn-off" href="/relay3/off">KAPAT</a>
        </div>
      </div>

      <div class="relay-card corner-box">
        <div class="relay-name">Röle &nbsp;04</div>
        <div class="relay-btns">
          <a class="btn btn-on" href="/relay4/on">AÇ</a>
          <a class="btn btn-off" href="/relay4/off">KAPAT</a>
        </div>
      </div>

    </div>
  </div>

  <div class="section">
    <div class="section-label">Sistem Kontrolü</div>
    <div class="pc-card corner-box">
      <div class="pc-info">
        <div class="pc-title">Bilgisayar Güç Ünitesi</div>
        <div class="pc-desc">PC_POWER &nbsp;·&nbsp; Pin 25 &nbsp;·&nbsp; Kısa Temas</div>
      </div>
      <a class="btn-pc" href="/pc/on">&#9654; BAŞLAT</a>
    </div>
  </div>

  <div class="footer">ESP32 &nbsp;·&nbsp; 192.168.1.106 &nbsp;·&nbsp; J.A.R.V.I.S. J4</div>

</div>

<script>
  // Saat/Tarih güncelle
  function updateClock() {
    var now = new Date();
    var h = String(now.getHours()).padStart(2,'0');
    var m = String(now.getMinutes()).padStart(2,'0');
    var d = String(now.getDate()).padStart(2,'0');
    var mo = String(now.getMonth()+1).padStart(2,'0');
    var y = now.getFullYear();
    var clk = document.getElementById('clk');
    var dt = document.getElementById('dt');
    if(clk) clk.textContent = h+':'+m;
    if(dt) dt.textContent = d+'/'+mo+'/'+y;
  }
  updateClock();
  setInterval(updateClock, 1000);
</script>
</body>
</html>)rawhtml";
  return html;
}

// 🔔 Buzzer fonksiyonu
void buzzerBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW);
}

// Röle kontrol fonksiyonları
void handleRoot() { server.send(200, "text/html", generatePage()); }

void relayControl(int pin, bool state, const char* name) {
  digitalWrite(pin, state ? LOW : HIGH); // Röle aktif LOW ise tersle
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(name);
  lcd.setCursor(0,1);
  lcd.print(state ? "ACIK" : "KAPALI");

  lastLcdUpdate = millis();   // LCD güncelleme zamanı kaydedildi
  showIpAfterDelay = true;    // 5 sn sonra IP yazdırılacak

  buzzerBeep();  // 🔔 işlem sonrası buzzer

  server.send(200, "text/html", generatePage());
}

void setup() {
  lcd.init();
  lcd.backlight();

  // Röle pinleri çıkış
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(PC_POWER, OUTPUT);

  // Buzzer çıkış
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Röleleri kapalı başlat
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);
  digitalWrite(PC_POWER, HIGH);

  // Açılış yazısı
  lcd.setCursor(0,0);
  lcd.print("J.A.R.V.I.S. J4");
  delay(3000);

  // Statik IP
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);

  lcd.clear();
  lcd.print("WiFi Baglaniyor");
  while(WiFi.status() != WL_CONNECTED) { delay(500); }

  lcd.clear();
  lcd.print("IP:");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(3000);

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Web yönlendirmeleri
  server.on("/", handleRoot);
  server.on("/relay1/on", [](){ relayControl(RELAY1, true, "Role 1"); });
  server.on("/relay1/off", [](){ relayControl(RELAY1, false, "Role 1"); });

  server.on("/relay2/on", [](){ relayControl(RELAY2, true, "Role 2"); });
  server.on("/relay2/off", [](){ relayControl(RELAY2, false, "Role 2"); });

  server.on("/relay3/on", [](){ relayControl(RELAY3, true, "Role 3"); });
  server.on("/relay3/off", [](){ relayControl(RELAY3, false, "Role 3"); });

  server.on("/relay4/on", [](){ relayControl(RELAY4, true, "Role 4"); });
  server.on("/relay4/off", [](){ relayControl(RELAY4, false, "Role 4"); });

  server.on("/pc/on", [](){
    relayControl(PC_POWER, true, "PC Power");
    delay(1000);
    digitalWrite(PC_POWER, HIGH); // kısa süre basılıp bırakılır
    buzzerBeep();  // 🔔 PC açma sonrası buzzer
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // 5 sn sonra IP'yi tekrar LCD'ye yazdır
  if (showIpAfterDelay && millis() - lastLcdUpdate >= 5000) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IP:");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP());
    showIpAfterDelay = false; // Tekrar yazmayı kapat
  }
}
