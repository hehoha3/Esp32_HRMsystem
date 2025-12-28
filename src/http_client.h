#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class HTTP_CLIENT {
public:
    HTTP_CLIENT(const char* ssid, const char* password, const char* serverUrl);
    ~HTTP_CLIENT();
    bool begin();
    String httpGetRequest(const String &endpoint);

private:
    const char* _ssid;
    const char* _password;
    const char* _serverUrl;
    HTTPClient _httpClient;
};

#endif // HTTP_CLIENT_H