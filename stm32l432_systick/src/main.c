// This example uses the SysTick timer to generate a 1kHz periodic 
// interrupt

#include <eeng1030_lib.h>
void setup(void);
void delay(volatile uint32_t dly);
void initADC();
int readADC();
void initDAC();
void writeDAC(int value);

int main()
{
    setup();
    SysTick->LOAD = 80000-1; // Systick clock = 80MHz. 80000000/80000=1000
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt
	__asm(" cpsie i "); // enable interrupts globally
    while(1)
    {
    
    }
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void setup()
{
    initClocks();
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
    pinMode(GPIOB,3,1); // make PB3 an output.
    
}
void SysTick_Handler(void)
{    
    GPIOB->ODR ^= (1 << 3);
}
