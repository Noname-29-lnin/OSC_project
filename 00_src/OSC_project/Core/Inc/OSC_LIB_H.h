/**************************************************************************//**
 * @file     OSC_LIB_H.h
 * @brief    
 * @version  V1
 * @date     
 ******************************************************************************/
 #ifndef OSC_LIB_H_
 #define OSC_LIB_H_

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "i2c-lcd.h"
#include "main.h"

// Dinh nghia ham read and write
#define __rw volatile
#define __r  volatile
#define __w  const volatile
	
// Dinh nghia cau lenh
#define cbi(reg, flag) reg &= ~(1<<flag)
#define sbi(reg, flag) reg |=  (1<<flag)

#define ADC_BUFFER_SIZE 1000
#define SAMPLE_TIME_FREQ 1000 // 1s

/**
  * @brief  Flags for controlling the oscilloscope
  */ 
// Ð?nh nghia c?u trúc
typedef struct {
    __rw uint8_t SETUP : 1;  /**< Setup status flag (1 bit) */
    __rw uint8_t MODE  : 1;  /**< Mode selection flag (1 bit) */
    __rw uint8_t START : 1;  /**< Start measurement flag (1 bit) */
    __rw uint8_t STOP  : 1;  /**< Stop measurement flag (1 bit) */
    __rw uint8_t AC    : 1;  /**< AC coupling flag (1 bit) */
    __rw uint8_t DC    : 1;  /**< DC coupling flag (1 bit) */
    __rw uint8_t ERROR : 1;  /**< Error flag (1 bit) */
		__rw uint8_t TIMER2: 1;
	  __rw uint8_t TIMER3: 1;
    __rw uint8_t UNUSED: 1;  /**< Unused bit for padding */
} OSC_FLAG_s;

/**
  * @brief  Measured values of the oscilloscope
  */ 
typedef struct { // VALUE
    __rw uint16_t RANGE;      /**< Measurement range */
    __rw uint32_t HEXAC;     /**< Hexadecimal representation of measured data form AC*/
		__rw uint32_t HEXDC;     /**< Hexadecimal representation of measured data form DC*/
    __rw uint32_t VMAX;      /**< Maximum peak value */
    __rw uint32_t VMIN;      /**< Minimum peak value */
    __rw uint32_t ZERO;      /**< Zero crossing point */
    __rw float VRMS;         /**< RMS voltage (Root Mean Square) */
    __rw float VPP;          /**< Peak-to-Peak voltage */
    __rw float FREQ;         /**< Frequency of the signal in Hz */
    __rw float VDC;          /**< DC voltage */
} OSC_VALUE_s;

/**
  * @brief  Status indicators for the oscilloscope
  */ 
typedef enum __OSC_STATUS_e { // STATUS
		STATUS_IDLE = 0U,
    STATUS_SETUP, /**< Setup mode status */
    STATUS_MODE,  /**< Mode selection status */
    STATUS_START, /**< Measurement started */
    STATUS_STOP,  /**< Measurement stopped */
} OSC_STATUS_e;

/**
  * @brief  Measurement range settings for the oscilloscope
  */ 
typedef enum __OSC_RANGE_e { // Range
    X1 = 1,   /**< 1x range */
    X8 = 5,   /**< 8x range */
    X10 = 10, /**< 10x range */
    X20 = 20, /**< 20x range */
} OSC_RANGE_e;

/**
  * @brief  Temporary variables for storing intermediate data
  */ 
	typedef struct __OSC_TEMP_s { 
			uint16_t RANGE;       /**< Current measurement range */
			uint32_t VMAX;				/**< Vol max when measure AC */
			uint32_t VMIN;				/**< Vol min when measure AC */
			uint32_t ZERO; 				/**< Zero-crossing count */
			float VPP;            /**< Peak-to-peak voltage */
			float FREQ;           /**< Frequency */
			float VDC;            /**< DC voltage */
	} OSC_TEMP_s;

// Khai báo các bi?n v?i extern

extern OSC_FLAG_s FLAG;
extern OSC_VALUE_s VALUE;
extern OSC_TEMP_s TEMP;
extern OSC_STATUS_e STATUS;
extern volatile uint8_t status;
extern uint16_t OSC_Buf[ADC_BUFFER_SIZE];
	
// Cac ham xu ly trong Main
	void OSC_Init(void);
	void HandleSetup(void);
	void HandleMode(void);
	void HandleStart(void);
	void HandleStop(void);
//	void HandleError(void);
// Cac ham xu ly tren LCD
	void UpdateLCDStartAC(void);
		void UpdateACValues(void);
	void UpdateLCDStartDC(void);
		void UpdateDCValues(void);
	void UpdateLCDStopAC(void);
		void UpdateLCDStopDC(void);
	
// Cac ham hien thi len LCD
	void performSetup(void);
	void performMode(void);
		void performModeDc(void);
		void performModeAc(void);
		void performModeUnkown(void);
		void UpdateRangeDisplay(uint16_t GPIO_Pin);
	void performStart(void);
	void performStop(void);
	void performUnkown(void);
// Cac ham xu ly trong ngat
	void HandleButtonSetup(void);
	void HandleButtonMode(void);
	void HandleButtonStart(void);
//void performMeasurement(); //Perform measurement
float ValueDc(uint32_t HEX, uint16_t Range);
float ValueVac(uint32_t HEX, uint16_t Range);
float ValueVpp(uint32_t Vmax, uint32_t Vmin, uint16_t Range);
float ValueVrms(uint32_t HEX, uint32_t Count, uint16_t Range);
float ValueFreq(uint32_t Count, float Time);
 #endif