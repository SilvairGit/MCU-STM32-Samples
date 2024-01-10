#ifndef EMG_L_TEST_H
#define EMG_L_TEST_H

#include <stdbool.h>
#include <stdint.h>

#include "Mesh.h"
#include "Utils.h"


enum EmgLTest_ELSubOpcode
{
    EMG_L_TEST_EL_SUBOPCODE_INHIBIT_ENTER         = 0x00,
    EMG_L_TEST_EL_SUBOPCODE_INHIBIT_EXIT          = 0x02,
    EMG_L_TEST_EL_SUBOPCODE_STATE_GET             = 0x04,
    EMG_L_TEST_EL_SUBOPCODE_STATE_STATUS          = 0x05,
    EMG_L_TEST_EL_SUBOPCODE_PROPERTY_STATUS       = 0x09,
    EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_GET    = 0x0A,
    EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_CLEAR  = 0x0B,
    EMG_L_TEST_EL_SUBOPCODE_OPERATION_TIME_STATUS = 0x0D,
    EMG_L_TEST_EL_SUBOPCODE_REST_ENTER            = 0x0E,
    EMG_L_TEST_EL_SUBOPCODE_REST_EXIT             = 0x10,
};

enum EmgLTest_ElState
{
    EMG_L_TEST_EL_STATE_NORMAL                      = 0x03,
    EMG_L_TEST_EL_STATE_EMERGENCY                   = 0x05,
    EMG_L_TEST_EL_STATE_EXTENDED_EMERGENCY          = 0x06,
    EMG_L_TEST_EL_STATE_REST                        = 0x08,
    EMG_L_TEST_EL_STATE_INHIBIT                     = 0x0A,
    EMG_L_TEST_EL_STATE_DURATION_TEST_IN_PROGRESS   = 0x0C,
    EMG_L_TEST_EL_STATE_FUNCTIONAL_TEST_IN_PROGRESS = 0x0E,
    EMG_L_TEST_EL_STATE_BATTERY_DISCHARGED          = 0x0F
};

enum EmgLTest_PropertyId
{
    EMG_L_TEST_PROPERTY_ID_LIGHTNESS    = 0xFF80,
    EMG_L_TEST_PROPERTY_ID_PROLONG_TIME = 0xFF83
};

enum EmgLTest_ELTestSubOpcode
{
    EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_GET    = 0x00,
    EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_START  = 0x01,
    EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_STOP   = 0x02,
    EMG_L_TEST_EL_TEST_SUBOPCODE_FUNCTIONAL_TEST_STATUS = 0x03,
    EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_GET      = 0x04,
    EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_START    = 0x05,
    EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_STOP     = 0x06,
    EMG_L_TEST_EL_TEST_SUBOPCODE_DURATION_TEST_STATUS   = 0x07
};

enum EmgLTest_TestStatus
{
    EMG_L_TEST_TEST_STATUS_FINISHED = 0x00,
    EMG_L_TEST_TEST_STATUS_UNKNOWN  = 0x07
};

struct PACKED EmgLTest_ElStateStatus
{
    enum EmgLTest_ElState state;
};

struct PACKED EmgLTest_ElPropertyStatus
{
    enum EmgLTest_PropertyId property_id;
    uint16_t                 property_value;
};

struct PACKED EmgLTest_ElOperationTimeStatus
{
    uint32_t total_operation_time;
    uint32_t emergency_time;
};

struct PACKED EmgLTest_TestResult
{
    uint8_t lamp_fault : 1;
    uint8_t battery_fault : 1;
    uint8_t circuit_fault : 1;
    uint8_t battery_duration_fault : 1;
    uint8_t rfu : 4;
};

struct PACKED EmgLTest_EltFunctionalTestStatus
{
    enum EmgLTest_TestStatus   status;
    struct EmgLTest_TestResult result;
};

struct PACKED EmgLTest_EltDurationTestStatus
{
    enum EmgLTest_TestStatus   status;
    struct EmgLTest_TestResult result;
    uint16_t                   test_length;
};

STATIC_ASSERT(sizeof(struct EmgLTest_ElStateStatus) == 1, Wrong_size_of_the_struct_EmgLTest_ElStateStatus);
STATIC_ASSERT(sizeof(struct EmgLTest_ElPropertyStatus) == 4, Wrong_size_of_the_struct_EmgLTest_ElPropertyStatus);
STATIC_ASSERT(sizeof(struct EmgLTest_ElOperationTimeStatus) == 8, Wrong_size_of_the_struct_EmgLTest_ElOperationTimeStatus);
STATIC_ASSERT(sizeof(struct EmgLTest_EltFunctionalTestStatus) == 2, Wrong_size_of_the_struct_EmgLTest_EltFunctionalTestStatus);
STATIC_ASSERT(sizeof(struct EmgLTest_EltDurationTestStatus) == 4, Wrong_size_of_the_struct_EmgLTest_EltDurationTestStatus);


void EmgLTest_Init(void);
bool EmgLTest_IsInitialized(void);

#endif
