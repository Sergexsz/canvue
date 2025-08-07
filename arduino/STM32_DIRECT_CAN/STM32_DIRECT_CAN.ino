#include <assert.h>
#include <Arduino.h>

#define DEBUG 0


uint8_t messageCounter = 0x0;

/* Symbolic names for bit rate of CAN message                                */
typedef enum { CAN_33KBPS,
               CAN_50KBPS,
               CAN_100KBPS,
               CAN_125KBPS,
               CAN_250KBPS,
               CAN_500KBPS,
               CAN_1000KBPS } BITRATE;

/* Real speed for bit rate of CAN message                                    */
uint32_t SPEED[7] = { 33333, 50 * 1000, 100 * 1000, 125 * 1000, 250 * 1000, 500 * 1000, 1000 * 1000 };

/* Symbolic names for formats of CAN message                                 */
typedef enum { STANDARD_FORMAT = 0,
               EXTENDED_FORMAT } CAN_FORMAT;

/* Symbolic names for type of CAN message                                    */
typedef enum { DATA_FRAME = 0,
               REMOTE_FRAME } CAN_FRAME;


typedef struct
{
  uint32_t id;     /* 29 bit identifier                               */
  uint8_t data[8]; /* Data field                                      */
  uint8_t len;     /* Length of data field in bytes                   */
  uint8_t ch;      /* Object channel(Not use)                         */
  uint8_t format;  /* 0 - STANDARD, 1- EXTENDED IDENTIFIER            */
  uint8_t type;    /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
} CAN_msg_t;

typedef struct
{
  uint16_t baud_rate_prescaler;          /// [1 to 1024]
  uint8_t time_segment_1;                /// [1 to 16]
  uint8_t time_segment_2;                /// [1 to 8]
  uint8_t resynchronization_jump_width;  /// [1 to 4] (recommended value is 1)
} CAN_bit_timing_config_t;

#define CAN_STM32_ERROR_UNSUPPORTED_BIT_RATE 1000
#define CAN_STM32_ERROR_MSR_INAK_NOT_SET 1001
#define CAN_STM32_ERROR_MSR_INAK_NOT_CLEARED 1002
#define CAN_STM32_ERROR_UNSUPPORTED_FRAME_FORMAT 1003

/*
 * Calculation of bit timing dependent on peripheral clock rate
 */
