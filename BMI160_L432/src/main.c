#include "eeng1030_lib.h"
void setup(void);
void initI2C(void);
void delay(volatile uint32_t dly);
int main()
{
    setup();
    while(1)
    {
        GPIOB->ODR |= (1 << 3);
        delay(10000);                
        I2C1->TXDR=0xaa;
        GPIOB->ODR &= ~(1 << 3);
        delay(10000);

    }
}
void setup()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // enable GPIOA and GPIOB
    initI2C();    
}
void initI2C(void)
{
    initClocks();
    pinMode(GPIOB,3,1); // make PB3 (internal LED) an output
    pinMode(GPIOB,6,2); // alternative functions for PB6 and PB7
    pinMode(GPIOB,7,2); 
    selectAlternateFunction(GPIOB,6,4); // Alternative function 4 = I2C1_SCL
    selectAlternateFunction(GPIOB,7,4); // Alternative function 4 = I2C1_SDA
    RCC->APB1ENR1 |= (1 << 21); // enable I2C1
    I2C1->TIMINGR = (3 << 28) + (4 << 20) + (2 << 16) + (0x0f << 8) + (0x13); // from table 138 in the reference manual
    I2C1->ICR |= 0xffff; // clear all pending interrupts
    I2C1->CR2 = 0x5a;
    I2C1->CR1 |= (1 << 0);
    

}
void delay(volatile uint32_t dly)
{
    while(dly--);
}