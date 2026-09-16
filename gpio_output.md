# Zephyr GPIO Output with DeviceTree

이 문서는 Zephyr에서 DeviceTree의 node label을 이용하여 GPIO output을 설정하고 제어하는 기본 흐름을 정리한다.

예제에서는 Green LED의 node label을 다음과 같이 사용한다.

```text
green_led0
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
gpio_pin_set_dt()
```

---

## 1. DeviceTree 정의

예를 들어 Green LED가 GPIO0의 19번 핀에 연결되어 있고,
HIGH일 때 LED가 켜지는 구조라고 가정한다.

```dts
/ {
    led_green {
        green_led0: green_led_0 {
            gpios = <&gpio0 19 GPIO_ACTIVE_HIGH>;
        };
    };
};
```

여기서:

```dts
green_led0:
```

가 **node label**이다.

반면:

```dts
green_led_0
```

는 node name이다.

따라서 C 코드에서는 다음과 같이 사용한다.

```c
DT_NODELABEL(green_led0)
```

---

## 2. 필요한 include

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <errno.h>
```

---

## 3. DT_NODELABEL()

DTS의 node label을 C에서 가져온다.

```c
#define GREEN_LED_NODE DT_NODELABEL(green_led0)
```

이 매크로는 DTS의 다음 노드를 가리킨다.

```dts
green_led0: green_led_0 {
    gpios = <&gpio0 19 GPIO_ACTIVE_HIGH>;
};
```

---

## 4. GPIO_DT_SPEC_GET()

DeviceTree의 `gpios` property를 `struct gpio_dt_spec` 형태로 가져온다.

```c
static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
```

한 줄로 직접 작성할 수도 있다.

```c
static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(DT_NODELABEL(green_led0), gpios);
```

`GPIO_DT_SPEC_GET()`은 일반 런타임 함수가 아니라 DeviceTree 정보를 이용하는 매크로이다.

---

## 5. struct gpio_dt_spec

예제의 `green_led`에는 개념적으로 다음 정보가 들어 있다.

```text
port     = gpio0
pin      = 19
dt_flags = GPIO_ACTIVE_HIGH
```

C 코드에서는 다음과 같이 접근할 수 있다.

```c
green_led.port
green_led.pin
green_led.dt_flags
```

---

## 6. gpio_is_ready_dt()

GPIO controller가 정상적으로 준비되어 있는지 확인한다.

```c
if (!gpio_is_ready_dt(&green_led)) {
    return -ENODEV;
}
```

---

## 7. gpio_pin_configure_dt()

LED GPIO를 output으로 설정한다.

초기 상태를 OFF로 만들고 싶다면:

```c
int ret;

ret = gpio_pin_configure_dt(
    &green_led,
    GPIO_OUTPUT_INACTIVE
);

if (ret < 0) {
    return ret;
}
```

`GPIO_OUTPUT_INACTIVE`는 DeviceTree의 active polarity를 반영하여
논리적으로 inactive 상태로 초기화한다.

이 예제에서는:

```dts
GPIO_ACTIVE_HIGH
```

이므로 inactive 상태는 물리적으로 LOW가 된다.

즉 초기화 직후:

```text
Logical state = inactive
Physical GPIO = LOW
LED = OFF
```

---

## 8. LED ON

LED를 켤 때는 다음과 같이 한다.

```c
gpio_pin_set_dt(&green_led, 1);
```

`gpio_pin_set_dt()`는 DeviceTree의 `GPIO_ACTIVE_HIGH` 또는
`GPIO_ACTIVE_LOW` 설정을 반영한 logical value를 사용한다.

이 예제에서는 `GPIO_ACTIVE_HIGH`이므로:

```text
gpio_pin_set_dt(..., 1)
        ↓
Physical GPIO = HIGH
        ↓
LED ON
```

---

## 9. LED OFF

LED를 끌 때는:

```c
gpio_pin_set_dt(&green_led, 0);
```

이 예제에서는:

```text
gpio_pin_set_dt(..., 0)
        ↓
Physical GPIO = LOW
        ↓
LED OFF
```

---

## 10. 전체 기본 예제

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <errno.h>


#define GREEN_LED_NODE DT_NODELABEL(green_led0)


static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);


int green_led_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&green_led)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &green_led,
        GPIO_OUTPUT_INACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    return 0;
}


int green_led_on(void)
{
    return gpio_pin_set_dt(
        &green_led,
        1
    );
}


int green_led_off(void)
{
    return gpio_pin_set_dt(
        &green_led,
        0
    );
}
```