int16_t ComputeCANTimings(const uint32_t peripheral_clock_rate,
                          const uint32_t target_bitrate,
                          CAN_bit_timing_config_t* const out_timings) {
  if (target_bitrate < 1000) {
    return -CAN_STM32_ERROR_UNSUPPORTED_BIT_RATE;
  }

  assert(out_timings != NULL);  // NOLINT
  memset(out_timings, 0, sizeof(*out_timings));

  /*
     * Hardware configuration
     */
  static const uint8_t MaxBS1 = 16;
  static const uint8_t MaxBS2 = 8;

  /*
     * Ref. "Automatic Baudrate Detection in CANopen Networks", U. Koppe, MicroControl GmbH & Co. KG
     *      CAN in Automation, 2003
     *
     * According to the source, optimal quanta per bit are:
     *   Bitrate        Optimal Maximum
     *   1000 kbps      8       10
     *   500  kbps      16      17
     *   250  kbps      16      17
     *   125  kbps      16      17
     */
  const uint8_t max_quanta_per_bit = (uint8_t)((target_bitrate >= 1000000) ? 10 : 17);  // NOLINT
  assert(max_quanta_per_bit <= (MaxBS1 + MaxBS2));

  static const uint16_t MaxSamplePointLocationPermill = 900;

  /*
     * Computing (prescaler * BS):
     *   BITRATE = 1 / (PRESCALER * (1 / PCLK) * (1 + BS1 + BS2))       -- See the Reference Manual
     *   BITRATE = PCLK / (PRESCALER * (1 + BS1 + BS2))                 -- Simplified
     * let:
     *   BS = 1 + BS1 + BS2                                             -- Number of time quanta per bit
     *   PRESCALER_BS = PRESCALER * BS
     * ==>
     *   PRESCALER_BS = PCLK / BITRATE
     */
  const uint32_t prescaler_bs = peripheral_clock_rate / target_bitrate;

  /*
     * Searching for such prescaler value so that the number of quanta per bit is highest.
     */
  uint8_t bs1_bs2_sum = (uint8_t)(max_quanta_per_bit - 1);  // NOLINT

  while ((prescaler_bs % (1U + bs1_bs2_sum)) != 0) {
    if (bs1_bs2_sum <= 2) {
      return -CAN_STM32_ERROR_UNSUPPORTED_BIT_RATE;  // No solution
    }
    bs1_bs2_sum--;
  }

  const uint32_t prescaler = prescaler_bs / (1U + bs1_bs2_sum);
  if ((prescaler < 1U) || (prescaler > 1024U)) {
    return -CAN_STM32_ERROR_UNSUPPORTED_BIT_RATE;  // No solution
  }

  /*
     * Now we have a constraint: (BS1 + BS2) == bs1_bs2_sum.
     * We need to find such values so that the sample point is as close as possible to the optimal value,
     * which is 87.5%, which is 7/8.
     *
     *   Solve[(1 + bs1)/(1 + bs1 + bs2) == 7/8, bs2]  (* Where 7/8 is 0.875, the recommended sample point location *)
     *   {{bs2 -> (1 + bs1)/7}}
     *
     * Hence:
     *   bs2 = (1 + bs1) / 7
     *   bs1 = (7 * bs1_bs2_sum - 1) / 8
     *
     * Sample point location can be computed as follows:
     *   Sample point location = (1 + bs1) / (1 + bs1 + bs2)
     *
     * Since the optimal solution is so close to the maximum, we prepare two solutions, and then pick the best one:
     *   - With rounding to nearest
     *   - With rounding to zero
     */
  uint8_t bs1 = (uint8_t)(((7 * bs1_bs2_sum - 1) + 4) / 8);  // Trying rounding to nearest first  // NOLINT
  uint8_t bs2 = (uint8_t)(bs1_bs2_sum - bs1);                // NOLINT
  assert(bs1_bs2_sum > bs1);

  {
    const uint16_t sample_point_permill = (uint16_t)(1000U * (1U + bs1) / (1U + bs1 + bs2));  // NOLINT

    if (sample_point_permill > MaxSamplePointLocationPermill)  // Strictly more!
    {
      bs1 = (uint8_t)((7 * bs1_bs2_sum - 1) / 8);  // Nope, too far; now rounding to zero
      bs2 = (uint8_t)(bs1_bs2_sum - bs1);
    }
  }

  const bool valid = (bs1 >= 1) && (bs1 <= MaxBS1) && (bs2 >= 1) && (bs2 <= MaxBS2);

  /*
     * Final validation
     * Helpful Python:
     * def sample_point_from_btr(x):
     *     assert 0b0011110010000000111111000000000 & x == 0
     *     ts2,ts1,brp = (x>>20)&7, (x>>16)&15, x&511
     *     return (1+ts1+1)/(1+ts1+1+ts2+1)
     */
  if ((target_bitrate != (peripheral_clock_rate / (prescaler * (1U + bs1 + bs2)))) || !valid) {
    // This actually means that the algorithm has a logic error, hence assert(0).
    assert(0);  // NOLINT
    return -CAN_STM32_ERROR_UNSUPPORTED_BIT_RATE;
  }

  out_timings->baud_rate_prescaler = (uint16_t)prescaler;
  out_timings->resynchronization_jump_width = 1;  // One is recommended by UAVCAN, CANOpen, and DeviceNet
  out_timings->time_segment_1 = bs1;
  out_timings->time_segment_2 = bs2;

  if (DEBUG) {
    Serial.print("target_bitrate=");
    Serial.println(target_bitrate);
    Serial.print("peripheral_clock_rate=");
    Serial.println(peripheral_clock_rate);

    Serial.print("timings.baud_rate_prescaler=");
    Serial.println(out_timings->baud_rate_prescaler);
    Serial.print("timings.time_segment_1=");
    Serial.println(out_timings->time_segment_1);
    Serial.print("timings.time_segment_2=");
    Serial.println(out_timings->time_segment_2);
    Serial.print("timings.resynchronization_jump_width=");
    Serial.println(out_timings->resynchronization_jump_width);
  }
  return 0;
}

