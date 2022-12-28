
#include "stm32l476xx.h"
#include "SysClock.h"
#include "SysTimer.h"

void System_Clock_Init(void);
void GPIO_Clock_Enable(void);
void GPIOA_Pin_Init(void);
void GPIOB_Pin_Init(void);
void HalfStep360(int x);
void HalfStep180(int x);

int main(void){
	// Initialize and clear system
 	System_Clock_Init(); // Switch System Clock to 80 MHz
	
	GPIO_Clock_Enable();
	GPIOA_Pin_Init();
	GPIOB_Pin_Init();
	SysTick_Init();
	while(1){
		// TURN 360 HALFSTEP WHEN UP BUTTON PRESSED
		while(GPIOA->IDR & 0x8){
			HalfStep360(512);
		}
		// TURN 180 HALFSTEP WHEN DOWN BUTTON PRESSED
		while(GPIOA->IDR & 0x20){
			HalfStep180(256);
	}
}
}

void HalfStep360(int num){
	unsigned char HalfStep[8] = {0x44, 0x04, 0x84, 0x80, 0x88, 0x08, 0x48, 0x40};
	int i, j;
	for(j = 0; j < num; j++){
		for(i = 7; i >= 0; i--){
			GPIOB->ODR &= ~(0x00CC);
			GPIOB->ODR |= HalfStep[i];
			delay(3); // use SysTick delay function to generate some delay
		}
	}
}

void HalfStep180(int num){
	unsigned char HalfStep[8] = {0x44, 0x04, 0x84, 0x80, 0x88, 0x08, 0x48, 0x40};
	int i, j;
	for(j = 0; j < num; j++){
		for(i = 0; i <= 7; i++){
			GPIOB->ODR &= ~(0x00CC);
			GPIOB->ODR |= HalfStep[i];
			delay(5); // use SysTick delay function to generate some delay
		}
	}
}

void GPIOA_Pin_Init(){
	//INITIALIZE PORT A PIN 3 and 5 - for up and down button
	GPIOA->MODER &= ~(0xCC0);				//Mode mask
	GPIOA->PUPDR &= ~(0xCC0);				
	GPIOA->PUPDR |= 0x880;  // SET AS PULL DOWN, 10: pull down
}

void GPIOB_Pin_Init(){
	// INITIALIZE PORT B PIN 2, 3, 6, and 7 as output for step motor
	GPIOB->MODER &= ~(0x0F0F0);				//Mode mask
	GPIOB->MODER |= 0x01UL<<(2*2);
	GPIOB->MODER |= 0x01UL<<(2*3);
	GPIOB->MODER |= 0x01UL<<(2*6);
	GPIOB->MODER |= 0x01UL<<(2*7);
	GPIOB->OTYPER &= ~(0x0CC);  // configure those four pins as open-drain. 1: output open drain.
	GPIOB->PUPDR &= ~(0x0F0F0);// No pull up no pull down
}

// SET UP ALL CLOCK AND GPIO FOR DESIRED USE
void GPIO_Clock_Enable(){
	//RCC->AHB2ENR |= 0x00000013;
	RCC->AHB2ENR |= 0x00000003;
}


