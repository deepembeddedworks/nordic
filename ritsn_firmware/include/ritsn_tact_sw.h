#ifndef RITSN_TACT_SW_H
#define RITSN_TACT_SW_H

#include <zephyr/drivers/gpio.h>

/**
 * @brief Tact 스위치 GPIO를 초기화합니다.
 *
 * Tact 스위치 핀을 입력 모드로 설정하고 인터럽트는
 * 비활성화 상태로 초기화합니다.
 *
 * Pull-up과 Active-Low 설정은 Devicetree의 gpios 속성을
 * 통해 적용됩니다.
 *
 * @return 0 성공
 * @return -ENODEV GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_tact_sw_init(void);

/**
 * @brief Tact 스위치 인터럽트를 활성화합니다.
 *
 * Tact 스위치가 논리적인 활성 상태가 되는 순간 콜백 함수가
 * 호출되도록 GPIO 인터럽트를 설정합니다.
 *
 * GPIO_ACTIVE_LOW 스위치라면 버튼을 눌러 입력이 LOW가 되는
 * 순간 인터럽트가 발생합니다.
 *
 * @param callback_handler 인터럽트가 발생할 때 호출할 함수
 *
 * @return 0 성공
 * @return -EACCES Tact 스위치가 초기화되지 않음
 * @return -EINVAL callback_handler가 NULL
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_tact_sw_interrupt_enable(
    gpio_callback_handler_t callback_handler);

/**
 * @brief Tact 스위치 인터럽트를 비활성화합니다.
 *
 * 등록된 콜백 객체는 유지하지만 GPIO 핀에서 인터럽트가
 * 발생하지 않도록 설정합니다.
 *
 * 이후 ritsn_tact_sw_interrupt_enable()을 호출하면
 * 다시 인터럽트를 사용할 수 있습니다.
 *
 * @return 0 성공
 * @return -EACCES Tact 스위치가 초기화되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_tact_sw_interrupt_disable(void);

/**
 * @brief 현재 Tact 스위치의 논리적인 입력값을 읽습니다.
 *
 * DTS에 GPIO_ACTIVE_LOW가 설정되어 있으면:
 *
 * - 버튼 눌림: 1
 * - 버튼 해제: 0
 *
 * @return 1 버튼이 눌린 상태
 * @return 0 버튼이 눌리지 않은 상태
 * @return 음수 Zephyr GPIO 오류 코드
 */
int ritsn_tact_sw_get(void);

#endif /* RITSN_TACT_SW_H */