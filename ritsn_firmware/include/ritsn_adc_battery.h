#ifndef RITSN_ADC_BATTERY_H
#define RITSN_ADC_BATTERY_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 배터리 ADC와 측정회로 전원 GPIO를 초기화합니다.
 *
 * 다음 작업을 수행합니다.
 *
 * - 배터리 ADC 채널 설정
 * - 배터리 측정회로 전원 GPIO 출력 설정
 * - 배터리 측정회로 전원 OFF
 * - 저전압 상태 초기화
 *
 * @return 0 성공
 * @return -ENODEV ADC 또는 GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr 오류 코드
 */
int ritsn_battery_init(void);

/**
 * @brief 배터리 측정회로의 전원을 켭니다.
 *
 * 이 함수는 전원 GPIO만 활성화하며 안정화 시간을 기다리지 않습니다.
 * 호출자는 전원을 켠 후 필요한 안정화 시간을 직접 관리해야 합니다.
 *
 * 일반적인 사용 순서:
 *
 * 1. ritsn_battery_power_on()
 * 2. main에서 안정화 시간 확인
 * 3. ritsn_battery_read_mv()
 * 4. ritsn_battery_power_off()
 *
 * @return 0 성공
 * @return -EACCES 배터리 모듈이 초기화되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_battery_power_on(void);

/**
 * @brief 배터리 측정회로의 전원을 끕니다.
 *
 * 이 함수는 별도의 대기 없이 전원 GPIO를 즉시 비활성화합니다.
 * 배터리 측정이 끝나거나 측정 중 오류가 발생한 경우에도
 * 호출하는 것이 좋습니다.
 *
 * @return 0 성공
 * @return -EACCES 배터리 모듈이 초기화되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_battery_power_off(void);

/**
 * @brief 현재 ADC를 이용하여 배터리 전압을 측정합니다.
 *
 * 이 함수는 다음 작업만 수행합니다.
 *
 * - ADC raw 값 측정
 * - ADC 값을 mV로 변환
 * - 분압비를 적용하여 실제 배터리 전압 계산
 *
 * 이 함수는 다음 작업을 수행하지 않습니다.
 *
 * - 배터리 측정회로 전원 ON
 * - 측정회로 안정화 시간 대기
 * - 배터리 측정회로 전원 OFF
 *
 * 따라서 호출 전에 main에서 ritsn_battery_power_on()을 호출하고,
 * 안정화 시간이 지난 것을 확인해야 합니다. 측정 후에는
 * ritsn_battery_power_off()을 호출해야 합니다.
 *
 * @param[out] battery_mv 측정된 실제 배터리 전압(mV)을
 *                        저장할 포인터
 *
 * @return 0 성공
 * @return -EACCES 초기화되지 않았거나 측정회로 전원이 꺼져 있음
 * @return -EINVAL battery_mv가 NULL
 * @return -ERANGE ADC 측정 전압이 유효하지 않음
 * @return 기타 음수 Zephyr ADC 오류 코드
 */
int ritsn_battery_read_mv(int32_t *battery_mv);

/**
 * @brief 배터리 전압을 측정하고 저전압 상태를 확인합니다.
 *
 * 이 함수는 내부에서 ritsn_battery_read_mv()를 호출합니다.
 * 따라서 전원 ON/OFF 및 안정화 시간 대기를 수행하지 않습니다.
 *
 * 호출 전에 main에서 배터리 측정회로의 전원을 켜고
 * 안정화 시간이 지난 것을 확인해야 합니다.
 *
 * 저전압 판단에는 다음과 같은 히스테리시스가 적용됩니다.
 *
 * - 정상 상태에서 2200mV 이하: 저전압 상태 진입
 * - 저전압 상태에서 2400mV 이상: 정상 상태 복구
 *
 * @param[out] battery_mv 측정 전압을 저장할 포인터.
 *                        측정값이 필요하지 않으면 NULL을
 *                        전달할 수 있습니다.
 *
 * @return true 저전압 상태
 * @return false 정상 상태
 *
 * @note ADC 측정이 실패하면 이전 저전압 상태를 유지하여 반환합니다.
 *       이 함수는 bool만 반환하므로 측정 오류 코드는 제공하지 않습니다.
 *       오류 처리가 필요하면 ritsn_battery_read_mv()를 직접 사용하십시오.
 */
bool ritsn_battery_is_low(int32_t *battery_mv);

#endif /* RITSN_BATTERY_H */