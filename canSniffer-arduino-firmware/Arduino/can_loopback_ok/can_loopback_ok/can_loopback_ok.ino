#include <Arduino.h>
#include "driver/twai.h"
#include "pin_config.h"

#define SERIAL_SPEED 115200

bool canStarted = false;

void startCAN()
{
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX,
                                    (gpio_num_t)CAN_RX,
                                    TWAI_MODE_NORMAL);

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();

    canStarted = true;
}

void sendFrame(uint32_t id, uint8_t *data, uint8_t len)
{
    twai_message_t msg;

    msg.identifier = id;
    msg.extd = 0;
    msg.rtr = 0;
    msg.data_length_code = len;

    for (int i = 0; i < len; i++)
        msg.data[i] = data[i];

    twai_transmit(&msg, 0);

    // SOFTWARE LOOPBACK
    Serial.print(id, HEX);
    Serial.print(",00,00,");

    for (int i = 0; i < len; i++)
    {
        if (data[i] < 16)
            Serial.print("0");

        Serial.print(data[i], HEX);
    }

    Serial.println();
}

void processPythonFrame(String line)
{
    line.trim();

    int comma1 = line.indexOf(',');
    int comma2 = line.indexOf(',', comma1 + 1);
    int comma3 = line.indexOf(',', comma2 + 1);

    if (comma1 < 0 || comma2 < 0 || comma3 < 0)
        return;

    String idStr = line.substring(0, comma1);
    String dataStr = line.substring(comma3 + 1);

    uint32_t id = strtol(idStr.c_str(), NULL, 16);

    int dataLen = dataStr.length() / 2;
    if (dataLen > 8) dataLen = 8;

    uint8_t data[8];

    for (int i = 0; i < dataLen; i++)
    {
        String byteStr = dataStr.substring(i * 2, i * 2 + 2);
        data[i] = strtol(byteStr.c_str(), NULL, 16);
    }

    sendFrame(id, data, dataLen);
}

void readCAN()
{
    twai_message_t msg;

    while (twai_receive(&msg, 0) == ESP_OK)
    {
        Serial.print(msg.identifier, HEX);
        Serial.print(",00,00,");

        for (int i = 0; i < msg.data_length_code; i++)
        {
            if (msg.data[i] < 16)
                Serial.print("0");

            Serial.print(msg.data[i], HEX);
        }

        Serial.println();
    }
}

void setup()
{
    Serial.begin(SERIAL_SPEED);
    delay(500);
    startCAN();
}

void loop()
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        processPythonFrame(cmd);
    }

    if (canStarted)
        readCAN();
}