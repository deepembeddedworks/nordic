RITSN potentiometer ADC 모듈 구현 지침

사용자가 먼저 입력할 DTS node label

아래 두 항목을 실제 보드 DTS에 선언된 node label과 정확히 같은 철자로 채운 다음 Claude Code에 이 문서를 전달한다. DT_NODELABEL(...) 안에는 DTS의 label: 식별자를 쓰며, 노드 이름이나 문자열 label 속성값을 쓰지 않는다.

potentiometer ADC/전원 노드의 node label: [여기에 입력]
전원 GPIO가 별도 노드에 있다면 그 node label: [여기에 입력 / 같은 노드의 power-gpios라면 해당 없음]

Claude Code는 입력된 label에 대해 DT_NODE_EXISTS(DT_NODELABEL(...)) 등을 사용해 빌드 시 존재 여부를 확인한다. 첫 번째 노드의 io-channels, power-gpios 속성 또는 두 번째 노드의 GPIO 정의 중 실제 DTS 구조에 맞는 항목으로 ADC 및 전원을 가져온다. 이 문서에 적힌 label과 실제 DTS가 다르면 임의로 다른 노드에 연결하지 말고 차이를 보고한다.

산출물

애플리케이션의 include/ritsn_potentiometer.h와 소스 디렉터리의 ritsn_potentiometer.c를 구현하고 빌드에 등록한다. 소스 디렉터리 구조와 CMake 규칙은 실제 프로젝트를 따른다. 이 문서는 potentiometer의 전원 제어, ADC 초기화와 1회 측정, 측정 후 high-Z 복귀만 다룬다. 배터리, DIP, tact, LED, 레이더 구간 선택은 구현하지 않는다.

먼저 회로와 DTS 확인

Potentiometer의 ADC 신호가 실제로 연결된 AIN 입력과 전원 제어 GPIO의 물리 핀을 회로도 및 보드 DTS/overlay에서 확인한다. 기존 설계상 ADC 입력 후보는 AIN0이지만 실제 회로로 검증한다.

전원 제어 회로가 GPIO HIGH에서 켜지는지, LOW에서 켜지는지, GPIO를 high-Z(GPIO_INPUT, pull 없음)로 두었을 때 확실히 OFF가 되는지 회로의 저항과 트랜지스터 연결로 확인한다. high-Z 상태에서 전원 enable 입력이 떠서 ON이 될 수 있다면 high-Z를 OFF 동작으로 사용하지 말고, 회로에 맞는 확정된 OFF 출력 상태를 사용한다. 필요한 경우 회로 변경 또는 외부 pull 저항을 명시한다.

ADC 입력 핀과 전원 제어 핀은 별도 핀이다. 측정 후 어느 핀을 high-Z로 만드는지 회로도에 맞춰 지정한다. ADC가 활성인 동안 연결된 입력이 주변 회로를 역으로 공급하거나 전류를 소모하는지도 확인한다.

ADC resolution, gain, reference, acquisition time 및 입력 최대 전압을 확인한다. 초기 검토값은 10 bit, gain 1, internal reference, 15 µs이다. 최종 DTS 설정은 보드의 0–1.8 V 입력 범위와 해당 NCS 버전의 ADC 제약에 맞춘다.

공개 API (include/ritsn_potentiometer.h)

프로젝트 관례에 맞게 선언하되 다음 기능과 반환 의미를 제공한다.

#ifndef RITSN_POTENTIOMETER_H_
#define RITSN_POTENTIOMETER_H_

#include <stdint.h>

/* ADC 채널과 전원 GPIO를 준비하고 유휴 상태를 설정한다. */
int ritsn_potentiometer_init(void);

/* 1회 측정하여 mV를 기록한다. 성공 시 0, 실패 시 음수 errno. */
int ritsn_potentiometer_read_mv(int32_t *millivolts);

#endif /* RITSN_POTENTIOMETER_H_ */

read_mv(NULL)은 -EINVAL, 초기화 전 읽기는 -ENODEV 같은 일관된 오류를 반환한다. 출력 포인터의 값은 측정 성공 시에만 갱신한다. 기존 프로젝트에 API 이름 규칙이 이미 있으면 그 규칙을 적용하고 호출부도 함께 맞춘다.

초기화: ritsn_potentiometer_init()

ADC_DT_SPEC_GET 또는 프로젝트의 동등한 DT spec으로 ADC 장치를 얻고 device_is_ready()를 확인한다. 확인된 DTS 노드를 사용하며 C 코드에 AIN 핀 번호를 중복 하드코딩하지 않는다.

