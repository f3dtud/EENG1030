#include <stdint.h>
void init_display(void);
void putPixel(uint16_t x, uint16_t y, uint16_t colour);
void putImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *Image, int hOrientation, int vOrientation);
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t Colour);
void fillRectangle(uint16_t x,uint16_t y,uint16_t width, uint16_t height, uint16_t colour);
uint16_t RGBToWord(uint16_t R, uint16_t G, uint16_t B);
void printText(const char *Text,uint16_t x, uint16_t y, uint16_t ForeColour, uint16_t BackColour);
void drawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t Colour);