/**
 * Print registers.
*/
void printRegister(const char* buf, uint32_t reg) {
  if (DEBUG == 0) return;
  Serial.print(buf);
  Serial.print(reg, HEX);
  Serial.println();
}

/**
 * Initializes the CAN filter registers.
 *
 * @preconditions   - This register can be written only when the filter initialization mode is set (FINIT=1) in the CAN_FMR register.
 * @params: index   - Specified filter index. index 27:14 are available in connectivity line devices only.
 * @params: scale   - Select filter scale.
 *                    0: Dual 16-bit scale configuration
 *                    1: Single 32-bit scale configuration
 * @params: mode    - Select filter mode.
 *                    0: Two 32-bit registers of filter bank x are in Identifier Mask mode
 *                    1: Two 32-bit registers of filter bank x are in Identifier List mode
 * @params: fifo    - Select filter assigned.
 *                    0: Filter assigned to FIFO 0
 *                    1: Filter assigned to FIFO 1
 * @params: bank1   - Filter bank register 1
 * @params: bank2   - Filter bank register 2
 *
 */
void CANSetFilter(uint8_t index, uint8_t scale, uint8_t mode, uint8_t fifo, uint32_t bank1, uint32_t bank2) {
  if (index > 27) return;

  CAN1->FA1R &= ~(0x1UL << index);  // Deactivate filter

  if (scale == 0) {
    CAN1->FS1R &= ~(0x1UL << index);  // Set filter to Dual 16-bit scale configuration
  } else {
    CAN1->FS1R |= (0x1UL << index);  // Set filter to single 32 bit configuration
  }
  if (mode == 0) {
    CAN1->FM1R &= ~(0x1UL << index);  // Set filter to Mask mode
  } else {
    CAN1->FM1R |= (0x1UL << index);  // Set filter to List mode
  }

  if (fifo == 0) {
    CAN1->FFA1R &= ~(0x1UL << index);  // Set filter assigned to FIFO 0
  } else {
    CAN1->FFA1R |= (0x1UL << index);  // Set filter assigned to FIFO 1
  }

  CAN1->sFilterRegister[index].FR1 = bank1;  // Set filter bank registers1
  CAN1->sFilterRegister[index].FR2 = bank2;  // Set filter bank registers2

  CAN1->FA1R |= (0x1UL << index);  // Activate filter
}

/**
 * Initializes the CAN controller with specified bit rate.
 *
 * @params: bitrate - Specified bitrate. If this value is not one of the defined constants, bit rate will be defaulted to 125KBS
 * @params: remap   - Select CAN port. 
 *                    =0:CAN_RX mapped to PA11, CAN_TX mapped to PA12
 *                    =1:Not used
 *                    =2:CAN_RX mapped to PB8, CAN_TX mapped to PB9 (not available on 36-pin package)
 *                    =3:CAN_RX mapped to PD0, CAN_TX mapped to PD1 (available on 100-pin and 144-pin package)
 *
 */
