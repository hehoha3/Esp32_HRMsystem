#include "lcd_hmi.h"

// constructor
LCD_HMI::LCD_HMI(uint8_t rx, uint8_t tx, uint32_t baud)
    : rxPin(rx), txPin(tx), baudRate(baud) {}

void LCD_HMI::begin()
{
    Serial2.begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

// send text / cmd to and element on the LCD
void LCD_HMI::sendTextToElement(const String &cmd)
{
    // Send command to display
    Serial2.print(cmd);

    // Send required terminator bytes
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0xFF);

    // Debug output
    Serial.print("[LCD] Sent to TJC: ");
    Serial.println(cmd);
}

// read response from LCD
String LCD_HMI::readResponse()
{
    String response = "";
    while (Serial2.available())
    {
        response = Serial2.readStringUntil('\xff');
    }
    return response;
}