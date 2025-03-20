// This example uses demonstrates interfacing an SPI display with the STM32L432
// Also makes use of the systick timer to provide calibrated delays
#include <eeng1030_lib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "display.h"
void setup(void);
void delay_ms(volatile uint32_t dly);
void initSerial(uint32_t baudrate);
void eputc(char c);
int count;
void initEncoder(void);
int main()
{
    uint16_t dial_background;
    uint16_t dial_x,dial_oldx;
    setup();
    init_display();
    initEncoder();
    drawRectangle(0,0,159,79,RGBToWord(255,0,0));
    printText("Hola Mundo!",5, 10, RGBToWord(255,255,0),0);
    printText("Hello World",5, 20, RGBToWord(128,128,255),0);
    printText("Hola Mundo!",5, 30, RGBToWord(0,255,0),0);
    dial_background = RGBToWord(255,242,64);
    fillRectangle(0,0,160,80,dial_background);    
    dial_x = dial_oldx = 0;
    while(1)
    {
        printf("count %d\r\n",TIM2->CNT);
        dial_x = TIM2->CNT;
        if (dial_x != dial_oldx)
        {
            drawLine(dial_oldx,0,dial_oldx,79,dial_background);
            drawLine(dial_x,0,dial_x,79,0);
            dial_oldx = dial_x;
        }
        delay_ms(10);
    }
}

void setup()
{
    initClocks();    
    SysTick->LOAD = 80000-1; // Systick clock = 80MHz. 80000000/80000=1000
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt
	__asm(" cpsie i "); // enable interrupts globally
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
    initSerial(9600);
}
void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3);  // enable the transmitter
	USART2->CR1 |= (1 << 0);
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    while(len--)
    {
        eputc(*data);    
        data++;
    }    
    return 0;
}
void eputc(char c)
{
    while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART2->TDR=c;
}       

// I/O List for Encoder:
// PA0 : TIM2_CH1 (Alternative function 1) : Clock pin on encoder
// PB3 : TIM2_CH2 (Alternative function 1) : DT pin on encoder (quadrature type input)
// PB4 : GPIOA - SW input from encoder (maybe)
void initEncoder()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // make sure GPIOA,B are turned on   
    pinMode(GPIOA,0,2);
    selectAlternateFunction(GPIOA,0,1);
    pinMode(GPIOB,3,2);
    selectAlternateFunction(GPIOB,3,1);
    pinMode(GPIOB,4,0);
    RCC->APB1ENR1 |= (1 << 0); // enable Timer 2
    TIM2->ARR = 159;
    TIM2->SMCR = 0b010;
    TIM2->CCMR1 = (2 << 8)+(1 << 0)+ (0x0f<<4)+ (0x0f<<12);
    TIM2->CR1 |= 1;

}