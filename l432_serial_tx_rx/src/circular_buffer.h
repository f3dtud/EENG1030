#include <stdint.h>
#define CIRC_BUF_SIZE 16
typedef struct {
	char data[CIRC_BUF_SIZE];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
} circular_buffer;


void init_circ_buf(circular_buffer *buf);
int put_circ_buf(circular_buffer *buf,char c);
int get_circ_buf(circular_buffer *buf,char *c);