bool CANInit(BITRATE bitrate, int remap) {

  // Reference manual
  // https://www.st.com/content/ccc/resource/technical/document/reference_manual/59/b9/ba/7f/11/af/43/d5/CD00171190.pdf/files/CD00171190.pdf/jcr:content/translations/en.CD00171190.pdf

  RCC->APB1ENR |= 0x2000000UL;  // Enable CAN clock
  RCC->APB2ENR |= 0x1UL;        // Enable AFIO clock
  AFIO->MAPR &= 0xFFFF9FFF;     // reset CAN remap
                                // CAN_RX mapped to PA11, CAN_TX mapped to PA12




  if (remap == 0) {



    RCC->APB2ENR |= 0x4UL;  // Enable GPIOA clock
    // GPIOA->CRH &= ~(0xFF000UL);  // Configure PA12(0b0000) and PA11(0b0000)
    //                              // 0b0000
    //                              //   MODE=00(Input mode)
    //                              //   CNF=00(Analog mode)

    // GPIOA->CRH |= 0xB8FFFUL;  // Configure PA12(0b1011) and PA11(0b1000)
    //                           // 0b1011
    //                           //   MODE=11(Output mode, max speed 50 MHz)
    //                           //   CNF=10(Alternate function output Push-pull
    //                           // 0b1000
    //                           //   MODE=00(Input mode)
    //                           //   CNF=10(Input with pull-up / pull-down)


    // Затрагиваем только PA11 и PA12 (биты 16–19 и 12–15)
    GPIOA->CRH &= ~((0xF << 16) | (0xF << 12));  // очистить PA11 и PA12
    GPIOA->CRH |= ((0xB << 16) | (0x8 << 12));   // настроить PA12 (1011),    PA11 (1000)
  }

  if (remap == 2) {
    AFIO->MAPR |= 0x00004000;  // set CAN remap
                               // CAN_RX mapped to PB8, CAN_TX mapped to PB9 (not available on 36-pin package)

    RCC->APB2ENR |= 0x8UL;    // Enable GPIOB clock
    GPIOB->CRH &= ~(0xFFUL);  // Configure PB9(0b0000) and PB8(0b0000)
                              // 0b0000
                              //   MODE=00(Input mode)
                              //   CNF=00(Analog mode)

    GPIOB->CRH |= 0xB8UL;  // Configure PB9(0b1011) and PB8(0b1000)
                           // 0b1011
                           //   MODE=11(Output mode, max speed 50 MHz)
                           //   CNF=10(Alternate function output Push-pull
                           // 0b1000
                           //   MODE=00(Input mode)
                           //   CNF=10(Input with pull-up / pull-down)

    GPIOB->ODR |= 0x1UL << 8;  // PB8 Upll-up
  }

  if (remap == 3) {
    AFIO->MAPR |= 0x00005000;  // set CAN remap
                               // CAN_RX mapped to PD0, CAN_TX mapped to PD1 (available on 100-pin and 144-pin package)

    RCC->APB2ENR |= 0x20UL;   // Enable GPIOD clock
    GPIOD->CRL &= ~(0xFFUL);  // Configure PD1(0b0000) and PD0(0b0000)
                              // 0b0000
                              //   MODE=00(Input mode)
                              //   CNF=00(Analog mode)

    GPIOD->CRH |= 0xB8UL;  // Configure PD1(0b1011) and PD0(0b1000)
                           // 0b1000
                           //   MODE=00(Input mode)
                           //   CNF=10(Input with pull-up / pull-down)
                           // 0b1011
                           //   MODE=11(Output mode, max speed 50 MHz)
                           //   CNF=10(Alternate function output Push-pull

    GPIOD->ODR |= 0x1UL << 0;  // PD0 Upll-up
  }


  CAN1->MCR |= 0x1UL;  // Require CAN1 to Initialization mode
  while (!(CAN1->MSR & 0x1UL))
    ;  // Wait for Initialization mode

  //CAN1->MCR = 0x51UL;                 // Hardware initialization(No automatic retransmission)
  CAN1->MCR = 0x41UL;  // Hardware initialization(With automatic retransmission)

  // Set bit timing register
  CAN_bit_timing_config_t timings;
  // Serial.print("bitrate=");
  // Serial.println(bitrate);
  uint32_t target_bitrate = SPEED[bitrate];
  // Serial.print("target_bitrate=");
  // Serial.println(target_bitrate);
  int result = ComputeCANTimings(HAL_RCC_GetPCLK1Freq(), target_bitrate, &timings);
  // Serial.print("ComputeCANTimings result=");
  // Serial.println(result);

  if (result) {
    Serial.println("Error in CAN timing calculation");
    return false;  // Exit gracefully
  }
  // if (result)
  // while (true)
  // ;
  CAN1->BTR = (((timings.resynchronization_jump_width - 1U) & 3U) << 24U) | (((timings.time_segment_1 - 1U) & 15U) << 16U) | (((timings.time_segment_2 - 1U) & 7U) << 20U) | ((timings.baud_rate_prescaler - 1U) & 1023U);

  // Configure Filters to default values
  CAN1->FMR |= 0x1UL;       // Set to filter initialization mode
  CAN1->FMR &= 0xFFFFC0FF;  // Clear CAN2 start bank

  // bxCAN has 28 filters.
  // These filters are shared by both CAN1 and CAN2.
  // STM32F103 has only CAN1, so all 28 are used for CAN1
  CAN1->FMR |= 0x1C << 8;  // Assign all filters to CAN1

  // Set fileter 0
  // Single 32-bit scale configuration
  // Two 32-bit registers of filter bank x are in Identifier Mask mode
  // Filter assigned to FIFO 0
  // Filter bank register to all 0
  CANSetFilter(0, 1, 0, 0, 0x0UL, 0x0UL);

  CAN1->FMR &= ~(0x1UL);  // Deactivate initialization mode

  uint16_t TimeoutMilliseconds = 1000;
  bool can1 = false;
  CAN1->MCR &= ~(0x1UL);  // Require CAN1 to normal mode

  // Wait for normal mode
  // If the connection is not correct, it will not return to normal mode.
  for (uint16_t wait_ack = 0; wait_ack < TimeoutMilliseconds; wait_ack++) {
    if ((CAN1->MSR & 0x1UL) == 0) {
      can1 = true;
      break;
    }
    delayMicroseconds(1000);
  }
  //Serial.print("can1=");
  //Serial.println(can1);
  if (can1) {
    Serial.println("CAN initialize ok");
  } else {
    Serial.println("CAN initialize fail!!");
    return false;
  }
  return true;
}


