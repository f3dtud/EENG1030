#include <stdint.h>
#include "circular_buffer.h"

void init_circ_buf(circular_buffer *buf)
{
	buf->head=0;
	buf->tail=0;
}
int put_circ_buf(circular_buffer *buf,char c)
{
	uint32_t new_head;
	if (buf->count < CIRC_BUF_SIZE)
	{
		new_head=(((buf->head)+1)%CIRC_BUF_SIZE);
		buf->data[buf->head]=c;
		buf->head=new_head;
		buf->count++;
		return 0;	
	}
	else
	{
		return -1;
	}
}
int get_circ_buf(circular_buffer *buf,char *c)
{
	uint32_t new_tail;
	if (buf->count > 0)
	{
		new_tail=(((buf->tail)+1)%CIRC_BUF_SIZE);
		*c=buf->data[buf->tail];
		buf->data[buf->tail] = '-'; //debug
		buf->tail=new_tail;
		buf->count--;
		return 0;	
	}
	else
	{
		return -1;
	}
}