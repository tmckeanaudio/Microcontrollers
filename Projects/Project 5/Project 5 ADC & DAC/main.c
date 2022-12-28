#include "stm32l476xx.h"
#include "LCD.h"
#include "ADC.h"
#include "DAC.h"
#include "LED.h"
#include "TIM.h"
#include "SysTimer.h"
#include "SysClock.h"
#include "stdio.h"

volatile uint32_t result;
volatile uint32_t result_DAC;
volatile uint32_t count;
volatile uint32_t i;
unsigned char str[6];		
volatile uint32_t temp;

int main(void){
	int i;
  System_Clock_Init(); // 80MHz
	SysTick_Init();
	LED_Init();
	LCD_PIN_Init();
	LCD_Clock_Init();	
	LCD_Configure();
	LCD_Clear();
	LCD_DisplayString((uint8_t*)"******");
	//LCD_bar();
	//sprintf(str, "%d", count);
		
	LCD_DisplayString(str);	
		
	TIM4_Init();
	// Analog Inputs: 
	//  PA1 (ADC12_IN 6)
	//  0V <=> 0, 3.0V <=> 4095
	ADC_Init();
	ADC1->CR |= ADC_CR_ADSTART; //start ADC			
	// Analog Outputs: PA5 (DAC2)
	DAC_Init();
	
	while(1){	
		//count++;
		sprintf(str, "%d", result);
		
		LCD_DisplayString(str);
		for(i=0;i<100000;i++);  // delay
		
	}
}

//******************************************************************************************
// 	ADC 1/2 Interrupt Handler
//******************************************************************************************
void ADC1_2_IRQHandler(void){
	NVIC_ClearPendingIRQ(ADC1_2_IRQn);
	
	//for a regular channel, check EOC flag
	// ADC End of Conversion (EOC)
	if ((ADC1->ISR & ADC_ISR_EOC) == ADC_ISR_EOC) {
		// It is cleared by software writing 1 to it 
		//or by reading the corresponding ADCx_DR register
		//ADC1->ISR |= ADC_ISR_EOC;
		result = ADC1->DR; //reading DR clears EOC flag
		if (result <= 0xE74){
			Green_LED_On();
			temp = 1;
		}else{
			Green_LED_Off();
			temp = 0;
		}
		//sprintf(str, "%d", result);
		count++;
		
	}
	
	// ADC End of Injected Sequence of Conversions  (JEOS)
	//if ((ADC1->ISR & ADC_ISR_EOS) == ADC_ISR_EOS) {
		// It is cleared by software writing 1 to it.
		//ADC1->ISR |= ADC_ISR_EOS;		
	//}
}
// Timer 4's frequency is 80MHz/(79+1)/(999+1)=1Khz.
void TIM4_IRQHandler(void){

	if((TIM4->SR & TIM_SR_CC1IF) != 0){
		TIM4->SR &= ~TIM_SR_CC1IF;
		result_DAC=(i++); 
		if(temp == 1){
				if (result_DAC % 503 <= 251){
					DAC->DHR12R2 = 4095;
				}else{
					DAC->DHR12R2 = 0;
				}
					result_DAC = result_DAC % 503;
		}

	}

	if((TIM4->SR & TIM_SR_TIF) != 0) 
		TIM4->SR &= ~TIM_SR_TIF;
	
	if((TIM4->SR & TIM_SR_UIF) != 0) 
		TIM4->SR &= ~TIM_SR_UIF;
		
	return;
}