---

## 11. main() 사용 예제

```c
int main(void)
{
    int ret;

    ret = green_led_init();

    if (ret < 0) {
        printk("Green LED init failed: %d\n", ret);
        return ret;
    }

    while (1) {

        green_led_on();

        k_msleep(500);

        green_led_off();

        k_msleep(500);
    }

    return 0;
}
```

이 코드는 Green LED를 500 ms 간격으로 깜빡인다.

---

## 12. GPIO_OUTPUT_ACTIVE로 초기화

초기화와 동시에 LED를 ON 상태로 만들 수도 있다.

```c
gpio_pin_configure_dt(
    &green_led,
    GPIO_OUTPUT_ACTIVE
);
```

`GPIO_ACTIVE_HIGH`인 경우:

```text
GPIO_OUTPUT_ACTIVE
        ↓
Physical GPIO = HIGH
        ↓
LED ON
```

---

## 13. GPIO_ACTIVE_HIGH의 의미

DTS:

```dts
gpios = <&gpio0 19 GPIO_ACTIVE_HIGH>;
```

의 의미는 HIGH 상태를 logical active 상태로 사용한다는 뜻이다.

따라서:

```text
Logical 1
    ↓
Physical HIGH
    ↓
LED ON

Logical 0
    ↓
Physical LOW
    ↓
LED OFF
```

application에서는 보통 물리적인 HIGH/LOW를 직접 신경 쓰기보다:

```c
gpio_pin_set_dt(&green_led, 1);
```

또는:

```c
gpio_pin_set_dt(&green_led, 0);
```

처럼 logical active/inactive 기준으로 제어하면 된다.

---

## 14. 핵심 패턴

GPIO output을 DeviceTree 기반으로 사용할 때 가장 기본적인 패턴은 다음과 같다.

```c
#define NODE DT_NODELABEL(green_led0)

static const struct gpio_dt_spec gpio =
    GPIO_DT_SPEC_GET(NODE, gpios);

if (!gpio_is_ready_dt(&gpio)) {
    return -ENODEV;
}

gpio_pin_configure_dt(
    &gpio,
    GPIO_OUTPUT_INACTIVE
);

gpio_pin_set_dt(
    &gpio,
    1
);
```

---

## 15. 핵심 함수와 매크로 정리

| 항목 | 역할 |
|---|---|
| `DT_NODELABEL(green_led0)` | DTS node label을 C에서 가져온다 |
| `GPIO_DT_SPEC_GET()` | `gpios` property를 `gpio_dt_spec`으로 가져온다 |
| `gpio_is_ready_dt()` | GPIO controller가 준비되었는지 확인한다 |
| `gpio_pin_configure_dt()` | GPIO pin을 output으로 설정한다 |
| `gpio_pin_set_dt()` | GPIO의 logical output 값을 설정한다 |
| `GPIO_OUTPUT_INACTIVE` | output 설정 후 inactive 상태로 초기화 |
| `GPIO_OUTPUT_ACTIVE` | output 설정 후 active 상태로 초기화 |
| `GPIO_ACTIVE_HIGH` | HIGH를 active 상태로 해석 |

---

## 16. prj.conf

GPIO driver 사용을 위해 기본적으로 다음 설정을 사용한다.

```conf
CONFIG_GPIO=y
```

---

## 17. 요약

DTS:

```dts
green_led0: green_led_0 {
    gpios = <&gpio0 19 GPIO_ACTIVE_HIGH>;
};
```

C:

```c
#define GREEN_LED_NODE DT_NODELABEL(green_led0)

static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
```

Ready 확인:

```c
gpio_is_ready_dt(&green_led);
```

Output 설정:

```c
gpio_pin_configure_dt(
    &green_led,
    GPIO_OUTPUT_INACTIVE
);
```

LED ON:

```c
gpio_pin_set_dt(
    &green_led,
    1
);
```

LED OFF:

```c
gpio_pin_set_dt(
    &green_led,
    0
);
```

전체 관계:

```text
green_led0:
    ↓
DT_NODELABEL(green_led0)
    ↓
GPIO_DT_SPEC_GET()
    ↓
struct gpio_dt_spec
    ↓
gpio_is_ready_dt()
    ↓
gpio_pin_configure_dt(GPIO_OUTPUT_INACTIVE)
    ↓
gpio_pin_set_dt(1 / 0)
```
