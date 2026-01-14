#include <Preferences.h>
#include "http_client.h"
#include "lcd_hmi.h"
#include "rfid_rc522.h"

#define LCD_RX_PIN 16
#define LCD_TX_PIN 17
#define LCD_BAUD_RATE 9600

#define RFID_SS_PIN 5
#define RFID_RST_PIN 22

#define WARN_DEVICEs_PIN 4
#define DOOR_PIN 12
#define CHECK_PIN 27

// Timing configuration (tune as needed)
const uint32_t RFID_SCAN_INTERVAL_MS = 200;
const uint32_t LCD_POLL_INTERVAL_MS = 50;
const uint32_t SEND_DEBAUNCE_MS = 1500;
const uint32_t RESET_DEVICES_TIMEOUT_MS = 5000;
const uint32_t RESET_WARN_TIMEOUT_MS = 369;

// Variables
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
String SERVER = "";
uint8_t RFID_KEY[MFRC522::MF_KEY_SIZE];
const char NAMESPACE[7] = "config";

uint32_t lastRfScan = 0;
uint32_t lastLcdPoll = 0;
uint32_t lastRfidDetectedTime = 0;
uint32_t lastSentTime = 0;
String lastSentTag = "";
bool lcdReseted = true;

const size_t scannedSSID_size = 5;
String scannedSSIDs[scannedSSID_size];
uint8_t selectedBtnIndex = 0;

uint32_t lastWarnIsON = 0;
uint32_t lastCheckIsON = 0;

bool warnState = false;
bool doorState = false;
bool checkState = false;

// objects
Preferences prefs;
LCD_HMI lcd(LCD_RX_PIN, LCD_TX_PIN, LCD_BAUD_RATE);
RFID_RC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
HTTP_CLIENT httpClient(WIFI_SSID, WIFI_PASSWORD, SERVER);

// declare functions
String bytesToHexString(const uint8_t *data, size_t len);
String hexStringToId(const String &hexStr);
void pollLcdForUnsolicitedResponses();
bool connectWifi();

//! SETUP func
void setup()
{
    Serial.begin(115200);

    // load configs from NVS
    prefs.begin(NAMESPACE, true);
    WIFI_SSID = prefs.getString("WIFI_SSID", "");
    WIFI_PASSWORD = prefs.getString("WIFI_PASS", "");
    SERVER = prefs.getString("SERVER_IP", "");
    prefs.getBytes("RFID_KEYA", RFID_KEY, sizeof(RFID_KEY));
    prefs.end();

    // Serial.println("[ESP] Loaded configs ============");
    // Serial.println("[ESP] WIFI SSID: " + WIFI_SSID);
    // Serial.println("[ESP] WIFI PASSWORD: " + WIFI_PASSWORD);
    // Serial.println("[ESP] SERVER URL: " + SERVER);

    // set ssid, password & server url for an object client
    httpClient.setCredentials(WIFI_SSID, WIFI_PASSWORD);
    httpClient.setServerIP(SERVER);

    // set RFID Key A for reader
    rfid.setRFIDKey(RFID_KEY);

    // reset a scanned SSID list
    for (int i = 0; i < scannedSSID_size; i++)
    {
        scannedSSIDs[i] = "";
    }

    // initialize peripherals
    lcd.begin();
    rfid.begin();
    delay(500);
    
    connectWifi();
    
    // Setup for device pin
    pinMode(WARN_DEVICEs_PIN, OUTPUT);
    digitalWrite(WARN_DEVICEs_PIN, LOW);

    pinMode(DOOR_PIN, OUTPUT);
    digitalWrite(DOOR_PIN, LOW);

    pinMode(CHECK_PIN, OUTPUT);
    digitalWrite(CHECK_PIN, LOW);

    Serial.println("[ESP] System Initialized");
}

//! LOOP func
void loop()
{
    uint32_t now = millis();

    // 1) Regularly poll LCD for unsolicited responses (keeps listening active)
    if (now - lastLcdPoll >= LCD_POLL_INTERVAL_MS)
    {
        lastLcdPoll = now;
        pollLcdForUnsolicitedResponses();
    }

    if (!lcdReseted && doorState && now - lastRfidDetectedTime >= RESET_DEVICES_TIMEOUT_MS)
    {
        lcd.sendTextToElement("t2.txt=\"\"");
        lcd.sendTextToElement("t3.txt=\"\"");
        Serial.println("[RFID] No tag for 10s -> LCD cleared");

        // turn of the Notification LED
        digitalWrite(DOOR_PIN, LOW);
        
        lcdReseted = true;
        doorState = false;
    }

    // reset the check device
    if (checkState && now - lastCheckIsON >= RESET_DEVICES_TIMEOUT_MS) {
        checkState = false;
        digitalWrite(CHECK_PIN, LOW);
    }

    if (warnState && now - lastWarnIsON >= RESET_WARN_TIMEOUT_MS)
    {
        warnState = false;
        digitalWrite(WARN_DEVICEs_PIN, LOW);
    }

    // 2) Periodic RFID scan
    if (now - lastRfScan >= RFID_SCAN_INTERVAL_MS)
    {
        lastRfScan = now;

        uint8_t blockData[16];
        uint8_t ret = rfid.readBlockData(4, blockData, sizeof(blockData));

        if (ret == 0)
        {
            lastRfidDetectedTime = now;

            // format block data as hex
            String endpoint = "/staff-id?id=";
            String hexString = bytesToHexString(blockData, sizeof(blockData));
            String asciiString = hexStringToId(hexString);
            endpoint += asciiString;

            // debug print
            Serial.print("[RFID] Read block 4 -> ");
            Serial.println(asciiString);

            // check debounce
            bool isSameAsLast = (hexString == lastSentTag);
            bool debounceExpired = (now - lastSentTime >= SEND_DEBAUNCE_MS);

            if (!isSameAsLast || debounceExpired)
            {
                // send to server
                String httpResp = httpClient.httpGetRequest(endpoint);
                Serial.println("[HTTP] Server resp: " + httpResp);

                // parse the HTTP Response
                JsonDocument doc;

                DeserializationError err = deserializeJson(doc, httpResp);

                if (err)
                {
                    Serial.print("Json parse is failed: ");
                    Serial.println(err.c_str());
                    return;
                }

                String name = doc["name"];
                bool check = doc["check"];

                // send to LCD
                lcd.sendTextToElement("t2.txt=\"" + name + "\"");
                Serial.print("[LCD] Sent: ");
                Serial.println(httpResp);
                lcd.sendTextToElement("t3.txt=\"OK\"");

                // open the door
                digitalWrite(DOOR_PIN, HIGH);

                if (check)
                {
                    lastCheckIsON = now;
                    checkState = true;
                    digitalWrite(CHECK_PIN, HIGH);
                }

                // String checkStatus = check ? "OK" : "NOT";
                // lcd.sendTextToElement("t3.txt=\"" + checkStatus + "\"");

                // update last sent info
                lastSentTag = hexString;
                lastSentTime = millis();
                lcdReseted = false;
                doorState = true;
            }
            else
            {
                // skip sending duplicate tag within debounce window
                Serial.println("[RFID] Duplicate tag - send suppressed (debounce)");
            }
        }
        else if (ret == 2)
        {
            lastWarnIsON = now;
            warnState = true;
            digitalWrite(WARN_DEVICEs_PIN, HIGH);
        }
        else
        {
            // failed to read; optional: reduce log spam by not printing every failure
            // comment/uncomment next line depending on desired verbosity
            // Serial.println("[RFID] Failed to read block data");
        }
    }

    // small idle delay to yield CPU (tunable)
    delay(5);
}

