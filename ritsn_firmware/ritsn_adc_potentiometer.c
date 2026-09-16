#include "ritsn_adc_potentiometer.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#define POTENTIOMETER_NODE DT_NODELABEL(potentiometer0)

#if !DT_NODE_HAS_STATUS(POTENTIOMETER_NODE, okay)
#error "potentiometer0 Devicetree node is disabled or does not exist"
#endif

/*
 * DTS의 저항비 설정을 가져옵니다.
 *
 * DTS 속성 이름:
 *     output-ohms → output_ohms
 *     full-ohms   → full_ohms
 *
 * 두 값이 모두 1이면:
 *
 * Vpotentiometer = Vadc × 1 / 1
 *                = Vadc
 */
#define POTENTIOMETER_OUTPUT_OHMS \
    DT_PROP(POTENTIOMETER_NODE, output_ohms)

#define POTENTIOMETER_FULL_OHMS \
    DT_PROP(POTENTIOMETER_NODE, full_ohms)

/*
 * 잘못된 저항값으로 인한 0 나누기를 컴파일 단계에서 방지합니다.
 */
BUILD_ASSERT(
    POTENTIOMETER_OUTPUT_OHMS > 0,
    "potentiometer output-ohms must be greater than zero");

BUILD_ASSERT(
    POTENTIOMETER_FULL_OHMS > 0,
    "potentiometer full-ohms must be greater than zero");

/*
 * Devicetree의 io-channels 속성에서 ADC 설정을 가져옵니다.
 */
static const struct adc_dt_spec potentiometer_adc =
    ADC_DT_SPEC_GET(POTENTIOMETER_NODE);

/*
 * Devicetree의 power-gpios 속성에서 전원 제어 GPIO 정보를
 * 가져옵니다.
 */
static const struct gpio_dt_spec potentiometer_power =
    GPIO_DT_SPEC_GET(POTENTIOMETER_NODE, power_gpios);

static bool potentiometer_initialized;
static bool potentiometer_power_enabled;

int ritsn_potentiometer_init(void)
{
    int ret;

    /*
     * ADC 장치가 사용할 준비가 되었는지 확인합니다.
     */
    if (!adc_is_ready_dt(&potentiometer_adc)) {
        return -ENODEV;
    }

    /*
     * 전원 제어 GPIO가 사용할 준비가 되었는지 확인합니다.
     */
    if (!gpio_is_ready_dt(&potentiometer_power)) {
        return -ENODEV;
    }

    /*
     * 전원 GPIO를 출력으로 설정합니다.
     *
     * 초기화 직후 potentiometer 측정회로의 전원은 OFF입니다.
     */
    ret = gpio_pin_configure_dt(
        &potentiometer_power,
        GPIO_OUTPUT_INACTIVE);

    if (ret < 0) {
        return ret;
    }

    /*
     * DTS에 정의된 ADC 채널 설정을 적용합니다.
     */
    ret = adc_channel_setup_dt(&potentiometer_adc);

    if (ret < 0) {
        return ret;
    }

    potentiometer_power_enabled = false;
    potentiometer_initialized = true;

    return 0;
}

int ritsn_potentiometer_power_on(void)
{
    int ret;

    if (!potentiometer_initialized) {
        return -EACCES;
    }

    ret = gpio_pin_set_dt(
        &potentiometer_power,
        1);

    if (ret < 0) {
        return ret;
    }

    potentiometer_power_enabled = true;

    return 0;
}

int ritsn_potentiometer_power_off(void)
{
    int ret;

    if (!potentiometer_initialized) {
        return -EACCES;
    }

    ret = gpio_pin_set_dt(
        &potentiometer_power,
        0);

    if (ret < 0) {
        return ret;
    }

    potentiometer_power_enabled = false;

    return 0;
}

int ritsn_potentiometer_read_mv(
    int32_t *potentiometer_mv)
{
    int ret;
    int16_t raw_value = 0;
    int32_t adc_mv;
    int64_t calculated_mv;

    struct adc_sequence sequence = {
        .buffer = &raw_value,
        .buffer_size = sizeof(raw_value),
    };

    if (!potentiometer_initialized) {
        return -EACCES;
    }

    if (!potentiometer_power_enabled) {
        return -EACCES;
    }

    if (potentiometer_mv == NULL) {
        return -EINVAL;
    }

    /*
     * 호출자가 오류 발생 시 이전 값을 측정값으로 오인하지
     * 않도록 출력값을 먼저 초기화합니다.
     */
    *potentiometer_mv = 0;

    /*
     * DTS의 ADC 설정을 바탕으로 sequence를 구성합니다.
     */
    ret = adc_sequence_init_dt(
        &potentiometer_adc,
        &sequence);

    if (ret < 0) {
        return ret;
    }

    /*
     * ADC 변환만 수행합니다.
     *
     * 전원 제어 및 안정화 시간 관리는 main에서 수행합니다.
     */
    ret = adc_read(
        potentiometer_adc.dev,
        &sequence);

    if (ret < 0) {
        return ret;
    }

    adc_mv = (int32_t)raw_value;

    /*
     * ADC raw 값을 mV 단위로 변환합니다.
     */
    ret = adc_raw_to_millivolts_dt(
        &potentiometer_adc,
        &adc_mv);

    if (ret < 0) {
        return ret;
    }

    if (adc_mv < 0) {
        return -ERANGE;
    }

    /*
     * DTS에 정의된 저항비를 적용합니다.
     *
     * 현재 설정:
     *
     * output-ohms = 1
     * full-ohms   = 1
     *
     * 따라서 계산 결과는 adc_mv와 같습니다.
     */
    calculated_mv =
        (int64_t)adc_mv * POTENTIOMETER_FULL_OHMS;

    calculated_mv /=
        POTENTIOMETER_OUTPUT_OHMS;

    *potentiometer_mv = (int32_t)calculated_mv;

    return 0;
}