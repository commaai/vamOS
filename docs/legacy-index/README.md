# Legacy ↔ Mainline Device Index (comma mici / SDA845)

Baseline captured **2026-06-12** from the live legacy kernel running on mici
(`comma@10.0.0.22`), `Linux 4.9.103`, model `comma mici`, `qcom,sda845-mtp`.

Purpose: enumerate every device openpilot opens by a **fixed path**, record where
the legacy kernel places it, and flag where the mainline vamOS DTS
(`kernel/dts/sdm845-comma-*.dts`) does not yet reproduce that location.

The hard constraint: **openpilot hardcodes bus numbers and dev nodes.** Linux i2c
adapter numbers and `/dev/ttyHS*`/`/dev/spidev*` names are assigned from the DT
`aliases` node, *not* from node labels or probe order. The legacy DT pins them via
aliases; the mainline DTS currently does not, so devices land on different buses.

## openpilot device-location requirements (hardcoded, verified on device)

| Device | openpilot path | source |
|---|---|---|
| IMU (LSM6DS3 accel/gyro/temp) | **i2c bus 1** | `system/sensord/sensord.py:20` `I2C_BUS_IMU = 1` |
| Panda | **`/dev/spidev0.0`** | `selfdrive/pandad/spi.cc:31` |
| GPS (u-blox) | **`/dev/ttyHS0`** | `system/ubloxd/pigeond.py:19`, `process_config.py:26` |
| Amplifier (tici only) | i2c bus 0 | `system/hardware/tici/amplifier.py:73` |
| GPIO (sensor irq etc.) | `gpiochip0` line 84 | `system/sensord/sensord.py:32` |

## Legacy ground truth (live `/sys`, `i2cdetect`, `/dev`)

| Device | Legacy location | i2cdetect | DT node | DT alias |
|---|---|---|---|---|
| IMU LSM6DS3 | i2c-**1** @ 0x6a | `6a` on bus 1 | `i2c@890000` | `i2c1` |
| Touch fts (FT3168) | i2c-**2** @ 0x38 | `UU` on bus 2 | `i2c@894000` | `i2c2` |
| Power mon ina231 | i2c-**0** @ 0x40 | `0-0040` | `i2c@a88000` | `i2c0` |
| Panda | `/dev/spidev0.0` (`spi:panda`) | — | `spi@880000` | `spi0` |
| GPS | `/dev/ttyHS0` (group gpio) | — | `qup_uart@0x898000` | `hsuart0` |
| Console | `/dev/ttyMSM0` | — | `qup_uart@0xa84000` | `serial0` |
| GPIO main | `/dev/gpiochip0` (group gpio) | — | tlmm pinctrl | — |

Legacy DT aliases (the authoritative bus→address map):
```
i2c0     -> /soc/i2c@a88000      (ina231 power)
i2c1     -> /soc/i2c@890000      (IMU)
i2c2     -> /soc/i2c@894000      (touch)
spi0     -> /soc/spi@880000      (panda)
serial0  -> /soc/qcom,qup_uart@0xa84000   (console ttyMSM0)
hsuart0  -> /soc/qcom,qup_uart@0x898000   (GPS ttyHS0)
sdhc2    -> /soc/sdhci@8804000
ufshc1   -> /soc/ufshc@1d84000
```

Other live state:
- Net: `wlan0` (ath10k/wcn3990), `rmnet_ipa0` (modem), `bond0`, `ppp0`.
- Block: UFS `sda`–`sdf`; `sde` is the main LUN (48 partitions).
- GPU/DRM: `/dev/dri/card0` + `renderD128`.
- Camera (legacy CAMSS): `/dev/media0`, `/dev/media1`, many `/dev/v4l-subdev*`.
- Kernel is monolithic (no loadable modules; everything built-in).
- 32 split gpiochips on legacy (downstream TLMM); mainline consolidates to one.

Full soc compatible/status dump: `soc-compatibles.txt`.