#define STM32_CAN_TIR_TXRQ (1U << 0U)  // Bit 0: Transmit Mailbox Request
#define STM32_CAN_RIR_RTR (1U << 1U)   // Bit 1: Remote Transmission Request
#define STM32_CAN_RIR_IDE (1U << 2U)   // Bit 2: Identifier Extension
#define STM32_CAN_TIR_RTR (1U << 1U)   // Bit 1: Remote Transmission Request
#define STM32_CAN_TIR_IDE (1U << 2U)   // Bit 2: Identifier Extension

#define CAN_EXT_ID_MASK 0x1FFFFFFFU
#define CAN_STD_ID_MASK 0x000007FFU

/**
 * Decodes CAN messages from the data registers and populates a 
 * CAN message struct with the data fields.
 * 
 * @preconditions A valid CAN message is received
 * @params CAN_rx_msg - CAN message structure for reception
 * 
 */
void CANReceive(CAN_msg_t* CAN_rx_msg) {
  uint32_t id = CAN1->sFIFOMailBox[0].RIR;
  if ((id & STM32_CAN_RIR_IDE) == 0) {  // Standard frame format
    CAN_rx_msg->format = STANDARD_FORMAT;
    ;
    CAN_rx_msg->id = (CAN_STD_ID_MASK & (id >> 21U));
  } else {  // Extended frame format
    CAN_rx_msg->format = EXTENDED_FORMAT;
    ;
    CAN_rx_msg->id = (CAN_EXT_ID_MASK & (id >> 3U));
  }

  if ((id & STM32_CAN_RIR_RTR) == 0) {  // Data frame
    CAN_rx_msg->type = DATA_FRAME;
  } else {  // Remote frame
    CAN_rx_msg->type = REMOTE_FRAME;
  }

  CAN_rx_msg->len = (CAN1->sFIFOMailBox[0].RDTR) & 0xFUL;

  CAN_rx_msg->data[0] = 0xFFUL & CAN1->sFIFOMailBox[0].RDLR;
  CAN_rx_msg->data[1] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 8);
  CAN_rx_msg->data[2] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 16);
  CAN_rx_msg->data[3] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 24);
  CAN_rx_msg->data[4] = 0xFFUL & CAN1->sFIFOMailBox[0].RDHR;
  CAN_rx_msg->data[5] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 8);
  CAN_rx_msg->data[6] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 16);
  CAN_rx_msg->data[7] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 24);

  // Release FIFO 0 output mailbox.
  // Make the next incoming message available.
  CAN1->RF0R |= 0x20UL;
}

/**
 * Encodes CAN messages using the CAN message struct and populates the 
 * data registers with the sent.
 * 
 * @params CAN_tx_msg - CAN message structure for transmission
 * 
 */
