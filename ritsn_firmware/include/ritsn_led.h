#ifndef RITSN_LED_H
#define RITSN_LED_H

/**
 * @brief 녹색 LED GPIO를 초기화합니다.
 *
 * 초기화가 완료되면 녹색 LED는 꺼진 상태가 됩니다.
 *
 * @return 0 성공
 * @return -ENODEV GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_green_led_init(void);

/**
 * @brief 빨간색 LED GPIO를 초기화합니다.
 *
 * 초기화가 완료되면 빨간색 LED는 꺼진 상태가 됩니다.
 *
 * @return 0 성공
 * @return -ENODEV GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_red_led_init(void);

/**
 * @brief 녹색 LED를 켭니다.
 *
 * GPIO_ACTIVE_HIGH 또는 GPIO_ACTIVE_LOW 설정은
 * Devicetree의 gpios 속성을 따릅니다.
 *
 * @return 0 성공
 * @return 음수 Zephyr GPIO 오류 코드
 */
int ritsn_green_led_on(void);

/**
 * @brief 빨간색 LED를 켭니다.
 *
 * GPIO_ACTIVE_HIGH 또는 GPIO_ACTIVE_LOW 설정은
 * Devicetree의 gpios 속성을 따릅니다.
 *
 * @return 0 성공
 * @return 음수 Zephyr GPIO 오류 코드
 */
int ritsn_red_led_on(void);

/**
 * @brief 녹색 LED를 끕니다.
 *
 * @return 0 성공
 * @return 음수 Zephyr GPIO 오류 코드
 */
int ritsn_green_led_off(void);

/**
 * @brief 빨간색 LED를 끕니다.
 *
 * @return 0 성공
 * @return 음수 Zephyr GPIO 오류 코드
 */
int ritsn_red_led_off(void);

#endif /* RITSN_LED_H */