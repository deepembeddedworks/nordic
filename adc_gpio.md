# Zephyr ADC Measurement with GPIO Power Control

이 문서는 Zephyr에서 DeviceTree의 node label을 이용하여 다음 구조를 구현하는 기본 예제를 정리한다.

- `potentiometer0` node label 사용
- ADC channel 사용
- ADC 측정 전에 GPIO를 HIGH로 설정
- 짧은 안정화 시간 대기
- ADC 측정
- 측정 후 GPIO를 LOW로 설정

전체 흐름은 다음과 같다.

```text
potentiometer0
    ↓
DT_NODELABEL()
    ↓
 ┌────────────────────┬─────────────────┐
 ↓                    ↓
GPIO_DT_SPEC_GET()     ADC_DT_SPEC_GET()
 ↓                    ↓
Power GPIO             ADC Channel
 ↓                    ↓
gpio_pin_set_dt(1)     adc_channel_setup_dt()
 ↓
settle delay
 ↓
adc_read()
 ↓
gpio_pin_set_dt(0)
```

---

## 1. DeviceTree 정의

예를 들어 potentiometer가 ADC channel 0에 연결되어 있고,
측정할 때만 GPIO0의 29번 핀을 HIGH로 만들어 전원을 공급한다고 가정한다.

```dts
/ {
    potentiometer0: potentiometer {
        io-channels = <&adc 0>;
        power-gpios = <&gpio0 29 GPIO_ACTIVE_HIGH>;
    };
};

&adc {
    status = "okay";
};
```

여기서:

```dts
potentiometer0:
```

가 node label이다.

따라서 C 코드에서는:

```c
DT_NODELABEL(potentiometer0)
```

를 사용한다.

---

## 2. 필요한 include

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include <errno.h>
#include <stdint.h>
```

---

## 3. DT_NODELABEL()

DTS의 node label을 C에서 가져온다.

```c
#define POT_NODE DT_NODELABEL(potentiometer0)
```

---

## 4. GPIO_DT_SPEC_GET()

`power-gpios` property를 `struct gpio_dt_spec` 형태로 가져온다.

```c
static const struct gpio_dt_spec pot_power =
    GPIO_DT_SPEC_GET(POT_NODE, power_gpios);
```

한 줄로 직접 작성하면:

```c
static const struct gpio_dt_spec pot_power =
    GPIO_DT_SPEC_GET(DT_NODELABEL(potentiometer0), power_gpios);
```

---

## 5. ADC_DT_SPEC_GET()

`io-channels` property를 ADC spec으로 가져온다.

```c
static const struct adc_dt_spec pot_adc =
    ADC_DT_SPEC_GET(POT_NODE);
```

한 줄로 직접 작성하면:

```c
static const struct adc_dt_spec pot_adc =
    ADC_DT_SPEC_GET(DT_NODELABEL(potentiometer0));
```

---

## 6. GPIO와 ADC spec의 의미

GPIO 쪽은 개념적으로 다음 정보가 들어 있다.

```text
port     = gpio0
pin      = 29
dt_flags = GPIO_ACTIVE_HIGH
```

ADC 쪽은 DeviceTree에 정의된 channel 정보를 기반으로 다음과 같은 정보가 구성된다.

```text
ADC device
ADC channel id
gain
reference
acquisition time
resolution
input selection
```

---

## 7. GPIO ready 확인

```c
if (!gpio_is_ready_dt(&pot_power)) {
    return -ENODEV;
}
```

---

## 8. ADC ready 확인

```c
if (!adc_is_ready_dt(&pot_adc)) {
    return -ENODEV;
}
```

---

## 9. Power GPIO 초기화

기본 상태에서는 potentiometer 측정 회로 전원을 OFF로 둔다.

```c
ret = gpio_pin_configure_dt(
    &pot_power,
    GPIO_OUTPUT_INACTIVE
);

if (ret < 0) {
    return ret;
}
```

DTS에:

```dts
GPIO_ACTIVE_HIGH
```

가 있으므로 `GPIO_OUTPUT_INACTIVE`는 실제 핀을 LOW 상태로 만든다.

---

## 10. ADC channel setup

ADC channel은 초기화 시 한 번 설정한다.

```c
ret = adc_channel_setup_dt(&pot_adc);

if (ret < 0) {
    return ret;
}
```

---

## 11. 전체 초기화 함수

```c
int potentiometer_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&pot_power)) {
        return -ENODEV;
    }

    if (!adc_is_ready_dt(&pot_adc)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &pot_power,
        GPIO_OUTPUT_INACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    ret = adc_channel_setup_dt(&pot_adc);

    if (ret < 0) {
        return ret;
    }

    return 0;
}
```

---

## 12. ADC 측정 전 Power GPIO HIGH

측정을 시작하기 전에:

```c
ret = gpio_pin_set_dt(
    &pot_power,
    1
);
```

을 호출한다.

`GPIO_ACTIVE_HIGH`이므로:

```text
Logical 1
    ↓