String bytesToHexString(const uint8_t *data, size_t len)
{
    String hexStr = "";
    const char hexChars[] = "0123456789ABCDEF";

    for (size_t i = 0; i < len; i++)
    {
        hexStr += hexChars[(data[i] >> 4) & 0x0F];
        hexStr += hexChars[data[i] & 0x0F];
    }
    return hexStr;
}

String hexStringToId(const String &hexStr)
{
    String result = "";
    result.reserve(hexStr.length() / 2);

    for (size_t i = 0; i < hexStr.length(); i += 2)
    {
        char c1 = hexStr[i];
        char c2 = hexStr[i + 1];

        uint8_t high =
            (c1 >= '0' && c1 <= '9') ? (c1 - '0') : (c1 >= 'A' && c1 <= 'F') ? (c1 - 'A' + 10)
                                                : (c1 >= 'a' && c1 <= 'f')   ? (c1 - 'a' + 10)
                                                                             : 0;

        uint8_t low =
            (c2 >= '0' && c2 <= '9') ? (c2 - '0') : (c2 >= 'A' && c2 <= 'F') ? (c2 - 'A' + 10)
                                                : (c2 >= 'a' && c2 <= 'f')   ? (c2 - 'a' + 10)
                                                                             : 0;

        uint8_t value = (high << 4) | low;

        if (value == 0x00)
            break;

        result += (char)value;
    }

    return result;
}

// read unsolicited responses from LCD when no request is sent
void pollLcdForUnsolicitedResponses()
{
    String response = lcd.readResponse();
    if (response.length() > 0)
    {
        Serial.print("[LCD unsolicited] ");
        Serial.println(response);
        if (response.startsWith("reConnect"))
        {
            if (httpClient.reconnecWifi())
            {
                lcd.sendTextToElement("page 0");
            }
            else
            {
                lcd.sendTextToElement("page connErr");
            }
        }
        else if (response.startsWith("scan"))
        {
            Serial.println("SCANNING WIFI");
            lcd.sendTextToElement("t0.txt=\"Wifi Scanning...\"");

            uint8_t n = httpClient.scanWifi(scannedSSIDs, scannedSSID_size);

            if (n > 0)
            {
                lcd.sendTextToElement("t0.txt=\"Scan Success\"");

                for (uint8_t i = 0; i < n; i++)
                {
                    lcd.sendTextToElement("b" + String(i + 1) + ".txt=\"" + String(scannedSSIDs[i]) + "\"");
                    lcd.sendTextToElement("vis b" + String(i + 1) + ",1");
                }
            }
            else
            {
                lcd.sendTextToElement("t0.txt=\"Scan Failed !\"");
            }
        }
        else if (response.startsWith("wifi_"))
        {
            int idx = response.substring(4).toInt();
            selectedBtnIndex = idx;
            WIFI_SSID = scannedSSIDs[idx];
            Serial.println("Wifi selected: " + WIFI_SSID);

            // show keyboard screen
            lcd.sendTextToElement("page keybdA");
        }
        else if (response.startsWith("pw:"))
        {
            WIFI_PASSWORD = response.substring(3);

            // set ssid & password for an object client
            httpClient.setCredentials(WIFI_SSID, WIFI_PASSWORD);

            // if connect success then save ssid & password to NVS
            if (connectWifi())
            {
                prefs.begin(NAMESPACE, false);
                prefs.putString("WIFI_SSID", WIFI_SSID);
                prefs.putString("WIFI_PASS", WIFI_PASSWORD);
                prefs.end();
            }
        }
    }
}

bool connectWifi()
{
    // show loading screen on LCD
    lcd.sendTextToElement("page loading");

    if (httpClient.begin())
    {
        lcd.sendTextToElement("page 0");
        return true;
    }
    else
    {
        lcd.sendTextToElement("page connErr");
        return false;
    }
}