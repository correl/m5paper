#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "config.h"

class App {
public:
    virtual void loop() = 0;
    enum Choices {Clock, OTA};
};

class OTA: public App {
public:
    OTA() {
        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setEpdMode(epd_text);
        M5.Display.setCursor(0, 10);
        M5.Display.printf("SSID: %s\n", WIFI_SSID);
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
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.setEpdMode(epd_fast);
    }
    void loop() {
        static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
        auto dt = M5.Rtc.getDateTime();
        M5.Display.setCursor(0, M5.Display.height() / 3);
        M5.Display.setFont(&fonts::Orbitron_Light_24);
        M5.Display.printf("%s\n%04d.%02d.%02d\n"
                          , wd[dt.date.weekDay]
                          , dt.date.year
                          , dt.date.month
                          , dt.date.date
                          );
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.printf("%02d:%02d     "
                          , dt.time.hours
                          , dt.time.minutes
                          );
        delay(10000);
    }
};

AsyncWebServer server(80);
App* app;
App::Choices current_app;

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
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi! I am ESP32.");
    });

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");

    app = new Clock;
    current_app = App::Clock;
}

void switch_app(App::Choices next) {
    delete app;
    switch (next) {
    case App::Clock:
        app = new Clock;
        break;
    case App::OTA:
    default:
        app = new OTA;
    }
    current_app = next;
}

void loop()
{
    M5.update();

    App::Choices next = current_app;
    if (M5.BtnA.wasClicked()) {
        next = App::Clock;
    } else if (M5.BtnC.wasClicked()) {
        next = App::OTA;
    }
    if (next != current_app) {
        switch_app(next);
    } else {
        app->loop();
    }
    yield();
}
