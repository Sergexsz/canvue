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


MCP_CAN CANB(CAN1_CS);  // Объект для работы с MCP2515
MCP_CAN CANF(CAN2_CS);  // Объект для работы с MCP2515

bool CanBus::begin() {
  connectCAN1();
  connectCAN2();
  return can1_active && can2_active;
}

void CanBus::connectCAN1() {
  uint8_t initResult = CANB.begin(MCP_STDEXT, CAN1_SPEED, MCP_16MHZ);
  if (initResult == CAN_OK) {
    can1_active = true;
    delay(50);  // Пауза перед конфигурацией фильтров

    // // Установить маски: учитывать все 29 бит
    // CANB.init_Mask(0, 1, 0x1FFFFFFF);
    // CANB.init_Mask(1, 1, 0x1FFFFFFF);

    // // Установить фильтры: разрешить только ID 0x92F85450
    // CANB.init_Filt(0, 1, SECURITY);
    // CANB.init_Filt(1, 1, RBOX);
    // CANB.init_Filt(2, 1, DRIVERBOX);  // блокирующие
    // CANB.init_Filt(3, 1, 0x00000000);
    // CANB.init_Filt(4, 1, 0x00000000);
    // CANB.init_Filt(5, 1, 0x00000000);

    CANB.setMode(MCP_NORMAL);  // Set operation mode to normal so the MCP2515 sends acks to received data.
  } else {
    can1_active = false;
  }
}

void CanBus::connectCAN2() {
  uint8_t initResult = CANF.begin(MCP_STDEXT, CAN2_SPEED, MCP_16MHZ);
  if (initResult == CAN_OK) {
    can2_active = true;
    delay(50);  // Пауза перед конфигурацией фильтров

    // CANF.init_Mask(0, 0, 0x07FF0000);  // Init first mask...
    // CANF.init_Mask(1, 0, 0x07FF0000);  // Init second mask...

    // CANF.init_Filt(0, 0, FDASH << 16);  // Init first filter...
    // CANF.init_Filt(1, 0, VSA << 16);    // Init second filter...


    CANF.setMode(MCP_NORMAL);
  } else {
    can2_active = false;
  }
}


void CanBus::sendCan1(int id, bool ext, int dlc, byte *buf) {
  CANB.sendMsgBuf(id, ext, dlc, buf);
}

void CanBus::sendCan2(int id, bool ext, int dlc, byte *buf) {
  CANF.sendMsgBuf(id, ext, dlc, buf);
}

void CanBus::readData() {
  if (!can1_active) {
    Serial.println("connectCAN 1...");
    connectCAN1();
  }
  if (!can2_active) {
    Serial.println("connectCAN 2...");
    connectCAN1();
  }
  if (CANF.checkReceive() == CAN_MSGAVAIL) {
    if (CANF.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      // Read data: len = data length, buf = data byte(s)
      processData();
    }
  }

  if (CANB.checkReceive() == CAN_MSGAVAIL) {
    if (CANB.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      processData();
    }
  }
}

void CanBus::processData() {

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
