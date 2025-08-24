#include "can.h"



void setup() {
  Serial.begin(115200);
  Serial.println("starting...");

  //запустить кан
  can.begin();
  can.converter = true;  //шлет все данные в сериал
}

void loop() {
  // put your main code here, to run repeatedly:
  can.readData();  // считать что там у шинах кан

  checkSerialInput();  // проверить входящее
}

// получить бинарные данные из шины
void checkSerialInput() {
  static uint8_t buffer[15];
  static uint8_t index = 0;
  static bool receiving = false;

  while (Serial1.available()) {
    uint8_t byte = Serial1.read();

    if (!receiving) {
      if (byte == 0xAA) {
        index = 0;
        receiving = true;
        buffer[index++] = byte;
      }
    } else {
      buffer[index++] = byte;
      if (index >= 15) {
        receiving = false;
        if (buffer[14] == 0x55) {
          parseBinaryPacket(buffer);
        } else {
          // Ошибка пакета — проигнорировать
        }
        index = 0;
      }
    }
  }
}

void parseBinaryPacket(uint8_t* buffer) {
  // Структура пакета:
  // buffer[0] = 0xAA
  // buffer[1..4] = CAN ID
  // buffer[5] = длина
  // buffer[6..13] = данные
  // buffer[14] = 0x55

  uint32_t id = (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4];
  uint8_t len = buffer[5];
  if (len > 8) len = 8;

  bool ext = (id <= 0x7FF) ? 0 : 1;    // is extended
  can.sendCan1(id, ext, len, buffer);  // отправить
  can.sendCan2(id, ext, len, buffer);  // отправить
}
