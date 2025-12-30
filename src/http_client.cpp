#include "http_client.h"

// constructor
HTTP_CLIENT::HTTP_CLIENT(String ssid, String password, String serverUrl)
    : _ssid(ssid), _password(password), _serverUrl(serverUrl) {}

    // destructor
HTTP_CLIENT::~HTTP_CLIENT()
{
    _httpClient.end();
}

// start function
bool HTTP_CLIENT::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    Serial.print("[HTTP] Connecting to WiFi");
    unsigned long startAttemptTime = millis();

    // Wait for connection or timeout after 10 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[HTTP] Failed to connect to WiFi");
        return false;
    }

    Serial.println("[HTTP] Connected to WiFi with IP: " + WiFi.localIP().toString());
    return true;
}

String HTTP_CLIENT::httpGetRequest(const String &endpoint)
{
    String fullUrl = String(_serverUrl) + endpoint;
    _httpClient.begin(fullUrl);
    int httpCode = _httpClient.GET();

    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = _httpClient.getString();
            _httpClient.end();
            return payload;
        }
        else
        {
            Serial.printf("[HTTP] GET request failed, HTTP code: %d\n", httpCode);
        }
    }
    else
    {
        Serial.printf("[HTTP] GET request failed, error: %s\n", _httpClient.errorToString(httpCode).c_str());
    }

    _httpClient.end();
    return String("");
}

bool HTTP_CLIENT::reconnecWifi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true; // Already connected
    }

    Serial.println("[HTTP] Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(_ssid, _password);
    unsigned long startAttemptTime = millis();

    // Wait for connection or timeout after 10 seconds
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[HTTP] Failed to reconnect to WiFi");
        return false;
    }

    Serial.println("[HTTP] Reconnected to WiFi with IP: " + WiFi.localIP().toString());
    return true;
}

// scan available Wifi networks around Esp
uint8_t HTTP_CLIENT::scanWifi(String scannedList[])
{
    // reset a scanned SSID list
    for (int i = 0; i < 6; i++)
    {
        scannedList[i] = "";
    }

    int n = WiFi.scanNetworks();
    Serial.println("[HTTP] Wifi scan completed.");

    for (int i = 0; i < min(n, 6); i++)
    {
        scannedList[i] = WiFi.SSID(i);
    }
    return (uint8_t)min(n, 6); // just take first 6 SSIDs
}

// set ssid & password
void HTTP_CLIENT::setCredentials(String ssid, String password)
{
    _ssid = ssid;
    _password = password;
}

// set backend server Url
void HTTP_CLIENT::setServerIP(String serverURL)
{
    _serverUrl = serverURL;
}