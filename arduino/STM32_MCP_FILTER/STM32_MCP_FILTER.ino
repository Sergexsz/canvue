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
}

 