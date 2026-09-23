RITSN green/red LED 모듈 구현 지침

사용자가 먼저 입력할 DTS node label

아래에 보드 DTS의 node label(label: 식별자)을 정확히 입력한다. DT_NODELABEL(...)에서 사용할 값이다. 문자열 label 속성값이나 alias와 혼동하지 않는다.

green LED node label: [여기에 입력]
red LED node label:   [여기에 입력]

Claude Code는 입력한 두 node label의 존재 여부와 각 노드의 gpios 속성을 확인한다. 지정된 노드가 없으면 핀을 추측하지 말고 차이를 보고한다.

목표와 산출물

include/ritsn_led.h와 ritsn_led.c를 만들거나 기존 같은 기능의 모듈을 확장하고, 프로젝트 빌드에 등록한다. 기능은 green/red LED 초기화, 각각 ON/OFF로 한정한다. 깜박임 타이머, 저전압 판단, 문 열림 상태 처리 등은 호출부가 담당한다.

회로와 DTS 확인

이전 설계 기록의 후보 핀은 green P0.19, red P0.20이며 둘 다 active-high이다. 실제 회로도와 DTS를 먼저 확인한다. 핀 번호나 극성을 소스에 하드코딩하지 않는다.

DTS에서 GPIO_ACTIVE_HIGH 또는 실제 회로에 맞는 극성을 gpios에 설정한다. gpio_pin_set_dt(&led, 1)은 논리적 ON, gpio_pin_set_dt(&led, 0)은 논리적 OFF가 되게 한다.

두 핀을 다른 주변장치가 점유하지 않는지 확인한다. 출력의 구동 전압과 LED 직렬 저항을 실제 회로에서 확인한다.

공개 API (include/ritsn_led.h)

#ifndef RITSN_LED_H_
#define RITSN_LED_H_

/* 두 GPIO를 준비하고 두 LED를 OFF로 시작한다. */
int ritsn_led_init(void);

int ritsn_led_green_on(void);
int ritsn_led_green_off(void);
int ritsn_led_red_on(void);
int ritsn_led_red_off(void);

#endif /* RITSN_LED_H_ */

모든 함수는 성공 시 0, 오류 시 음수 errno를 반환한다. 초기화 전 on/off를 호출하면 -ENODEV 등 일관된 오류를 반환한다. 기존 프로젝트에서 이미 API 이름을 정했다면 호출부와 함께 일관되게 적용한다.

구현

입력한 node label별로 GPIO_DT_SPEC_GET(DT_NODELABEL(...), gpios) 또는 실제 DTS에 맞는 표현을 사용한다. DT_NODE_EXISTS 및 device_is_ready()로 설정 가능 여부를 검사한다.

ritsn_led_init()에서 green과 red를 각각 gpio_pin_configure_dt(..., GPIO_OUTPUT_INACTIVE)로 설정한다. 초기 상태는 둘 다 OFF다. 중간에 실패하면 이미 설정된 LED도 가능한 범위에서 OFF로 돌리고 오류를 반환한다.

on 함수는 해당 LED에 gpio_pin_set_dt(..., 1), off 함수는 gpio_pin_set_dt(..., 0)을 사용하고 GPIO API 오류를 그대로 전달한다.

두 LED는 독립적으로 동작한다. green을 켜거나 끄는 함수가 red 상태를 바꾸지 않으며 반대도 같다.

모듈 자체에 sleep, busy wait, 반복 점멸, 전역 애플리케이션 정책을 넣지 않는다. 인터럽트에서 사용하려는 호출부가 있다면 해당 GPIO 드라이버의 ISR 호출 가능 여부를 확인하고, 불명확하면 스레드 또는 work 문맥에서 LED를 제어한다.

빌드와 확인

소스 파일을 실제 애플리케이션의 CMakeLists.txt에 등록하고 헤더 경로를 확인한다.

부팅 직후 양쪽이 꺼지는지, 각각 독립적으로 ON/OFF되는지, 반대쪽 LED 상태가 유지되는지 실물에서 확인한다.

최종 node label, GPIO 핀, active polarity, 수정 파일과 빌드 결과를 보고한다. 보드가 없으면 실물 확인이 되지 않았음을 분명히 한다.