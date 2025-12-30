#include "rfid_rc522.h"

// constructor
RFID_RC522::RFID_RC522(uint8_t ss_pin, uint8_t rst_pin)
    : _ssPin(ss_pin), _rstPin(rst_pin), _mfrc522(ss_pin, rst_pin) {}

// RFID begin function
void RFID_RC522::begin()
{
    // start SPI
    SPI.begin();

    // start reader
    _mfrc522.PCD_Init();
    delay(50);

    Serial.print("[RC522] Reader is ready to read RFID Tag");
}

// Read block data function
bool RFID_RC522::readBlockData(uint8_t block_num, uint8_t *data, uint8_t data_length)
{
    if (!isCardPresent())
        return false;

    if (!blockAuth(block_num))
        return false;

    uint8_t buffer[18];
    uint8_t bufferSize = sizeof(buffer);
    MFRC522::StatusCode status = _mfrc522.MIFARE_Read(block_num, buffer, &bufferSize);

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("[RC522] Error when read data from block ");
        Serial.print(block_num);
        Serial.print(": ");
        Serial.println(_mfrc522.GetStatusCodeName(status));
        return false;
    }

    data_length = (data_length < bufferSize) ? data_length : bufferSize;
    memcpy(data, buffer, data_length);

    // stop encrypto
    _mfrc522.PICC_HaltA();
    _mfrc522.PCD_StopCrypto1();

    return true;
}

// compare this UID to expected UID
bool RFID_RC522::compareUid(const uint8_t *expected_UID, uint8_t expected_length)
{
    if (_mfrc522.uid.size != expected_length)
        return false;

    for (uint8_t i = 0; i < expected_length; i++)
    {
        if (_mfrc522.uid.uidByte[i] != expected_UID[i])
            return false;
    }
    return true;
}

// check if card is present
bool RFID_RC522::isCardPresent()
{
    if (!_mfrc522.PICC_IsNewCardPresent() || !_mfrc522.PICC_ReadCardSerial())
    {
        delay(50);
        return false;
    }
    return true;
}

// AUTH to a block
bool RFID_RC522::blockAuth(uint8_t block_num)
{
    Serial.print("[RC522] Card UID: ");
    for (uint8_t i = 0; i < 4; i++)
    {
        Serial.print(_mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(_mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    MFRC522::StatusCode status = _mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        block_num,
        &_key,
        &(_mfrc522.uid));

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("[RC522] AUTH is Error: ");
        Serial.println(_mfrc522.GetStatusCodeName(status));
        return false;
    }
    return true;
}

// set RFID Key A
void RFID_RC522::setRFIDKey(const uint8_t *keyBytes)
{
    for (uint8_t i = 0; i < MFRC522::MF_KEY_SIZE; i++)
    {
        _key.keyByte[i] = keyBytes[i];
    }

    Serial.print("[RC522] RFID Key A: ");
    for (uint8_t i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
        Serial.print(_key.keyByte[i] < 0x10 ? " 0" : " ");
        Serial.print(_key.keyByte[i], HEX);
    }
    Serial.println();
}