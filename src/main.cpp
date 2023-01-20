#include <M5EPD.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <AsyncElegantOTA.h>

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

AsyncWebServer server(80);
M5EPD_Canvas canvas(&M5.EPD);

void setup_wifi() {
}

void setup(void) {
    M5.begin();
    M5.EPD.SetRotation(90);
    M5.EPD.Clear(true);
    M5.RTC.begin();
    Serial.println("createCanvas");
    canvas.createCanvas(540, 960);

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

    canvas.setTextSize(3);
    canvas.drawString(ssid, 0, 0);
    canvas.drawString(WiFi.localIP().toString(), 0, 20);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

void loop()
{
    yield();
}
