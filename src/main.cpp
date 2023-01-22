#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

class App {
public:
    virtual void loop() = 0;
};

class OTA: public App {
public:
    OTA() {
        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setCursor(0, 10);
        M5.Display.printf("SSID: %s\n", ssid);
        M5.Display.printf("IP Address: %s\n", WiFi.localIP().toString());
        M5.Display.printf("\n\n");
        M5.Display.printf("Web server running on port 80\n");
        M5.Display.printf("ElegantOTA available at\n"
                          "http://%s/update\n",
                          WiFi.localIP().toString());

        button_power.initButton(&M5.Display, M5.Display.width() / 2, M5.Display.height() / 2, 200, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "Power", 3, 3);
        button_power.drawButton();
        M5.Display.endWrite();
    }
    void loop() {
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            if (button_power.contains(t.x, t.y)) {
                button_power.press(true);
            } else {
                button_power.press(false);
            }
        } else {
            button_power.press(false);
        }
        if (button_power.justReleased()) {
            M5.Display.clearDisplay(TFT_WHITE);
            M5.Power.powerOff();
        }
        yield();
    }
protected:
    LGFX_Button button_power;
};

class Clock: public App {
public:
    Clock() {
        M5.Display.clearDisplay(TFT_WHITE);
    }
    void loop() {
        static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
        auto dt = M5.Rtc.getDateTime();
        M5.Display.setCursor(0, 15);
        M5.Display.printf("RTC : %04d/%02d/%02d (%s)  %02d:%02d:%02d"
               , dt.date.year
               , dt.date.month
               , dt.date.date
               , wd[dt.date.weekDay]
               , dt.time.hours
               , dt.time.minutes
               , dt.time.seconds
               );
        delay(500);
    }
};

AsyncWebServer server(80);
App* app;

void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.init();
    M5.Display.setTextSize(3);
    M5.Display.startWrite();
    M5.Display.setCursor(0, M5.Display.height() / 2);
    M5.Display.printf("Starting up...");
    M5.Display.endWrite();

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

    app = new OTA;
}

void loop()
{
    M5.update();
    if (M5.BtnA.wasClicked()) {
        delete app;
        app = new Clock;
    } else if (M5.BtnC.wasClicked()) {
        delete app;
        app = new OTA;
    }
    app->loop();
    yield();
}
