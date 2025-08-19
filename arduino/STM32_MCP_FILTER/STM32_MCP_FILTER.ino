#include "can.h"
#include "Android.h"

#define CAN1_CS = PB8;             // 1
#define CAN1_SPEED = CAN_500KBPS;  // скорость 1

#define CAN2_CS = PB9;             // 2
#define CAN2_SPEED = CAN_500KBPS;  // скорость 2



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
}

 