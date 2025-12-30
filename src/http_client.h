#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class HTTP_CLIENT
{
public:
    HTTP_CLIENT(String ssid, String password, String serverUrl);
    ~HTTP_CLIENT();
    bool begin();
    bool reconnecWifi();
    uint8_t scanWifi(String scannedList[]);
    void setCredentials(String ssid, String password);
    void setServerIP(String serverURL);
    String httpGetRequest(const String &endpoint);

private:
    String _ssid;
    String _password;
    String _serverUrl;
    HTTPClient _httpClient;
};

#endif // HTTP_CLIENT_H