#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <SPI.h>
#include <MFRC522.h>

class RFID_RC522 
{
public:
    RFID_RC522(uint8_t ss_pin, uint8_t rst_pin);
    void begin();
    bool readBlockData(uint8_t block_num, uint8_t *data, uint8_t data_length);
    bool compareUid(const uint8_t *expected_UID, uint8_t expected_length);

private:
    uint8_t _ssPin;
    uint8_t _rstPin;
    MFRC522 _mfrc522;
    MFRC522::MIFARE_Key _key;

    uint8_t KEY_A[6] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6};

    bool isCardPresent();
    bool blockAuth(uint8_t block_num);
};

#endif //RFID_RC522_H