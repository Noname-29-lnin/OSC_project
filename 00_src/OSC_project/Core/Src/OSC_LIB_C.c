#include "OSC_LIB_H.h"

static void OSC_DelayInit(void);
static void OSC_DelayUs(uint32_t us);
static void OSC_DelayMs(uint32_t ms);
// ============= Cac ham xy ly chuong trinh trong Main =============

void OSC_Init(void){
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC Ready");
	HD44780_SetCursor(0, 1);
	HD44780_PrintStr("   ...SETUP OSC");
	
	//OSC_DelayInit();
	status = STATUS_IDLE;
}

void HandleSetup(void){
	FLAG.SETUP  = 0;
	FLAG.ERROR  = 0;
	FLAG.UNUSED = 0;
	while(FLAG.MODE != 1){} // cho co MODE bat
}

void HandleMode(void){
//	FLAG.MODE = 0;
	while(status == STATUS_MODE){			// Van con o mode MODE
		if(FLAG.AC == 1 && FLAG.DC == 0){
			FLAG.ERROR  = 0;
			performModeAc();
			UpdateRangeDisplay(AC_RANGE_Pin);
		} else if(FLAG.AC == 0 && FLAG.DC == 1){
				FLAG.ERROR  = 0;
				performModeDc();
				UpdateRangeDisplay(DC_RANGE_Pin);
		} else if(FLAG.AC == 0 && FLAG.DC == 0){
				performModeUnkown();
				FLAG.ERROR = 1;
				HAL_Delay(1000);
//				OSC_DelayMs(1000);
				break;
		}
	}
}

//void HandleStart(void){
//	HAL_Delay(500);
////	OSC_DelayMs(500);
//	HD44780_Clear();
//	
//	if (FLAG.AC == 1 && FLAG.DC == 0) {
//		FLAG.ERROR  = 0;
//		HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_RESET);
//		UpdateLCDStartAC();
//			while (status == STATUS_START && FLAG.START == 1 && FLAG.STOP == 0){
//				UpdateACValues();
//				if(FLAG.TIMER2 == 1){ // ngat o 1ms
//					FLAG.TIMER2 = 0;
//					uint16_t adc_zero = 2048;
//				uint16_t threshold = 102;
//			for(int i = 0; i < sizeof(OSC_Buf); i ++){
//				uint16_t adc_value = OSC_Buf[i];
//				if(adc_value > VALUE.VMAX){
//					VALUE.VMAX = adc_value;
//				} else if(adc_value < VALUE.VMIN){
//					VALUE.VMIN = adc_value;
//				}
//				
//				if((adc_value >= (adc_zero - threshold)) && (adc_value <= (adc_zero + threshold))){
//					VALUE.ZERO ++;
//				}
//			}
//				}
//			}	
//	} else if (FLAG.DC == 1 && FLAG.AC == 0) {
//		FLAG.ERROR  = 0;
//		HAL_GPIO_WritePin(MODE_CONT_GPIO_Port, MODE_CONT_Pin, GPIO_PIN_SET);
//		UpdateLCDStartDC();
//			while (status == STATUS_START && FLAG.START == 1 && FLAG.STOP == 0){
//					UpdateDCValues();
//				}
//	} else if(FLAG.DC == 0 && FLAG.AC == 0) {
//			FLAG.ERROR = 1;
//			performUnkown();
//			HAL_Delay(1000);
////			OSC_DelayMs(1000);
//	}
//}

void HandleStop(void){
	HAL_Delay(500);
//	OSC_DelayMs(500);
	HD44780_Clear();
	
	if (FLAG.AC == 1 && FLAG.DC == 0) {
		TEMP.FREQ = VALUE.FREQ;
		TEMP.VPP = VALUE.VPP;
		UpdateLCDStopAC();
	} else if (FLAG.DC == 1 && FLAG.AC == 0) {
		TEMP.VDC = VALUE.VDC;
		UpdateLCDStopDC();
	}
	while (status == STATUS_START && FLAG.START == 0 && FLAG.STOP == 1){}
}

// ============= Cac ham xy ly hien thi =============
/**
	* @brief	Hien thi khi ket qua khi chay che do AC
*/


void UpdateLCDStartAC(void) {
    HD44780_Clear();
    HD44780_SetCursor(0, 0);
    HD44780_PrintStr("VPP:          V");
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("FREQ:        Hz");
}

/**
	* @brief	Hien thi ket qua khi chay che do DC
*/
void UpdateLCDStartDC(void) {
    HD44780_Clear();
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("VDC:          V");
}

