#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <sntp.h>
#include "config.h"

using namespace std;

class App {
public:
    enum Choices {Clock, System, OTA, Text};
    virtual Choices loop() = 0;
    virtual void shutdown() = 0;
};

class OTA: public App {
public:
    OTA() {
        M5.Display.setEpdMode(epd_text);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.print("Connecting to WiFi");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        M5.Display.println("");

        // Wait for connection
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            M5.Display.print(".");
        }
        M5.Display.println("");
        M5.Display.print("Connected to ");
        M5.Display.println(WIFI_SSID);
        M5.Display.print("IP address: ");
        M5.Display.println(WiFi.localIP());

        M5.Display.print("Syncing time");
        configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
        while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
            M5.Display.print('.');
            delay(1000);
        }
        M5.Display.println("");
        time_t t = time(nullptr)+1; // Advance one second.
        while (t > time(nullptr));  /// Synchronization in seconds
        M5.Rtc.setDateTime( gmtime( &t ) );

        server = new AsyncWebServer(80);
        server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "Hi! I am ESP32.");
        });

        AsyncElegantOTA.begin(server);    // Start ElegantOTA
        server->begin();
        M5.Display.println("HTTP server started");

        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setCursor(0, 10);
        M5.Display.printf("SSID: %s\n", WIFI_SSID);
        M5.Display.printf("IP Address: %s\n", WiFi.localIP().toString());
        M5.Display.printf("\n\n");
        M5.Display.printf("Web server running on port 80\n");
        M5.Display.printf("ElegantOTA available at\n"
                          "http://%s/update\n",
                          WiFi.localIP().toString());

        M5.Display.endWrite();
    }
    App::Choices loop() {
        return App::OTA;
    }
    void shutdown() {
        M5.Display.print("Stopping web server...");
        server->end();
        delete server;
        M5.Display.println("ok");
        M5.Display.print("Disconnecting WiFi...");
        WiFi.disconnect();
        M5.Display.println("ok");
    }
protected:
    AsyncWebServer* server;
};

class System: public App {
public:
    System() {
        M5.Display.setEpdMode(epd_text);
        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setCursor(0, 10);

        button_ota.initButton(&M5.Display, M5.Display.width() / 2, M5.Display.height() / 2, 200, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "Update", 3, 3);
        button_ota.drawButton();
        button_power.initButton(&M5.Display, M5.Display.width() / 2, M5.Display.height() / 2 + 200, 200, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "Power", 3, 3);
        button_power.drawButton();
        M5.Display.endWrite();
        button_power.press(false);
        button_ota.press(false);
    }
    App::Choices loop() {
        auto t = M5.Touch.getDetail();
        if (t.wasPressed()) {
            if (button_power.contains(t.x, t.y)) {
                button_power.press(true);
            } else {
                button_power.press(false);
            }
            if (button_ota.contains(t.x, t.y)) {
                button_ota.press(true);
            } else {
                button_ota.press(false);
            }
        } else {
            button_power.press(false);
            button_ota.press(false);
        }
        if (button_power.justReleased()) {
            M5.Display.clearDisplay(TFT_WHITE);
            M5.Power.powerOff();
        }
        if (button_ota.justReleased()) {
            return App::OTA;
        }
        return App::System;
    }
    void shutdown() {

    }
protected:
    LGFX_Button button_power;
    LGFX_Button button_ota;
};

class Clock: public App {
public:
    Clock() {
        M5.Display.setEpdMode(epd_quality);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setEpdMode(epd_fast);
        M5.Display.setFont(&fonts::Orbitron_Light_32);
    }
    App::Choices loop() {
        static constexpr const char* const wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
        static auto last_t = time(nullptr) - 10;
        auto t = time(nullptr);
        int battery = M5.Power.getBatteryLevel();

        if (t >= (last_t + 10)) {
            last_t = t;
            auto tm = localtime(&t);
            M5.Display.setCursor(0,0);
            M5.Display.setFont(&fonts::Font2);
            M5.Display.printf("%3d%%     ", battery);
            M5.Display.setCursor(0, M5.Display.height() / 3);
            M5.Display.setFont(&fonts::Orbitron_Light_24);
            M5.Display.printf("%s\n%04d.%02d.%02d\n"
                              , wd[tm->tm_wday]
                              , tm->tm_year + 1900
                              , tm->tm_mon + 1
                              , tm->tm_mday
                              );
            M5.Display.setFont(&fonts::Orbitron_Light_32);
            M5.Display.printf("%02d:%02d     "
                              , tm->tm_hour
                              , tm->tm_min
                              );
        }
        return App::Clock;
    }
    void shutdown() {

    }
};

