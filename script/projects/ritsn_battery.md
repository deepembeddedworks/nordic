RITSN battery ADC 모듈 구현 지침

사용자가 먼저 입력할 DTS node label

다음 항목을 실제 보드 DTS에 선언된 node label(label: 식별자)과 같은 철자로 채운 다음 Claude Code에 전달한다. 노드 이름이나 문자열 label 속성과 구분한다.

battery ADC/전원 노드의 node label: [여기에 입력]
전원 GPIO가 별도 노드에 있다면 그 node label: [여기에 입력 / 같은 노드의 power-gpios라면 해당 없음]

Claude Code는 지정한 DT_NODELABEL(...)의 존재와 io-channels, power-gpios 등 실제 DT 속성을 확인한다. 지정된 label이 DTS에 없다면 임의의 다른 노드를 선택하지 말고 차이를 보고한다.

산출물과 범위

기존 프로젝트 구조에 맞춰 include/ritsn_battery.h와 ritsn_battery.c를 만들거나 기존 동일 기능을 확장하고, 빌드에 등록한다. 목표는 배터리 분압 회로 전원 제어, ADC 초기화, 전압 1회 측정과 환산, 유휴 상태 복귀다. LED 표시, ESB 송신, 레이더 제어는 이 모듈에서 구현하지 않는다.

회로 확인

이전 설계 기록상 battery ADC는 AIN1, 분압 하단 저항은 180 kΩ, 전체 저항은 540 kΩ(상단 360 kΩ + 하단 180 kΩ)이다. 회로도와 DTS로 확인하고 다르면 실제 회로값을 따른다. 확인된 회로라면 배터리 전압은 ADC 노드 전압의 3배다.

분압 회로를 공급하거나 차단하는 GPIO의 핀, ON/OFF 극성, 유휴 시 안전한 상태를 실제 회로에서 확인한다. 과거 기록에 전원 제어 후보 P0.29가 있으나 DIP 입력 핀 기록과 충돌한다. 회로 확인 전 P0.29를 코드나 DTS에 확정하지 않는다.

전원 GPIO를 high-Z(GPIO_INPUT, pull 없음)로 전환했을 때 배터리 분압 회로가 실제로 꺼지는지 외부 pull과 스위칭 소자 연결을 확인한다. high-Z에서 enable 핀이 뜨거나 누설이 생긴다면 확정된 OFF 출력 레벨을 사용한다. ADC 입력 핀의 high-Z 유휴 상태와 전원 GPIO의 OFF 상태는 각각 별도로 검토한다.

ADC gain, reference, acquisition time, resolution, 입력 허용 전압을 실제 NCS 버전과 회로에 맞춘다. 초기 검토값은 gain 1, 내부 reference, 15 µs, 10 bit다. 분압 전압이 ADC에서 측정 가능한 범위를 넘지 않는지 확인한다.

공개 API (include/ritsn_battery.h)

#ifndef RITSN_BATTERY_H_
#define RITSN_BATTERY_H_

#include <stdint.h>

#define RITSN_BATTERY_LOW_MV      2200
#define RITSN_BATTERY_RECOVERY_MV 2400

/* ADC 채널과 분압 회로 전원 GPIO를 준비하고 유휴 상태로 둔다. */
int ritsn_battery_init(void);

/* 배터리 단자 전압을 mV로 반환한다. 성공 0, 실패 음수 errno. */
int ritsn_battery_read_mv(int32_t *battery_mv);

#endif /* RITSN_BATTERY_H_ */

read_mv(NULL)은 -EINVAL, 초기화 전 호출은 -ENODEV처럼 일관된 오류를 반환한다. 출력값은 성공했을 때만 갱신한다. 저전압 판단이 필요하다면 호출부에서 2200 mV 이하 저전압 진입, 2400 mV 이상 회복의 히스테리시스를 적용한다. 모듈 안에 장시간 대기나 LED 점멸 루프를 넣지 않는다.

초기화: ritsn_battery_init()

DTS node label에서 ADC spec과 실제 전원 GPIO를 가져온다. device_is_ready()를 확인하고 adc_channel_setup_dt()로 채널을 준비한다. GPIO가 실재하는 경우 GPIO 장치도 확인한다.

전원 제어 GPIO를 실제 회로에서 OFF가 보장되는 상태로 설정한다. 검증된 경우에만 pull 없는 입력(high-Z)을 OFF 상태로 사용한다.

ADC 입력 핀은 저누설 유휴 상태로 둔다. ADC peripheral 또는 pinctrl 구성을 측정 후 변경한다면 다음 읽기에서 복원할 방법을 마련한다. 입력에 의도하지 않은 pull을 켜지 않는다.

준비 상태를 기록하고 반환한다. 초기화 후 계속 전원을 공급하거나 측정 루프를 시작하지 않는다.

1회 읽기: ritsn_battery_read_mv()

입력 인자와 초기화 상태를 확인한다. 공유 ADC 자원은 다른 ADC 호출과 겹치지 않게 보호한다.

이전 측정 후 분리한 ADC 입력 경로를 측정 가능한 상태로 복구한다.

분압 회로 power GPIO를 ON으로 설정하고 10 ms 안정화 시간을 둔다. 대기는 스레드 문맥에서만 수행한다.

ADC raw 값을 한 번 읽고 실제 reference/gain에 맞춰 ADC 핀 전압(mV)으로 환산한다. 저항값이 확인된 경우 battery_mv = adc_mv * 540000 / 180000에 해당하는 배터리 단자 전압을 계산한다. 정수 계산의 오버플로를 피하고 회로 오차를 고려한다.

성공 및 모든 실패 경로에서 분압 회로를 OFF로 만들고, 실제 nRF/Zephyr pinctrl·ADC 동작에 맞는 방식으로 ADC 경로를 유휴 high-Z 상태로 되돌린다. 전원 GPIO가 high-Z에서 확실히 OFF가 되는 회로일 때만 해당 핀도 high-Z로 둔다. 종료 처리 실패도 호출부에 전달한다.

다음 호출 때 다시 활성화하여 반복 측정할 수 있어야 한다. ADC 오류 때는 이전 값이나 임의의 0 mV를 정상 측정처럼 반환하지 않는다.

DTS, 주기와 검증

지정한 node label에 io-channels와 ADC 채널 설정을 연결한다. power GPIO는 검증된 회로의 핀과 극성을 사용한다. 없는 전원 제어 회로를 소프트웨어 설정으로 가정하지 않는다.

기존 메인 루프에서는 배터리를 약 1시간에 한 번 측정하도록 스케줄한다. 이 모듈 내부에 1시간 대기나 타이머 ISR의 ADC 측정을 넣지 않는다.

초기화 전·NULL 인자·ADC 오류·두 번 연속 읽기를 확인한다. 전원 ON은 측정할 때만, 측정 직후와 실패 후에는 OFF인지 실제 전압과 소비전류로 검증한다.

예상되는 배터리 전압에서 ADC raw, ADC 핀 전압, 계산된 배터리 전압을 대조한다. 최종 핀, power 극성, 유휴 상태, 저항값, 빌드 결과와 실측 결과를 보고한다. 보드가 없다면 실물 검증 여부를 정확히 구분한다.