/**
	* @brief	Hien thi ket qua khi chay che do AC
	* @note   Can phai khai bao gia tri TEMP dau vao hien thi
*/
void UpdateLCDStopAC(void) {
    HD44780_Clear();
    HD44780_SetCursor(0, 0);
    HD44780_PrintStr("VPP:          V");
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("FREQ:        Hz");
    HD44780_SetCursor(5, 0);
    HD44780_Value(TEMP.VPP);
    HD44780_SetCursor(6, 1);
    HD44780_Value(TEMP.FREQ);
}

/**
	* @brief	Hien thi ket qua khi chay che do DC
	* @note   Can phai khai bao gia tri TEMP dau vao hien thi
*/
void UpdateLCDStopDC(void) {
    HD44780_Clear();
    HD44780_SetCursor(0, 1);
    HD44780_PrintStr("VDC:          V");
    HD44780_SetCursor(5, 1);
    HD44780_Value(TEMP.VDC);
}


/**
	* @brief	Hien thi ket qua khi chay che do AC
	* @param	VALUE.VPP là gia tri Vpp
	*					VALUE.FREQ là gia tri Freq
*/
void UpdateACValues(void) {
    HD44780_SetCursor(5, 0);
    HD44780_Value(VALUE.VPP);
    HD44780_SetCursor(6, 1);
    HD44780_Value(VALUE.FREQ);
}

/**
	* @brief	Hien thi ket qua khi chay che do AC
	* @param	VALUE.VDC là gia tri VDc
*/
void UpdateDCValues(void) {
    HD44780_SetCursor(5, 1);
    HD44780_Value(VALUE.VDC);
}


// ============= Cac ham hien thi =============

/**
	* @brief	Hien thi khi nhan nut SETUP
*/
void performSetup(void){
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC SETUP");
}

/**
	* @brief	Hien thi khi nhan nut MODE
*/
void performMode(void){
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC MODE");
}
	// perform MODE: AC
	void performModeAc(void){
		HD44780_SetCursor(0, 1);
		HD44780_PrintStr("MODE: AC");
	}
	// perform MODE: DC
	void performModeDc(void){
		HD44780_SetCursor(0, 1);
		HD44780_PrintStr("MODE: DC");
	}
	// perform MODE: ?
	void performModeUnkown(void){
		HD44780_SetCursor(0, 1);
		HD44780_PrintStr("MODE: ?          ");
	}
// perform RANGE
void UpdateRangeDisplay(uint16_t GPIO_Pin) {
    if (status == STATUS_MODE) {
        HD44780_SetCursor(13, 1);
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_Pin) == GPIO_PIN_RESET) {
            HD44780_PrintStr("X1 ");
						VALUE.RANGE = X1;
        } else {
            HD44780_PrintStr("X8 ");
						VALUE.RANGE = X8;
        }
    }
}
	
/**
	* @brief	Hien thi khi nhan nut START
*/
void performStart(void){
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC START");
}

/**
	* @brief	Hien thi khi nhan nut STOP
*/
void performStop(void){
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC STOP");
}

