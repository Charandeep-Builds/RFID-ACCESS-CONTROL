// RFID Access Control - ESP32-S3 + RC522 + 0.96" OLED + Google Sheets
// Allowed cards get "Welcome", others get "Denied". Every tap is logged with its status.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Settings (fill these in before uploading) ----------
const char* WIFI_SSID  = "YOUR_WIFI_NAME";
const char* WIFI_PASS  = "YOUR_WIFI_PASSWORD";
const char* SCRIPT_URL = "YOUR_APPS_SCRIPT_WEB_APP_URL";   // ends in /exec

// ---------- Pins ----------
#define RC522_SS    10   // SDA on the RC522
#define RC522_MOSI  11
#define RC522_SCK   12
#define RC522_MISO  13
#define RC522_RST   14
#define OLED_SDA     8
#define OLED_SCL     9
#define BUZZER_PIN   4
#define LED_PIN      5

// ---------- Allowed cards ----------
// Only cards in this list get access. Anyone else is denied and logged.
// Tap an unknown card, copy its UID from the sheet, and add it here.
struct Card { const char* uid; const char* name; };
Card allowedCards[] = {
  {"21 AF FC 04", "Charandeep"},
};
const int NUM_ALLOWED = sizeof(allowedCards) / sizeof(allowedCards[0]);

MFRC522 rfid(RC522_SS, RC522_RST);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

String lastUid = "";
unsigned long lastScanTime = 0;
const unsigned long REPEAT_BLOCK_MS = 3000;   // ignore the same card for 3 seconds

// ---------- OLED helper ----------
void showMessage(const String& big, const String& small = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(big);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.println(small);
  display.display();
}

// ---------- Buzzer helpers ----------
void beepGranted() {
  tone(BUZZER_PIN, 2000, 150);
  delay(200);
}

void beepDenied() {
  tone(BUZZER_PIN, 400, 600);   // one long low buzz
  delay(650);
}

// ---------- Card helpers ----------
String readUid() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

// Returns the name if the card is allowed, or "Unknown" if not.
String lookupName(const String& uid) {
  for (int i = 0; i < NUM_ALLOWED; i++) {
    if (uid == allowedCards[i].uid) return allowedCards[i].name;
  }
  return "Unknown";
}

// Spaces and symbols can't go straight into a URL, so encode them.
String urlEncode(const String& s) {
  const char* hex = "0123456789ABCDEF";
  String out = "";
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

// ---------- WiFi ----------
void connectWiFi() {
  showMessage("WiFi...", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    showMessage("WiFi OK", WiFi.localIP().toString());
  } else {
    showMessage("No WiFi", "Check name/password");
  }
  delay(1500);
}

// ---------- Google Sheets ----------
bool sendToSheet(const String& name, const String& uid, const String& status) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(3000);
    if (WiFi.status() != WL_CONNECTED) return false;
  }

  WiFiClientSecure client;
  client.setInsecure();              // skip certificate check (fine for a prototype)

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // Google redirects once
  String url = String(SCRIPT_URL)
             + "?name="   + urlEncode(name)
             + "&uid="    + urlEncode(uid)
             + "&status=" + urlEncode(status);

  if (!http.begin(client, url)) return false;
  int code = http.GET();
  http.end();
  return code == 200;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  showMessage("Booting");

  SPI.begin(RC522_SCK, RC522_MISO, RC522_MOSI, RC522_SS);
  rfid.PCD_Init();

  connectWiFi();
  showMessage("Tap card");
}

// ---------- Main loop ----------
void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = readUid();
  rfid.PICC_HaltA();

  // Ignore the same card held on the reader
  if (uid == lastUid && millis() - lastScanTime < REPEAT_BLOCK_MS) return;
  lastUid = uid;
  lastScanTime = millis();

  String name = lookupName(uid);
  bool allowed = (name != "Unknown");

  if (allowed) {
    digitalWrite(LED_PIN, HIGH);          // later: this is where a relay unlocks the door
    beepGranted();
    showMessage("Welcome", name + "\nAccess granted");
  } else {
    beepDenied();
    showMessage("DENIED", "Not a member.\nPlease see manager.");
  }

  sendToSheet(name, uid, allowed ? "Allowed" : "Denied");

  delay(2000);
  digitalWrite(LED_PIN, LOW);
  showMessage("Tap card");
}
