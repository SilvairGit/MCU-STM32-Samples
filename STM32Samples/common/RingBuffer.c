#include "RingBuffer.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "Assert.h"

static bool     IsOverflow(struct RingBuffer *p_ring_buffer, uint16_t len);
static uint16_t MaxQueueBufferLen(struct RingBuffer *p_ring_buffer, uint16_t table_len);

void RingBuffer_Init(struct RingBuffer *p_ring_buffer, uint8_t *p_buf_pointer, size_t buf_len)
{
    ASSERT((p_ring_buffer != NULL) && (p_buf_pointer != NULL));

    p_ring_buffer->p_buf   = p_buf_pointer;
    p_ring_buffer->buf_len = buf_len;
    p_ring_buffer->wr      = 0;
    p_ring_buffer->rd      = 0;
}

bool RingBuffer_IsEmpty(struct RingBuffer *p_ring_buffer)
{
    ASSERT(p_ring_buffer != NULL);

    return p_ring_buffer->wr == p_ring_buffer->rd;
}

void RingBuffer_SetWrIndex(struct RingBuffer *p_ring_buffer, uint16_t value)
{
    ASSERT(p_ring_buffer != NULL);

    p_ring_buffer->wr = (value) % p_ring_buffer->buf_len;
}

void RingBuffer_IncrementRdIndex(struct RingBuffer *p_ring_buffer, uint16_t value)
{
    ASSERT(p_ring_buffer != NULL);

    p_ring_buffer->rd = (p_ring_buffer->rd + value) % p_ring_buffer->buf_len;
}

bool RingBuffer_DequeueByte(struct RingBuffer *p_ring_buffer, uint8_t *p_read_byte)
{
    ASSERT((p_ring_buffer != NULL) && (p_read_byte != NULL));

    if (RingBuffer_IsEmpty(p_ring_buffer))
    {
        return false;
    }
    *p_read_byte = p_ring_buffer->p_buf[(p_ring_buffer->rd)++];

    if (p_ring_buffer->rd >= p_ring_buffer->buf_len)
    {
        p_ring_buffer->rd = 0;
    }

    return true;
}

bool RingBuffer_QueueBytes(struct RingBuffer *p_ring_buffer, uint8_t *p_table, uint16_t table_len)
{
    ASSERT((p_ring_buffer != NULL) && (p_table != NULL));

    if (IsOverflow(p_ring_buffer, table_len))
    {
        return false;
    }

    uint16_t cpy_len = MaxQueueBufferLen(p_ring_buffer, table_len);
    memcpy(&p_ring_buffer->p_buf[p_ring_buffer->wr], p_table, cpy_len);

    if (cpy_len != table_len)
    {
        uint16_t rest_of_table_len = table_len - cpy_len;
        memcpy(p_ring_buffer->p_buf, &p_table[cpy_len], rest_of_table_len);
    }

    RingBuffer_SetWrIndex(p_ring_buffer, p_ring_buffer->wr + table_len);

    return true;
}

uint8_t *RingBuffer_GetMaxContinuousBuffer(struct RingBuffer *p_ring_buffer, uint16_t *p_buf_len)
{
    ASSERT((p_ring_buffer != NULL) && (p_buf_len != NULL));

    if (p_ring_buffer->rd + RingBuffer_DataLen(p_ring_buffer) > p_ring_buffer->buf_len)
    {
        *p_buf_len = p_ring_buffer->buf_len - p_ring_buffer->rd;
    }
    else
    {
        *p_buf_len = RingBuffer_DataLen(p_ring_buffer);
    }

    return &p_ring_buffer->p_buf[p_ring_buffer->rd];
}

uint16_t RingBuffer_DataLen(struct RingBuffer *p_ring_buffer)
{
    ASSERT(p_ring_buffer != NULL);

    return (p_ring_buffer->wr - p_ring_buffer->rd) % p_ring_buffer->buf_len;
}

static bool IsOverflow(struct RingBuffer *p_ring_buffer, uint16_t len)
{
    return (len + RingBuffer_DataLen(p_ring_buffer)) > p_ring_buffer->buf_len;
}

static uint16_t MaxQueueBufferLen(struct RingBuffer *p_ring_buffer, uint16_t table_len)
{
    if (IsOverflow(p_ring_buffer, table_len))
    {
        return 0;
    }

    if (p_ring_buffer->wr + table_len > p_ring_buffer->buf_len)
    {
        return p_ring_buffer->buf_len - p_ring_buffer->wr;
    }
    else
    {
        return table_len;
    }
}
