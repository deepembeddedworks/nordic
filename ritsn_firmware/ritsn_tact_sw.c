#include "ritsn_tact_sw.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define TACT_SW_NODE DT_NODELABEL(tact_sw0)

#if !DT_NODE_HAS_STATUS(TACT_SW_NODE, okay)
#error "tact_sw0 Devicetree node is disabled or does not exist"
#endif

/*
 * Tact 스위치의 GPIO 정보입니다.
 *
 * static:
 *     tact_sw 객체를 이 소스 파일 내부에서만 사용합니다.
 *
 * const:
 *     Devicetree에서 얻은 포트, 핀 번호 및 GPIO 플래그를
 *     실행 중에 변경하지 않습니다.
 */
static const struct gpio_dt_spec tact_sw =
    GPIO_DT_SPEC_GET(TACT_SW_NODE, gpios);

/*
 * Zephyr GPIO 드라이버에 등록할 콜백 객체입니다.
 *
 * 콜백 객체는 인터럽트가 사용되는 동안 계속 존재해야 하므로
 * 함수의 지역변수가 아닌 static 전역변수로 선언합니다.
 */
static struct gpio_callback tact_sw_callback;

/*
 * 초기화 및 콜백 등록 상태를 저장합니다.
 *
 * static 전역변수는 명시적으로 초기화하지 않아도 false로
 * 자동 초기화됩니다.
 */
static bool tact_sw_initialized;
static bool callback_registered;

int ritsn_tact_sw_init(void)
{
    int ret;

    /*
     * Tact 스위치가 연결된 GPIO 컨트롤러가 준비되었는지
     * 확인합니다.
     */
    if (!gpio_is_ready_dt(&tact_sw)) {
        return -ENODEV;
    }

    /*
     * Tact 스위치 핀을 입력으로 설정합니다.
     *
     * GPIO_PULL_UP과 GPIO_ACTIVE_LOW는 DTS의 gpios 속성에
     * 설정된 값을 사용합니다.
     */
    ret = gpio_pin_configure_dt(
        &tact_sw,
        GPIO_INPUT);

    if (ret < 0) {
        return ret;
    }

    /*
     * 초기화 직후에는 인터럽트를 비활성화합니다.
     *
     * ritsn_tact_sw_interrupt_enable()을 호출해야
     * 버튼 인터럽트가 활성화됩니다.
     */
    ret = gpio_pin_interrupt_configure_dt(
        &tact_sw,
        GPIO_INT_DISABLE);

    if (ret < 0) {
        return ret;
    }

    tact_sw_initialized = true;

    return 0;
}

int ritsn_tact_sw_interrupt_enable(
    gpio_callback_handler_t callback_handler)
{
    int ret;

    /*
     * GPIO가 초기화되지 않았다면 인터럽트를 설정할 수 없습니다.
     */
    if (!tact_sw_initialized) {
        return -EACCES;
    }

    /*
     * 호출할 콜백 함수가 반드시 필요합니다.
     */
    if (callback_handler == NULL) {
        return -EINVAL;
    }

    /*
     * 기존 콜백이 등록되어 있으면 먼저 제거합니다.
     *
     * 이렇게 하면 다른 콜백 함수로 변경하거나 인터럽트를
     * 다시 활성화해도 콜백이 중복 등록되지 않습니다.
     */
    if (callback_registered) {
        ret = gpio_remove_callback(
            tact_sw.port,
            &tact_sw_callback);

        if (ret < 0) {
            return ret;
        }

        callback_registered = false;
    }

    /*
     * 콜백 객체에 호출할 함수와 감시할 GPIO 핀을 설정합니다.
     */
    gpio_init_callback(
        &tact_sw_callback,
        callback_handler,
        BIT(tact_sw.pin));

    /*
     * 설정한 콜백 객체를 GPIO 컨트롤러에 등록합니다.
     */
    ret = gpio_add_callback(
        tact_sw.port,
        &tact_sw_callback);

    if (ret < 0) {
        return ret;
    }

    callback_registered = true;

    /*
     * GPIO가 비활성 상태에서 활성 상태로 변할 때 인터럽트를
     * 발생시킵니다.
     *
     * DTS에 GPIO_ACTIVE_LOW가 설정되어 있다면 물리적인 입력이
     * HIGH에서 LOW로 변할 때 논리적인 활성 상태가 됩니다.
     */
    ret = gpio_pin_interrupt_configure_dt(
        &tact_sw,
        GPIO_INT_EDGE_TO_ACTIVE);

    if (ret < 0) {
        /*
         * 인터럽트 설정에 실패했다면 앞에서 등록한 콜백도
         * 제거하여 상태를 원상 복구합니다.
         */
        (void)gpio_remove_callback(
            tact_sw.port,
            &tact_sw_callback);

        callback_registered = false;

        return ret;
    }

    return 0;
}

int ritsn_tact_sw_interrupt_disable(void)
{
    /*
     * 초기화되지 않은 GPIO에는 인터럽트 설정을 적용할 수 없습니다.
     */
    if (!tact_sw_initialized) {
        return -EACCES;
    }

    /*
     * GPIO 콜백 객체는 유지하면서 핀 인터럽트만 비활성화합니다.
     */
    return gpio_pin_interrupt_configure_dt(
        &tact_sw,
        GPIO_INT_DISABLE);
}

int ritsn_tact_sw_get(void)
{
    int ret;

    if (!tact_sw_initialized) {
        return -EACCES;
    }

    /*
     * 논리적인 GPIO 입력값을 읽습니다.
     *
     * GPIO_ACTIVE_LOW가 적용되어 있으면:
     *
     * 물리적 LOW  → 논리값 1
     * 물리적 HIGH → 논리값 0
     */
    ret = gpio_pin_get_dt(&tact_sw);

    if (ret < 0) {
        return ret;
    }

    /*
     * GPIO 드라이버가 1보다 큰 활성값을 반환하더라도
     * 외부에는 정확히 0 또는 1을 반환합니다.
     */
    return (ret != 0) ? 1 : 0;
}