# Zephyr GPIO Input with DeviceTree

이 문서는 Zephyr에서 DeviceTree의 node label을 이용하여 GPIO input을 설정하고 읽는 기본 흐름을 정리한다.

예제에서는 DIP switch의 node label을 다음과 같이 사용한다.

```text
dip_sw0
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
gpio_pin_get_dt()
```

## 1. DeviceTree 정의

```dts
/ {
    dip_switch {
        dip_sw0: dip_sw_0 {
            gpios = <&gpio0 28
                     (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
        };
    };
};
```

여기서 `dip_sw0:`가 node label이고 `dip_sw_0`는 node name이다.

따라서 C 코드에서는 다음과 같이 사용한다.

```c
DT_NODELABEL(dip_sw0)
```

## 2. 필요한 include

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <errno.h>
```

## 3. DT_NODELABEL()

```c
#define DIP_SW0_NODE DT_NODELABEL(dip_sw0)
```

## 4. GPIO_DT_SPEC_GET()

```c
static const struct gpio_dt_spec dip_sw0 =
    GPIO_DT_SPEC_GET(DIP_SW0_NODE, gpios);
```

한 줄로 쓰면:

```c
static const struct gpio_dt_spec dip_sw0 =
    GPIO_DT_SPEC_GET(DT_NODELABEL(dip_sw0), gpios);
```

`GPIO_DT_SPEC_GET()`은 일반 런타임 함수가 아니라 DeviceTree 정보를 이용하는 매크로이다.

## 5. struct gpio_dt_spec

`GPIO_DT_SPEC_GET()`을 통해 얻은 구조체에는 대략 다음 정보가 들어 있다.

```text
port     = gpio0
pin      = 28
dt_flags = GPIO_PULL_UP | GPIO_ACTIVE_LOW
```

C에서는 다음과 같이 접근할 수 있다.

```c
dip_sw0.port
dip_sw0.pin
dip_sw0.dt_flags
```

## 6. gpio_is_ready_dt()

GPIO controller가 준비되어 있는지 확인한다.

```c
if (!gpio_is_ready_dt(&dip_sw0)) {
    return -ENODEV;
}
```

## 7. gpio_pin_configure_dt()

GPIO를 input으로 설정한다.

```c
int ret;

ret = gpio_pin_configure_dt(
    &dip_sw0,
    GPIO_INPUT
);

if (ret < 0) {
    return ret;
}
```

DTS에 이미 다음 flag가 있기 때문에:

```dts
GPIO_PULL_UP | GPIO_ACTIVE_LOW
```

`gpio_pin_configure_dt(&dip_sw0, GPIO_INPUT);`를 호출하면 input 설정과 함께 DTS의 pull-up 설정도 반영된다.

즉 결과적으로:

```text
GPIO Input
+
Internal Pull-up
+
Active Low
```

구성이 된다.

## 8. gpio_pin_get_dt()

GPIO input 값을 읽는다.

```c
int value;

value = gpio_pin_get_dt(&dip_sw0);
```

에러까지 처리하면:

```c
int value;

value = gpio_pin_get_dt(&dip_sw0);

if (value < 0) {
    return value;
}

if (value) {
    printk("DIP ON\n");
} else {
    printk("DIP OFF\n");
}
```

## 9. GPIO_ACTIVE_LOW의 의미

일반적인 회로:

```text
VDD
 |
Internal Pull-up
 |
GPIO P0.28
 |
DIP Switch
 |
GND
```

스위치 OFF:

```text
Physical GPIO = HIGH
Logical value = 0
```

스위치 ON:

```text
Physical GPIO = LOW
Logical value = 1
```

`gpio_pin_get_dt()`는 `GPIO_ACTIVE_LOW`를 반영한 logical value를 반환한다.

## 10. 전체 기본 예제

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <errno.h>


#define DIP_SW0_NODE DT_NODELABEL(dip_sw0)


static const struct gpio_dt_spec dip_sw0 =
    GPIO_DT_SPEC_GET(DIP_SW0_NODE, gpios);


int dip_sw0_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&dip_sw0)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &dip_sw0,
        GPIO_INPUT
    );

    if (ret < 0) {
        return ret;
    }

    return 0;
}


int dip_sw0_read(void)
{
    int value;

    value = gpio_pin_get_dt(&dip_sw0);

    if (value < 0) {
        return value;
    }

    return value;
}
```

## 11. main() 사용 예제

```c
int main(void)
{
    int ret;
    int value;

    ret = dip_sw0_init();

    if (ret < 0) {
        printk("DIP init failed: %d\n", ret);
        return ret;
    }

    while (1) {

        value = dip_sw0_read();

        if (value < 0) {
            printk("DIP read failed: %d\n", value);
        } else if (value) {
            printk("DIP ON\n");
        } else {
            printk("DIP OFF\n");
        }

        k_msleep(500);
    }

    return 0;
}
```

## 12. 핵심 패턴

```c
#define NODE DT_NODELABEL(dip_sw0)

static const struct gpio_dt_spec gpio =
    GPIO_DT_SPEC_GET(NODE, gpios);

if (!gpio_is_ready_dt(&gpio)) {
    return -ENODEV;
}

gpio_pin_configure_dt(
    &gpio,
    GPIO_INPUT
);

value = gpio_pin_get_dt(&gpio);
```

## 13. 핵심 함수와 매크로 정리

| 항목 | 역할 |
|---|---|
| `DT_NODELABEL(dip_sw0)` | DTS node label을 C에서 가져온다 |
| `GPIO_DT_SPEC_GET()` | `gpios` property를 `gpio_dt_spec`으로 변환한다 |
| `gpio_is_ready_dt()` | GPIO controller가 준비되었는지 확인한다 |
| `gpio_pin_configure_dt()` | GPIO pin을 input/output 등으로 설정한다 |
| `gpio_pin_get_dt()` | GPIO logical input 값을 읽는다 |
| `gpio_pin_set_dt()` | GPIO logical output 값을 설정한다 |
| `GPIO_INPUT` | GPIO input 설정 |
| `GPIO_PULL_UP` | Internal pull-up 사용 |
| `GPIO_ACTIVE_LOW` | LOW를 active 상태로 해석 |

## 14. prj.conf

```conf
CONFIG_GPIO=y
```

## 15. 요약

DTS:

```dts
dip_sw0: dip_sw_0 {
    gpios = <&gpio0 28
             (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
};
```

C:

```c
#define DIP_SW0_NODE DT_NODELABEL(dip_sw0)

static const struct gpio_dt_spec dip_sw0 =
    GPIO_DT_SPEC_GET(DIP_SW0_NODE, gpios);
```

Ready 확인:

```c
gpio_is_ready_dt(&dip_sw0);
```

Input 설정:

```c
gpio_pin_configure_dt(
    &dip_sw0,
    GPIO_INPUT
);
```

Input 읽기:

```c
gpio_pin_get_dt(&dip_sw0);
```

전체 관계:

```text
dip_sw0:
   ↓
DT_NODELABEL(dip_sw0)
   ↓
GPIO_DT_SPEC_GET()
   ↓
struct gpio_dt_spec
   ↓
gpio_is_ready_dt()
   ↓
gpio_pin_configure_dt(GPIO_INPUT)
   ↓
gpio_pin_get_dt()
```
