#include <SPI.h>
#include <mcp2515.h>

// frames
struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsgTemp; 

MCP2515 mcp2515(9); //cs pin bt boart 4
//MCP2515 mcp2515(10); //cs pin bt boart 4.1

// au pins:
int aunstb = 4;
int aumode = 10; // 4.1 (10 for 4)


bool headlights = true; // головной свет
bool mpcs_active = false; // эмултятор блока водителя легенды
bool show_out = false; // показывать исходящие
bool show_in = false; // показывать входящие
bool autoLock = true; // автозапирание
bool is_parking = true; // сейчас в паркинге
uint32_t sniffId = false; // указать какой конкретно айди сканируем
bool can_sleep = true; // шина спит
unsigned long sleepTimer;
int sleepDuration = 1000;

unsigned long sendTimer;

//remote starter data
unsigned long rsBlinkTimer;
int rsBlinkDuration = 4000;
bool rs_active = false; // автозапуск активирован
bool doors_closed = false; // двери заперты
uint32_t deviceList[8]; 

bool dash_emulate = true; // emulate dash for DPMS
int  dash_emulate_counter = 0; // counter to scale times

void setup() {
  Serial.begin(115200);

  pinMode(aumode, OUTPUT);
  pinMode(aunstb, OUTPUT);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_33KBPS, MCP_16MHZ);

  //mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
  mcp2515.setNormalMode();

  standByModule(true); // activate module

  Serial.println("Honda Can shiffer");
  Serial.println("ID  DLC   DATA");

  canMsg1.can_id  = 0x8AF81111;
  canMsg1.can_dlc = 0;

  //8EF81390 [4] 0 0 1 0 - мигнуть поворотниками 1 раз
  //8EF81690 [1] 1 - запирает маишну на охрану
  sendTimer = millis();

}