void CANSend(CAN_msg_t* CAN_tx_msg) {
  volatile int count = 0;

  uint32_t out = 0;
  if (CAN_tx_msg->format == EXTENDED_FORMAT) {  // Extended frame format
    out = ((CAN_tx_msg->id & CAN_EXT_ID_MASK) << 3U) | STM32_CAN_TIR_IDE;
  } else {  // Standard frame format
    out = ((CAN_tx_msg->id & CAN_STD_ID_MASK) << 21U);
  }

  // Remote frame
  if (CAN_tx_msg->type == REMOTE_FRAME) {
    out |= STM32_CAN_TIR_RTR;
  }

  CAN1->sTxMailBox[0].TDTR &= ~(0xF);
  CAN1->sTxMailBox[0].TDTR |= CAN_tx_msg->len & 0xFUL;

  CAN1->sTxMailBox[0].TDLR = (((uint32_t)CAN_tx_msg->data[3] << 24) | ((uint32_t)CAN_tx_msg->data[2] << 16) | ((uint32_t)CAN_tx_msg->data[1] << 8) | ((uint32_t)CAN_tx_msg->data[0]));
  CAN1->sTxMailBox[0].TDHR = (((uint32_t)CAN_tx_msg->data[7] << 24) | ((uint32_t)CAN_tx_msg->data[6] << 16) | ((uint32_t)CAN_tx_msg->data[5] << 8) | ((uint32_t)CAN_tx_msg->data[4]));

  // Send Go
  CAN1->sTxMailBox[0].TIR = out | STM32_CAN_TIR_TXRQ;

  // Wait until the mailbox is empty
  while (CAN1->sTxMailBox[0].TIR & 0x1UL && count++ < 1000000)
    ;

  // The mailbox don't becomes empty while loop
  if (CAN1->sTxMailBox[0].TIR & 0x1UL) {
    Serial.println("Send Fail");
    Serial.println(CAN1->ESR);
    Serial.println(CAN1->MSR);
    Serial.println(CAN1->TSR);
  }
}

/**
 * Returns whether there are CAN messages available.
 *
 * @returns If pending CAN messages are in the CAN controller
 *
 */
uint8_t CANMsgAvail(void) {
  // Check for pending FIFO 0 messages
  return CAN1->RF0R & 0x3UL;
}


uint8_t counter = 0;
uint8_t frameLength = 0;
unsigned long previousMillis = 0;  // stores last time output was updated
const long interval = 1000;        // transmission interval (milliseconds)

CAN_msg_t CAN_TX_msg;
CAN_msg_t CAN_RX_msg;


void parseData(const char* input) {
  bool debug = 0;

  // Проверяем, что строка начинается с 'w'
  if (input[0] != 'w') {
    if (debug) {
      Serial.println("Invalid command");
    }
    return;
  }

  // Указатель на строку после 'w'
  const char* ptr = input + 1;

  // Извлекаем ID до первой точки
  char idStr[16];  // Максимум 15 символов для ID
  int i = 0;
  while (ptr[i] != '.' && ptr[i] != '\0') {
    idStr[i] = ptr[i];
    i++;
  }
  idStr[i] = '\0';                              // Завершаем строку
  unsigned long id = strtoul(idStr, NULL, 10);  // Преобразуем строку в число

  ptr += i + 1;  // Перемещаем указатель на часть строки после первой точки

  // Извлекаем количество байт данных до следующей точки
  char byteCountStr[8];
  i = 0;
  while (ptr[i] != '.' && ptr[i] != '\0') {
    byteCountStr[i] = ptr[i];
    i++;
  }
  byteCountStr[i] = '\0';              // Завершаем строку
  int byteCount = atoi(byteCountStr);  // Преобразуем в целое число

  ptr += i + 1;  // Перемещаем указатель на байты данных

  // Извлекаем байты данных
  if (debug) {
    Serial.print("ID: ");
    Serial.println(id, HEX);
  }
  CAN_TX_msg.id = id;

  if (id < 1) {
    return;
  }

  if (id <= 0x7FF) {
    CAN_TX_msg.format = STANDARD_FORMAT;
    byteCount = 8;  // гарантированно не менее 8
    // Стандартный 11-битный ID
  } else {
    CAN_TX_msg.format = EXTENDED_FORMAT;
    // Расширенный 29-битный ID
  }

  if (debug) {
    Serial.print("Byte count: ");
    Serial.println(byteCount, HEX);
  }
  CAN_TX_msg.len = byteCount;

  for (int j = 0; j < byteCount; j++) {
    int byteData = 0;
    i = 0;
    while (ptr[i] != '.' && ptr[i] != '\0') {
      byteData = byteData * 10 + (ptr[i] - '0');  // Чтение цифры как числа
      i++;
    }
    CAN_TX_msg.data[j] = byteData;
    ptr += i + 1;  // Перемещаем указатель на следующий байт
  }

  if (id <= 0x7FF) {
    // заменяем последний байт на рандомный
    CAN_TX_msg.data[7] = random(0x00, 0x100);  // или random(0, 256);
  }

  if (debug) {
    Serial.print("Data bytes: ");
    for (int a = 0; a < 8; a++) {
      Serial.print(CAN_TX_msg.data[a], HEX);
      Serial.print(" ");  // Выводим каждый байт
    }
    Serial.print("format: ");
    Serial.println(CAN_TX_msg.format);
  }


  CANSend(&CAN_TX_msg);
}


