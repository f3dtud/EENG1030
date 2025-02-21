/* Demonstrates the use of an interrupt on a GPIO port
GPIO interrupts are routed through the EXTI (Extended 
Inerrupts and Event Controler ).  This makes it a little
more complicated that an interrupt which is managed directly by
the NVIC (Nested Vector Interrupt Controller) 
This example uses a button on PB4 to generate an interrupt.  
When the button is pressed, PB4 is pulled low.
PB4 is routed through the EXTI4 interrupt */
#include "eeng1030_lib.h"
void setup(void);
void delay(volatile uint32_t dly);
int main()
{
    setup();
    while(1)
    {        
        GPIOB->ODR &= ~(1 << 3);
        delay(1000000);        
    }
}
void setup()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // enable GPIOA and GPIOB
    initClocks();
    pinMode(GPIOB,3,1);
    pinMode(GPIOB,4,0);
    enablePullUp(GPIOB,4);    
    RCC->APB2ENR |= (1 << 0); // enable SYSCFG
    SYSCFG->EXTICR[1] &= ~(7 << 0); // clear perhaps previously set bits
    SYSCFG->EXTICR[1] |= (1 << 0);  // map EXTI2 interrupt to PB4
    EXTI->FTSR1 |= (1 << 4); // select falling edge trigger for PB4 input
    EXTI->IMR1 |= (1 << 4);  // enable PB4 interrupt
    NVIC->ISER[0] |= (1 << 10); // IRQ 10 maps to EXTI4
    __enable_irq();
}
void delay(volatile uint32_t dly)
{
    while (dly--);
}
void EXTI4_IRQHandler()
{
    GPIOB->BSRR = (1 << 3); // set PB3 to turn on LED 
    EXTI->PR1 = (1 << 4);   // clear interrupt pending flag
}