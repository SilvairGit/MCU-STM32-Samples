#include "DeltaEncoder.h"

#include "Config.h"
#include "Utils.h"

#if ENABLE_CLIENT

#include <limits.h>
#include <stdbool.h>

#include "EncoderHal.h"
#include "Log.h"
#include "Mesh.h"
#include "Timestamp.h"
#include "UartProtocol.h"

#define DELTA_INTVL_MS 100             /**< Defines the shortest interval beetwen two Delta Set messages. */
#define DELTA_NEW_TID_INTVL 350        /**< Defines the shortest interval beetwen generation of new TID. */
#define DELTA_TRANSITION_TIME_MS 200   /**< Defines transition time */
#define DELTA_DELAY_TIME_MS 40         /**< Defines message delay in milliseconds */
#define DELTA_NUMBER_OF_REPEATS 0      /**< Defines number of repeats while sending mesh message request */
#define DELTA_NUMBER_OF_REPEATS_LAST 2 /**< Defines number of repeats while sending mesh message request */
#define DELTA_STEP_VALUE 0x500         /**< Defines Generic Delta minimal step */

static uint8_t *pInstanceIndex = NULL;
static uint8_t *pDeltaTid      = NULL;

void DeltaEncoder_Setup(uint8_t *p_instance_idx, uint8_t *p_tid)
{
    if (!EncoderHal_IsInitialized())
    {
        EncoderHal_Init();
    }

    pInstanceIndex = p_instance_idx;
    pDeltaTid      = p_tid;
}

void DeltaEncoder_Loop(void)
{
    static int long last_message_time = UINT32_MAX;

    long encoder_pos;
    encoder_pos = EncoderHal_GetPosition();

    if (encoder_pos != 0)
    {
        if (Timestamp_GetTimeElapsed(last_message_time, Timestamp_GetCurrent()) >= DELTA_INTVL_MS)
        {
            static uint32_t last_delta_message_time = UINT32_MAX;
            static int      delta                   = 0;

            bool is_new_tid = (Timestamp_GetTimeElapsed(last_delta_message_time, Timestamp_GetCurrent()) > DELTA_NEW_TID_INTVL);
            if (is_new_tid)
            {
                delta = 0;
                Mesh_IncrementTid(pDeltaTid);
            }

            delta += encoder_pos;
            Mesh_SendGenericDeltaSet(*pInstanceIndex,
                                     DELTA_STEP_VALUE * delta,
                                     DELTA_TRANSITION_TIME_MS,
                                     DELTA_DELAY_TIME_MS,
                                     DELTA_NUMBER_OF_REPEATS,
                                     *pDeltaTid);

            if (is_new_tid)
                LOG_D("Delta Start %d", delta);
            else
                LOG_D("Delta Continue %d", delta);

            EncoderHal_SetPosition(0);
            last_delta_message_time = Timestamp_GetCurrent();
            last_message_time       = Timestamp_GetCurrent();
        }
    }
}

#else

void DeltaEncoder_Setup(uint8_t *p_instance_idx, uint8_t *p_tid)
{
    UNUSED(p_instance_idx);
    UNUSED(p_tid);
}

void DeltaEncoder_Loop(void)
{
}

#endif
