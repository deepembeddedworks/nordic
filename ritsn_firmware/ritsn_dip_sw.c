#include "ritsn_dip_sw.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define DIP_SW_NODE nodelabel
#define dip_sw_node DT_NODELABEL(DIP_SW_NODE)

#if !DT_NODE_EXISTS(dip_sw_node)
#error "dip sw Devicetree node does not exist"
#endif

#if !DT_NODE_HAS_STATUS(dip_sw_node, okay)
#error "dip_sw0 Devicetree node is disabled"
#endif

/*
 * dip_sw0 노드의 gpios 속성에는 반드시 6개의 GPIO가 있어야 합니다.
 */
BUILD_ASSERT(
    DT_PROP_LEN(dip_sw_node, gpios) == RITSN_DIP_SW_SIZE,
    "dip_sw0 must contain exactly 6 GPIO entries");

/*
 * 각 DIP 스위치의 GPIO 정보입니다.
 *
 * static:
 *     이 배열은 ritsn_dip_sw.c 내부에서만 사용합니다.
 *
 * const:
 *     Devicetree에서 얻은 GPIO 설정 정보를 실행 중에 변경하지 않습니다.
 */
static const struct gpio_dt_spec dip_sw[RITSN_DIP_SW_SIZE] = {
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 0),
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 1),
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 2),
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 3),
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 4),
    GPIO_DT_SPEC_GET_BY_IDX(dip_sw_node, gpios, 5),
};

int ritsn_dip_sw_init(uint8_t dip_sw_num)
{
    int ret;

    /*
     * DIP 스위치 번호의 범위를 검사합니다.
     */
    if (dip_sw_num >= RITSN_DIP_SW_SIZE) {
        return -EINVAL;
    }

    /*
     * 해당 GPIO 컨트롤러가 사용할 준비가 되었는지 확인합니다.
     *
     * gpio_is_ready_dt()는 bool 값을 반환합니다.
     */
    if (!gpio_is_ready_dt(&dip_sw[dip_sw_num])) {
        return -ENODEV;
    }

    /*
     * DIP 스위치 핀을 입력으로 설정합니다.
     *
     * GPIO_PULL_UP과 GPIO_ACTIVE_LOW 같은 설정은
     * Devicetree의 gpios 플래그에서 가져옵니다.
     */
    ret = gpio_pin_configure_dt(
        &dip_sw[dip_sw_num],
        GPIO_INPUT);

    if (ret < 0) {
        return ret;
    }

    return 0;
}

int ritsn_dip_sw_init_all(void)
{
    int ret;

    /*
     * 0번부터 5번까지 모든 DIP 스위치를 초기화합니다.
     */
    for (uint8_t i = 0U; i < RITSN_DIP_SW_SIZE; i++) {
        ret = ritsn_dip_sw_init(i);

        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

int ritsn_dip_sw_read(
    uint8_t dip_sw_num,
    uint8_t *value)
{
    int ret;

    /*
     * DIP 스위치 번호와 출력 포인터를 검사합니다.
     */
    if (dip_sw_num >= RITSN_DIP_SW_SIZE) {
        return -EINVAL;
    }

    if (value == NULL) {
        return -EINVAL;
    }

    /*
     * GPIO 입력값을 읽습니다.
     *
     * gpio_pin_get_dt()는:
     *
     *   1 이상 : 논리적인 활성 상태
     *   0      : 논리적인 비활성 상태
     *   음수   : 오류
     *
     * DTS에 GPIO_ACTIVE_LOW가 설정되어 있으면 물리적인 LOW가
     * 논리적인 활성 상태 1로 변환됩니다.
     */
    ret = gpio_pin_get_dt(&dip_sw[dip_sw_num]);

    if (ret < 0) {
        return ret;
    }

    /*
     * GPIO 드라이버가 1 이상의 값을 반환할 가능성을 고려하여
     * 외부에는 정확히 0 또는 1만 전달합니다.
     */
    *value = (ret != 0) ? 1U : 0U;

    return 0;
}

int ritsn_dip_sw_read_all(uint8_t *value)
{
    int ret;
    uint8_t result = 0U;
    uint8_t sw_value;

    if (value == NULL) {
        return -EINVAL;
    }

    /*
     * 6개의 DIP 스위치를 차례로 읽고 하나의 6비트 값으로 만듭니다.
     *
     * DIP 0 → bit 0
     * DIP 1 → bit 1
     * DIP 2 → bit 2
     * DIP 3 → bit 3
     * DIP 4 → bit 4
     * DIP 5 → bit 5
     */
    for (uint8_t i = 0U; i < RITSN_DIP_SW_SIZE; i++) {
        ret = ritsn_dip_sw_read(i, &sw_value);

        if (ret < 0) {
            return ret;
        }

        result |= (uint8_t)(sw_value << i);
    }

    /*
     * 모든 GPIO를 성공적으로 읽은 경우에만 호출자 변수에 저장합니다.
     */
    *value = result;

    return 0;
}