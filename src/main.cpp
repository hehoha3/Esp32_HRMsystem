#include "http_client.h"
#include "lcd_hmi.h"
#include "rfid_rc522.h"

#define LCD_RX_PIN 16
#define LCD_TX_PIN 17
#define LCD_BAUD_RATE 9600

#define RFID_SS_PIN 5
#define RFID_RST_PIN 22

// Timing configuration (tune as needed)
const uint32_t RFID_SCAN_INTERVAL_MS = 200;
const uint32_t LCD_POLL_INTERVAL_MS = 50;
const uint32_t LCD_RESPONSE_TIMEOUT_MS = 800;
const uint32_t SEND_DEBAUNCE_MS = 1500;
const uint32_t RFID_IDLE_TIMEOUT_MS = 5000;

const char *WIFI_SSID = "BA LEN BON";
const char *WIFI_PASSWORD = "0comatkhau";
const char *SERVER = "http://192.168.50.93:1880";

LCD_HMI lcd(LCD_RX_PIN, LCD_TX_PIN, LCD_BAUD_RATE);
RFID_RC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
HTTP_CLIENT httpClient(WIFI_SSID, WIFI_PASSWORD, SERVER);

uint32_t lastRfScan = 0;
uint32_t lastLcdPoll = 0;
uint32_t lastRfidDetectedTime = 0;
uint32_t lastSentTime = 0;
String lastSentTag = "";
bool lcdReseted = false;

String bytesToHexString(const uint8_t *data, size_t len);
String hexToId(const String &hexStr);
void pollLcdForUnsolicitedResponses();
String waitForLcdResponse(uint32_t timeoutMs);

void setup()
{
    Serial.begin(115200);

    // initialize peripherals
    lcd.begin();
    rfid.begin();
    delay(500);
    
    if (httpClient.begin())
    {
        lcd.sendTextToElement("page 0");
    }
    else
    {
        lcd.sendTextToElement("page connErr");
    }

    Serial.println("System Initialized");
}

void loop()
{
    uint32_t now = millis();

    // 1) Regularly poll LCD for unsolicited responses (keeps listening active)
    if (now - lastLcdPoll >= LCD_POLL_INTERVAL_MS)
    {
        lastLcdPoll = now;
        pollLcdForUnsolicitedResponses();
    }

    if (!lcdReseted && now - lastRfidDetectedTime >= RFID_IDLE_TIMEOUT_MS)
    {
        lcd.sendTextToElement("t2.txt=\"\"");
        lcd.sendTextToElement("t3.txt=\"\"");
        Serial.println("[RFID] No tag for 10s -> LCD cleared");

        lcdReseted = true;
    }

    // 2) Periodic RFID scan
    if (now - lastRfScan >= RFID_SCAN_INTERVAL_MS)
    {
        lastRfScan = now;

        uint8_t blockData[16];
        bool ok = rfid.readBlockData(4, blockData, sizeof(blockData));

        if (ok)
        {
            lastRfidDetectedTime = now;

            // format block data as hex
            String endpoint = "/staff-id?id=";
            String hexString = bytesToHexString(blockData, sizeof(blockData));
            String asciiString = hexToId(hexString);
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
                // send to LCD
                lcd.sendTextToElement("t2.txt=\"" + httpResp + "\"");
                Serial.print("[LCD] Sent: ");
                Serial.println(httpResp);
                lcd.sendTextToElement("t3.txt=\"OK\"");

                // wait for reply (short timeout)
                String lcdResp = waitForLcdResponse(LCD_RESPONSE_TIMEOUT_MS);
                if (lcdResp.length() > 0)
                {
                    Serial.print("[LCD reply after send] ");
                    Serial.println(lcdResp);
                }
                else
                {
                    Serial.println("[LCD reply after send] (no response, timeout)");
                }

                // update last sent info
                lastSentTag = hexString;
                lastSentTime = millis();
                lcdReseted = false;
            }
            else
            {
                // skip sending duplicate tag within debounce window
                Serial.println("[RFID] Duplicate tag - send suppressed (debounce)");
            }
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

String hexToId(const String &hexStr)
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

void pollLcdForUnsolicitedResponses()
{
    String response = lcd.readResponse();
    if (response.length() > 0)
    {
        Serial.print("[LCD unsolicited] ");
        Serial.println(response);
    }
}

String waitForLcdResponse(uint32_t timeoutMs)
{
    uint32_t startTime = millis();
    String response = "";

    while (millis() - startTime < timeoutMs)
    {
        response += lcd.readResponse();
        if (response.length() > 0)
        {
            break;
        }
        delay(10); // small delay to avoid busy-waiting
    }
    return response;
}