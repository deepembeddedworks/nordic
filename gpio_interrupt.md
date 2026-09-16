# Zephyr GPIO Interrupt with DeviceTree

이 문서는 Zephyr에서 DeviceTree의 node label을 이용하여 GPIO interrupt를 설정하는 기본 흐름을 정리한다.

예제에서는 Tact Switch의 node label을 다음과 같이 사용한다.

```text
tact_sw0
```

전체 흐름은 다음과 같다.

```text
DTS node label
    ↓
DT_NODELABEL()
    ↓
GPIO_DT_SPEC_GET()
    ↓
struct gpio_dt_spec
    ↓
gpio_is_ready_dt()
    ↓
gpio_pin_configure_dt()
    ↓
gpio_pin_interrupt_configure_dt()
    ↓
gpio_init_callback()
    ↓
gpio_add_callback()
    ↓
Interrupt 발생
    ↓
callback 실행
    ↓
flag = true
    ↓
main loop에서 실제 처리
```

---

## 1. DeviceTree 정의

예를 들어 Tact Switch가 GPIO0의 25번 핀에 연결되어 있고,
버튼을 누르면 GND로 연결되는 구조라고 가정한다.

```dts
/ {
    tact_switch {
        tact_sw0: tact_sw_0 {
            gpios = <&gpio0 25
                     (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        };
    };
};
```

여기서:

```dts
tact_sw0:
```

가 node label이다.

반면:

```dts
tact_sw_0
```

는 node name이다.

따라서 C 코드에서는 다음과 같이 사용한다.

```c
DT_NODELABEL(tact_sw0)
```

---

## 2. 필요한 include

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <stdbool.h>
#include <errno.h>
```

---

## 3. DT_NODELABEL()

DTS의 node label을 C에서 가져온다.

```c
#define TACT_SW0_NODE DT_NODELABEL(tact_sw0)
```

이 매크로는 DTS의 다음 노드를 가리킨다.

```dts
tact_sw0: tact_sw_0 {
    gpios = <&gpio0 25
             (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
};
```

---

## 4. GPIO_DT_SPEC_GET()

DeviceTree의 `gpios` property를 `struct gpio_dt_spec` 형태로 가져온다.

```c
static const struct gpio_dt_spec tact_sw0 =
    GPIO_DT_SPEC_GET(TACT_SW0_NODE, gpios);
```

한 줄로 직접 작성할 수도 있다.

```c
static const struct gpio_dt_spec tact_sw0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(tact_sw0), gpios);
```

---

## 5. struct gpio_dt_spec

예제의 `tact_sw0`에는 개념적으로 다음 정보가 들어 있다.

```text
port     = gpio0
pin      = 25
dt_flags = GPIO_PULL_UP | GPIO_ACTIVE_LOW
```

C 코드에서는 다음과 같이 접근할 수 있다.

```c
tact_sw0.port
tact_sw0.pin
tact_sw0.dt_flags
```

---

## 6. Interrupt용 callback 구조체

Zephyr GPIO interrupt는 callback 구조체를 사용한다.

```c
static struct gpio_callback tact_sw0_cb;
```

그리고 interrupt 발생 여부를 main loop에 전달하기 위해 flag를 둘 수 있다.

```c
static volatile bool tact_sw0_flag = false;
```

---

## 7. Interrupt callback

Interrupt가 발생하면 실행되는 callback 함수이다.

```c
static void tact_sw0_handler(
    const struct device *port,
    struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    tact_sw0_flag = true;
}
```

callback 안에서는 가능한 한 긴 작업을 하지 않는 것이 좋다.

즉 다음처럼 flag만 세우고:

```c
tact_sw0_flag = true;
```

실제 처리는 main loop에서 수행한다.

---

## 8. gpio_is_ready_dt()

GPIO controller가 준비되어 있는지 확인한다.

```c
if (!gpio_is_ready_dt(&tact_sw0)) {
    return -ENODEV;
}
```

---

## 9. gpio_pin_configure_dt()

Tact Switch GPIO를 input으로 설정한다.

```c
int ret;

