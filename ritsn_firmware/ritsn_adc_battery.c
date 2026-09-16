#include "ritsn_adc_battery.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#define BATTERY_NODE DT_NODELABEL(vbatt)

#if !DT_NODE_HAS_STATUS(BATTERY_NODE, okay)
#error "vbatt Devicetree node is disabled or does not exist"
#endif

/*
 * 분압저항:
 *
 * Battery+ --- 120 kΩ --- ADC --- 100 kΩ --- GND
 *
 * Vbattery = Vadc × 220 kΩ / 100 kΩ
 *          = Vadc × 2.2
 */
#define BATTERY_DIVIDER_OUTPUT_OHMS  100000
#define BATTERY_DIVIDER_FULL_OHMS    220000LL

#define BATTERY_LOW_MV               2200
#define BATTERY_RECOVERY_MV          2400

static const struct adc_dt_spec battery_adc =
    ADC_DT_SPEC_GET(BATTERY_NODE);

static const struct gpio_dt_spec battery_power =
    GPIO_DT_SPEC_GET(BATTERY_NODE, power_gpios);

static bool battery_initialized;
static bool battery_power_enabled;
static bool battery_low_state;

int ritsn_battery_init(void)
{
    int ret;

    if (!adc_is_ready_dt(&battery_adc)) {
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&battery_power)) {
        return -ENODEV;
    }

    /*
     * 초기화 직후 배터리 측정회로 전원은 OFF입니다.
     */
    ret = gpio_pin_configure_dt(
        &battery_power,
        GPIO_OUTPUT_INACTIVE);

    if (ret < 0) {
        return ret;
    }

    ret = adc_channel_setup_dt(&battery_adc);
    if (ret < 0) {
        return ret;
    }

    battery_power_enabled = false;
    battery_low_state = false;
    battery_initialized = true;

    return 0;
}

int ritsn_battery_power_on(void)
{
    int ret;

    if (!battery_initialized) {
        return -EACCES;
    }

    ret = gpio_pin_set_dt(&battery_power, 1);
    if (ret < 0) {
        return ret;
    }

    battery_power_enabled = true;

    return 0;
}

int ritsn_battery_power_off(void)
{
    int ret;

    if (!battery_initialized) {
        return -EACCES;
    }

    ret = gpio_pin_set_dt(&battery_power, 0);
    if (ret < 0) {
        return ret;
    }

    battery_power_enabled = false;

    return 0;
}

int ritsn_battery_read_mv(int32_t *battery_mv)
{
    int ret;
    int16_t raw_value = 0;
    int32_t adc_mv;
    int64_t calculated_mv;

    struct adc_sequence sequence = {
        .buffer = &raw_value,
        .buffer_size = sizeof(raw_value),
    };

    if (!battery_initialized) {
        return -EACCES;
    }

    if (!battery_power_enabled) {
        return -EACCES;
    }

    if (battery_mv == NULL) {
        return -EINVAL;
    }

    /*
     * 이 함수에서는 다음 작업을 하지 않습니다.
     *
     * - 배터리 측정회로 전원 ON
     * - 안정화 시간 대기
     * - 배터리 측정회로 전원 OFF
     *
     * 위 작업은 main에서 관리합니다.
     */
    ret = adc_sequence_init_dt(
        &battery_adc,
        &sequence);

    if (ret < 0) {
        return ret;
    }

    /*
     * ADC 변환이 완료될 때까지의 짧은 하드웨어 대기는
     * adc_read() 내부에 존재하지만, 별도의 10ms k_sleep은 없습니다.
     */
    ret = adc_read(
        battery_adc.dev,
        &sequence);

    if (ret < 0) {
        return ret;
    }

    adc_mv = (int32_t)raw_value;

    ret = adc_raw_to_millivolts_dt(
        &battery_adc,
        &adc_mv);

    if (ret < 0) {
        return ret;
    }

    if (adc_mv < 0) {
        return -ERANGE;
    }

    calculated_mv =
        (int64_t)adc_mv * BATTERY_DIVIDER_FULL_OHMS;

    calculated_mv /=
        BATTERY_DIVIDER_OUTPUT_OHMS;

    *battery_mv = (int32_t)calculated_mv;

    return 0;
}

bool ritsn_battery_is_low(int32_t *battery_mv)
{
    int ret;
    int32_t measured_mv;

    /*
     * 이 함수 역시 전원 ON/OFF나 대기를 하지 않습니다.
     * main에서 측정회로 전원을 켜고 안정화 시간이 지난 뒤
     * 호출해야 합니다.
     */
    ret = ritsn_battery_read_mv(&measured_mv);

    if (ret < 0) {
        return battery_low_state;
    }

    if (battery_mv != NULL) {
        *battery_mv = measured_mv;
    }

    if (!battery_low_state) {
        if (measured_mv <= BATTERY_LOW_MV) {
            battery_low_state = true;
        }
    } else {
        if (measured_mv >= BATTERY_RECOVERY_MV) {
            battery_low_state = false;
        }
    }

    return battery_low_state;
}