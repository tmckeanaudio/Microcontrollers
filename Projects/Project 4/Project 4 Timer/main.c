/*
----------------------------------------------------------------------------------------------------------------------------
| File:     main.c                                                                                                          |                                                                                                            |
| Hardware: Discovery kit STM32L476G-DISCO                                                                                  | 
| Program Objectives: To enable a ultrasonic distance sensor to measure a distance            																|		
| Display distance on LCD                                                                                                   |
----------------------------------------------------------------------------------------------------------------------------
*/ 

#include "stm32l476xx.h"
#include "LCD.h"
#include <string.h>
#include "stdio.h"

volatile float timespan = 0;
volatile float pulse_width=0;
volatile uint32_t last_captured=0;
volatile uint32_t signal_polarity=0; //assume input is low initially.
volatile uint32_t underflow_counter=0;
uint32_t ARR_VALUE=0xFFFF;

void itoa(unsigned int n, char *s);
void System_Clock_Init(void);
void TIM4_C1_Init(void);
void TIM1_C2_Init(void);
void TIM4_IRQHandler(void);
void Delay(int);
void Display_Centimeters(void);

int main(void) {
	System_Clock_Init(); //16 MHz.
	TIM1_C2_Init(); // Trigger signal
	TIM4_C1_Init(); // Timer input capture
	LCD_Initialization();
	LCD_Clear();	

	while(1) {
		Display_Centimeters();  // Displayed with first 2 digits being decimal places.
		Delay(100);
	}
}

void Display_Centimeters(void) {
	// Display distance in centimeters on LCD
	unsigned char str[6];			// x = measurement
	float distance;

	pulse_width = (float)timespan*(1.0/1000000)*1000; 
	//in usec. Here assume clock frequency (after prescale) is 1MHz
	distance = (pulse_width/58.0); 
  itoa(distance, str);      // 10 means decimal
	sprintf(str, "%.2f", distance);
	LCD_DisplayString(str);		// Display measurement on LCD
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Select HSI (16 MHz) as system clock
void System_Clock_Init(void){
	
  // Enable High Speed Internal Clock (HSI = 16 MHz)
  RCC->CR |= ((uint32_t)RCC_CR_HSION);
	
  // Wait until HSI is ready
  while ( (RCC->CR & (uint32_t) RCC_CR_HSIRDY) == 0 );
	
  // Select HSI as system clock source 
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSI;  //01: HSI16 oscillator used as system clock

  // Wait till HSI is used as system clock source 
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) == 0 ) {;}		
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PB.6 (TIM4_CH1): Input capture for the sensor echo
//     
void TIM4_C1_Init(void) {
	// enable GPIO clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	// MODE: 00: Input mode,              01: General purpose output mode
	//       10: Alternate function mode, 11: Analog mode (reset state) 
	GPIOB->MODER &= ~(0x3<<(2*6)); // Clear bit 13 and bit 12
	GPIOB->MODER |= (0x2<<(2*6)); // Set to Alternate Function Mode
	
	GPIOB->AFR[0] &= ~GPIO_AFRL_AFRL6; // Clear pin 6 for alternate function Page 282
	GPIOB->AFR[0] |= (0x2<<(24)); 
	//Set pin 6 to alternate function 2 (enables TIM4), each pin needs 4 bits to configure	
	
	GPIOB->OSPEEDR &= ~(0x3<<(2*6)); // Clear bits 12 and 13 to set Output speed of the pin
	GPIOB->OSPEEDR |= (0x3<<(2*6));  // Set output speed of the pin to 16MHz
	GPIOB->PUPDR &= ~(0x3 << (2*6));
	
	// Enable the clock of timer 4
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;
	TIM4->PSC = 15; // 16MHz/(15+1) = 1MHz
	TIM4->ARR = ARR_VALUE;
	// Set to down counter
	TIM4->CR1 &= ~TIM_CR1_DIR; // control register 1
	TIM4->CR1 |=  TIM_CR1_DIR;
	// Set the direction as input and select the active input
	// CC1S[1:0] for channel 1; 00 = output
	// 01 = input, CC1 is mapped on timer Input (TI) 1
	// 10 = input, CC1 is mapped on timer Input 2
	// 11 = input, CC1 is mapped on slave timer
	TIM4->CCMR1 &= ~TIM_CCMR1_CC1S; //capture/compare mode register 1 (TIMx_CCMR1), Page 906.
	TIM4->CCMR1 |= TIM_CCMR1_CC1S_0;  // 0x1; 
	
	// Disable digital filtering by clearing IC1F[3:0] bits
	TIM4->CCMR1 &= ~TIM_CCMR1_IC1F;	
	
	// Program the input prescaler
	// To capture each valid transition, set the input prescaler to zero;
	// IC1PSC[1:0] bits (input capture 1 prescaler)
	TIM4->CCMR1 &= ~(TIM_CCMR1_IC1PSC); // Clear filtering because we need to capture every event
	// Select the edge of the active transition
	//TIMx capture/compare enable register (TIMx_CCER), Page 911
	// CC1NP (bit 3) and CC1P (bit 1) bits
	// 00 = rising edge, 01 = falling edge,10 = reserved,11 = both edges
	TIM4->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1NP; //both edges detected
	// Enable Capture/compare for CC1
	TIM4->CCER |= TIM_CCER_CC1E;

	// Enable related interrupts, Page 920
	TIM4->DIER |= TIM_DIER_CC1IE;//Enable Capture/Compare interrupts for channel 1
	TIM4->DIER |= TIM_DIER_UIE; // Enable update interrupts (i.e., overflow or underflow)
	TIM4->CR1  |= TIM_CR1_CEN; // Enable the counter
	
	//Set priority and interrupt in NVIC
	NVIC_SetPriority(TIM4_IRQn, 1); // Set priority to 1
	NVIC_EnableIRQ(TIM4_IRQn);      // Enable TIM4 interrupt in NVIC
}



