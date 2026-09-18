Goal

Create a new board directory from Acconeer's boards/acconeer/xm126 and rename the XM126-specific filenames to the new board name.

Board name

At the beginning, ask the user for the new board name unless it was already supplied in the prompt.

Example:

ritsn_touchless

Accept only a Zephyr-style lowercase name containing a-z, 0-9, and _. Store it as NEW_BOARD. Do not assume the example is the requested name.

Procedure

Locate this source directory from the project root:

boards/acconeer/xm126

Confirm that it contains:

board.cmake
board.yml
Kconfig
Kconfig.xm126
pre_dt_board.cmake
xm126.dts
xm126_defconfig
xm126.yaml

Copy the directory so the original XM126 board remains unchanged:

cp -a boards/acconeer/xm126 "boards/acconeer/${NEW_BOARD}"

If the destination already exists, stop and ask the user what to do. Never overwrite or merge it automatically.

In the copied directory, rename only these files:

Kconfig.xm126  -> Kconfig.${NEW_BOARD}
xm126.dts      -> ${NEW_BOARD}.dts
xm126_defconfig -> ${NEW_BOARD}_defconfig
xm126.yaml     -> ${NEW_BOARD}.yaml

Do not rename these common files:

board.cmake
board.yml
Kconfig
pre_dt_board.cmake

Print the final file list and verify that no filename in the new directory still contains xm126.

## Goal

Modify the board created in Step 1 for the user's custom hardware. Use `AcconeerRadar_Sch_REV.0.0.1.pdf` as the authoritative source for every pin number.

## Inputs

If not already supplied, ask for:

1. the new board/project name, such as `ritsn_touchless`;
2. the path to `AcconeerRadar_Sch_REV.0.0.1.pdf`.

Set the provided lowercase board name as `NEW_BOARD`. Work only inside:

```text
boards/acconeer/${NEW_BOARD}
```

Do not modify `boards/acconeer/xm126`.

## 1. Verify the schematic before editing

Open and visually inspect the PDF at high resolution. Do not rely only on extracted PDF text. Trace each named net from the peripheral to U3, and record this table before editing:

| Function | Schematic net | nRF52840 port/pin | Polarity or ADC input |
| --- | --- | --- | --- |
| Tact switch | schematic result | schematic result | active low, interrupt |
| DIP 1-6 | D_SW1-D_SW6 | six schematic results | pull-up, active low |
| Green LED | GREEN_LED | schematic result | active high |
| Red LED | RED_LED | schematic result | active high |
| Battery ADC | BATTERY_ADC | verify P0.03/AIN1 | ADC channel 1 |
| Potentiometer ADC | POT_ADC | verify P0.02/AIN0 | ADC channel 0 |

Cross-check every GPIO against both the net connection and the QFAA pin label. If a connection is unreadable, ambiguous, or conflicts with the table above, stop and ask the user. Never infer a GPIO from the old XM126 DTS or from a similarly named schematic.

## 2. Convert the MCU package and project identity

In `${NEW_BOARD}.dts`, replace:

```dts
#include <nordic/nrf52840_qiaa.dtsi>
```

with:

```dts
#include <nordic/nrf52840_qfaa.dtsi>
```

Within the new board directory, replace XM126 board identity references with the supplied project name, including lowercase `xm126` and uppercase `XM126` forms where they identify the board. Update the appropriate contents of:

```text
board.cmake
board.yml
Kconfig
Kconfig.${NEW_BOARD}
pre_dt_board.cmake
${NEW_BOARD}.dts
${NEW_BOARD}_defconfig
${NEW_BOARD}.yaml
```

Do not blindly replace unrelated vendor names, copyright text, compatible strings for real hardware devices, or paths outside the new board directory.

After editing, run:

```bash
rg -n 'xm126|XM126|qiaa|QIAA' "boards/acconeer/${NEW_BOARD}"
```

Review every remaining match; do not merely delete it.

## 3. DTS scope

Preserve required Acconeer radar, clock, flash, SRAM, UART, SPI, pinctrl, and chosen configuration already needed by the board. For the custom peripherals, add only these logical groups:

```text
tact_sw
dip_sw
green_led
red_led
adc_battery
adc_potentiometer
```

Do not add other application-specific peripherals in this step.

Every new DTS node label must begin with `ritsn_`. Examples:

```text
ritsn_tact_sw
ritsn_dip_sw
ritsn_dip_sw0 ... ritsn_dip_sw5
ritsn_green_led
ritsn_red_led
ritsn_adc_battery
ritsn_adc_potentiometer
```

Use valid existing Zephyr bindings whenever possible. Do not invent a custom `compatible` without also confirming that its binding exists.

### Tact switch

- Use the exact GPIO traced from the schematic.
- Configure it as `GPIO_PULL_UP | GPIO_ACTIVE_LOW`.
- Represent it with a standard input-key binding suitable for GPIO interrupt handling.
- Configure only the DTS polarity and pull-up here; the application will register the interrupt callback later.

### DIP switches

- Create six distinct children in schematic order `D_SW1` through `D_SW6`.
- Use the six exact GPIOs traced from the schematic.
- Configure every input as `GPIO_PULL_UP | GPIO_ACTIVE_LOW`.
- Keep the DIP bit order explicit and do not reorder pins numerically.

### LEDs

- Create separate green and red LED nodes using the standard GPIO LED binding.
- Use the exact schematic GPIO for each color.
- Configure both as `GPIO_ACTIVE_HIGH` with GPIO output control.
- Do not swap LED colors based on component reference alone; verify the `GREEN_LED` and `RED_LED` nets.

### ADC inputs

