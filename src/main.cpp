#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define UIBUTTONS 4
#define MAX_UIBUTTON_TEXT_SIZE 20
#define UIBUTTON_SIZE (TFT_HEIGHT / UIBUTTONS)
#define MAX_POINTS 3

#define NUMBER_SHUTTERS 3
#define MAX_SHUTTER_NAME_SIZE 15

#define MAX_SHUTTER_CMD_SIZE 20

#define MAX_URL_SIZE 100

typedef struct {
    uint16_t color;
    char text[MAX_UIBUTTON_TEXT_SIZE];
    void (*callback)();
} UIButton_t;

typedef enum {
    SHUTTER_CMD_UP,
    SHUTTER_CMD_STOP,
    SHUTTER_CMD_DOWN,
    NUMBER_SHUTTER_CMDS
} shutter_cmd_t;

void drawUIButton(uint8_t UIButtonIndex, bool selected);
void drawUIButtons();
void UILoading();
void nextShutter();
void sendShutterCmd(shutter_cmd_t shutter_cmd);
void sendShutterCmdUp();
void sendShutterCmdStop();
void sendShutterCmdDown();

const char* ssid     = "ssid";
const char* password = "pass";

UIButton_t UIButtons[UIBUTTONS] = {
    { NAVY, "Shutter", nextShutter },
    { DARKGREEN, "Up", sendShutterCmdUp },
    { DARKGREY, "Stop", sendShutterCmdStop },
    { MAROON, "Down", sendShutterCmdDown }
};

uint8_t UIButtonIndex = 0;

unsigned long last_press;

bool last_warning_level;

char shutterNames[NUMBER_SHUTTERS][MAX_SHUTTER_NAME_SIZE] = {
    "bedroom-guest",
    "living-room",
    "bedroom-1"
};

char shutterCmds[NUMBER_SHUTTER_CMDS][MAX_SHUTTER_CMD_SIZE] = {
    "up",
    "stop",
    "down"
};

uint8_t selectedShutter = NUMBER_SHUTTERS - 1;

void nextShutter() {
    selectedShutter = (selectedShutter + 1) % NUMBER_SHUTTERS;
    strncpy(UIButtons[UIButtonIndex].text, shutterNames[selectedShutter], MAX_UIBUTTON_TEXT_SIZE);
    drawUIButton(UIButtonIndex, true);
}

void sendShutterCmd(shutter_cmd_t shutter_cmd) {
    static char URL[MAX_URL_SIZE];

    snprintf(URL, MAX_URL_SIZE,
            "http://shutter-%.*s.lan/control?cmd=event,shutter_%.*s",
            MAX_SHUTTER_NAME_SIZE, shutterNames[selectedShutter],
            MAX_SHUTTER_CMD_SIZE, shutterCmds[shutter_cmd]);

    HTTPClient http;
    http.begin(URL);
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            M5.Lcd.fillScreen(DARKGREEN);
            M5.Lcd.setTextDatum(MC_DATUM);
            M5.Lcd.setTextColor(WHITE),
                M5.Lcd.drawString("OK", TFT_WIDTH / 2, TFT_HEIGHT / 2);
        }
    } else {
        M5.Lcd.fillScreen(RED);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(WHITE),
            M5.Lcd.drawString("ERROR", TFT_WIDTH / 2, TFT_HEIGHT / 2);
    }
    http.end();
    delay(1000);
    drawUIButtons();
}

void sendShutterCmdUp() {
    sendShutterCmd(SHUTTER_CMD_UP);
}

void sendShutterCmdStop() {
    sendShutterCmd(SHUTTER_CMD_STOP);
}

void sendShutterCmdDown() {
    sendShutterCmd(SHUTTER_CMD_DOWN);
}

void drawUIButton(uint8_t UIButtonIndex, bool selected) {
    uint16_t color = UIButtons[UIButtonIndex].color;
    uint16_t fillColor = color;

    if (selected) {
        M5.Lcd.fillRect(0, 40 * UIButtonIndex, TFT_WIDTH, 40, WHITE);
        uint8_t padding = 3;
        M5.Lcd.fillRect(
            padding, 40 * UIButtonIndex + padding,
            TFT_WIDTH - padding * 2, 40 - padding * 2,
            fillColor
        );
    } else {
        M5.Lcd.fillRect(0, 40 * UIButtonIndex, 80, 40, fillColor);
    }

    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawString(
        UIButtons[UIButtonIndex].text,
        TFT_WIDTH / 2,
        40 * UIButtonIndex+ UIBUTTON_SIZE / 2
    );
}

void drawUIButtons() {
    for (uint8_t i = 0; i < UIBUTTONS; i++) {
        drawUIButton(i, i == UIButtonIndex);
    }
}

void UILoading() {
    static char points[MAX_POINTS + 1];
    static uint8_t step = 0;
    if (step == 0) {
        strcpy(points, ".  ");
    } else {
        points[step] = '.';
    }
    M5.Lcd.drawString(points, TFT_WIDTH / 2, TFT_HEIGHT / 2 + 10);
    step = (step + 1) % MAX_POINTS;
}

// the setup routine runs once when M5StickC starts up
void setup() {

    // initialize the M5StickC object
    M5.begin(true, true, false);

    pinMode(M5_LED, OUTPUT);
    digitalWrite(M5_LED, HIGH);

    // text print
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);

    WiFi.begin(ssid, password);

    M5.Lcd.fillScreen(PURPLE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextColor(WHITE, PURPLE),
    M5.Lcd.drawString("Connecting", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 10);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        UILoading();
    }

    nextShutter();
    drawUIButtons();
    last_press = millis();
    last_warning_level = M5.Axp.GetWarningLevel();
}


void loop() {
    if (M5.BtnB.wasPressed()) {
        drawUIButton(UIButtonIndex, false);
        UIButtonIndex = (UIButtonIndex + 1) % UIBUTTONS;
        drawUIButton(UIButtonIndex, true);
        last_press = millis();
    }

    if (M5.BtnA.wasPressed()) {
        UIButtons[UIButtonIndex].callback();
        last_press = millis();
    }

    bool warning_level = M5.Axp.GetWarningLevel();
    if (warning_level && !last_warning_level) {
        digitalWrite(M5_LED, LOW);
        M5.Lcd.fillScreen(RED);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setTextColor(WHITE),
        M5.Lcd.drawString("LOW BAT", TFT_WIDTH / 2, TFT_HEIGHT / 2);
        delay(1000);
        drawUIButtons();
    }
    last_warning_level = warning_level;

    if (millis() - last_press > 10000) {
        M5.Axp.DeepSleep();
    }

    M5.update();
    delay(10);
}
