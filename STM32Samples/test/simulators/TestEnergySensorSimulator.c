#include "EnergySensorSimulator.c"
#include "MockAssert.h"
#include "MockLuminaire.h"
#include "unity.h"

void setUp(void)
{
    AccumulatedEnergy_uWh = 0;
}

void test_Init(void)
{
    IsInitialized = false;
    TEST_ASSERT_EQUAL(false, EnergySensorSimulator_IsInitialized());

    EnergySensorSimulator_Init();
    TEST_ASSERT_EQUAL(true, EnergySensorSimulator_IsInitialized());
}

void test_Power(void)
{
    uint16_t lightness[] = {
        0,
        32768,
        52428,
        65535,
    };
    uint32_t expected_power_mw[] = {
        4000,
        22000,
        32800,
        40000,
    };

    size_t i;
    for (i = 0; i < ARRAY_SIZE(lightness); i++)
    {
        Luminaire_GetDutyCycle_ExpectAndReturn(lightness[i]);
        TEST_ASSERT_EQUAL(expected_power_mw[i], EnergySensorSimulator_GetPower_mW());
    }
}

void test_Voltage(void)
{
    TEST_ASSERT_EQUAL(230000, EnergySensorSimulator_GetVoltage_mV());
}

void test_Current(void)
{
    uint16_t lightness[] = {
        0,
        32768,
        65535,
    };
    uint32_t expected_current_ma[] = {
        17,
        95,
        173,
    };

    size_t i;
    for (i = 0; i < ARRAY_SIZE(lightness); i++)
    {
        Luminaire_GetDutyCycle_ExpectAndReturn(lightness[i]);
        TEST_ASSERT_EQUAL(expected_current_ma[i], EnergySensorSimulator_GetCurrent_mA());
    }
}

void test_EnergyIdle(void)
{
    uint16_t lightness_idle            = 0;
    uint32_t expected_energy_wh_idle[] = {
        0,
        1,
        2,
        3,
    };
    uint32_t expected_time_to_reach_energy_level_s[] = {
        901,
        900,
        900,
        900,
    };

    Luminaire_GetDutyCycle_IgnoreAndReturn(lightness_idle);

    size_t i;
    for (i = 0; i < ARRAY_SIZE(expected_energy_wh_idle); i++)
    {
        size_t j;
        for (j = 0; j < expected_time_to_reach_energy_level_s[i]; j++)
        {
            TEST_ASSERT_EQUAL(expected_energy_wh_idle[i], EnergySensorSimulator_GetEnergy_Wh());
            EnergySensorSimulator_Loop();
        }
    }
}

void test_EnergyMax(void)
{
    uint16_t lightness_max             = UINT16_MAX;
    uint32_t expected_energy_wh_idle[] = {
        0,
        1,
        2,
        3,
    };
    uint32_t expected_time_to_reach_energy_level_s[] = {
        91,
        90,
        90,
        90,
    };

    Luminaire_GetDutyCycle_IgnoreAndReturn(lightness_max);

    size_t i;
    for (i = 0; i < ARRAY_SIZE(expected_energy_wh_idle); i++)
    {
        size_t j;
        for (j = 0; j < expected_time_to_reach_energy_level_s[i]; j++)
        {
            TEST_ASSERT_EQUAL(expected_energy_wh_idle[i], EnergySensorSimulator_GetEnergy_Wh());
            EnergySensorSimulator_Loop();
        }
    }
}
