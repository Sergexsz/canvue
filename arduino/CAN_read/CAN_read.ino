#include <SPI.h>
#include <mcp2515.h>

// frames
struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsgTemp; 

// MCP2515 mcp2515(9); //cs pin bt boart 4
MCP2515 mcp2515(10); //cs pin bt boart 4.1

// au pins:
int aunstb = 4;
int aumode = 10; // 4.1 (10 for 4)


bool show_out = false; // показывать исходящие
bool show_in = true; // показывать входящие
uint32_t sniffId = false; // указать какой конкретно айди сканируем
bool can_sleep = true; // шина спит
unsigned long sleepTimer;
int sleepDuration = 1000;

unsigned long sendTimer;

uint32_t deviceList[8];

void setup() {
  Serial.begin(115200);

  pinMode(aumode, OUTPUT);
  pinMode(aunstb, OUTPUT);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS, MCP_16MHZ);
  // выбрать какой осцилятор стоит
    // mcp2515.setBitrate(CAN_33KBPS, MCP_8MHZ);
  //mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ); // ф-кан 500кб/с
  mcp2515.setNormalMode();

  standByModule(true); // activate module

  canMsg1.can_id  = 0x8AF81111;
  canMsg1.can_dlc = 0;
  sendTimer = millis();
}

void loop() {
  if (millis() - sleepTimer > sleepDuration) {
    if (!can_sleep)
      Serial.println("Can is sleeping now...zzzz");
    can_sleep = true;
  }
  /**
     KEYS
  */
  if (Serial.available() > 0) {
    // read the incoming byte:
    char incomingByte = Serial.read();
    switch (incomingByte) {
      case ('?'): {
          Serial.println(" d - show/hide outgoing");
          Serial.println(" e - show/hide incoming");
          break;
        }
      case ('k'): {
          Serial.println("Enter device ID to sniffId");
          if (Serial.available() > 0) {
            long bytes = Serial.parseInt();
            sniffId = bytes;
            Serial.print("now sniffering only for --->"); Serial.println(sniffId, HEX);
          }
          break;
        }
      case ('u'): {
          wakeup();
          break;
        }
      case ('d'): {
          show_out = !show_out;
          break;
        }
      case ('e'): {
          show_in = !show_in;
          break;
        }
      case ('w'): {
         // читает команду из программы и отправляет в шину
          String readString;
          while (Serial.available()){
            delay(1);
            if (Serial.available() > 0)
            {
              char c = Serial.read();
              readString += c;
            }
          }

          // 2331513105.2.66.192
          String device = getValue(readString, '.', 0);
          String dlc = getValue(readString, '.', 1);
          String b0 = getValue(readString, '.', 2);
          String b1 = getValue(readString, '.', 3);
          Serial.print(F("device="));
          Serial.println(device);
          Serial.print(F("dlc="));
          Serial.println(dlc);
          Serial.print(F("b0="));
          Serial.println(b0);

          canMsgTemp.can_id  = device.toInt();
          canMsgTemp.can_dlc = dlc.toInt();
          canMsgTemp.data[0] = b0.toInt();
          canMsgTemp.data[1] = b1.toInt();
          sendMessage();
          break;
        }
    }
  }


  /**
     READ CAN
  */
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    sleepTimer = millis();
    can_sleep = false;
    if (show_in) {
      // filter
      if (sniffId && (sniffId != canMsg.can_id))  return;
      // or print
      printIncoming();
    }
  } else {
    /**
       SEND CAN BY TIMER
    */
    if (millis() - sendTimer > 100) {
      // every 100ms check
      sendTimer = millis();
    }

  }
}

void standByModule(bool state) {
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

void control(bool state) {
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

void wakeup() {
  if (can_sleep) {
    standByModule(false);
    control(true);
    mcp2515.sendMessage(&canMsg1);
    delay(50);
    standByModule(true);
    control(false);
//     Serial.println("wake up success done");
  }
}


void sendMessage() {
  if (show_out) {
    Serial.print(">> ");
    Serial.println(canMsgTemp.can_id, HEX);
  }
  // wakeup();
  mcp2515.sendMessage(&canMsgTemp);
  delay(10);
}

void getName(byte id) {
  byte mx = id;
}

void printIncoming() {
  //  Serial.print(canMsg.can_id, DEC); // print ID
  Serial.print("<--");
  Serial.print(canMsg.can_id, HEX); // print ID

  Serial.print("[");
  Serial.print(canMsg.can_dlc, HEX); // print DLC
  Serial.print("]");

  for (int i = 0; i < canMsg.can_dlc; i++)  { // print the data
    Serial.print(".");
    Serial.print(canMsg.data[i], HEX);
    //    Serial.print(canMsg.data[i], DEC);
    //    Serial.print(") " );
  }

  Serial.println();
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