ret = gpio_pin_configure_dt(
    &tact_sw0,
    GPIO_INPUT
);

if (ret < 0) {
    return ret;
}
```

DTS에 이미:

```dts
GPIO_PULL_UP | GPIO_ACTIVE_LOW
```

가 있기 때문에 input 설정 시 pull-up도 함께 반영된다.

결과적으로:

```text
GPIO Input
+
Internal Pull-up
+
Active Low
```

구성이 된다.

---

## 10. gpio_pin_interrupt_configure_dt()

버튼을 누르는 순간 interrupt가 발생하도록 설정한다.

```c
ret = gpio_pin_interrupt_configure_dt(
    &tact_sw0,
    GPIO_INT_EDGE_TO_ACTIVE
);

if (ret < 0) {
    return ret;
}
```

`GPIO_ACTIVE_LOW`이므로 logical active 상태는 물리적으로 LOW이다.

즉 일반적인 pull-up 버튼에서는:

```text
Button Released
Physical GPIO = HIGH
Logical state = inactive

Button Pressed
Physical GPIO = LOW
Logical state = active
```

따라서:

```c
GPIO_INT_EDGE_TO_ACTIVE
```

는 보통 실제 물리 신호 기준으로:

```text
HIGH → LOW
```

즉 버튼을 누르는 순간 interrupt가 발생한다.

---

## 11. gpio_init_callback()

callback 구조체를 초기화한다.

```c
gpio_init_callback(
    &tact_sw0_cb,
    tact_sw0_handler,
    BIT(tact_sw0.pin)
);
```

여기서:

```c
BIT(tact_sw0.pin)
```

은 callback이 감시할 GPIO pin mask이다.

예를 들어 pin이 25라면 개념적으로:

```text
BIT(25)
```

가 된다.

---

## 12. gpio_add_callback()

초기화한 callback을 GPIO controller에 등록한다.

```c
ret = gpio_add_callback(
    tact_sw0.port,
    &tact_sw0_cb
);

if (ret < 0) {
    return ret;
}
```

이 단계까지 완료되어야 interrupt가 발생했을 때 callback이 호출된다.

---

## 13. 전체 초기화 예제

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <stdbool.h>
#include <errno.h>


#define TACT_SW0_NODE DT_NODELABEL(tact_sw0)


static const struct gpio_dt_spec tact_sw0 =
    GPIO_DT_SPEC_GET(TACT_SW0_NODE, gpios);


static struct gpio_callback tact_sw0_cb;

static volatile bool tact_sw0_flag = false;


static void tact_sw0_handler(
    const struct device *port,
    struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    tact_sw0_flag = true;
}


int tact_sw0_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&tact_sw0)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &tact_sw0,
        GPIO_INPUT
    );

    if (ret < 0) {
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(
        &tact_sw0,
        GPIO_INT_EDGE_TO_ACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    gpio_init_callback(
        &tact_sw0_cb,
        tact_sw0_handler,
        BIT(tact_sw0.pin)
    );

    ret = gpio_add_callback(
        tact_sw0.port,
        &tact_sw0_cb
    );

    if (ret < 0) {
        return ret;
    }

    return 0;
}
```

---

## 14. main()에서 flag 처리

Interrupt callback 안에서는 실제 작업을 길게 하지 않고,
main loop에서 flag를 확인해서 처리한다.

```c
int main(void)
{
    int ret;

    ret = tact_sw0_init();

    if (ret < 0) {
        printk("Tact switch init failed: %d\n", ret);
        return ret;
    }

    while (1) {

        if (tact_sw0_flag) {

            tact_sw0_flag = false;

            /*
             * 실제 application 처리
             */
            printk("TACT SW pressed\n");
        }

        /*
         * Radar processing 또는
         * 다른 application processing
         */
        k_msleep(1);
    }

    return 0;
}
```

