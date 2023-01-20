#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

AsyncWebServer server(80);

void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setTextSize(3);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi! I am ESP32.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");

    M5.Display.printf("SSID: %s\n", ssid);
    M5.Display.printf("IP Address: %s\n", WiFi.localIP().toString());
    M5.Display.printf("\n\n");
    M5.Display.printf("Web server running on port 80\n");
    M5.Display.printf("ElegantOTA available at\n"
                      "http://%s/update\n",
                      WiFi.localIP().toString());
}

void loop()
{
    yield();
}