class Text: public App {
public:
    Text() {
        String sample =
            "I've seen things you people wouldn't believe.\n"
            "Attack ships on fire off the shoulder of Orion.\n"
            "I watched c-beams glitter in the dark near the Tannhäuser Gate.\n"
            "All those moments will be lost in time, like tears in rain.\n"
            "Time to die.";
        M5.Display.startWrite();
        M5.Display.setEpdMode(epd_quality);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::FreeSans9pt7b);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(0, 0);
        printPage(M5.Display, sample);
        M5.Display.endWrite();
    }
    App::Choices loop() {
        return App::Text;
    }
    void shutdown() {

    }
protected:
    uint8_t getTextHeight(M5GFX display) {
        const lgfx::v1::IFont* font = display.getFont();
        lgfx::v1::FontMetrics metrics;
        font->getDefaultMetric(&metrics);
        return metrics.y_advance * display.getTextSizeY();
    }

    bool endOfPage(M5GFX display) {
        return display.getCursorY() + getTextHeight(display) > display.height();
    }
    
    int printPage(M5GFX display, String s, int index = 0) {
        int next = index;
        bool in_whitespace = false;
        display.setTextWrap(false, false);

        for (int i = index; i < s.length(); i++) {
            if (endOfPage(display)) {
                break;
            }
            if (s.charAt(i) == '\n') {
                if (! in_whitespace) {
                    String word = s.substring(next, i);
                    if (display.textWidth(word) + display.getCursorX() > display.width()) {
                        display.print("\n");
                        if (endOfPage(display)) {
                            break;
                        }
                        word.trim();
                        display.print(word);
                        next = i;
                    } else {
                        display.print(word);
                        next = i;
                    }
                }
                in_whitespace = true;
                display.print("\n");
                if (endOfPage(display)) {
                    break;
                }
            } else if (s.charAt(i) == ' ') {
                if (! in_whitespace) {
                    String word = s.substring(next, i);
                    if (display.textWidth(word) + display.getCursorX() > display.width()) {
                        display.print("\n");
                        if (endOfPage(display)) {
                            break;
                        }
                        word.trim();
                        display.print(word);
                        next = i;
                    } else {
                        display.print(word);
                        next = i;
                    }
                }
                in_whitespace = true;
                next = i;
            } else if (i == s.length() - 1) {
                // End of the input, print it if we can.

                String word = s.substring(next);
                if (display.textWidth(words) + display.getCursorX() > display.width()) {
                    display.print("\n");
                    if (endOfPage(display)) {
                        break;
                    }
                    word.trim();
                    display.print(word);
                    next = i;
                } else {
                    display.print(word);
                    next = i;
                }
            } else {
                in_whitespace = false;
            }
        }
        return next;
    }
};

App* app;
App::Choices current_app;

void setup(void) {
    auto cfg = M5.config();
    M5.begin(cfg);

    setenv("TZ", NTP_TIMEZONE, 1);
    tzset();

    M5.Display.init();
    M5.Display.setTextSize(3);
    M5.Display.setCursor(0, M5.Display.height() / 4);
    M5.Display.println("Starting up...");

    app = new Clock;
    current_app = App::Clock;
}

void switch_app(App::Choices next) {
    app->shutdown();
    delete app;
    M5.update();
    switch (next) {
    case App::Clock:
        app = new Clock;
        break;
    case App::Text:
        app = new Text;
        break;
    case App::System:
        app = new System;
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
    } else if (M5.BtnB.wasClicked()) {
        next = App::System;
    } else if (M5.BtnC.wasClicked()) {
        next = App::Text;
    }
    try {
        if (next != current_app) {
            switch_app(next);
        } else {
            next = app->loop();
            if (next != current_app) {
                switch_app(next);
            }
        }
    } catch (...) {
        switch_app(App::System);
    }
    yield();
}
