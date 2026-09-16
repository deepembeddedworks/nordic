#ifndef RITSN_DIP_SW_H
#define RITSN_DIP_SW_H

#include <stdint.h>

/**
 * @brief RITSN 보드의 DIP 스위치 개수입니다.
 */
#define RITSN_DIP_SW_SIZE 6U

/**
 * @brief 지정한 DIP 스위치 GPIO를 초기화합니다.
 *
 * DIP 스위치 번호에 해당하는 GPIO 핀을 입력 모드로 설정합니다.
 * Pull-up과 Active-Low 설정은 Devicetree의 gpios 속성을 따릅니다.
 *
 * @param dip_sw_num 초기화할 DIP 스위치 번호.
 *                   유효 범위는 0부터 RITSN_DIP_SW_SIZE - 1까지입니다.
 *
 * @return 0 성공
 * @return -EINVAL 잘못된 DIP 스위치 번호
 * @return -ENODEV GPIO 장치가 준비되지 않음
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_dip_sw_init(uint8_t dip_sw_num);

/**
 * @brief 모든 DIP 스위치 GPIO를 초기화합니다.
 *
 * 0번부터 5번까지 모든 DIP 스위치를 입력 모드로 설정합니다.
 *
 * @return 0 모든 DIP 스위치 초기화 성공
 * @return 음수 특정 DIP 스위치 초기화 실패
 */
int ritsn_dip_sw_init_all(void);

/**
 * @brief 지정한 DIP 스위치 값을 읽습니다.
 *
 * Devicetree에 GPIO_ACTIVE_LOW가 설정되어 있으면
 * 물리적인 LOW 입력은 논리값 1로 변환됩니다.
 *
 * 일반적인 Active-Low DIP 스위치에서는:
 *
 * - 스위치 ON  → 1
 * - 스위치 OFF → 0
 *
 * @param dip_sw_num 읽을 DIP 스위치 번호.
 *                   유효 범위는 0부터 RITSN_DIP_SW_SIZE - 1까지입니다.
 * @param[out] value 읽은 값을 저장할 포인터.
 *                   결과는 0 또는 1입니다.
 *
 * @return 0 성공
 * @return -EINVAL 잘못된 인수 또는 NULL 포인터
 * @return 기타 음수 Zephyr GPIO 오류 코드
 */
int ritsn_dip_sw_read(uint8_t dip_sw_num, uint8_t *value);

/**
 * @brief 6개의 DIP 스위치를 모두 읽어 하나의 값으로 반환합니다.
 *
 * 각 DIP 스위치 값은 다음 비트에 저장됩니다.
 *
 * - DIP 0 → bit 0
 * - DIP 1 → bit 1
 * - DIP 2 → bit 2
 * - DIP 3 → bit 3
 * - DIP 4 → bit 4
 * - DIP 5 → bit 5
 *
 * 따라서 반환되는 유효 값의 범위는 0부터 63까지입니다.
 *
 * 예:
 * DIP 0과 DIP 2가 ON이면 결과는 0b000101, 즉 5입니다.
 *
 * @param[out] value 6개 DIP 스위치의 조합 값을 저장할 포인터.
 *
 * @return 0 성공
 * @return -EINVAL value가 NULL
 * @return 기타 음수 DIP 스위치 읽기 오류 코드
 */
int ritsn_dip_sw_read_all(uint8_t *value);

#endif /* RITSN_DIP_SW_H */