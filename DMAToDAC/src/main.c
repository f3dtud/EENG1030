// DMA to the DAC triggered by Timer 2 channel 3 (DMA1, Channel 1) CxS[3:0]=0111 
// DAC output is on PA4
// 

#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initDAC(void);
void writeDAC(int value);
void initTimer2(void);
/*
Octave/matlab code to generate the sine wave table:
clear
angle_step=pi/128;
angle=[0:angle_step:2*pi-angle_step];
s=sin(angle);
s=(s+1)/2;
s=s*4095;
s=floor(s);
f=fopen('sine.txt','wt');
for(i=1:length(s))
  fprintf(f,"%d,",s(i));
end
fclose(f);

*/
const uint16_t sine_table[]={2047,2097,2147,2198,2248,2298,2347,2397,2446,2496,2545,2593,2641,2689,2737,2784,2831,2877,2922,2968,3012,3056,3100,3142,3185,3226,3267,3307,3346,3384,3422,3459,3495,3530,3564,3597,3630,3661,3692,3721,3749,3777,3803,3829,3853,3876,3898,3919,3939,3957,3975,3991,4006,4020,4033,4045,4055,4064,4072,4079,4085,4089,4092,4094,4095,4094,4092,4089,4085,4079,4072,4064,4055,4045,4033,4020,4006,3991,3975,3957,3939,3919,3898,3876,3853,3829,3803,3777,3749,3721,3692,3661,3630,3597,3564,3530,3495,3459,3422,3384,3346,3307,3267,3226,3185,3142,3100,3056,3012,2968,2922,2877,2831,2784,2737,2689,2641,2593,2545,2496,2446,2397,2347,2298,2248,2198,2147,2097,2047,1997,1947,1896,1846,1796,1747,1697,1648,1598,1549,1501,1453,1405,1357,1310,1263,1217,1172,1126,1082,1038,994,952,909,868,827,787,748,710,672,635,599,564,530,497,464,433,402,373,345,317,291,265,241,218,196,175,155,137,119,103,88,74,61,49,39,30,22,15,9,5,2,0,0,0,2,5,9,15,22,30,39,49,61,74,88,103,119,137,155,175,196,218,241,265,291,317,345,373,402,433,464,497,530,564,599,635,672,710,748,787,827,868,909,952,994,1038,1082,1126,1172,1217,1263,1310,1357,1405,1453,1501,1549,1598,1648,1697,1747,1796,1846,1896,1947,1997};

int vin;
int main()
{

    int i=0;
    setup();
    while(1)
    {        
        writeDAC(sine_table[i]);
        i++;
        delay(10);
        if (i > 255)
            i=0;
    }
}
void setup(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,0,3);  // analog input
    initDAC();
    initTimer2();
    RCC->AHB1ENR |= (1 << 0); // enable DMA1
    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CCR |= (1 << 10) + (1 < 8) + (1 << 7) + (1 << 5) + (1 << 4);
    DMA1_Channel1->CNDTR = 256;
    DMA1_Channel1->CPAR = &(DAC->DHR12R1);
    DMA1_Channel1->CMAR = sine_table;
    DMA1_CSELR->CSELR = 7; 
}


void initDAC()
{
    // Configure PA4 as a DAC output
    RCC->AHB2ENR |= (1 << 0);     // ensure GPIOA is enabled    
    GPIOA->MODER |= (1 << 8) | (1 << 9); // Set mode to analogue (DAC) 
    RCC->APB1ENR1 |= (1 << 29);   // Enable the DAC
    RCC->APB1RSTR1 &= ~(1 << 29); // Take DAC out of reset
    DAC->CR &= ~(1 << 0);         // Enable = 0
    DAC->CR |= (1 << 0);          // Enable = 1
}
void writeDAC(int value)
{
    DAC->DHR12R1 = value;
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void initTimer2(void)
{
    RCC->APB1ENR1 |= (1 << 0); // enable Timer 2
    TIM2->CR1 = 0;
    TIM2->CCMR2 = (0b110 << 12) + (1 << 11)+(1 << 10);
    TIM2->CCER |= (1 << 12);
    TIM2->ARR = 1000-1;
    TIM2->CCR4 = 500;
    TIM2->EGR |= (1 << 0);
    TIM2->CR1 = (1 << 7);
    TIM2->CR1 |= (1 << 0);  
}