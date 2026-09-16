#include "ritsn_led.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define GREEN_LED_NODE nodelable
#define RED_LED_NODE nodelable

#define green_led_node DT_NODELABEL(GREEN_LED_NODE)
#define red_led_node   DT_NODELABEL(RED_LED_NODE)

#if !DT_NODE_HAS_STATUS(green_led_node, okay)
#error "green_led Devicetree node is disabled or does not exist"
#endif

#if !DT_NODE_HAS_STATUS(red_led_node, okay)
#error "red_led Devicetree node is disabled or does not exist"
#endif

/*
 * 녹색 LED의 GPIO 정보입니다.
 *
 * static:
 *     이 객체는 ritsn_led.c 내부에서만 사용합니다.
 *
 * const:
 *     Devicetree에서 얻은 GPIO 설정을 실행 중에 변경하지 않습니다.
 */
static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(green_led_node, gpios);

/*
 * 빨간색 LED의 GPIO 정보입니다.
 */
static const struct gpio_dt_spec red_led =
    GPIO_DT_SPEC_GET(red_led_node, gpios);

int ritsn_green_led_init(void)
{
    /*
     * gpio_is_ready_dt()는 bool 값을 반환하므로
     * ret < 0이 아니라 !gpio_is_ready_dt()로 검사합니다.
     */
    if (!gpio_is_ready_dt(&green_led)) {
        return -ENODEV;
    }

    /*
     * 녹색 LED GPIO를 출력으로 설정하고,
     * 초기 논리 상태를 비활성 상태로 설정합니다.
     *
     * GPIO_OUTPUT_INACTIVE:
     *     초기화 직후 LED가 꺼져 있도록 설정합니다.
     */
    return gpio_pin_configure_dt(
        &green_led,
        GPIO_OUTPUT_INACTIVE);
}

int ritsn_red_led_init(void)
{
    if (!gpio_is_ready_dt(&red_led)) {
        return -ENODEV;
    }

    /*
     * 빨간색 LED GPIO를 출력으로 설정하고,
     * 초기화 직후 LED가 꺼진 상태가 되도록 합니다.
     */
    return gpio_pin_configure_dt(
        &red_led,
        GPIO_OUTPUT_INACTIVE);
}

int ritsn_green_led_on(void)
{
    /*
     * gpio_pin_set_dt()에는 논리값을 전달합니다.
     *
     * 1은 활성 상태입니다.
     * DTS가 ACTIVE_HIGH이면 물리적인 HIGH가 출력되고,
     * DTS가 ACTIVE_LOW이면 물리적인 LOW가 출력됩니다.
     */
    return gpio_pin_set_dt(&green_led, 1);
}

int ritsn_red_led_on(void)
{
    return gpio_pin_set_dt(&red_led, 1);
}

int ritsn_green_led_off(void)
{
    /*
     * 0은 비활성 상태입니다.
     *
     * 실제 출력 레벨은 DTS의 ACTIVE_HIGH/ACTIVE_LOW 설정에
     * 따라 자동으로 변환됩니다.
     */
    return gpio_pin_set_dt(&green_led, 0);
}

int ritsn_red_led_off(void)
{
    return gpio_pin_set_dt(&red_led, 0);
}