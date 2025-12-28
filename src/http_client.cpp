#include "http_client.h"

HTTP_CLIENT::HTTP_CLIENT(const char *ssid, const char *password, const char *serverUrl)
    : _ssid(ssid), _password(password), _serverUrl(serverUrl) {}

HTTP_CLIENT::~HTTP_CLIENT()
{
    _httpClient.end();
}

bool HTTP_CLIENT::begin()
{
    WiFi.begin(_ssid, _password);
    Serial.print("Connecting to WiFi");
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
        Serial.println("Failed to connect to WiFi");
        return false;
    }

    Serial.println("Connected to WiFi with IP: " + WiFi.localIP().toString());
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
            Serial.printf("GET request failed, HTTP code: %d\n", httpCode);
        }
    }
    else
    {
        Serial.printf("GET request failed, error: %s\n", _httpClient.errorToString(httpCode).c_str());
    }

    _httpClient.end();
    return String("");
}