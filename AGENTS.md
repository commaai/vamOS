# vamOS

## building

```bash
./vamos build kernel    # builds kernel in docker, outputs build/boot.img
./vamos build system    # builds system image (Dockerfile + Void Linux rootfs)
```
- always rebuild immediately after config/DTS/userspace changes. don't ask, just build
- kernel and system builds can run in parallel

## flashing

poll for EDL mode before flashing (don't ask the user, just wait):
```bash
while ! lsusb 2>/dev/null | grep -qE "QUSB_BULK|\(QDL Mode\)"; do sleep 1; done
./vamos flash kernel
./vamos flash system
./vamos flash firmware
```

## testing

during debug and implementation, when the device is online and reachable via SSH, prefer modifying it directly (e.g. pushing files, editing configs, restarting services over SSH) rather than rebuilding and reflashing. reflashing requires swapping USB cables and human intervention — avoid it unless the change genuinely requires a new kernel or system image (kernel config, DTS, partition layout). userspace files, firmware, scripts, and service configs can almost always be tested live on-device first.

before creating a PR, do a clean end-to-end rebuild and reflash to validate the changes work without any manual on-device modifications.

## ssh

device must be plugged in via its secondary USB port (USB NCM gadget):
```bash
ssh comma@192.168.42.2
```

if "Permission denied", use the setup key:
```bash
curl -sL https://raw.githubusercontent.com/commaai/openpilot/refs/heads/master/system/hardware/tici/id_rsa -o /tmp/setup_rsa && chmod 600 /tmp/setup_rsa
ssh -i /tmp/setup_rsa comma@192.168.42.2
```
