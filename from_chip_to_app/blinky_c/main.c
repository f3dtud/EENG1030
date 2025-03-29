#include <stdint.h>
#include "../include/STM32G431.h"


void delay(uint32_t dly)
{
    while(dly--);
}
void initClock();

int main()
{    
    RCC->RCC_AHB2ENR |= (1 << 0); // enable Port A    
    GPIOA->MODER &= ~(1 << (1+(2*8))); // Make bit 8 an output
    GPIOA->MODER |= (1 << (2*8));
    while(1)
    {
        GPIOA->ODR |= (1<<8);
        delay(1000000);
        GPIOA->ODR &= ~(1<<8);
        delay(1000000);
    }
}
