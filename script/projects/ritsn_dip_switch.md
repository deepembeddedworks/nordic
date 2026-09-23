RITSN DIP switch 모듈 구현 지침

사용자가 먼저 입력할 DTS node label

실제 보드 DTS의 label: 식별자를 아래에 입력한다. 노드 이름이나 문자열 label 속성이 아니라 DT_NODELABEL(...)에서 쓸 이름이다.

DIP 스위치 6개 GPIO를 담은 노드의 node label: [여기에 입력]

Claude Code는 입력한 node label이 존재하는지 확인하고, 그 노드의 gpios 또는 프로젝트에서 실제 사용하는 속성으로 D_SW1부터 D_SW6까지의 순서를 확인한다. 이름이 없거나 연결 순서를 확정할 수 없다면 임의로 GPIO를 배정하지 말고 보고한다.

목표와 산출물

include/ritsn_dip_switch.h와 ritsn_dip_switch.c를 구현하고 프로젝트 빌드에 등록한다. 역할은 6개 DIP 입력 초기화, 값 읽기, 읽은 뒤 pull-up 해제 및 high-Z 전환이다. 스위치는 한쪽이 GND에 연결되어 있으며 내부 pull-up을 사용한다. 외부 pull-up이 없다는 전제로 부팅 후 GPIO를 설정한 다음 읽는다.

이 단계에서는 DIP 값을 출력하거나 호출부에 6비트 ID로 전달한다. ESB 초기화나 무선 송신은 구현하지 않는다.

회로 및 DTS 확인

실제 회로에서 D_SW1~D_SW6가 각각 연결된 GPIO를 확인하고 DTS에 GPIO_ACTIVE_LOW로 정의한다. 이전 설계 핀 후보는 P0.28, P0.29, P0.30, P0.31, P1.15, P1.9이며 실제 회로도가 우선한다.

특히 P0.29는 배터리 분압 전원 GPIO로도 기록된 적이 있다. 회로와 DTS를 확인하기 전에는 두 용도로 동시에 사용하지 않는다.

전원 투입 직후 GPIO 초기화 전에는 내부 pull-up이 작동하지 않을 수 있다. 이 모듈은 GPIO 초기화 후 DIP를 읽는 용도다.

DTS의 GPIO_ACTIVE_LOW라면 gpio_pin_get_dt()의 논리값은 스위치 ON/닫힘 = 1, OFF/열림 = 0이 된다. gpio_pin_get_raw()와 혼용하지 않는다.

공개 API (include/ritsn_dip_switch.h)

#ifndef RITSN_DIP_SWITCH_H_
#define RITSN_DIP_SWITCH_H_

#include <stdint.h>

#define RITSN_DIP_SWITCH_COUNT 6U

/* GPIO 장치를 확인하고 6개 입력을 pull-up 상태로 준비한다. */
int ritsn_dip_switch_init(void);

/* D_SW1을 bit 0, D_SW6을 bit 5로 읽어 0..63을 반환한다. */
int ritsn_dip_switch_read(uint8_t *value);

/* pull-up을 끄고 6개 핀을 high-Z 유휴 상태로 전환한다. */
int ritsn_dip_switch_release(void);

#endif /* RITSN_DIP_SWITCH_H_ */

성공은 0, 실패는 음수 errno로 반환한다. read(NULL)은 -EINVAL을 반환한다. init()은 읽기 가능한 상태로 만든다. release() 후 다시 읽으려면 init()이 내부 pull-up 입력 설정을 재적용할 수 있게 구현한다. 읽기 전 잠깐의 입력 안정화가 필요한지는 실제 GPIO 회로 및 SDK를 확인한다.

동작 순서

ritsn_dip_switch_init()에서 여섯 gpio_dt_spec 장치의 준비 상태를 검사한다. 모두 GPIO_INPUT | GPIO_PULL_UP으로 설정한다. 한 핀이라도 실패하면 오류를 반환하고 이미 켜진 pull-up도 가능한 범위에서 해제한다.

ritsn_dip_switch_read()에서 D_SW1~D_SW6을 차례로 gpio_pin_get_dt()로 읽는다. 오류가 나면 출력값을 갱신하지 않는다. 성공 시 ON인 스위치의 비트만 세워 *value에 기록한다.

읽기가 끝나면 호출부에서 ritsn_dip_switch_release()를 호출한다. 여섯 핀을 pull-up/pull-down 없이 GPIO 입력으로 다시 설정한다. 출력 구동을 하지 않는 유휴 high-Z 상태다. GPIO controller와 pinctrl이 이 설정을 그대로 유지하는지 확인한다.

다시 읽어야 한다면 init() → read() → release() 순서를 반복한다. 일반 사용에서는 부팅 시 한 번 읽고 uint8_t 변수에 저장한 후 release한다.

GPIO high-Z는 내부 pull-up을 꺼 전류 경로를 없애는 목적이다. release 후에는 입력이 뜨므로 DIP 값을 읽어서는 안 된다. 스위치가 ON이 되어 GND에 연결되어 있어도, pull-up을 해제하면 GND 방향의 정상상태 전류가 흐르지 않는다. 재읽기 전에 반드시 pull-up을 복구한다.

호출부 예시

uint8_t dip_id = 0;
int err = ritsn_dip_switch_init();

if (err == 0) {
    err = ritsn_dip_switch_read(&dip_id);
    int release_err = ritsn_dip_switch_release();
    if (err == 0) {
        err = release_err;
    }
}

if (err == 0) {
    /* dip_id는 0..63. 나중에 ESB prefix 등에 사용 가능 */
}

실제 프로젝트 스타일에 맞춰 오류 복구와 로깅을 정리한다. read()가 실패해도 release()를 실행하는 구조를 유지한다.

완료 확인

6개 스위치가 모두 OFF면 0x00, D_SW1만 ON이면 0x01, D_SW6만 ON이면 0x20, 모두 ON이면 0x3F인지 확인한다.

release() 후 pull-up이 꺼지고 GPIO 출력 구동이 없는지 확인한다. 이후 init()을 다시 호출하고 DIP 값이 정상적으로 재측정되는지 확인한다.

실제 보드/NCS로 빌드하고, 최종 node label·핀 매핑·비트 순서·소비전류 확인 결과를 보고한다. 보드가 없으면 실측하지 못한 항목을 구분한다.