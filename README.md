# RFID Access Control with Live Google Sheets Logging

A card-based access system built on the ESP32-S3. Tap a registered card and you get a welcome message, a beep and a green light. Tap an unknown card and you're denied. Every tap, allowed or denied, is logged to a Google Sheet within seconds, with the time, name, card ID and status.

**Use cases:** attendance for gyms, coaching centres, offices and college clubs, or entry control for a room, lab or storage area.

## Demo

[Watch the 24-second demo](demo.mp4): a registered card gets access, an unknown card is denied, and both taps land in the sheet live.

## How it works

```
Card tap → RC522 reader → ESP32-S3 → WiFi → Google Apps Script → Google Sheet
                              ↓
                    OLED message + buzzer + LED
```

1. The RC522 reads the card's unique ID over SPI.
2. The ESP32-S3 checks the ID against its list of allowed cards.
3. Allowed cards show **Welcome** with the person's name, a short high beep and the LED. Unknown cards show **DENIED** with a long low buzz.
4. The ESP32 sends the name, card ID and status to a Google Apps Script web app, which adds a row to the sheet.

## Hardware

| Part | Notes |
|---|---|
| ESP32-S3 DevKitC-1 (N16R8) | Built-in WiFi |
| RC522 RFID reader | 13.56 MHz, with cards and key tags |
| 0.96" OLED (SSD1306, I2C) | Status messages |
| Buzzer | Different tones for allowed and denied |
| LED + 220 Ω resistor | Stands in for a door lock relay |
| Breadboard and jumper wires | |

## Wiring

| Component | Pin | ESP32-S3 GPIO |
|---|---|---|
| RC522 | 3.3V | 3V3 |
| RC522 | GND | GND |
| RC522 | SDA (chip select) | 10 |
| RC522 | MOSI | 11 |
| RC522 | SCK | 12 |
| RC522 | MISO | 13 |
| RC522 | RST | 14 |
| RC522 | IRQ | not connected |
| OLED | VCC | 3V3 |
| OLED | GND | GND |
| OLED | SDA | 8 |
| OLED | SCL | 9 |
| Buzzer | + | 4 |
| LED (through resistor) | + | 5 |

## Setup

### 1. Google Sheet and Apps Script
1. Create a Google Sheet with the headers `Timestamp`, `Name`, `Card ID` and `Status` in row 1.
2. Open **Extensions → Apps Script** and paste in the code from `apps_script.gs`.
3. Click **Deploy → New deployment → Web app**. Set *Execute as* to **Me** and *Who has access* to **Anyone**.
4. Copy the web app URL, which ends in `/exec`.

### 2. Arduino IDE
1. Install the **esp32** board package by Espressif Systems.
2. Install these libraries: **MFRC522** (GithubCommunity), **Adafruit SSD1306** and **Adafruit GFX**.
3. Select **ESP32S3 Dev Module** as the board.
4. Open `rfid_access_control.ino` and fill in your WiFi name, password and web app URL.
5. Upload the sketch.

### 3. Register cards
Tap a new card. It will be denied, and its ID will appear in the sheet. Add that ID to the `allowedCards[]` list in the code with a name, then upload again.

## Troubleshooting

- **"No WiFi" on the screen:** the ESP32 only connects to 2.4 GHz networks. If you're using a phone hotspot, set its band to 2.4 GHz and its security to WPA2-Personal, not WPA3.
- **Nothing reaches the sheet:** make sure the Apps Script is deployed with access set to *Anyone*. After editing the script, deploy a new version under **Manage deployments**.
- **The reader doesn't respond:** check the SPI wires, especially that MOSI and MISO aren't swapped.

## Roadmap

- [ ] Relay and solenoid lock, so allowed cards actually open a door
- [ ] Store the member list in a second sheet tab, so members can be added without re-uploading code
- [ ] Offline mode: cache the member list on the ESP32 so the door works without WiFi, and upload taps once it reconnects
- [ ] Fingerprint sensor as a second way to get in
- [ ] Enclosure and a custom PCB

## Built by

Charandeep, ECE student at BITS Pilani Hyderabad and RMIT. Open to freelance work on ESP32 and IoT prototypes.