adc_channel_setup_dt()로 채널을 설정한다. 전원 GPIO가 있다면 GPIO_DT_SPEC_GET으로 얻고 준비 상태를 검사한다.

전원 GPIO를 회로상 OFF가 보장되는 상태로 놓는다. 검증된 high-Z가 OFF 상태라면 pull 없는 GPIO 입력으로 설정한다. 확정된 출력 레벨이 필요하면 그 레벨로 설정한다.

ADC 입력 핀은 유휴 시 누설과 역급전을 막는 구성으로 둔다. ADC peripheral/pinctrl을 측정 전에 복구해야 한다면 읽기 함수에서 복구하도록 설계한다. ADC 입력에 내부 pull-up/down을 활성화하지 않는다.

초기화 완료 상태를 기록하여 이후 read_mv()에서 다시 읽을 수 있게 한다. 초기화가 ADC 값을 계속 읽거나 potentiometer 전원을 계속 켜 두는 상태가 되어서는 안 된다.

1회 읽기: ritsn_potentiometer_read_mv()

인자와 초기화 상태를 검사한다. 같은 ADC 채널을 다른 코드와 공유한다면 호출 간 동시 접근을 막는다.

지난 측정 후 ADC 핀/주변장치를 분리했다면 측정 가능한 ADC 입력 상태로 복구한다. 필요한 채널 재설정과 pinctrl active 전환을 수행한다.

전원 GPIO를 회로상 ON 상태로 설정한다. 실제로 전원을 공급했음을 확인한 뒤 10 ms 정착 시간을 기다린다. 대기는 GPIO ISR이 아닌 스레드 문맥에서 수행한다.

adc_read_dt() 또는 SDK 버전에 맞는 ADC API로 raw 값을 한 번 읽는다. 실제 ADC gain/reference에 맞춰 adc_raw_to_millivolts_dt() 등으로 mV를 계산한다. 성공한 경우에만 *millivolts를 기록한다.

성공과 모든 실패 경로에서 동일한 종료 처리를 수행한다: potentiometer 전원을 회로상 OFF 상태로 돌리고, 회로/SDK에서 허용하는 방식으로 해당 ADC 입력 경로를 high-Z 유휴 상태로 돌린다. 단, high-Z가 전원 OFF를 보장하지 않는 GPIO를 무조건 입력으로 바꾸지 않는다. OFF 처리 자체가 실패하면 그 오류도 호출부에 전달한다.

다음 read_mv() 호출에서는 2단계부터 다시 활성화하여 반복 측정이 가능해야 한다. 포인터 반환값과 오류 코드는 명확하게 유지한다.

High-Z는 핀에 출력 전압을 걸지 않는 전기적 상태를 뜻한다. gpio_pin_set(..., 0)만으로 high-Z가 되지는 않는다. GPIO를 GPIO_INPUT으로 변경해도 ADC peripheral이 입력을 점유하거나 pull이 켜져 있으면 기대한 유휴 상태가 아닐 수 있다. 실제 nRF pinctrl/SAADC 동작을 확인해 구현한다.

DTS와 빌드

ADC 채널 정의에 io-channels 및 채널의 gain/reference/acquisition/resolution을 회로와 Zephyr binding에 맞춰 기재한다. power GPIO가 실재하면 별도 power-gpios 속성 또는 프로젝트가 사용하는 DT 노드를 정의한다.

ritsn_potentiometer.c를 기존 Zephyr CMakeLists.txt에 넣고 header 검색 경로를 확인한다. DTS 속성명과 node label은 실제 보드와 binding에 맞춰 사용한다.

이미 있는 potentiometer/ADC 모듈이 있으면 이름만 다른 중복 구현을 만들지 말고 이 인터페이스로 정리한다.

완료 확인

실제 NCS/Zephyr 버전에서 빌드한다. 초기화 전 읽기, NULL 인자, ADC 읽기 실패 뒤 전원 OFF, 연속 두 번 이상 읽기를 확인한다.

가변저항을 최솟값·중간값·최댓값으로 돌려 ADC raw와 mV 값이 일관되게 변하는지 확인한다.

유휴 상태에서 전원 GPIO와 ADC 입력 핀의 전압과 소비전류를 측정하여 high-Z/전원 OFF가 회로에서 의도대로 동작하는지 확인한다. 소프트웨어상 GPIO_INPUT 설정만으로 저전력 동작이 증명되었다고 판단하지 않는다.

최종 핀, ON/OFF 극성, 유휴 상태, ADC 입력 복구 방식, 수정 파일, 빌드 결과 및 실측 결과를 보고한다. 회로도나 보드가 없으면 확정하지 못한 사항을 따로 적는다.