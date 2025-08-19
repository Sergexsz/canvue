//**************************************************************//
//  Name    : CAN LIB                                           //
//  Author  : SERGEY KHRAPIN                                    //
//  Date    : 21 Aor, 2025                                      //
//  Version : 1.0                                               //
//  Notes   : Steering wheel control                            //
//          :                                                   //
//****************************************************************

#ifndef CAN_H
#define CAN_H
#include "Arduino.h"


class CanBus {
private:
  static const uint32_t MPCS = 0x92F83035;       // driver mpcs LEGEND
  static const uint32_t MICU = 0x92F83210;       // micu
  static const uint32_t RBOX = 0x8AF81111;       // relay box
  static const uint32_t DRIVERBOX = 0x92F83010;  // driver box
  static const uint32_t Dashboard = 0x92F85050;  // gauses
  static const uint32_t Brighness = 0x92F85450;  // gauses Brighness
  static const uint32_t RS = 0x8EF89498;         // remote starter
  static const uint32_t SECURITY = 0x92F81010;   // security doors

  //fcan
  static const uint32_t FDASH2 = 0x1F4;  // Dashboard Fast Can
  static const uint32_t FDASH = 0x0D4;   //
  static const uint32_t VSA = 0x090;     //
  static const uint32_t ECUVIN = 0x40C;  // vin number data

  void connectCAN1();
  void connectCAN2();

  void handleVIN();
  void handleOdometer();
  bool vinReady = false;  // Флаг, что VIN собран
public:
  bool debug = false;
  long unsigned int rxId;
  unsigned char ext = 0;
  unsigned char len = 0;
  unsigned char rxBuf[8]; 
  // char msgString[128];  // Array to store serial string
  bool begin();
  void readData();
  void processData(); 

  bool can1_active = true;
  bool can2_active = true;
  bool converter = false;
  char vin[18];  // VIN код максимум 17 символов
};

extern CanBus can;


#endif