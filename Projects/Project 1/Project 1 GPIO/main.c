#include "stm32l476xx.h"
uint32_t mask0;
uint32_t mask1;
uint32_t mask2;
int main(void){
	
  // Enable the clock to GPIO Ports A, B, and E	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	
	// Set PB2 as output
  GPIOB->MODER &= ~(0x03<<(2*2));
  GPIOB->MODER |= (1UL<<(2*2));
	
	// Set PE8 as output
  GPIOE->MODER &= ~(0x03<<(2*8));
  GPIOE->MODER |= (1UL<<(2*8));
	
	// Set PB2 output type as push-pull
	GPIOB->OTYPER |= 0UL<<2;
	
	// Set PE8 output type as push-pull
	GPIOB->OTYPER |= 0UL<<8;
	
	// Set PA0 (center) as input
	GPIOA->MODER &= ~0x3; //or 3UL
	
	//Set PA5 (down) as input
	GPIOA->MODER &= ~(0x03<<(2*5));
	
	//Set PA3 (up) as input
	GPIOA->MODER &= ~(0x03<<(2*3));
	
	//Set PA3 and PA5 as output push-pull
	GPIOA->OTYPER |= 0UL<<3;
	GPIOA->OTYPER |= 0UL<<5;

	// Configure PA0, PA3, and PA5 as no pull up no pull-down.
	GPIOA->PUPDR &= ~0x3;
	
	GPIOA->PUPDR &= ~(0x3<<(2*3));
	GPIOA->PUPDR |= 2UL<<(2*3);
	
	GPIOA->PUPDR &= ~(0x3<<(2*5));
	GPIOA->PUPDR |= 2UL<<(2*5);
	
	//Declare masks to help read joystick buttons
	mask0 = 1UL;
	mask1 = 1UL<<3;
	mask2 = 1UL<<5;
	
	// begin main loop
  while(1){
    
		// Toggle RED & GREEN LEDs when middle button is pressed
		if((GPIOA->IDR & mask0) == mask0){
			GPIOB->ODR ^= GPIO_ODR_ODR_2;
			GPIOE->ODR ^= GPIO_ODR_ODR_8;
			while((GPIOA->IDR & mask0) != 0x00);
		}
		// Turn on RED & GREEN LEDs when up button is pressed
		if(((GPIOA->IDR & mask1)>>3) != 0x00){
			GPIOB->ODR |= GPIO_ODR_ODR_2;
			GPIOE->ODR |= GPIO_ODR_ODR_8;
			while((GPIOA->IDR & mask1) != 0x00);
  }
		// Turn off RED & GREEN LEDs when down button is pressed
			if(((GPIOA->IDR & mask2)>>5) != 0x00){
			GPIOB->ODR &= ~(GPIO_ODR_ODR_2);
			GPIOE->ODR &= ~(GPIO_ODR_ODR_8);
			while((GPIOA->IDR & mask2) != 0x00);
  }
	}
}
