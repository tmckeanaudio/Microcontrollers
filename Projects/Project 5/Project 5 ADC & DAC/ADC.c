#include "ADC.h"
#include "LED.h"
#include "SysTimer.h"
#include "stm32l476xx.h"
#include <stdint.h>

//******************************************************************************************
// STM32L4x6xx Errata sheet
// When the delay between two consecutive ADC conversions is higher than 1 ms the result of 
// the second conversion might be incorrect. The same issue occurs when the delay between the 
// calibration and the first conversion is higher than 1 ms.
// Workaround
// When the delay between two ADC conversions is higher than the above limit, perform two ADC 
// consecutive conversions in single, scan or continuous mode: the first is a dummy conversion 
// of any ADC channel. This conversion should not be taken into account by the application.

//******************************************************************************************
// ADC Wakeup
// By default, the ADC is in deep-power-down mode where its supply is internally switched off
// to reduce the leakage currents.
//******************************************************************************************
void ADC_Wakeup (void) {
	
	int wait_time;
	
	// To start ADC operations, the following sequence should be applied
	// DEEPPWD = 0: ADC not in deep-power down
	// DEEPPWD = 1: ADC in deep-power-down (default reset state)
	if ((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)
		ADC1->CR &= ~ADC_CR_DEEPPWD; // Exit deep power down mode if still in that state
	
	// Enable the ADC internal voltage regulator
	// Before performing any operation such as launching a calibration or enabling the ADC, the ADC
	// voltage regulator must first be enabled and the software must wait for the regulator start-up time.
	ADC1->CR |= ADC_CR_ADVREGEN;	
	
	// Wait for ADC voltage regulator start-up time
	// The software must wait for the startup time of the ADC voltage regulator (T_ADCVREG_STUP) 
	// before launching a calibration or enabling the ADC.
	// T_ADCVREG_STUP = 20 us
	wait_time = 20 * (80000000 / 1000000);
	while(wait_time != 0) {
		wait_time--;
	}   
}

//******************************************************************************************
// 	ADC Common Configuration
//******************************************************************************************	
void ADC_Common_Configuration(){
	
	// I/O analog switches voltage booster.
	// The I/O analog switches resistance increases when the VDDA voltage is too low. This
	// requires to have the sampling time adapted accordingly (cf datasheet for electrical
	// characteristics). This resistance can be minimized at low VDDA by enabling an internal
	// voltage booster with BOOSTEN bit in the SYSCFG_CFGR1 register.
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN;
	
	// V_REFINT enable. To enable the conversion of internal channels.
	ADC123_COMMON->CCR |= ADC_CCR_VREFEN;  //Page 530 of Reference manual
	
	// ADC Clock Source: System Clock, PLLSAI1, PLLSAI2
	// Maximum ADC Clock: 80 MHz
	
	// ADC prescaler to select the frequency of the clock to the ADC
	ADC123_COMMON->CCR &= ~ADC_CCR_PRESC;   // 0000: input ADC clock not divided
	
	// ADC clock mode
	//   00: CK_ADCx (x=123) (Asynchronous clock mode),
	//   01: HCLK/1 (Synchronous clock mode).
	//   10: HCLK/2 (Synchronous clock mode)
	//   11: HCLK/4 (Synchronous clock mode)	 
	ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;  // HCLK = 80MHz
	ADC123_COMMON->CCR |=  ADC_CCR_CKMODE_0; //01

	// Independent Mode
	ADC123_COMMON->CCR &= ~ADC_CCR_DUAL;
	ADC123_COMMON->CCR |= 6U;  // 00110: Regular simultaneous mode only
}


//******************************************************************************************
// 	ADC Pin Initialization
//  PA1 (ADC12_IN6), PA2 (ADC12_IN7)
//******************************************************************************************
void ADC_Pin_Init(void){	
	// Enable the clock of GPIO Port A
	RCC->AHB2ENR |=   RCC_AHB2ENR_GPIOAEN;
		
	// GPIO Mode: Input(00), Output(01), AlterFunc(10), Analog(11, reset)
	// Configure PA1 (ADC12_IN6), PA2 (ADC12_IN7) as Analog
	//GPIOA->MODER |=  3U<<(2*1) | 3U<<(2*2);  // Mode 11 = Analog
	GPIOA->MODER |=  3U<<(2*1);  // Mode 11 = Analog
	
	// GPIO Push-Pull: No pull-up, pull-down (00), Pull-up (01), Pull-down (10), Reserved (11)
	GPIOA->PUPDR &= ~( 3U<<(2*1) | 3U<<(2*2)); // No pull-up, no pull-down
	
	// GPIO port analog switch control register (ASCR)
	// 0: Disconnect analog switch to the ADC input (reset state)
	// 1: Connect analog switch to the ADC input
	//GPIOA->ASCR |= GPIO_ASCR_EN_1 | GPIO_ASCR_EN_2;
	GPIOA->ASCR |= GPIO_ASCR_EN_1; //pin 1, i.e., PA.1
}

//******************************************************************************************
// Initialize ADC	
//******************************************************************************************	
void ADC_Init(void){
	
	// Enable the clock of ADC
	RCC->AHB2ENR  |= RCC_AHB2ENR_ADCEN;
	RCC->AHB2RSTR	|= RCC_AHB2RSTR_ADCRST;// Page 231 of reference manual, 1: reset ADC interface
	
	(void)RCC->AHB2RSTR; // insert some short delay here.
	
	RCC->AHB2RSTR	&= ~RCC_AHB2RSTR_ADCRST;
	
	ADC_Pin_Init();
	ADC_Common_Configuration();
	ADC_Wakeup();
	
	//-----------------ADC configuration register 1 
	ADC1->CFGR &= ~ADC_CFGR_RES;     	
	// Resolution, (00 = 12-bit, 01 = 10-bit, 10 = 8-bit, 11 = 6-bit)
	ADC1->CFGR &= ~ADC_CFGR_ALIGN;   	
	// Data Alignment (0 = Right alignment, 1 = Left alignment)

	//EXT12 TIM4_TRGO event 1100, EXT12
	// see page 495 of textbook, 
	// page 446 of reference manual.
	ADC1->CFGR &= ~ADC_CFGR_EXTSEL;
	ADC1->CFGR |= ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2;
	
	// EXTEN[1:0], external trigger enable and polarity selection for regular channels.
	// Configuring the trigger polarity for regular external triggers
	// 00: Hardware Trigger detection disabled, software trigger detection enabled
	// 01: Hardware Trigger with detection on the rising edge
	// 10: Hardware Trigger with detection on the falling edge
	// 11: Hardware Trigger with detection on both the rising and falling edges
	ADC1->CFGR &= ~ADC_CFGR_EXTEN;
	ADC1->CFGR |=  ADC_CFGR_EXTEN_0; //rising edge trigger

	ADC1->CFGR &= ~ADC_CFGR_CONT;   // ADC Single/continuous conversion mode for regular conversion		
	//----------------------------------------------------
	
	
	//------------- ADC regular sequence register 1 (ADC_SQR1)
	ADC1->SQR1 &= ~ADC_SQR1_L;            
	// 0000: 1 conversion in the regular channel conversion sequence
	
	// Specify the channel number of the 1st conversion in regular sequence
	ADC1->SQR1 &= ~ADC_SQR1_SQ1;
	ADC1->SQR1 |=  ( 6U << 6 );           	
	// we put number 6 there to choose PA1: ADC12_IN6 
	//--------------------------------------------------------

	ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_6; 	// Single-ended for PA1: ADC12_IN6 
	
	// ADC Sample Time
	// This sampling time must be enough for the input voltage source to charge the embedded
	// capacitor to the input voltage level.
	// Software is allowed to write these bits only when ADSTART=0 and JADSTART=0
	//   000: 2.5 ADC clock cycles      001: 6.5 ADC clock cycles
	//   010: 12.5 ADC clock cycles     011: 24.5 ADC clock cycles
	//   100: 47.5 ADC clock cycles     101: 92.5 ADC clock cycles
	//   110: 247.5 ADC clock cycles    111: 640.5 ADC clock cycles	
	// ADC_SMPR3_SMP5 = Channel 5 Sample time selection
	// L1: ADC1->SMPR3 		&= ~ADC_SMPR3_SMP5;		
	// sample time for first channel, NOTE: These bits must be written only when ADON=0. 
	ADC1->SMPR1  &= ~ADC_SMPR1_SMP6;      // ADC Sample Time
	ADC1->SMPR1  |= 3U << 18;             // 3: 24.5 ADC clock cycles @80MHz = 0.3 us
		
	ADC1->IER |= ADC_IER_EOC;  // Enable End of Regular Conversion interrupt
	//ADC1->IER |= ADC_IER_EOS; // Enable ADC End of Regular Sequence of Conversions Interrupt		
	NVIC_EnableIRQ(ADC1_2_IRQn);
	
	// Enable ADC1
	ADC1->CR |= ADC_CR_ADEN;  
	
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0); //check if ADC is ready.
	
}


