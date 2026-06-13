# Mainline vamOS Gaps vs Legacy (device locations openpilot needs)

Generated 2026-06-12 by diffing the legacy baseline (`README.md` here) against
`kernel/dts/sdm845-comma-common.dtsi` + `sdm845-comma-mici.dts` and the upstream
`arch/arm64/boot/dts/qcom/sdm845.dtsi` node map.

Mainline node labels are address-keyed differently from legacy bus numbers. The
relevant upstream geni nodes:
```
i2c4  = i2c@890000   (legacy i2c1 — IMU)
i2c5  = i2c@894000   (legacy i2c2 — touch)
i2c10 = i2c@a88000   (legacy i2c0 — ina231 power)
spi0  = spi@880000   (legacy spi0  — panda)
uart3 = serial@88c000
uart6 = serial@898000 (legacy hsuart0 — GPS)
uart9 = serial@a84000 (legacy serial0 — console)
```

## P0 — breaks openpilot if shipped as-is

> **Status (2026-06-12, flashed + verified on mici over serial,
> kernel 6.18.0-vamos):**
> - **Item 1 (i2c bus aliases): DONE & VERIFIED on-device.** After flashing,
>   the controllers map exactly as openpilot needs:
>   `i2c-0 → i2c@a88000`, `i2c-1 → i2c@890000` (IMU 0x6a present),
>   `i2c-2 → i2c@894000` (touch ft3168 0x38, UU=bound). IMU is on bus 1. ✅
> - **Item 2 (ina231): WITHDRAWN — not a gap; chip not populated on mici.**
>   Adding `ina231@40` produced `ina2xx 0-0040: -ENXIO` on-device (no ACK, empty
>   bus 0). Root cause: the comma four has **no ina231** — legacy
>   `comma_mici.dts` explicitly `/delete-node/ ti_ina321@40` ("Not populated on
>   mici; probing it costs ~500ms at boot"). It's a tizi/comma-three part.
>   openpilot's power monitoring on the four reads the PMIC `bms` power_supply
>   (`tici/hardware.py:246` `/sys/class/power_supply/bms/...`), not `/dev/i2c-0`.
>   The ina231 node + gpi_dma1 enable were reverted; `&i2c10` left enabled (no
>   children) only to keep bus-0 numbering parity. Lesson: validate against the
>   per-board legacy DTS, not just the generic common.dtsi.
>
> Implemented in `kernel/dts/sdm845-comma-common.dtsi` (the comma DTS is the
> build source of truth; `kernel/patches/` only modifies the upstream kernel
> tree). Net change = the `aliases` i2c remap + `&i2c10` enable.

### 1. i2c bus numbers don't match openpilot's hardcoded buses ❗
openpilot does `I2C_BUS_IMU = 1` (`/dev/i2c-1`), power mon on bus 0, touch on
bus 2. Linux assigns i2c adapter numbers from `of_alias_get_id(np,"i2c")`
(`i2c-core-base.c:1658`), which returns the **first** alias whose target matches
the node (`of_alias_get_id` in `of/base.c` breaks on first match).

Gotcha confirmed: upstream `sdm845.dtsi` ships **identity aliases**
(`i2c0=&i2c0 … i2c15=&i2c15`). So by default the IMU node `i2c@890000` = `i2c4`
= **bus 4**, touch `i2c@894000` = bus 5, power `i2c@a88000` = bus 10 — none
match openpilot. Adding `i2c1=&i2c4` alone leaves the stale `i2c4=&i2c4`
pointing at the same node (duplicate target → resolution depends on tree order).

**Fix (in the comma dtsi, overrides upstream):** remap so each physical node has
exactly the bus openpilot wants, and reassign the displaced low-numbered aliases
to the now-unused nodes to kill duplicate targets:
```
i2c0  = &i2c10;  /* i2c@a88000  power/ina231 -> bus 0 */
i2c1  = &i2c4;   /* i2c@890000  IMU          -> bus 1 */
i2c2  = &i2c5;   /* i2c@894000  touch        -> bus 2 */
i2c4  = &i2c0;   /* park the displaced identity aliases on unused nodes */
i2c5  = &i2c1;
i2c10 = &i2c2;
```

### 2. ina231 power monitor — WITHDRAWN (not present on comma four)
See status box above. The ina231 is a tizi/comma-three part; `comma_mici.dts`
deletes it on the four. No DT change needed for mici. The four's power telemetry
comes from the PMIC `bms` power_supply, already exposed by the mainline
qcom-spmi / pmic stack — confirm `/sys/class/power_supply/bms/` exists on
mainline as a separate (P1) check, but it is NOT an i2c/alias gap.

(Investigated and ruled out on-device: pinmux gpio55/56→qup10 applied, bus
controller enumerates, gpi_dma1 enable made no difference — the chip simply
isn't there.)

### Dropped after live verification (were earlier draft P0s)
- **GPS uart**: NOT a bug. `/dev/ttyHS0` resolves to `88c000.qcom,qup_uart`
  (upstream `uart3`) on the live device; `i2c@898000`/uart6 is `disabled`. The
  legacy `hsuart0` alias points at the disabled 898000 node and is itself stale.
  `mici.dts` `hsuart0 = &uart3` + enabling `uart3` is **correct**. (The uart6
  bluetooth node in common.dtsi is a tizi/3X artifact — no BT in mici DT.)
- **Panda spidev**: non-issue. Upstream already aliases `spi0=&spi0` → bus 0, so
  the `commaai,panda` child on `spi0` enumerates as `/dev/spidev0.0`.

## P1 — feature gaps (tracked in top-level README TODO)

| Area | Legacy presence | Mainline DTS | Note |
|---|---|---|---|
| Cameras (OS04C10) | CAMSS: media0/1, v4l-subdev*, `qcom,cci@ac4a000`, csiphy×4, vfe0/1 | absent | biggest gap; needs CCI+CSIPHY+VFE+sensor wiring |
| Sound | tavil/`sound-tavil`, full q6/dai stack | absent | README sound TODO |
| Venus (video enc/dec) | `qcom,venus@aae0000` | absent | README TODO |
| OpenCL/tinygrad | kgsl + renderD128 (GPU up) | gpu okay; no rusticl/msm path | userspace, not DT |

## P2 — behavioral differences to verify (not blockers)

- **Touch input event number (event0 vs event2):** RESOLVED for tooling; no DT
  fix needed. Touch is `/dev/input/event2` on legacy but `event0` on mainline,
  because legacy registers two input devices ahead of it — `qpnp_pon` (PMIC
  power-key, downstream `qcom,qpnp-power-on`) and `qbt1000` (fingerprint) — and
  mainline registers neither. On mainline the `qcom,pm8998-pon` MFD driver isn't
  bound to `pon@800` (no PON parent driver compiled in), so its `pwrkey`/`resin`
  input children never appear. **openpilot does NOT hardcode event2** — its UI
  uses raylib `get_touch_position()` which auto-discovers the touch device, so
  the kernel event-number shift does not affect openpilot. The only `event2`
  hardcode was the **mici skill's `mici_ui.py`**, now fixed to auto-detect via
  the stable `/dev/input/by-path/platform-894000.i2c-event` symlink (keyed on
  the i2c controller address, kernel-independent), with name- and fixed-fallback.
  Optional future work: enable `qcom,pm8998-pon` + pwrkey/resin on mainline to
  get a working power button (would also restore legacy-like event numbering),
  but that's a feature, not a requirement.
- **gpiochip count:** legacy exposes 32 split gpiochips; mainline qcom-pinctrl
  exposes one consolidated chip. openpilot uses `gpiochip0` line 84 — confirm
  the line offset is identical under the consolidated controller, else the irq
  line in `sensord.py` shifts.
- **`/dev/ttyMSM0` console:** legacy serial0 → a84000 = upstream `uart9`, which
  the dtsi already enables as the console. Consistent. ✓
- **UFS main LUN partitioning:** legacy `sde` has 48 partitions; ensure mainline
  UFS provisioning/GPT matches what AGNOS/openpilot expects.

## On-device functional test results (2026-06-12, mainline 6.18.0-vamos)

Verified over serial that the devices openpilot needs are not just enumerated at
the right location but actually working:

- **IMU (i2c-1 @ 0x6a):** WHO_AM_I=0x6a (LSM6DS3TR-C), STATUS_REG=0x07 (accel +
  gyro + temp data-ready), live accel sampling (Z≈+0.91g flat, values change
  between reads), temp ≈28.6°C. ✅
- **Touch (i2c-2 @ 0x38, ft3168):** driver owns 0x38 (raw i2cget = "resource
  busy"), input `EP0110M09` registered, ABS_MT axes configured to 0–239 × 0–535
  (matches DTS panel size — proves probe-time i2c read succeeded), `touch_count`
  sysfs present, both IRQ lines wired. ✅
- **Panda (spi0):** `/dev/spidev0.0` present, `spi:panda` bound. ✅
- **GPS (ttyHS0 = 88c000/uart3):** port live (`is a MSM`, irq 126), responds to
  termios. ✅

## Suggested next actions
1. ✅ DONE: `aliases` i2c remap implemented, flashed, and bus numbers verified
   on-device (IMU=bus1, touch=bus2, power=bus0).
2. ✅ DONE: ina231 investigated and withdrawn (not on mici).
3. ✅ DONE: touch event-node auto-detect fixed in mici_ui.py.
4. Optional P2: enable `qcom,pm8998-pon` pwrkey on mainline (power button).
5. Move on to P1 feature gaps: cameras (OS04C10), sound, Venus.