void loop() {

  if (millis() - sleepTimer > sleepDuration) {
    if (!can_sleep)
      Serial.println("Can is sleeping now...zzzz");
    can_sleep = true;
  }

  /**
     removeStarterBlinks
  */
  if (millis() - rsBlinkTimer > rsBlinkDuration) {
    rsBlinkTimer = millis();
    if (rs_active) {
      blinkArrows(0x02); // blink twice
    }
  }

  /**
     KEYS
  */
  if (Serial.available() > 0) {
    // read the incoming byte:
    char incomingByte = Serial.read();
    switch (incomingByte) {
      case ('?'): {
          Serial.println(" h - headlights On");
          Serial.println(" b - h-beam flash 1 time");
          Serial.println(" w - weakup event");
          Serial.println(" s - mpcs emu toggle");
          Serial.println(" d - show/hide outgoing");
          Serial.println(" e - show/hide incoming");
          Serial.println(" o - open window");
          Serial.println(" c - close window");
          Serial.println(" a - arrows blink");

          Serial.println(" l - Lock");
          Serial.println(" L - Unlock");
          Serial.println(" T - Trunk unlock");
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

      case ('h'): {
          headlights = !headlights;
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
      case ('b'): {
          highBeamFlash();
          Serial.println("flashed h-beam");
          break;
        }
      case ('m'): {
          mpcs_active = !mpcs_active;
          //
          break;
        }

      case ('o'): {
          canMsgTemp.can_id  = 0x96F13010; // HDS ID
          canMsgTemp.can_dlc = 8;
          canMsgTemp.data[0] = 0x30;
          canMsgTemp.data[1] = 0x02;
          canMsgTemp.data[2] = 0x03;
          canMsgTemp.data[3] = 0x00;
          canMsgTemp.data[4] = 0x00;
          canMsgTemp.data[5] = 0x00;
          canMsgTemp.data[6] = 0x00;
          canMsgTemp.data[7] = 0x00;
          sendMessage();

          Serial.println("Open windows");
          //
          break;
      } case ('c'): {
          canMsgTemp.can_id  = 0x96F13010; // HDS ID
          canMsgTemp.can_dlc = 8;
          canMsgTemp.data[0] = 0x30;
          canMsgTemp.data[1] = 0x01;
          canMsgTemp.data[2] = 0x03;
          canMsgTemp.data[3] = 0x00;
          canMsgTemp.data[4] = 0x00;
          canMsgTemp.data[5] = 0x00;
          canMsgTemp.data[6] = 0x00;
          canMsgTemp.data[7] = 0x00;
          sendMessage();

          Serial.println("Close window");
          //
          break;
        }

      case ('l'): {
          smartLock(true);
          Serial.println("Lock doors");
          //
          break;
        }
      case ('L'): {
          smartLock(false);
          Serial.println("Unlock doors");
          //
          break;
        }
      case ('T'): {
          smartTrunk();
          Serial.println("Unlock trunk");
          //
          break;
        }
      case ('a'): {
          blinkArrows(0x04);
          Serial.println("blinkArrows");
          //
          break;
        }

      case ('w'): {
          Serial.println("write outgoing");

          String readString;
          while (Serial.available())
          {
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
     READ BCAN
  */
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    sleepTimer = millis();
    can_sleep = false;

    //    rsCheckActive(); // check remote starter active
    //    autoLockCheck(); // auto lock doors

    //    helloflash();

    if (show_in) {
      // filter
      if (sniffId && (sniffId != canMsg.can_id))  return;
      // or print
      printIncoming();
    }
  } else {

    /**
       SEND BCAN BY TIMER
    */

    if (millis() - sendTimer > 100) {
      // every 100ms check
      sendTimer = millis();

      /**
        SEND
      */
      if (headlights) {
        canMsgTemp.can_id  = 0x8AF81111;
        canMsgTemp.can_dlc = 2;
        canMsgTemp.data[0] = 0x42;
        canMsgTemp.data[1] = 0xC0;
        sendMessage();
      }
      if (mpcs_active) {
        emulateMPCS();
      }

      if (dash_emulate) {
         emulateDashBoard();
      }
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
    Serial.println("wake up success done");
  }
}

void emulateDashBoard (){
 // 2 PCS
  // 92F85150 -> 2 -> 40 02        (300ms)
  // 92F85050 -> 6 -> 0 0 0 80 0 0 (100ms)

   dash_emulate_counter++; // update count + 

  // every 3 count
  if(dash_emulate_counter == 3){
    canMsgTemp.can_id  = 0x92F85150;
    canMsgTemp.can_dlc = 2;
    canMsgTemp.data[0] = 0x40;
    canMsgTemp.data[1] = 0x02;
    sendMessage();
    dash_emulate_counter = 0; // reset counter
  }

  //every count
  canMsgTemp.can_id  = 0x92F85050;
  canMsgTemp.can_dlc = 6;
  canMsgTemp.data[0] = 0x00;
  canMsgTemp.data[1] = 0x00;
  canMsgTemp.data[2] = 0x00;
  canMsgTemp.data[3] = 0x80;
  canMsgTemp.data[4] = 0x00;
  canMsgTemp.data[5] = 0x00;
  sendMessage();

}

void emulateMPCS() {
  /**
     Эмулирует наличие блока управления в двери водителя
     0x92F83035  1 0F
     0X92F83135
     0X92F83235

  */

  canMsgTemp.can_id  = 0x92F83035;
  canMsgTemp.can_dlc = 1;
  canMsgTemp.data[0] = 0x00;
  sendMessage();

  canMsgTemp.can_id  = 0X92F83135;
  canMsgTemp.can_dlc = 0;
  sendMessage();

  canMsgTemp.can_id  = 0X92F83235;
  canMsgTemp.can_dlc = 0;
  sendMessage();


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

void driverBlockLock(bool state) {
  canMsgTemp.can_id  = 0x92F83130;
  canMsgTemp.can_dlc = 1;
  canMsgTemp.data[0] = state ? 0x10 : 0x20;
  sendMessage();
}


void smartBlockLock(bool state) {
  canMsgTemp.can_id  = 0x8EF81690;
  canMsgTemp.can_dlc = 1;
  canMsgTemp.data[0] = state ? 0x01 : 0x02;
  sendMessage();
}

void smartLock(bool state) {
  canMsgTemp.can_id  = 0x8EF81290;
  canMsgTemp.can_dlc = 1;
  canMsgTemp.data[0] = state ? 0x01 : 0x02;
  sendMessage();
}

void smartTrunk() {
  canMsgTemp.can_id  = 0x8EF81290;
  canMsgTemp.can_dlc = 1;
  canMsgTemp.data[0] = 0x80;
  sendMessage();
}

void blinkArrows(byte count) {
  // 1-4 times
  canMsgTemp.can_id  = 0x8EF81390;
  canMsgTemp.can_dlc = 4;
  canMsgTemp.data[0] = 0x00;
  canMsgTemp.data[1] = 0x00;
  canMsgTemp.data[2] = count; //сколько раз надо мигнуть
  canMsgTemp.data[3] = 0x00;
  sendMessage();

}


void rsCheckActive() {
  if (0x92F83210 == canMsg.can_id) {
    doors_closed = canMsg.data[0] == 0x0E ? false : true;
  }
  if (0x8EF89498 == canMsg.can_id) {
    if (canMsg.data[2] != 0x00) {
      if (!rs_active) {
        Serial.println("Remote starter activated");
        Serial.println(canMsg.data[2], HEX);

        if (!doors_closed) {
          // close the doors
          smartBlockLock(true);
        }
      }
      rs_active = true;
    } else {
      if (rs_active) {
        Serial.println("remote starter stop");
      }
      rs_active = false;
    }
  }
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


void highBeamFlash() {
  canMsgTemp.can_id  = 0x8AF87070;
  canMsgTemp.can_dlc = 2;
  canMsgTemp.data[0] = 0x02;
  canMsgTemp.data[1] = 0x00;
  sendMessage();
}

void helloflash() {
  if (canMsg.can_id == 0x8EF81290 && canMsg.data[0] == 0x02) {
    highBeamFlash(); delay(500); highBeamFlash();
  }
}

void autoLockCheck() {
  if (autoLock) { 
    if (0x92F83210 == canMsg.can_id) {
      // is locked
      doors_closed = canMsg.data[0] == 0x0E ? false : true;
    }
    if (0x92F85050 == canMsg.can_id) {
      // get speed
      if (!doors_closed && (canMsg.data[0] > 0x0F)) {
        driverBlockLock(true); // close doors
      }
    }
    //    if (0x92F85150 == canMsg.can_id) {
    //      // get parking state
    //      if (canMsg.data[0] == 0x40 ) {
    //        if (!is_parking) { // if parking again
    //          is_parking = true;
    //          if (doors_closed)
    //            driverBlockLock(false); // open doors
    //        }
    //      }else{
    //        if(canMsg.data[0] != 0x00){
    //                  is_parking = false; // set no parking
    //        }
    //      }
    //    }
  }
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