- Battery: `io-channels = <&adc 1>` and `io-channel-names = "battery"`.
- Potentiometer: `io-channels = <&adc 0>` and `io-channel-names = "potentiometer"`.
- Under `&adc`, define channel 1 for `P0.03/AIN1` and channel 0 for `P0.02/AIN0`, after confirming both in the PDF.
- Use the nRF ADC input macros matching AIN1 and AIN0.
- Do not copy ADC channels or pin assignments from the QIAA-based XM126 configuration.
- Do not add battery-divider resistance values, ADC power-enable GPIOs, or potentiometer-switch GPIOs unless the user requests them in a later step.

## 4. Validation

Before finishing:

1. compare every DTS GPIO against the recorded schematic table;
2. confirm the DTS includes `nrf52840_qfaa.dtsi` and contains no QIAA include;
3. confirm all new node labels start with `ritsn_`;
4. confirm tact and all six DIP inputs are pull-up and active low;
5. confirm both LEDs are active high;
6. confirm battery uses ADC channel 1/AIN1 and potentiometer uses channel 0/AIN0;
7. run a DTS syntax/build check if the SDK environment is available;
8. show the user the final pin-mapping table and list every modified file.

Do not claim success if the PDF pin tracing or DTS compilation remains unresolved.

XM126 Custom Board Conversion — Step 3

Goal

Update the custom board's UART, A121 SPI, A121 ENABLE, and A121 INTERRUPT configuration using AcconeerRadar_Sch_REV.0.0.1.pdf as the authoritative schematic.

This step assumes Steps 1 and 2 are complete. Ask for the board/project name if it was not provided, then work only in:

boards/acconeer/<NEW_BOARD>

Do not modify boards/acconeer/xm126.

Verified QFAA pin map

The following connections were traced directly from U3 (nRF52840-QFAA) in the schematic:

Function

Schematic net

nRF52840 QFAA pin

UART RX

UART_RX

P0.05

UART TX

UART_TX

P0.11

A121 SPI MISO

SPI_MISO

P0.04

A121 SPI MOSI

SPI_MOSI

P0.08

A121 SPI clock

SPI_CLK

P1.09

A121 interrupt

INTERRUPT

P0.07

A121 enable

ENABLE

P0.14

A121 chip select

SPI_SS

directly connected to GND

Important: A121 SPI_SS is not connected to an nRF52840 GPIO. It is permanently tied low in this schematic. Do not assign a chip-select GPIO and do not copy XM126's CS pin.

The INTERRUPT signal here is the A121 radar interrupt, not the tact-switch interrupt from Step 2.

1. Check existing source expectations

Before editing, inspect the new board DTS, pinctrl definitions, and Acconeer HAL references:

rg -n 'uart|spi|enable|interrupt|cs-gpios|DT_ALIAS|DT_NODELABEL|DT_PATH|GPIO_DT_SPEC|SPI_DT_SPEC' .

Preserve the aliases and property names required by the Acconeer source. New custom node labels must start with ritsn_, but fixed aliases may remain when the existing HAL depends on them.

2. Update UART

Update the UART pinctrl configuration to:

RX = P0.05
TX = P0.11

Use the correct Nordic pinctrl form, equivalent to:

NRF_PSEL(UART_RX, 0, 5)
NRF_PSEL(UART_TX, 0, 11)

Update both default and sleep pinctrl states consistently. Preserve the existing UART controller, baud rate, console selection, and status = "okay" unless the user requests another change.

Do not reverse TX and RX based on the external header direction; these names are the MCU net names shown at U3.

3. Update A121 SPI

Update the SPI pinctrl configuration to:

MISO = P0.04
MOSI = P0.08
SCK  = P1.09

Use the correct Nordic pinctrl forms, equivalent to:

NRF_PSEL(SPIM_MISO, 0, 4)
NRF_PSEL(SPIM_MOSI, 0, 8)
NRF_PSEL(SPIM_SCK, 1, 9)

Apply the same physical signals to the appropriate default and sleep states. Preserve the SPI controller instance, frequency, mode, and A121 child configuration required by the existing Acconeer project unless they conflict with the custom schematic.

SPI chip select rule

The schematic connects A121 pin J2 SPI_SS directly to GND. Therefore:

do not define cs-gpios for the A121;

remove an inherited XM126 CS GPIO assignment from the custom board;

do not allocate a replacement GPIO;

do not let the HAL toggle a nonexistent CS line.

Check how the existing Acconeer HAL obtains its SPI specification. If it requires a valid CS GPIO unconditionally, adapt only the custom-board configuration/HAL path so SPI transfers operate with permanently asserted chip select. Do not fabricate a DTS GPIO to satisfy the API.

4. Update A121 control GPIOs

Create or update custom-board nodes/properties using:

ritsn_a121_enable:    P0.14
ritsn_a121_interrupt: P0.07

ENABLE is an MCU output to A121 F10 ENABLE.

INTERRUPT is an MCU input from A121 K8 INTERRUPT.

Preserve the active polarity and interrupt edge required by the A121 specification and the working XM126 HAL; change the physical GPIO to P0.14 and P0.07.

Do not apply the tact-switch pull-up/active-low settings to the A121 interrupt.

Preserve any alias or property name used by the Acconeer HAL.

5. Validation

Before finishing:

confirm UART RX=P0.05 and TX=P0.11;

confirm SPI MISO=P0.04, MOSI=P0.08, and SCK=P1.09;

confirm A121 INTERRUPT=P0.07 and ENABLE=P0.14;

confirm there is no A121 cs-gpios because SPI_SS is grounded;

check for duplicate assignments against tact, DIP, LED, ADC, SWD, and reset pins;

confirm all default/sleep pinctrl groups use the new QFAA mapping;

run a devicetree/build validation if the NCS environment is available;

report every modified file and any HAL adjustment required for permanent-low SPI_SS.

Do not change unrelated peripherals in this step.
