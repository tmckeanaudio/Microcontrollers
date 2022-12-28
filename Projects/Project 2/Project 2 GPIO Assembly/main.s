;*******************************************************************************

	INCLUDE core_cm4_constants.s		; Load Constant Definitions
	INCLUDE stm32l476xx_constants.s      

	AREA    main, CODE, READONLY
	EXPORT	__main						; make __main visible to linker
	ENTRY			
				
__main	PROC
	
    ; Enable the clock to GPIO Port A, B, and E	
	LDR r0, =RCC_BASE
	LDR r1, [r0, #RCC_AHB2ENR]
	ORR r1, r1, #RCC_AHB2ENR_GPIOAEN
	STR r1, [r0, #RCC_AHB2ENR]
	
	LDR r1, [r0, #RCC_AHB2ENR]
	ORR r1, r1, #RCC_AHB2ENR_GPIOBEN
	STR r1, [r0, #RCC_AHB2ENR]
	
	LDR r1, [r0, #RCC_AHB2ENR]
	ORR r1, r1, #RCC_AHB2ENR_GPIOEEN
	STR r1, [r0, #RCC_AHB2ENR]

	; MODE: 00: Input mode, 01: General purpose output mode
    ;       10: Alternate function mode, 11: Analog mode (reset state)
	
	LDR r0, =GPIOA_BASE ; Set GPIOA as input in MODER, output push-pull in OTYPER, and Pull down in PUPDR
	LDR r1, [r0, #GPIO_MODER]
	LDR r2, [r0, #GPIO_OTYPER]
	LDR r3, [r0, #GPIO_PUPDR]
	BIC r1, r1,  #(0x3<<(2*3))
	BIC r2, r2,  #(0x1<<3)
	BIC r3, r3,  #(0x3<<(2*3))
	ORR r3, r3,  #(0x2<<(2*3))
	STR r1, [r0, #GPIO_MODER]
	STR r2, [r0, #GPIO_OTYPER]
	STR r3, [r0, #GPIO_PUPDR]
	
	LDR r0, =GPIOB_BASE ; Set GPIOB as output in MODER, and as output push-pull in OTYPER
	LDR r1, [r0, #GPIO_MODER]
	LDR r2, [r0, #GPIO_OTYPER]
	BIC r1, r1,  #(0x3<<(2*2))
	ORR r1, r1,  #(0x1<<(2*2))
	BIC r2, r2,  #(0x1<<2)
	STR r1, [r0, #GPIO_MODER]
	STR r2, [r0, #GPIO_OTYPER]
	
	LDR r0, =GPIOE_BASE ; Set GPIOE as output in MODER, and as output push-pull in OTYPER
	LDR r1, [r0, #GPIO_MODER]
	LDR r2, [r0, #GPIO_OTYPER]
	BIC r1, r1,  #(0x3<<(2*8))
	ORR r1, r1,  #(0x1<<(2*8))
	BIC r2, r2,  #(0x1<<8)
	STR r1, [r0, #GPIO_MODER]
	STR r2, [r0, #GPIO_OTYPER]
	
loop1	
	LDR r0, =GPIOA_BASE
	LDR r1, [r0, #GPIO_IDR]
	AND r1, r1, #8

	CMP r1, #8
	BNE endloop1

	LDR r0, =GPIOB_BASE
	LDR r1, [r0, #GPIO_ODR]
	EOR r1, r1,  #(0x1<<2)
	STR r1, [r0, #GPIO_ODR]
	
	LDR r0, =GPIOE_BASE
	LDR r1, [r0, #GPIO_ODR]
	EOR r1, r1,  #(0x1<<8)
	STR r1, [r0, #GPIO_ODR]
	
loop2	
	LDR r0, =GPIOA_BASE
	LDR r1, [r0, #GPIO_IDR]
	AND r1, r1, #8
	CMP r1, #8
	BEQ loop2

endloop1 B loop1
  

	ENDP					
	ALIGN			
	AREA    myData, DATA, READWRITE		
	ALIGN
array	DCD   1, 2, 3, 4
	
	END