Physical GPIO HIGH
    ↓
Potentiometer 측정 회로 ON
```

이 된다.

---

## 13. 안정화 시간

GPIO를 HIGH로 올리자마자 ADC를 읽지 않고 잠시 기다릴 수 있다.

예:

```c
k_msleep(10);
```

전체 개념:

```text
GPIO HIGH
   ↓
10 ms wait
   ↓
ADC read
```

실제 필요한 시간은 회로의 RC 특성과 센서/회로 구조에 따라 조정할 수 있다.

---

## 14. adc_sequence 구성

ADC sample을 저장할 buffer를 준비한다.

```c
int16_t raw;

struct adc_sequence sequence = {
    .buffer = &raw,
    .buffer_size = sizeof(raw),
};
```

DeviceTree에 정의된 ADC resolution 등의 값을 sequence에 적용하려면:

```c
ret = adc_sequence_init_dt(
    &pot_adc,
    &sequence
);
```

를 사용한다.

---

## 15. ADC read

```c
ret = adc_read(
    pot_adc.dev,
    &sequence
);
```

성공하면 `raw`에 ADC raw 값이 저장된다.

---

## 16. 측정 후 Power GPIO LOW

ADC 측정이 끝나면:

```c
gpio_pin_set_dt(
    &pot_power,
    0
);
```

으로 측정 회로를 끈다.

`GPIO_ACTIVE_HIGH`이므로 logical 0은 physical LOW가 된다.

---

## 17. 전체 potentiometer_read() 예제

```c
int potentiometer_read(int16_t *raw)
{
    int ret;

    if (raw == NULL) {
        return -EINVAL;
    }

    /*
     * Potentiometer measurement circuit ON
     */
    ret = gpio_pin_set_dt(
        &pot_power,
        1
    );

    if (ret < 0) {
        return ret;
    }

    /*
     * Wait for input voltage to settle
     */
    k_msleep(10);

    struct adc_sequence sequence = {
        .buffer = raw,
        .buffer_size = sizeof(*raw),
    };

    ret = adc_sequence_init_dt(
        &pot_adc,
        &sequence
    );

    if (ret < 0) {
        gpio_pin_set_dt(&pot_power, 0);
        return ret;
    }

    ret = adc_read(
        pot_adc.dev,
        &sequence
    );

    /*
     * Potentiometer measurement circuit OFF
     */
    gpio_pin_set_dt(
        &pot_power,
        0
    );

    return ret;
}
```

---

## 18. 전체 예제 코드

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include <errno.h>
#include <stdint.h>


#define POT_NODE DT_NODELABEL(potentiometer0)


static const struct gpio_dt_spec pot_power =
    GPIO_DT_SPEC_GET(POT_NODE, power_gpios);


static const struct adc_dt_spec pot_adc =
    ADC_DT_SPEC_GET(POT_NODE);


int potentiometer_init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&pot_power)) {
        return -ENODEV;
    }

    if (!adc_is_ready_dt(&pot_adc)) {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(
        &pot_power,
        GPIO_OUTPUT_INACTIVE
    );

    if (ret < 0) {
        return ret;
    }

    ret = adc_channel_setup_dt(&pot_adc);

    if (ret < 0) {
        return ret;
    }

    return 0;
}


int potentiometer_read(int16_t *raw)
{
    int ret;

    if (raw == NULL) {
        return -EINVAL;
    }

    ret = gpio_pin_set_dt(
        &pot_power,
        1
    );

    if (ret < 0) {
        return ret;
    }

    k_msleep(10);

    struct adc_sequence sequence = {
        .buffer = raw,
        .buffer_size = sizeof(*raw),
    };

    ret = adc_sequence_init_dt(
        &pot_adc,
        &sequence
    );

    if (ret < 0) {
        gpio_pin_set_dt(&pot_power, 0);
        return ret;
    }

    ret = adc_read(
        pot_adc.dev,
        &sequence
    );

    gpio_pin_set_dt(
        &pot_power,
        0
    );

    return ret;
}
```

---

## 19. main() 사용 예제

```c
int main(void)
{
    int ret;
    int16_t raw;

    ret = potentiometer_init();

    if (ret < 0) {
        printk("Potentiometer init failed: %d\n", ret);
        return ret;
    }

    while (1) {

        ret = potentiometer_read(&raw);

        if (ret < 0) {
            printk("ADC read failed: %d\n", ret);
        } else {
            printk("ADC raw = %d\n", raw);
        }

        k_msleep(500);
    }

    return 0;
}
```

---

## 20. 500 ms flag 방식과 결합

main loop를 500 ms 동안 block하고 싶지 않다면,
앞에서 사용한 timer flag 방식과 결합할 수 있다.

