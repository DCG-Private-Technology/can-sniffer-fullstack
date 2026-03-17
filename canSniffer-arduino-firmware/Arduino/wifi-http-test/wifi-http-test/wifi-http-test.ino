#include <WiFi.h>
#include <WebServer.h>
#include "FastLED.h"
#include "driver/twai.h"
#include "pin_config.h"

#define NUM_LEDS 1
#define DATA_PIN WS2812B_DATA

CRGB leds[NUM_LEDS];

WebServer server(80);

const char* ssid = "BMW_CAN_WIFI";
const char* password = "123456790";

void sendCAN()
{
    twai_message_t message;
    message.identifier = 0xF1;
    message.extd = 0;
    message.rtr = 0;
    message.data_length_code = 8;

    for(int i=0;i<8;i++)
        message.data[i] = i+1;

    if(twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK)
        Serial.println("CAN sent");
    else
        Serial.println("CAN send FAIL");

    server.send(200,"text/html","CAN message sent");
}

void ledOn()
{
    leds[0] = CRGB::Green;
    FastLED.show();
    server.send(200,"text/html","LED ON");
}

void ledOff()
{
    leds[0] = CRGB::Black;
    FastLED.show();
    server.send(200,"text/html","LED OFF");
}

void handleRoot()
{
    String page = R"rawliteral(
    <html>
    <head>
    <title>BMW CAN TOOL</title>
    </head>
    <body>
    <h1>BMW CAN TEST</h1>
    <button onclick="location.href='/ledon'">LED ON</button><br><br>
    <button onclick="location.href='/ledoff'">LED OFF</button><br><br>
    <button onclick="location.href='/sendcan'">SEND CAN</button>
    </body>
    </html>
    )rawliteral";

    server.send(200,"text/html",page);
}

void setupCAN()
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)5,(gpio_num_t)4,TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    twai_driver_install(&g_config,&t_config,&f_config);
    twai_start();
}

void setup()
{
    Serial.begin(115200);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    leds[0] = CRGB::Blue;
    FastLED.show();

    WiFi.softAP(ssid,password);

    Serial.println("WiFi started");
    Serial.println(WiFi.softAPIP());

    setupCAN();

    server.on("/", handleRoot);
    server.on("/ledon", ledOn);
    server.on("/ledoff", ledOff);
    server.on("/sendcan", sendCAN);

    server.begin();
}

void loop()
{
    server.handleClient();
}