/**
	* @brief	Hien thi khi khong nhan dien duoc
*/
void performUnkown(void){
	HD44780_Clear();
	HD44780_SetCursor(0, 0);
	HD44780_PrintStr("OSC UnKnown");
}
// ============= Cac ham xu ly nut nhan =============
/**
	* @brief	Ham su ly hien thi trong ngat khi nhan nut SETUP
*/
void HandleButtonSetup(void){
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
		FLAG.SETUP 	= 1;
		FLAG.MODE 	= 0;
		FLAG.START 	= 0;
		FLAG.STOP 	= 0;
		FLAG.AC 		= 0;
		FLAG.DC 		= 0;
		FLAG.ERROR  = 0;
		FLAG.UNUSED = 0;
		status = STATUS_SETUP;
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
	* @brief	Ham su ly hien thi trong ngat khi nhan nut MODE
*/
void HandleButtonMode(void){
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	FLAG.SETUP  = 0;
	FLAG.MODE 	= 1;
	FLAG.UNUSED = 0;
	
		switch(status){
			case STATUS_SETUP: 	// SETUP -> MODE
					FLAG.AC 	= 1;
					FLAG.DC 	= 0;
					status = STATUS_MODE;
				break;
			case STATUS_MODE:		// MODE  -> MODE
					FLAG.AC 	= !FLAG.AC;
					FLAG.DC 	= !FLAG.DC;
					status = STATUS_MODE;
				break;
			case STATUS_START:	// START -> MODE
					FLAG.AC 	= 1;
					FLAG.DC 	= 0;
					status = STATUS_MODE;
				break;
			default:
					FLAG.UNUSED = 1;
				break;	
		}
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
	* @brief	Ham su ly hien thi trong ngat khi nhan nut START/STOP
*/
void HandleButtonStart(void){
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	FLAG.SETUP = 0;
	FLAG.MODE  = 0;
	FLAG.UNUSED = 0;
	
	switch(status){
		case STATUS_MODE: 	// MODE -> START
				FLAG.START = 1;
				FLAG.STOP  = 0;
				status = STATUS_START;
			break;
		case STATUS_START:	// START -> STOP
				FLAG.START = !FLAG.START;
				FLAG.STOP  = !FLAG.STOP;
				status = STATUS_START;
			break;
		default:
				FLAG.UNUSED = 1;
			break;
	}
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// ============= Cac ham tinh toan gia tri =============
/**
  * @brief  Calculate DC voltage from ADC value.
  * @param  HEX: ADC raw value (0-4095 for 12-bit ADC).
  * @param  Range: Measurement range (scaling factor).
  * @retval Calculated DC voltage.
  */
float ValueDc(uint32_t HEX, uint16_t Range) {
    float Temp = 0.0;
    Temp = (float)(((HEX * 3.3) / 4096.0) * Range);
    return Temp - Temp*0.05;
}

/**
  * @brief  Calculate AC voltage from ADC value.
  * @param  HEX: ADC raw value (0-4095 for 12-bit ADC).
  * @param  Range: Measurement range (scaling factor).
  * @retval Calculated AC voltage.
  */
float ValueVac(uint32_t HEX, uint16_t Range) {
    float Temp = 0.0;
    Temp = (float)(((2 * (HEX * 3.3) / 4096.0) - 3.3) * Range);
    return Temp - Temp*0.16;
}

/**
  * @brief  Calculate Peak-to-Peak voltage.
  * @param  HEX: Not used in the function but part of the structure for compatibility.
  * @param  Vmax: Maximum ADC value observed.
  * @param  Vmin: Minimum ADC value observed.
  * @param  Range: Measurement range (scaling factor).
  * @retval Calculated Peak-to-Peak voltage.
  */
float ValueVpp(uint32_t Vmax, uint32_t Vmin, uint16_t Range) {
    float Temp = 0.0;
    float Temp_Vmax = ValueVac(Vmax, Range);
    float Temp_Vmin = ValueVac(Vmin, Range);

    Temp = Temp_Vmax - Temp_Vmin;
    return Temp;
}

/**
  * @brief  Calculate RMS voltage.
  * @param  HEX: Sum of ADC values.
  * @param  Count: Number of samples.
  * @param  Range: Measurement range (scaling factor).
  * @retval Calculated RMS voltage.
  */
float ValueVrms(uint32_t HEX, uint32_t Count, uint16_t Range) {
    float Temp = 0.0;
    Temp = (float)((HEX * 3.3) / (4096.0 * Count) - 1.65) * (float)(Range * 0.7071);
    return Temp;
}

/**
  * @brief  Calculate frequency based on zero-crossing count.
  * @param  Count: Zero-crossing count (non-zero for valid calculation).
  * @param  Time: Measurement duration in seconds (e.g., 1 second).
  * @retval Calculated frequency (Hz).
  */
float ValueFreq(uint32_t Count, float Time) {
    if (Count == 0 || Time <= 0.0) return 0.0; // Avoid invalid calculation
    return (float)Count / (2.0 * Time); // Calculate frequency
}


static void OSC_DelayInit(void)
{
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
  CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

  DWT->CYCCNT = 0;

  /* 3 NO OPERATION instructions */
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
  __ASM volatile ("NOP");
}

static void OSC_DelayUs(uint32_t us) {
  uint32_t cycles = (SystemCoreClock/1000000L)*us;
  uint32_t start = DWT->CYCCNT;
  volatile uint32_t cnt;

  do
  {
    cnt = DWT->CYCCNT - start;
  } while(cnt < cycles);
}

static void OSC_DelayMs(uint32_t ms){
	uint32_t cycles = (SystemCoreClock / 1000L) * ms;  // Tính s? chu k? cho ms
    uint32_t start = DWT->CYCCNT;
    volatile uint32_t cnt;

    do {
        cnt = DWT->CYCCNT - start;
    } while (cnt < cycles);
}