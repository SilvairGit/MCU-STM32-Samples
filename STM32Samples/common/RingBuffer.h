#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct RingBuffer
{
    uint8_t *p_buf;
    size_t   buf_len;
    size_t   wr;
    size_t   rd;
};

void RingBuffer_Init(struct RingBuffer *p_ring_buffer, uint8_t *p_buf_pointer, size_t buf_len);

bool RingBuffer_IsEmpty(struct RingBuffer *p_ring_buffer);

bool RingBuffer_QueueBytes(struct RingBuffer *p_ring_buffer, uint8_t *p_table, uint16_t table_len);

bool RingBuffer_DequeueByte(struct RingBuffer *p_ring_buffer, uint8_t *p_read_byte);

void RingBuffer_SetWrIndex(struct RingBuffer *p_ring_buffer, uint16_t value);

void RingBuffer_IncrementRdIndex(struct RingBuffer *p_ring_buffer, uint16_t value);

uint16_t RingBuffer_DataLen(struct RingBuffer *p_ring_buffer);

uint8_t *RingBuffer_GetMaxContinuousBuffer(struct RingBuffer *p_ring_buffer, uint16_t *p_buf_len);

#endif