void setup() {
  Serial1.begin(115200);

  // bool ret = CANInit(CAN_33KBPS, 2);  // CAN_RX mapped to PB8, CAN_TX mapped to PB9
  // bool ret = CANInit(CAN_33KBPS, 0);  // CAN_RX mapped to PA11, CAN_TX mapped to PA12
  bool ret = CANInit(CAN_500KBPS, 0);  // CAN_RX mapped to PA11, CAN_TX mapped to PA12
  //bool ret = CANInit(CAN_500KBPS, 2);  // CAN_RX mapped to PB8, CAN_TX mapped to PB9
  //bool ret = CANInit(CAN_500KBPS, 3);  // CAN_RX mapped to PD0, CAN_TX mapped to PD1
  //bool ret = CANInit(CAN_1000KBPS, 0);  // CAN_RX mapped to PA11, CAN_TX mapped to PA12
  //bool ret = CANInit(CAN_1000KBPS, 2);  // CAN_RX mapped to PB8, CAN_TX mapped to PB9
  //bool ret = CANInit(CAN_1000KBPS, 3);  // CAN_RX mapped to PD0, CAN_TX mapped to PD1
  if (!ret)
    while (true)
      ;

  Serial1.println("Begin");
}

#define MAX_LINE_LENGTH 100
char serialBuffer[MAX_LINE_LENGTH];
uint8_t bufferIndex = 0;

// собирает входящее сообщение
void checkSerialInputOld() {

  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      serialBuffer[bufferIndex] = '\0'; // завершить строку
      parseData2(serialBuffer); // Разбор строки
      bufferIndex = 0;
    } else {
      if (bufferIndex < MAX_LINE_LENGTH - 1) {
        serialBuffer[bufferIndex++] = c;
      } else {
        bufferIndex = 0; // ошибка — сброс
      }
    }
  }
}

