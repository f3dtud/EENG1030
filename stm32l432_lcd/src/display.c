#include <display.h>
#include <stdint.h>
#include <stm32l432xx.h>
#include "eeng1030_lib.h"
void init_display()
{
    initSPI(SPI1);
}