예:

```c
static volatile bool flag_500ms;
```

500 ms timer callback:

```c
static void timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    flag_500ms = true;
}
```

main loop:

```c
while (1)
{
    if (flag_500ms)
    {
        flag_500ms = false;

        potentiometer_read(&raw);
    }

    /*
     * Radar processing
     * Tact switch processing
     * ESB processing
     * etc.
     */
}
```

즉:

```text
500 ms timer
    ↓
flag = true
    ↓
main loop
    ↓
Power GPIO HIGH
    ↓
settle
    ↓
ADC read
    ↓
Power GPIO LOW
```

구조로 사용할 수 있다.

---

## 21. ADC 값을 millivolt로 변환

ADC raw 값을 millivolt로 변환하려면:

```c
int32_t mv;

mv = raw;

ret = adc_raw_to_millivolts_dt(
    &pot_adc,
    &mv
);
```

성공하면 `mv`에 ADC input voltage의 mV 값이 들어간다.

예:

```c
int potentiometer_read_mv(int32_t *mv)
{
    int ret;
    int16_t raw;

    if (mv == NULL) {
        return -EINVAL;
    }

    ret = potentiometer_read(&raw);

    if (ret < 0) {
        return ret;
    }

    *mv = raw;

    ret = adc_raw_to_millivolts_dt(
        &pot_adc,
        mv
    );

    return ret;
}
```

---

## 22. 핵심 패턴

```c
#define POT_NODE DT_NODELABEL(potentiometer0)

static const struct gpio_dt_spec power =
    GPIO_DT_SPEC_GET(POT_NODE, power_gpios);

static const struct adc_dt_spec adc =
    ADC_DT_SPEC_GET(POT_NODE);
```

초기화:

```c
gpio_is_ready_dt(&power);
adc_is_ready_dt(&adc);

gpio_pin_configure_dt(
    &power,
    GPIO_OUTPUT_INACTIVE
);

adc_channel_setup_dt(&adc);
```

측정:

```c
gpio_pin_set_dt(&power, 1);

k_msleep(10);

adc_sequence_init_dt(
    &adc,
    &sequence
);

adc_read(
    adc.dev,
    &sequence
);

gpio_pin_set_dt(&power, 0);
```

---

## 23. 핵심 함수와 매크로 정리

| 항목 | 역할 |
|---|---|
| `DT_NODELABEL(potentiometer0)` | DTS node label을 C에서 가져온다 |
| `GPIO_DT_SPEC_GET()` | `power-gpios`를 `gpio_dt_spec`으로 가져온다 |
| `ADC_DT_SPEC_GET()` | `io-channels`를 `adc_dt_spec`으로 가져온다 |
| `gpio_is_ready_dt()` | GPIO controller ready 확인 |
| `adc_is_ready_dt()` | ADC device ready 확인 |
| `gpio_pin_configure_dt()` | Power GPIO를 output으로 설정 |
| `gpio_pin_set_dt()` | 측정 회로 전원 ON/OFF |
| `adc_channel_setup_dt()` | ADC channel 초기화 |
| `adc_sequence_init_dt()` | ADC sequence를 DTS 설정에 맞게 초기화 |
| `adc_read()` | ADC 실제 측정 |
| `adc_raw_to_millivolts_dt()` | raw 값을 mV로 변환 |

---

## 24. prj.conf

기본적으로:

```conf
CONFIG_GPIO=y
CONFIG_ADC=y
```

를 사용한다.

---

## 25. 요약

DTS:

```dts
potentiometer0: potentiometer {
    io-channels = <&adc 0>;
    power-gpios = <&gpio0 29 GPIO_ACTIVE_HIGH>;
};
```

C에서 node 가져오기:

```c
#define POT_NODE DT_NODELABEL(potentiometer0)
```

GPIO 가져오기:

```c
static const struct gpio_dt_spec pot_power =
    GPIO_DT_SPEC_GET(POT_NODE, power_gpios);
```

ADC 가져오기:

```c
static const struct adc_dt_spec pot_adc =
    ADC_DT_SPEC_GET(POT_NODE);
```

측정 순서:

```text
gpio_pin_set_dt(power, 1)
        ↓
      HIGH
        ↓
    settle
        ↓
   adc_read()
        ↓
gpio_pin_set_dt(power, 0)
        ↓
       LOW
```

전체 관계:

```text
potentiometer0:
        ↓
DT_NODELABEL(potentiometer0)
        ↓
 ┌───────────────┬────────────────┐
 ↓               ↓
GPIO_DT_SPEC_GET ADC_DT_SPEC_GET
 ↓               ↓
Power GPIO       ADC channel
 ↓               ↓
HIGH             setup
 └──────→ settle
          ↓
       adc_read()
          ↓
       Power LOW
```
