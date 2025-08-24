//**************************************************************//
//  Name    : Can Bus LIB.                                      //
//  Author  : SERGEY KHRAPIN                                    //
//  Version : 1.0                                               //
//  Notes   : Honda Canbus controller                           //
//          :                                                   //
//****************************************************************

#include "can.h"
#include <SPI.h>
#include "mcp_can.h"
#include "setup.h"


MCP_CAN CAN_1(CAN1_CS);  // Объект для работы с MCP2515
MCP_CAN CAN_2(CAN2_CS);  // Объект для работы с MCP2515

bool CanBus::begin() {

  pinMode(CAN1_AU_STB, OUTPUT);
  pinMode(CAN1_AU_ENB, OUTPUT);
  pinMode(CAN2_AU_STB, OUTPUT);
  pinMode(CAN2_AU_ENB, OUTPUT);

  connectCAN1();
  connectCAN2();
  return can1_active && can2_active;
}

void CanBus::connectCAN1() {
  if (!use_can1) {
    return;
  }
  uint8_t initResult = CAN_1.begin(MCP_STDEXT, CAN1_SPEED, CAN1_Q);
  if (initResult == CAN_OK) {
    can1_active = true;
    delay(50);  // Пауза перед конфигурацией фильтров

    // // Установить маски: учитывать все 29 бит
    // CAN_1.init_Mask(0, 1, 0x1FFFFFFF);
    // CAN_1.init_Mask(1, 1, 0x1FFFFFFF);

    // // Установить фильтры: разрешить только ID 0x92F85450
    // CAN_1.init_Filt(0, 1, SECURITY);
    // CAN_1.init_Filt(1, 1, RBOX);
    // CAN_1.init_Filt(2, 1, DRIVERBOX);  // блокирующие
    // CAN_1.init_Filt(3, 1, 0x00000000);
    // CAN_1.init_Filt(4, 1, 0x00000000);
    // CAN_1.init_Filt(5, 1, 0x00000000);

    CAN_1.setMode(MCP_NORMAL);  // Set operation mode to normal so the MCP2515 sends acks to received data.
    standByModule(true, 1);     // activate module
    control(false, 1);
  } else {
    can1_active = false;
  }
}

void CanBus::connectCAN2() {
  if (!use_can2) {
    return;
  }
  uint8_t initResult = CAN_2.begin(MCP_STDEXT, CAN2_SPEED, CAN2_Q);
  if (initResult == CAN_OK) {
    can2_active = true;
    delay(50);  // Пауза перед конфигурацией фильтров

    // CAN_2.init_Mask(0, 0, 0x07FF0000);  // Init first mask...
    // CAN_2.init_Mask(1, 0, 0x07FF0000);  // Init second mask...

    // CAN_2.init_Filt(0, 0, FDASH << 16);  // Init first filter...
    // CAN_2.init_Filt(1, 0, VSA << 16);    // Init second filter...


    CAN_2.setMode(MCP_NORMAL);
    standByModule(true, 2);  // activate module
    control(false, 2);
  } else {
    can2_active = false;
  }
}

// будит обе шины
void CanBus::wakeupCan() {
  if (is_can_sleep) {
    standByModule(false, 1);
    control(true, 1);
    CAN_1.sendMsgBuf(0x000000FF, 1, 0, {});
    delay(10);
    standByModule(true, 1);
    control(false, 1);
    //     Serial.println("wake up success done");
    standByModule(false, 2);
    control(true, 2);
    CAN_2.sendMsgBuf(0x000000FF, 1, 0, {});
    delay(10);
    standByModule(true, 2);
    control(false, 2);
  }
}

// переключение для AU
void CanBus::standByModule(bool state, int can) {
  int aunstb = can == 1 ? CAN1_AU_STB : CAN2_AU_STB;
  /**
     Stand-by control:
     high = normal and high-speed mode;
     low = sleep and wake-up mode
  */
  if (state) {
    digitalWrite(aunstb, HIGH);
  } else {
    digitalWrite(aunstb, LOW);
  }
}
// переключение для AU
void CanBus::control(bool state, int can) {
  int aumode = can == 1 ? CAN1_AU_ENB : CAN2_AU_ENB;
  /**
    Enable control:
    high = normal and wake-up  mode;
    low = sleep and high-speed mode
  */
  if (state) {
    digitalWrite(aumode, HIGH);
  } else {
    digitalWrite(aumode, LOW);
  }
}

void CanBus::sendCan1(int id, bool ext, int dlc, byte *buf) {
  if (!use_can1) {
    return;
  }
  wakeupCan();
  CAN_1.sendMsgBuf(id, ext, dlc, buf);
}

void CanBus::sendCan2(int id, bool ext, int dlc, byte *buf) {
  if (!use_can2) {
    return;
  }
  wakeupCan();
  CAN_2.sendMsgBuf(id, ext, dlc, buf);
}

void CanBus::readData() {

  // проверка на сон в шине (нет пакетов уже долго)
  if (millis() - sleepTimer > sleepDuration) {
    if (!is_can_sleep) {
      Serial.println("Can is sleeping now...zzzz");
      is_can_sleep = true;
    }
  }
  if (use_can1) {
    if (!can1_active) {
      Serial.println("connectCAN 1...");
      connectCAN1();
    }
    if (CAN_1.checkReceive() == CAN_MSGAVAIL) {
      if (CAN_1.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
        processData();
      }
    }
  }

  if (use_can2) {
    if (!can2_active) {
      Serial.println("connectCAN 2...");
      connectCAN2();
    }
    if (CAN_2.checkReceive() == CAN_MSGAVAIL) {
      if (CAN_2.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
        processData();
      }
    }
  }
}

// обработка сообщения в шине
void CanBus::processData() {
  sleepTimer = millis();  //сброс таймера сна
  is_can_sleep = false;   // сброс статуса сна

  if (converter) {
    Serial.print("<--");
    Serial.print(rxId, HEX);  // print ID
    Serial.print("[");
    Serial.print(len, HEX);  // print DLC
    Serial.print("]");
    for (int i = 0; i < len; i++) {  // print the data
      Serial.print(".");
      Serial.print(rxBuf[i], HEX);
    }
    Serial.println();
    // тут можно обработку сделать
  }
}


void CanBus::handleVIN() {
  // адрес 40С
  // данные
  // 00 00 00 00 04 00 00 0C
  // 01 4A 48 4D 43 4C 39 01 - 1 часть
  // 02 36 34 30 38 43 32 04 - 2 часть
  // 03 31 32 39 31 36 00 0B - 3 часть

  switch (rxBuf[0]) {
    case 0x01:
      memcpy(&vin[0], &rxBuf[1], 6);  // байты с 1 по 6 (индексы 1–6)
      break;

    case 0x02:
      memcpy(&vin[6], &rxBuf[1], 6);
      break;

    case 0x03:
      memcpy(&vin[12], &rxBuf[1], 5);  // Последний байт — 00 (терминатор)
      vin[17] = '\0';                  // Явный нуль-терминатор
      vinReady = true;
      break;
  }

  if (vinReady) {
    Serial.print("VIN: ");
    Serial.println(vin);
    vinReady = false;  // Чтобы напечатать только один раз
  }
}

void CanBus::handleOdometer() {
  // пробег в приборке
  //1F4	8	0	96	0	[6	54	53]	0
  //пробег 06 54 53 = 414803
  uint32_t odometer = ((uint32_t)rxBuf[5] << 16) | ((uint32_t)rxBuf[6] << 8) | rxBuf[7];
}




CanBus can;