void TIM4_IRQHandler(void) { 
		uint32_t current_captured;
		if ( (TIM4->SR & TIM_SR_CC1IF) !=0) { //check if interrupt flag is set. 
			current_captured = TIM4->CCR1; //reading CCR1 clears CC1IF 
			signal_polarity = 1 - signal_polarity;	//toggle polarity flag
			if (signal_polarity == 0) {  //calculate only when the current input is low.
				//if we use 1MHz as the counter clock freq, and use a function generator to generate
				// 1MHz pulse signal, then the pulse_width should be 10.
				timespan = (current_captured - last_captured) + (1+ARR_VALUE)*underflow_counter; 
				underflow_counter=0;
			}
			last_captured = current_captured;
		}
		if (( TIM4->SR & TIM_SR_UIF) !=0 ) { //check if underflow has taken place
			TIM4->SR &= ~TIM_SR_UIF; //clear UIF flag to prevent re-entering.
			underflow_counter++;
		}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PE.11 (TIM1_CH2): Trigger signal to the sensor,  pulse width = 10 us
// Timer 1: input clock 16 Mhz
//
void TIM1_C2_Init(void) { 
	// enable GPIO clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	// MODE: 00: Input mode,              01: General purpose output mode
	//       10: Alternate function mode, 11: Analog mode (reset state) 
	GPIOE->MODER &= ~(0x3<<(2*11)); // Clear bit 23 and bit 22
	GPIOE->MODER |= (0x2<<(2*11)); // Set to Alternate Function Mode for PE.11
	
	GPIOE->AFR[1] &= ~GPIO_AFRH_AFRH3; // Clear pin 11 for alternate function Page 282
	GPIOE->AFR[1] |= (0x1<<12); 
	//Set pin 11 to alternate function 1 (enables TIM1), each pin needs 4 bits to configure	
	
	GPIOE->OSPEEDR &= ~(0x3<<(2*11)); // Clear bits 23 and 22 to set Output speed of the pin
	GPIOE->OSPEEDR |= (0x3<<(2*11));  // Set output speed of the pin to 40MHz
	GPIOE->OTYPER &= ~(0x3<<11);
	GPIOE->PUPDR 	 &= ~(0x3<<(2*11));
	
	// Enable the clock of timer 1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	TIM1->PSC = 18;
	TIM1->ARR = 18;
	// control register 1 (0: Up counter, 1: Down counter)
	TIM1->CR1 &= ~TIM_CR1_DIR; 
	TIM1->CR1 |= TIM_CR1_DIR; // Down counter
	// Clear output compare mode bits 
	TIM1->CCMR1 &= ~TIM_CCMR1_OC2M; 
	// Select PWM Mode 1 output on channel 2 (OC2M = 110)
	TIM1->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
	// Output 2 preload enable
	TIM1->CCMR1 |= TIM_CCMR1_OC2PE;
	// Select output polarity: 0 = Active high, 1 = Active low
	TIM1->CCER &= ~TIM_CCER_CC2P;
	// Enable output of channel 2 
	TIM1->CCER |= TIM_CCER_CC2E;
	// Main output enable (MOE): 0 = Disable, 1: = Enable
	TIM1->BDTR |= TIM_BDTR_MOE;
	// Set CCR2 value to 10
	//TIM1->CCR2  = 10;
	// Enable Counter
	TIM1->CR1 |= TIM_CR1_CEN;
	
}


void Delay(int x){
	//input milliseconds, delay that number of milliseconds
	int a,b;
	for(a=0; a<x; a++){
		for(b=0; b<1000; b++){
		}
	}
}

void itoa(unsigned int n, char *s){

  char * p = s, temp;
  // Build the string backward

  for (; n != 0; n /= 10){
    *p = n % 10 + '0';
    p++;
  }

  *p = '\0';
  p--;  // skip NULL

  // Reverse the string
  for(; p > s; s++, p--){
    temp = *p;
    *p = *s;
    *s = temp;
  }

  return;
}


