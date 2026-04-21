# vamOS
a new operating system for comma 3X and comma four

[![fast](docs/fast.png)](https://discord.com/channels/469524606043160576/1262118077017882715/1482214461740683385)

## Usage

```
./vamos setup              # init submodules and udev rules
./vamos build kernel       # build boot.img
./vamos build system       # build system.img
./vamos vm prepare         # init qemu package
./vamos vm bash            # run bash in kernel and system
./vamos vm run             # run full system and kernel
./vamos vm kill            # kill qemu as system has no promt
./vamos flash kernel       # flash boot.img via EDL
./vamos flash system       # flash system.img via EDL
./vamos flash all          # flash both
./vamos profile diff A B   # diff two rootfs profiles
```

## Kernel Patches

Patches in `kernel/patches/` are applied in order to the Linux kernel tree. They follow this naming convention:

```
NNNN-SUBSYSTEM-description.patch
```

- `NNNN` — sequential number, zero-padded (0001, 0002, …)
- `SUBSYSTEM` — the area of the kernel being modified:
  - `defconfig` — kernel configuration files
  - `dts` — device tree sources
  - `driver` — driver changes
  - `core` — core kernel subsystem changes
- `description` — short kebab-case summary of the change

Example: `0001-defconfig-add-vamos.patch`

## TODO

comma threex:
- [x] ufs
- [x] display
- [ ] i2c
  - [ ] TODO: look at device tree
- [x] wifi
  - [ ] testing (set benchmarks, test case)
- [x] usb
- [x] modem
- [ ] sound
- [x] SPI
- [ ] GPS
- [ ] cameras (OX03C10)
  - [ ] kernel wiring
  - [ ] ISP
  - [ ] openpilot
- [x] graphics
  - [x] gpu
- [ ] opencl - via rusticl / msm_drm
- [ ] Venus? (video encode/decode)

comma four:
- [x] ufs
- [x] display
- [ ] i2c (IMU/temp/...)
- [x] wifi
  - [ ] testing (set benchmarks, test case)
- [x] usb
- [x] modem
- [ ] sound
- [x] SPI
- [ ] GPS
- [ ] cameras (OS04C10)
  - [ ] kernel wiring
  - [ ] ISP
  - [ ] openpilot
- [x] graphics
  - [x] gpu
- [ ] opencl - via rusticl / msm_drm
- [ ] Venus (video encode/decode)

openpilot support:
- [ ]

tinygrad support:
- [ ] msm_drm

validation:
- [ ] dmesg is clean (background in https://github.com/commaai/agnos-builder/issues/325)
- [ ] test_onroad passes
- [ ] testing closet
