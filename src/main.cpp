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
    enum Choices {Clock, System, OTA, NTPSync, Text, LifeTracker};
    virtual Choices loop() = 0;
    virtual void shutdown() = 0;
};

class OTA: public App {
public:
    OTA() {
        M5.Display.setEpdMode(epd_text);
        M5.Display.setRotation(0);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setTextSize(3);
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

class NTPSync: public App {
public:
    NTPSync() {
        M5.Display.setEpdMode(epd_text);
        M5.Display.setRotation(0);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setTextSize(3);
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
        time_t t = time(nullptr)+1; // Advance one second.
        while (t > time(nullptr));  /// Synchronization in seconds
        M5.Rtc.setDateTime( gmtime( &t ) );
        M5.Display.println("ok");
    }
    App::Choices loop() {
        return App::System;
    }
    void shutdown() {
        M5.Display.print("Disconnecting WiFi...");
        WiFi.disconnect();
        M5.Display.println("ok");
    }
};

class System: public App {
public:
    System() {
        M5.Display.setEpdMode(epd_text);
        M5.Display.setRotation(0);
        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setCursor(0, 10);

        int buttonHeight = 100;
        int buttonWidth = 400;
        int buttonSpacing = 50;

        int y = M5.Display.height() / 2 - (NUM_APPS * (buttonHeight + buttonSpacing)) / 2;
        for (int i = 0; i < NUM_APPS; i++) {
            buttons[i].button.initButton(&M5.Display,
                                         M5.Display.width() / 2,
                                         y + (i * (buttonHeight + buttonSpacing)),
                                         buttonWidth,
                                         buttonHeight,
                                         TFT_BLACK,
                                         TFT_LIGHTGRAY,
                                         TFT_BLACK,
                                         buttons[i].text,
                                         3,
                                         3);
            buttons[i].button.drawButton();
        }

        button_power.initButton(&M5.Display,
                                M5.Display.width() / 2,
                                y + (NUM_APPS * (buttonHeight + buttonSpacing)),
                                buttonWidth,
                                buttonHeight,
                                TFT_BLACK,
                                TFT_DARKGRAY,
                                TFT_WHITE,
                                "Power",
                                3,
                                3);
        button_power.drawButton();
        M5.Display.endWrite();
    }
    App::Choices loop() {
        auto t = M5.Touch.getDetail();
        if (t.wasClicked()) {
            if (!button_power.isPressed() && button_power.contains(t.x, t.y)) {
                button_power.press(true);
                button_power.drawButton(true);
            } else if (button_power.isPressed()) {
                button_power.press(false);
                button_power.drawButton(false);
            }
            for (int i = 0; i < NUM_APPS; i++) {
                if (!buttons[i].button.isPressed() && buttons[i].button.contains(t.x, t.y)) {
                    buttons[i].button.press(true);
                    buttons[i].button.drawButton(true);
                } else if (buttons[i].button.isPressed()) {
                    buttons[i].button.press(false);
                    buttons[i].button.drawButton(false);
                }
            }
        } else {
            if (button_power.isPressed()) {
                button_power.press(false);
                button_power.drawButton(false);
            }
            for (int i = 0; i < NUM_APPS; i++) {
                if (buttons[i].button.isPressed()) {
                    buttons[i].button.press(false);
                    buttons[i].button.drawButton(false);
                }
            }
        }
        if (button_power.justPressed()) {
            M5.Display.clearDisplay(TFT_WHITE);
            M5.Power.powerOff();
        }
        for (auto button: buttons) {
            if (button.button.justPressed()) {
                return button.app;
            }
        }
        ESP_LOGD("System", "Going to sleep");
        M5.Power.lightSleep(0, true);
        return App::System;
    }
    void shutdown() {

    }
protected:
    struct System_Button {
        App::Choices app;
        const char* text;
        LGFX_Button button;
    };
    static const int NUM_APPS = 5;
    System_Button buttons[NUM_APPS] = {
        {.app=App::Clock, .text="Clock"},
        {.app=App::Text, .text="Text Demo"},
        {.app=App::LifeTracker, .text="EDH Tracker"},
        {.app=App::OTA, .text="OTA Update"},
        {.app=App::NTPSync, .text="NTP Sync"},
    };
    LGFX_Button button_power;
};

class Clock: public App {
public:
    Clock() {
        M5.Display.setEpdMode(epd_quality);
        M5.Display.setRotation(0);
        M5.Display.clearDisplay(TFT_WHITE);
        M5.Display.setEpdMode(epd_fast);
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.setTextSize(3);
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
        M5.Display.startWrite();
        M5.Display.setEpdMode(epd_quality);
        M5.Display.setRotation(0);
        M5.Display.clearDisplay(TFT_WHITE);
        textArea = new M5Canvas(&M5.Display);
        textArea->createSprite(M5.Display.width() - 50, M5.Display.height() - 50);
        textArea->setFont(&fonts::DejaVu40);
        textArea->setTextSize(1);
        textArea->setTextColor(TFT_BLACK);
        pages[0] = 0;
        pages[currentPage + 1] = printPage(textArea, sample);
        textArea->pushSprite(25, 25);
        M5.Display.endWrite();
    }
    App::Choices loop() {
        if (M5.BtnA.wasClicked() && currentPage > 0) {
            currentPage -= 1;
            printPage(textArea, sample, pages[currentPage]);
            textArea->pushSprite(25, 25);
        } else if (M5.BtnC.wasClicked() && pages[currentPage + 1] >= 0) {
            currentPage += 1;
            pages[currentPage + 1] = printPage(textArea, sample, pages[currentPage]);
            textArea->pushSprite(25, 25);
        }
        return App::Text;
    }
    void shutdown() {

    }
protected:
    M5Canvas* textArea;
    int pages[100];
    int currentPage = 0;
    String sample =
        "Here is what to do if you want to get a lift from a Vogon: forget it. "
        "They are one of the most unpleasant races in the Galaxy.\n\n"
        "Not actually evil, but bad-tempered, bureaucratic, officious and callous. "
        "They wouldn't even lift a finger to save their own grandmothers from the Ravenous "
        "Bugblatter Beast of Traal without orders - signed in triplicate, sent in, sent back, "
        "queried, lost, found, subjected to public inquiry, lost again, and finally buried in "
        "soft peat for three months and recycled as firelighters.\n\n"
        "The best way to get a drink out of a Vogon is to stick your finger down his throat, "
        "and the best way to irritate him is to feed his grandmother to the Ravenous Bugblatter "
        "Beast of Traal.\n\n"
        "On no account should you allow a Vogon to read poetry at you.";

    bool endOfPage(lgfx::LovyanGFX*  display) {
        return display->getCursorY() + display->fontHeight() > display->height();
    }
    
    int printPage(lgfx::LovyanGFX* display, String s, int index = 0) {
        int next = index;
        bool in_whitespace = false;
        display->setTextWrap(false, false);
        display->fillScreen(TFT_WHITE);
        display->setCursor(0, 0);

        for (int i = index; i < s.length(); i++) {
            if (endOfPage(display)) {
                break;
            }
            if (s.charAt(i) == '\n') {
                if (! in_whitespace) {
                    String word = s.substring(next, i);
                    if (display->textWidth(word) + display->getCursorX() > display->width()) {
                        display->print("\n");
                        if (endOfPage(display)) {
                            break;
                        }
                        word.trim();
                        display->print(word);
                        next = i;
                    } else {
                        display->print(word);
                        next = i;
                    }
                }
                in_whitespace = true;
                if (endOfPage(display)) {
                    break;
                }
            } else if (s.charAt(i) == ' ') {
                if (! in_whitespace) {
                    String word = s.substring(next, i);
                    if (display->textWidth(word) + display->getCursorX() > display->width()) {
                        display->print("\n");
                        if (endOfPage(display)) {
                            break;
                        }
                        word.trim();
                        display->print(word);
                        next = i;
                    } else {
                        display->print(word);
                        next = i;
                    }
                }
                in_whitespace = true;
                next = i;
            } else if (i == s.length() - 1) {
                // End of the input, print it if we can.

                String word = s.substring(next);
                if (display->textWidth(word) + display->getCursorX() > display->width()) {
                    display->print("\n");
                    if (endOfPage(display)) {
                        break;
                    }
                    word.trim();
                    display->print(word);
                    next = i;
                } else {
                    display->print(word);
                    next = i;
                }
            } else {
                in_whitespace = false;
            }
        }
        next += 1;
        if (next >= s.length()) {
            return -1;
        } else {
            return next;
        }
    }
};

class LifeTracker: public App {
public:
    LifeTracker() {
        mode = Playing;
        M5.Display.setEpdMode(epd_quality);
        M5.Display.setRotation(1);
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.setTextSize(3);
        sidebarRect = {.x=0, .y=0, .width=M5.Display.width() / 10, .height=M5.Display.height(), .rotation=3};

        M5.Display.startWrite();
        M5.Display.clearDisplay(TFT_WHITE);

        // Sidebar
        time_t current_time = time(nullptr);
        last_interaction_time = current_time;
        next_clock_update = next_minute(current_time);
        drawSidebar();
        drawClock(current_time);
        drawBattery();

        // Players
        for (int i = 0; i < 4; i++) {
            int x_offset = sidebarRect.width;
            int width = (M5.Display.width() - x_offset) / 2;
            int height = M5.Display.height() / 2;

            players[i].lifeTotal = 40;
            for (int j = 0; j < 4; j ++) players[i].commanderDamage[j] = 0;
            switch (i + 1) {
            case 1:
                players[i].rect = { .x=sidebarRect.width, .y=0, .width=width, .height=height, .rotation=2};
                break;
            case 2:
                players[i].rect = { .x=sidebarRect.width + width, .y=0, .width=width, .height=height, .rotation=2};
                break;
            case 3:
                players[i].rect = { .x=sidebarRect.width, .y=height, .width=width, .height=height, .rotation=0};
                break;
            case 4:
                players[i].rect = { .x=sidebarRect.width + width, .y=height, .width=width, .height=height, .rotation=0};
                break;
            }
            drawPlayer(i);
        }

        M5.Display.endWrite();
    }

    App::Choices loop () {
        auto t = M5.Touch.getDetail();
        const time_t current_time = time(nullptr);
        if (t.wasClicked()) {
            last_interaction_time = current_time;

            switch (mode) {
            case Playing:
            case CommanderDamage:
                if (sidebarRect.contains(t.x, t.y)) {
                    showConfirm();
                    mode = ConfirmReset;
                } else {
                    for (int i = 0; i < 4; i++) clickPlayer(i, t.x, t.y);
                }
                break;
            case ConfirmReset:
                if (button_ok.contains(t.x, t.y)) {
                    button_ok.press(true);
                    M5.Display.startWrite();
                    for (int i = 0; i < 4; i++) {
                        players[i].lifeTotal = 40;
                        for (int j = 0; j < 4; j++) players[i].commanderDamage[j] = 0;
                        drawPlayer(i);
                    }
                    M5.Display.endWrite();
                    mode = Playing;
                } else if (button_cancel.contains(t.x, t.y)) {
                    button_cancel.press(true);
                    for (int i = 0; i < 4; i++) drawPlayer(i);
                    mode = Playing;
                }
                break;
            }
        }
        else if (t.wasHold()) {
            last_interaction_time = current_time;
            switch (mode) {
            case Playing:
            case CommanderDamage:
                for (int i = 0; i < 4; i++) holdPlayer(i, t.x, t.y);
                break;
            }
        }
        if (current_time >= next_clock_update) {
            ESP_LOGD("LifeTracker", "Updating clock and battery display");
            next_clock_update = next_minute(current_time);
            drawClock(current_time);
            drawBattery();
        } else {
            if (current_time - last_interaction_time > INACTIVITY_THRESHOLD) {
                int seconds = next_clock_update - current_time;
                ESP_LOGD("LifeTracker", "Sleep for %d seconds", seconds);
                M5.Power.lightSleep(seconds * 1000000, true); // Delay in microseconds
            }
        }
        return App::LifeTracker;
    }
    void shutdown() {
    }

protected:
    enum Mode {Playing, CommanderDamage, ConfirmReset};
    Mode mode;
    struct Rect {
        int x;
        int y;
        int width;
        int height;
        int rotation;

        bool contains(int _x, int _y) {
            return _x >= x && _x <= (x + width) && _y >= y && _y <= (y + height);
        }
    };
    struct Player {
        int lifeTotal;
        Rect rect;
        int commanderDamage[4];
    };
    Rect sidebarRect;
    Player players[4];
    int activePlayer = 0;
    LGFX_Button button_ok;
    LGFX_Button button_cancel;
    time_t last_interaction_time;
    time_t next_clock_update;
    const int INACTIVITY_THRESHOLD = 10; // Seconds of inactivity before app will enter sleep

    void drawPlayer(int player) {
        if (player < 0 || player >= 4) return;

        Rect* rect = &players[player].rect;
        int border = 5;
        int lifeTotal;
        bool isActive = false;
        auto lineColor = TFT_LIGHTGRAY;
        switch (mode) {
            case CommanderDamage:
                lifeTotal = players[activePlayer].commanderDamage[player];
                isActive = activePlayer == player;
                lineColor = TFT_BLACK;
                border = 7;
                break;
        default:
            lifeTotal = players[player].lifeTotal;
        }
        M5Canvas canvas(&M5.Display);
        canvas.createSprite(rect->width, rect->height);
        canvas.fillSprite(lineColor);
        canvas.fillRect(border, border, rect->width - border, rect->height - border, isActive ? TFT_LIGHTGRAY : TFT_WHITE);
        canvas.setRotation(rect->rotation);
        canvas.setTextColor(TFT_BLACK);
        canvas.setFont(&fonts::FreeMono24pt7b);
        canvas.setTextSize(1);
        canvas.drawString("-1", border * 2, border * 2);
        canvas.drawString("-5", border * 2, rect->height - border * 2 - canvas.fontHeight());
        canvas.drawString("+1", rect->width - border * 2 - canvas.textWidth("+1"), border * 2);
        canvas.drawString("+5", rect->width - border * 2 - canvas.textWidth("+5"), rect->height - border * 2 - canvas.fontHeight());
        int separator_y = 2 * rect->height / 3;
        int separator_length = rect->width / 5;
        canvas.fillRect(0, separator_y, separator_length, border, lineColor);
        canvas.fillRect(rect->width - separator_length, separator_y, separator_length, border, lineColor);
        canvas.setFont(&fonts::Orbitron_Light_32);
        canvas.setTextSize(3);
        canvas.drawCenterString(String(lifeTotal, DEC), rect->width / 2, (rect->height / 2) - (canvas.fontHeight() / 2));
        canvas.pushSprite(rect->x, rect->y);
    }

    void clickPlayer(int player, int x, int y) {
        Rect* rect = &players[player].rect;
        if (! rect->contains(x, y)) return;

        int rel_x;
        int rel_y;
        int life_change;
        int life_multiplier;

        rel_x = x - rect->x;
        rel_y = y - rect->y;
        switch(rect->rotation) {
        case 2:
            // Flipped 180 degrees
            rel_x = rect->width - rel_x;
            rel_y = rect->height - rel_y;
        }

        if (rel_x > rect->width / 2) {
            life_change = 1;
        } else {
            life_change = -1;
        }
        if (rel_y < 2 * rect->height / 3) {
            life_multiplier = 1;
        } else {
            life_multiplier = 5;
        }

        switch (mode) {
        case Playing:
            players[player].lifeTotal += (life_change * life_multiplier);
            break;
        case CommanderDamage:
            players[activePlayer].commanderDamage[player] += (life_change * life_multiplier);
            players[activePlayer].lifeTotal -= (life_change * life_multiplier);
            break;
        }
        drawPlayer(player);
    }

    void holdPlayer(int player, int x, int y) {
        Rect* rect = &players[player].rect;
        if (! rect->contains(x, y)) return;
        switch (mode) {
        case Playing:
            mode = CommanderDamage;
            activePlayer = player;
            break;
        case CommanderDamage:
            mode = Playing;
            break;
        }
        for (int i = 0; i < 4; i++) drawPlayer(i);
    }

    void showConfirm() {
        int play_area_width = M5.Display.width() - sidebarRect.width;
        int play_area_height = M5.Display.height();
        int width = 2 * play_area_width / 3;
        int height = 2 * play_area_height / 3;
        int x = sidebarRect.width + (play_area_width - width) / 2;
        int y = (play_area_height - height) / 2;
        int border = 10;
        int button_width = width / 3;

        M5.Display.startWrite();
        M5.Display.fillRect(x, y, width, height, TFT_DARKGRAY);
        M5.Display.fillRect(x + border, y + border, width - border * 2, height - border * 2, TFT_WHITE);
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.setTextSize(2);
        M5.Display.drawCenterString("Reset game?", x + (width / 2), y + (border * 2));
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setTextSize(1);
        button_ok.initButton(&M5.Display, x + 2 * button_width / 3, y + height - 100, button_width, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "OK", 3, 3);
        button_ok.drawButton();
        button_cancel.initButton(&M5.Display, x + width - 2 * button_width / 3, y + height - 100, button_width, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "Cancel", 3, 3);
        button_cancel.drawButton();
        M5.Display.endWrite();
    }

    void showCommanderDamage() {
        int play_area_width = M5.Display.width() - sidebarRect.width;
        int play_area_height = M5.Display.height();
        int width = 2 * play_area_width / 3;
        int height = 2 * play_area_height / 3;
        int x = sidebarRect.width + (play_area_width - width) / 2;
        int y = (play_area_height - height) / 2;
        int border = 10;
        int button_width = width / 3;

        M5.Display.startWrite();
        M5.Display.fillRect(x, y, width, height, TFT_DARKGRAY);
        M5.Display.fillRect(x + border, y + border, width - border * 2, height - border * 2, TFT_WHITE);
        M5.Display.setFont(&fonts::Orbitron_Light_32);
        M5.Display.setTextSize(2);
        M5.Display.drawCenterString("Reset game?", x + (width / 2), y + (border * 2));
        M5.Display.setFont(&fonts::Font0);
        M5.Display.setTextSize(1);
        button_ok.initButton(&M5.Display, x + 2 * button_width / 3, y + height - 100, button_width, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "OK", 3, 3);
        button_ok.drawButton();
        button_cancel.initButton(&M5.Display, x + width - 2 * button_width / 3, y + height - 100, button_width, 100, TFT_BLACK, TFT_LIGHTGRAY, TFT_BLACK, "Cancel", 3, 3);
        button_cancel.drawButton();
        M5.Display.endWrite();
    }


    void drawSidebar() {
        M5Canvas canvas(&M5.Display);
        canvas.createSprite(sidebarRect.width, sidebarRect.height);
        canvas.fillSprite(TFT_BLACK);
        canvas.setRotation(sidebarRect.rotation);
        canvas.setTextColor(TFT_WHITE);
        canvas.setFont(&fonts::Orbitron_Light_32);
        canvas.setTextSize(1);
        canvas.drawCenterString("TAP TO", canvas.width() / 2, (canvas.height() / 2) - canvas.fontHeight());
        canvas.drawCenterString("RESET", canvas.width() / 2, (canvas.height() / 2));
        canvas.pushSprite(sidebarRect.x, sidebarRect.y);
    }

    time_t next_minute(time_t timestamp) {
        return 60 * ((timestamp / 60) + 1);
    }

    void drawClock(time_t t) {
        M5Canvas canvas(&M5.Display);
        canvas.setFont(&fonts::Orbitron_Light_32);
        canvas.setTextSize(1);
        canvas.createSprite(canvas.fontHeight(), canvas.textWidth("00:00"));
        canvas.fillSprite(TFT_BLACK);
        canvas.setTextColor(TFT_WHITE);
        canvas.setRotation(3);
        auto tm = localtime(&t);
        canvas.printf("%02d:%02d", tm->tm_hour, tm->tm_min);
        canvas.pushSprite(0, 0);
    }

    void drawBattery() {
        M5Canvas canvas(&M5.Display);
        canvas.setFont(&fonts::Font0);
        canvas.setTextSize(3);
        canvas.createSprite(canvas.fontHeight(), canvas.textWidth("100%"));
        canvas.fillSprite(TFT_BLACK);
        canvas.setTextColor(TFT_WHITE);
        canvas.setRotation(3);
        int battery = M5.Power.getBatteryLevel();
        canvas.printf("%3d%%", battery);
        canvas.pushSprite(0, M5.Display.height() - canvas.width());
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

    app = new System;
    current_app = App::System;
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
    case App::LifeTracker:
        app = new LifeTracker;
        break;
    case App::NTPSync:
        app = new NTPSync;
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
    if (M5.BtnB.wasClicked()) {
        next = App::System;
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
