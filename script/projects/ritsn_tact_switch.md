RITSN tact switch 모듈 구현 지침

사용자가 먼저 입력할 DTS node label

실제 보드 DTS의 label: 식별자를 적는다. DT_NODELABEL(...)에 넣는 이름이며 노드 이름이나 문자열 label 속성과 구분한다.

tact switch GPIO 노드의 node label: [여기에 입력]

Claude Code는 입력된 node label과 GPIO 속성을 실제 DTS에서 확인한다. 일치하는 노드가 없다면 임의의 핀으로 대체하지 말고 차이를 보고한다.

목표와 산출물

include/ritsn_tact_switch.h와 ritsn_tact_switch.c를 구현하고 빌드에 등록한다. 스위치는 외부 pull-up 저항이 있고, 누르면 GPIO가 GND에 연결되는 active-low 회로다. 눌림 에지에서 인터럽트를 발생시키고, 초기화 시 등록한 사용자의 callback을 호출한다. 이번 모듈에서 ESB 전송이나 레이더 제어는 구현하지 않는다.

DTS와 극성

tact GPIO를 GPIO_ACTIVE_LOW로 정의한다. 하드웨어 회로의 외부 pull-up 저항을 확인하고, gpio_pin_configure_dt()에는 GPIO_INPUT만 사용한다. 내부 GPIO_PULL_UP 또는 GPIO_PULL_DOWN은 추가하지 않는다.

Zephyr의 gpio_pin_get_dt()는 active-low 논리값을 반환하므로 누른 상태는 1, 뗀 상태는 0이다.

눌림 이벤트에는 GPIO_INT_EDGE_TO_ACTIVE를 사용한다. GPIO_ACTIVE_LOW가 지정된 GPIO에서 물리적으로 HIGH→LOW가 되는 눌림 에지에 대응하는지 실제 SDK와 보드에서 확인한다. 뗌 에지나 양쪽 에지를 눌림으로 잘못 처리하지 않는다.

공개 API (include/ritsn_tact_switch.h)

#ifndef RITSN_TACT_SWITCH_H_
#define RITSN_TACT_SWITCH_H_

#include <stdbool.h>

/* 이 callback은 GPIO ISR 문맥에서 호출된다. */
typedef void (*ritsn_tact_switch_callback_t)(void *user_data);

/* GPIO/인터럽트를 준비하고 눌림 callback을 등록한다. */
int ritsn_tact_switch_init(ritsn_tact_switch_callback_t callback,
                           void *user_data);

/* 눌림 에지 인터럽트 활성화 및 비활성화. */
int ritsn_tact_switch_interrupt_enable(void);
int ritsn_tact_switch_interrupt_disable(void);

/* 현재 버튼 상태. 눌림=true, 뗌=false. */
int ritsn_tact_switch_get(bool *pressed);

#endif /* RITSN_TACT_SWITCH_H_ */

성공은 0, 실패는 음수 errno다. init(NULL, ...)의 허용 여부는 한 가지 정책으로 정하고 문서화한다. 여기서는 callback 필수로 구현하고 NULL이면 -EINVAL을 반환한다. get(NULL)도 -EINVAL이다. 애플리케이션 생명주기 동안 callback과 user_data가 유효해야 한다. 기존 프로젝트에 동일 모듈 또는 callback API가 있으면 중복 생성하지 않고 맞춰 확장한다.

구현 절차

지정한 node label에서 gpio_dt_spec을 얻고 device_is_ready()를 확인한다.

GPIO_INPUT으로 설정한다. 외부 pull-up을 사용하므로 내부 pull 설정을 요청하지 않는다.

모듈 내부에 struct gpio_callback과 사용자 callback/user_data를 보관한다. gpio_init_callback()으로 해당 핀만 등록하고 gpio_add_callback()의 실패를 처리한다.

등록이 완료된 뒤 gpio_pin_interrupt_configure_dt(..., GPIO_INT_EDGE_TO_ACTIVE)로 눌림 인터럽트를 켠다. 초기화가 실패하면 가능한 범위에서 인터럽트와 callback 등록을 되돌린다. 중복 init() 호출로 동일 callback이 반복 등록되지 않게 한다.

내부 GPIO callback에서 등록된 사용자 callback을 호출한다. GPIO ISR 문맥이므로 콜백에서는 플래그 설정, atomic 갱신, k_work_submit() 정도만 한다. k_sleep, ADC 측정, ESB 송신, printf 등 오래 걸리거나 블로킹될 수 있는 작업은 스레드/work 문맥에서 수행한다.

interrupt_disable()은 GPIO_INT_DISABLE로 눌림 인터럽트만 끄고 GPIO 읽기는 유지한다. interrupt_enable()은 동일한 눌림 에지 설정을 복구한다. 버튼이 이미 눌린 상태에서 활성화할 때 새 에지가 없으면 즉시 callback이 호출되지 않는다는 점을 확인한다.

get()은 gpio_pin_get_dt()의 논리값을 읽어 *pressed로 반환하고 읽기 오류를 전파한다. 출력 인자는 성공했을 때만 변경한다.

디바운스와 이벤트 전달

기계식 tact 버튼은 한 번 누를 때 접점 튐으로 인터럽트가 여러 번 발생할 수 있다. 사용자 callback이 곧바로 누름 1회를 뜻한다고 가정하지 않는다. 이번 단계에서는 콜백에서 이벤트 플래그만 세우고, 소비하는 애플리케이션 쪽에서 시간 기준 디바운스 또는 안정 상태 확인을 구현한다. 기존 프로젝트에 디바운스 정책이 있으면 그것을 따른다. 명세되지 않은 임의의 디바운스 시간은 이 모듈에서 확정하지 않는다.

사용 예

static atomic_t tact_pending;

static void tact_pressed_isr(void *user_data)
{
    (void)user_data;
    atomic_set(&tact_pending, 1);
}

static int tact_setup(void)
{
    return ritsn_tact_switch_init(tact_pressed_isr, NULL);
}
/* 다른 스레드 문맥에서 atomic_clear(&tact_pending) 결과로 눌림을 처리한다. */

예제를 실제 코드에 넣는다면 <zephyr/sys/atomic.h>를 포함하고, 프로젝트의 Zephyr 버전에 맞는 atomic API를 확인한다. 이벤트를 놓치면 안 되는 요구사항이 있다면 단일 플래그 대신 큐/카운터를 사용한다.

완료 확인

미누름 상태의 실제 핀은 외부 저항으로 HIGH, get()은 false; 누름 상태의 실제 핀은 LOW, get()은 true인지 확인한다.

눌림 에지에서 사용자 callback이 호출되고 뗌 에지에서는 호출되지 않는지 확인한다. 인터럽트 disable/enable 뒤 동작도 확인한다.

접점 튐에 의한 callback 중복 횟수를 실물에서 확인하고 호출부 디바운스 필요성을 보고한다. 외부 pull-up 값과 최종 node label/핀, 빌드 결과도 기록한다.