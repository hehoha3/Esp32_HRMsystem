#ifndef LCD_HMI_H
#define LCD_HMI_H

#include <Arduino.h>

class LCD_HMI
{
public:
    LCD_HMI(uint8_t rx, uint8_t tx, uint32_t baud);
    void begin();
    void sendTextToElement(const String &text);
    String readResponse();
private:
    uint8_t rxPin;
    uint8_t txPin;
    uint32_t baudRate;
};

#endif // LCD_HMI_H