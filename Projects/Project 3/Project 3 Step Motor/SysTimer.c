
#include "SysTimer.h"

//uint32_t msTicks=0;
volatile int32_t TimeDelay;


//******************************************************************************************
// Initialize SysTick	
//******************************************************************************************	
void SysTick_Init(void){
	// The RCC feeds the Cortex System Timer (SysTick) external clock with the AHB clock
	// (HCLK) divided by 8. The SysTick can work either with this clock or with the Cortex clock
	// (HCLK), configurable in the SysTick Control and Status Register.
	
	//  SysTick Control and Status Register
	SysTick->CTRL = 0;										// Disable SysTick IRQ and SysTick Counter
	
	// SysTick Reload Value Register
	SysTick->LOAD = 80000000/1000 - 1;    // 1ms, Default clock
	
	// SysTick Current Value Register
	SysTick->VAL = 0;

	NVIC_SetPriority(SysTick_IRQn, 1);		// Set Priority to 1. Function defined in core_cm4.h
	NVIC_EnableIRQ(SysTick_IRQn);					// Enable EXTI0_1 interrupt in NVIC. Function defined in core_cm4.h

	// Enables SysTick exception request
	// 1 = counting down to zero asserts the SysTick exception request
	// 0 = counting down to zero does not assert the SysTick exception request
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;  //defined in core_cm4.h
	
	// Select processor clock
	// 1 = processor clock;  0 = external clock
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;		
	
	// Enable SysTick Timer
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  
}

//******************************************************************************************
// SysTick Interrupt Handler
//******************************************************************************************
void SysTick_Handler(void){
	//msTicks++;
	if (TimeDelay > 0)    // Prevent it from being negative 
    TimeDelay--;        // TimeDelay is a global volatile variable 

}
	
//******************************************************************************************
// Delay in ms
//******************************************************************************************
void delay (uint32_t nTime){
  // nTime: specifies the delay time length
  TimeDelay = nTime;      // TimeDelay must be declared as volatile
  while(TimeDelay != 0);  // Busy wait
	
}
