# vamOS
a new operating system for comma 3X and comma four

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