// разбирает входящее сообщение на пакет
void parseData2(char* input) {
  const bool debug = true;

  if (input[0] != 'w') {
    if (debug) Serial.println("Invalid command");
    return;
  }

  // Перемещаем указатель на следующую часть после 'w'
  char* token = strtok(input + 1, ".");

  if (!token) return;
  unsigned long id = strtoul(token, NULL, 10);

  if (id == 0) return;

  CAN_TX_msg.id = id;
  CAN_TX_msg.format = (id <= 0x7FF) ? STANDARD_FORMAT : EXTENDED_FORMAT;

  if (debug) {
    Serial.print("ID: 0x");
    Serial.println(id, HEX);
  }

  // Получаем длину данных
  token = strtok(NULL, ".");
  if (!token) return;

  int byteCount = atoi(token);
  if (byteCount < 1 || byteCount > 8) return;

  CAN_TX_msg.len = byteCount;

  // Чтение data[]
  for (int i = 0; i < byteCount; i++) {
    token = strtok(NULL, ".");
    if (!token) break;

    CAN_TX_msg.data[i] = (uint8_t)atoi(token);
  }

  // Дополнение пустых байтов нулями
  for (int i = byteCount; i < 8; i++) {
    CAN_TX_msg.data[i] = 0;
  }

  // Если стандартный ID — заменяем последний байт на случайный
  if (CAN_TX_msg.format == STANDARD_FORMAT) {
    CAN_TX_msg.data[7] = random(0, 256);
  }

  if (debug) {
    Serial.print("Data: ");
    for (int i = 0; i < 8; i++) {
      Serial.print(CAN_TX_msg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.print("  Format: ");
    Serial.println(CAN_TX_msg.format == STANDARD_FORMAT ? "STD" : "EXT");
  }

  // Отправка CAN
  CANSend(&CAN_TX_msg);
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

  CAN_TX_msg.id = id;
  CAN_TX_msg.format = (id <= 0x7FF) ? STANDARD_FORMAT : EXTENDED_FORMAT;
  CAN_TX_msg.len = len;

  memcpy(CAN_TX_msg.data, &buffer[6], len);

  // Заполнение недостающих байтов нулями
  for (uint8_t i = len; i < 8; i++) {
    CAN_TX_msg.data[i] = 0;
  }

  // Заменяем последний байт, если стандартный ID
  if (CAN_TX_msg.format == STANDARD_FORMAT) {
    CAN_TX_msg.data[7] = random(0, 256);
  }

  CANSend(&CAN_TX_msg);
}

// test message
  //w200.8.0.0.0.0.15.0.0.0


void loop() {

    checkSerialInput(); // проверить входящее

   if (CANMsgAvail()) {
    CANReceive(&CAN_RX_msg);
    printIncoming();
    //  handleTaho();
  }

  // sendSpeed();
}
void printIncoming() {
  Serial1.print("<--");
  Serial1.print(CAN_RX_msg.id, HEX);  // print ID
  Serial1.print("[");
  Serial1.print(CAN_RX_msg.len, HEX);  // print DLC
  Serial1.print("]");
  if (CAN_RX_msg.type == DATA_FRAME) {
    for (int i = 0; i < CAN_RX_msg.len; i++) {  // print the data
      Serial1.print(".");
      Serial1.print(CAN_RX_msg.data[i], HEX);
    }
  }
  Serial1.println();
}

// void sendSpeed() {
//   int speed_kmh = 130;

//   uint16_t scaledSpeed = speed_kmh * 93.5;
//   CAN_TX_msg.id = 0xC8;
//   CAN_TX_msg.len = 8;
//   CAN_TX_msg.format = STANDARD_FORMAT;
//   CAN_TX_msg.data[0] = 0x00;
//   CAN_TX_msg.data[1] = 0x00;
//   CAN_TX_msg.data[2] = 0x00;
//   CAN_TX_msg.data[3] = 0x00;
//   CAN_TX_msg.data[4] = (scaledSpeed >> 8) & 0xFF;
//   CAN_TX_msg.data[5] = scaledSpeed & 0xFF;
//   CAN_TX_msg.data[6] = 0x00; 
//   CAN_TX_msg.data[7] = random(0x00, 0x100);

//   CANSend(&CAN_TX_msg);
// }

// void handleTaho() {
//   if (CAN_RX_msg.id == 0x12C) {
//     CAN_TX_msg.id = 0x20C;
//     CAN_TX_msg.len = 8;
//     CAN_TX_msg.format = STANDARD_FORMAT;
//     CAN_TX_msg.data[0] = 0x02;
//     CAN_TX_msg.data[1] = 0x3B;
//     CAN_TX_msg.data[2] = 0x0;
//     CAN_TX_msg.data[3] = 0x0;
//     CAN_TX_msg.data[4] = CAN_RX_msg.data[4];
//     CAN_TX_msg.data[5] = CAN_RX_msg.data[5];
//     CAN_TX_msg.data[6] = CAN_RX_msg.data[6];
//     CAN_TX_msg.data[7] = CAN_RX_msg.data[7];

//     CANSend(&CAN_TX_msg);
//   }
// }