---

## 15. 왜 callback에서는 flag만 세우는가

다음처럼 callback 안에서 긴 작업을 하는 것은 피하는 것이 좋다.

```c
static void tact_sw0_handler(...)
{
    adc_read(...);
    k_msleep(100);
    esb_send(...);
}
```

대신:

```c
static void tact_sw0_handler(...)
{
    tact_sw0_flag = true;
}
```

만 하고,

main loop에서:

```c
if (tact_sw0_flag) {
    tact_sw0_flag = false;

    adc_read(...);
    esb_send(...);
}
```

처럼 처리하는 것이 안전하다.

---

## 16. GPIO_INT_EDGE_TO_ACTIVE

```c
GPIO_INT_EDGE_TO_ACTIVE
```

는 logical inactive 상태에서 logical active 상태로 바뀔 때 interrupt를 발생시킨다.

DTS가:

```dts
GPIO_ACTIVE_LOW
```

이면:

```text
Physical HIGH → LOW
```

일 때 interrupt가 발생한다.

---

## 17. GPIO_INT_EDGE_TO_INACTIVE

버튼을 놓는 순간도 감지하고 싶다면:

```c
gpio_pin_interrupt_configure_dt(
    &tact_sw0,
    GPIO_INT_EDGE_TO_INACTIVE
);
```

를 사용할 수 있다.

`GPIO_ACTIVE_LOW`일 경우:

```text
Physical LOW → HIGH
```

일 때 interrupt가 발생한다.

---

## 18. GPIO_INT_EDGE_BOTH

누르는 순간과 놓는 순간을 모두 감지하려면:

```c
gpio_pin_interrupt_configure_dt(
    &tact_sw0,
    GPIO_INT_EDGE_BOTH
);
```

를 사용한다.

이 경우 callback 안이나 main에서 현재 상태를 읽을 수 있다.

```c
int state;

state = gpio_pin_get_dt(&tact_sw0);
```

---

## 19. callback에서 현재 상태 읽기

필요한 경우 callback 안에서 간단히 상태를 읽는 것은 가능하다.

```c
static void tact_sw0_handler(
    const struct device *port,
    struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    int state;

    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    state = gpio_pin_get_dt(&tact_sw0);

    if (state > 0) {
        tact_sw0_flag = true;
    }
}
```

하지만 일반적인 버튼 press interrupt라면:

```c
GPIO_INT_EDGE_TO_ACTIVE
```

자체가 press를 의미하므로 단순히 flag를 세우는 것만으로 충분하다.

---

## 20. Debounce

기계식 Tact Switch는 버튼을 한 번 눌러도 아주 짧은 시간 동안 여러 번 edge가 발생할 수 있다.

이를 bounce라고 한다.

개념적으로:

```text
한 번 누름

HIGH
 ↓
LOW
HIGH
LOW
HIGH
LOW
 ↓
안정 LOW
```

따라서 실제 application에서는 debounce가 필요할 수 있다.

가장 단순한 방법은 main에서 마지막 처리 시간을 확인하는 방식이다.

예:

```c
static int64_t last_press_time;

if (tact_sw0_flag) {

    int64_t now;

    tact_sw0_flag = false;

    now = k_uptime_get();

    if ((now - last_press_time) >= 50) {

        last_press_time = now;

        printk("Valid tact press\n");
    }
}
```

예를 들어 50 ms 이내의 중복 interrupt를 무시할 수 있다.

---

## 21. 핵심 패턴

GPIO interrupt를 DeviceTree 기반으로 사용할 때 가장 기본적인 패턴은 다음과 같다.

