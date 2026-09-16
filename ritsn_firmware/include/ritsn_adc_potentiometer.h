#ifndef RITSN_ADC_POTENTIOMETER_H
#define RITSN_ADC_POTENTIOMETER_H

#include <stdint.h>

/**
 * @brief Potentiometer ADC와 전원 GPIO를 초기화합니다.
 *
 * 초기화가 완료되면 potentiometer 측정회로의 전원은
 * 꺼진 상태가 됩니다.
 *
 * @return 0 성공
 * @return -ENODEV ADC 또는 GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr 오류 코드
 */
int ritsn_potentiometer_init(void);

/**
 * @brief Potentiometer 측정회로의 전원을 켭니다.
 *
 * 이 함수는 전원 GPIO만 활성화하며 안정화 시간을
 * 기다리지 않습니다.
 *
 * 안정화 시간은 main에서 관리해야 합니다.
 *
 * @return 0 성공
 * @return -EACCES 모듈이 초기화되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_potentiometer_power_on(void);

/**
 * @brief Potentiometer 측정회로의 전원을 끕니다.
 *
 * 이 함수는 별도의 대기 없이 전원 GPIO를 비활성화합니다.
 *
 * @return 0 성공
 * @return -EACCES 모듈이 초기화되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_potentiometer_power_off(void);

/**
 * @brief Potentiometer ADC 전압을 측정합니다.
 *
 * 이 함수는 다음 작업만 수행합니다.
 *
 * - ADC raw 값 측정
 * - ADC raw 값을 mV로 변환
 * - DTS에 정의된 저항비 적용
 *
 * 이 함수는 전원 ON/OFF 또는 안정화 시간 대기를 수행하지 않습니다.
 *
 * 호출 순서:
 *
 * 1. ritsn_potentiometer_power_on()
 * 2. main에서 안정화 시간 확인
 * 3. ritsn_potentiometer_read_mv()
 * 4. ritsn_potentiometer_power_off()
 *
 * output-ohms와 full-ohms가 모두 1이면 ADC 입력 전압이
 * 변환 없이 그대로 반환됩니다.
 *
 * @param[out] potentiometer_mv 측정 전압(mV)을 저장할 포인터
 *
 * @return 0 성공
 * @return -EACCES 초기화되지 않았거나 전원이 꺼져 있음
 * @return -EINVAL potentiometer_mv가 NULL
 * @return -ERANGE ADC 측정값이 유효하지 않음
 * @return 기타 음수 Zephyr ADC 오류 코드
 */
int ritsn_potentiometer_read_mv(int32_t *potentiometer_mv);

#endif /* RITSN_POTENTIOMETER_H */