```c
#define NODE DT_NODELABEL(tact_sw0)

static const struct gpio_dt_spec gpio =
    GPIO_DT_SPEC_GET(NODE, gpios);

static struct gpio_callback cb;

static volatile bool flag;


static void handler(
    const struct device *port,
    struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    flag = true;
}


if (!gpio_is_ready_dt(&gpio)) {
    return -ENODEV;
}

gpio_pin_configure_dt(
    &gpio,
    GPIO_INPUT
);

gpio_pin_interrupt_configure_dt(
    &gpio,
    GPIO_INT_EDGE_TO_ACTIVE
);

gpio_init_callback(
    &cb,
    handler,
    BIT(gpio.pin)
);

gpio_add_callback(
    gpio.port,
    &cb
);
```

---

## 22. 핵심 함수와 매크로 정리

| 항목 | 역할 |
|---|---|
| `DT_NODELABEL(tact_sw0)` | DTS node label을 C에서 가져온다 |
| `GPIO_DT_SPEC_GET()` | `gpios` property를 `gpio_dt_spec`으로 가져온다 |
| `gpio_is_ready_dt()` | GPIO controller가 준비되었는지 확인한다 |
| `gpio_pin_configure_dt()` | GPIO를 input으로 설정한다 |
| `gpio_pin_interrupt_configure_dt()` | GPIO interrupt edge를 설정한다 |
| `gpio_init_callback()` | callback 구조체를 초기화한다 |
| `gpio_add_callback()` | callback을 GPIO controller에 등록한다 |
| `GPIO_INT_EDGE_TO_ACTIVE` | inactive → active edge |
| `GPIO_INT_EDGE_TO_INACTIVE` | active → inactive edge |
| `GPIO_INT_EDGE_BOTH` | 양쪽 edge 모두 감지 |
| `GPIO_PULL_UP` | Internal pull-up 사용 |
| `GPIO_ACTIVE_LOW` | LOW를 logical active로 해석 |

---

## 23. prj.conf

GPIO driver 사용을 위해 기본적으로:

```conf
CONFIG_GPIO=y
```

를 사용한다.

Zephyr GPIO API를 사용하는 경우 application에서 직접:

```c
nrfx_gpiote_init();
```

같은 함수를 호출할 필요는 없다.

Zephyr GPIO driver가 interrupt 하드웨어 구성을 처리한다.

---

## 24. 요약

DTS:

```dts
tact_sw0: tact_sw_0 {
    gpios = <&gpio0 25
             (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
};
```

C:

```c
#define TACT_SW0_NODE DT_NODELABEL(tact_sw0)

static const struct gpio_dt_spec tact_sw0 =
    GPIO_DT_SPEC_GET(TACT_SW0_NODE, gpios);
```

Ready 확인:

```c
gpio_is_ready_dt(&tact_sw0);
```

Input 설정:

```c
gpio_pin_configure_dt(
    &tact_sw0,
    GPIO_INPUT
);
```

Interrupt 설정:

```c
gpio_pin_interrupt_configure_dt(
    &tact_sw0,
    GPIO_INT_EDGE_TO_ACTIVE
);
```

Callback 초기화:

```c
gpio_init_callback(
    &tact_sw0_cb,
    tact_sw0_handler,
    BIT(tact_sw0.pin)
);
```

Callback 등록:

```c
gpio_add_callback(
    tact_sw0.port,
    &tact_sw0_cb
);
```

Interrupt 발생 시:

```c
tact_sw0_flag = true;
```

main loop에서:

```c
if (tact_sw0_flag) {
    tact_sw0_flag = false;

    /*
     * 실제 application 처리
     */
}
```

전체 관계:

```text
tact_sw0:
    ↓
DT_NODELABEL(tact_sw0)
    ↓
GPIO_DT_SPEC_GET()
    ↓
gpio_is_ready_dt()
    ↓
gpio_pin_configure_dt(GPIO_INPUT)
    ↓
gpio_pin_interrupt_configure_dt()
    ↓
gpio_init_callback()
    ↓
gpio_add_callback()
    ↓
Interrupt
    ↓
callback
    ↓
flag = true
    ↓
main loop에서 실제 처리
```
