# Cameras: legacy baseline + the mainline interface problem

Captured 2026-06-13 from the live mici on the **legacy 4.9.103 AGNOS kernel**
(`build/boot-legacy.img`). This is the working reference the mainline kernel must
reproduce, and it documents *why* "interface with openpilot without modification"
is the hard part.

## Working legacy baseline (verified, images captured)

- DT model: `comma mici`. 3 cameras stream: camera 0 (road), 1 (wide road),
  2 (driver). `camerad` synced all three with incrementing frame_ids + hw
  timestamps. `snapshot.py` produced real JPEGs (`camera-proof/legacy-{back,front}.jpg`,
  1344x760, full ISP YUV→RGB).
- Additional controlled bundle captured 2026-06-14 with the same legacy 4.9.103
  kernel: `snapshot_standalone` captured all three cameras as 1344x760 RGB PNG
  plus NV12 (`snapshot.rc=0`, `=== captured 3/3 cameras ===`). Kernel dmesg shows
  chip-id success for slots 0/1/2 at slave addresses `0x6c/0x20/0x6c`, all reading
  `sensor_id:0x5304`.
- Sensors: openpilot ships **two** sensor drivers for mici —
  `system/camerad/sensors/{os04c10,ox03c10}.cc`. The DT does NOT name the part;
  camerad probes the chip-id over CCI I2C at runtime. CCI has 4 sensor slots
  (`qcom,cam-sensor@0..3`), phy/cci-master = (0,0),(1,0),(2,1),(3,1).

## How openpilot's camerad talks to the camera stack (the crux)

`system/camerad/cameras/spectra.cc` + `camera_qcom2.cc` open the **downstream
Qualcomm "Spectra" CAMSS ABI** — NOT the mainline `qcom-camss` V4L2 driver:

- `/dev/v4l/by-path/platform-soc:qcom_cam-req-mgr-video-index0`  (video0, **cam-req-mgr**)
- `/dev/v4l/by-path/platform-cam_sync-video-index0`              (video1, **cam_sync**)
- `/dev/v4l-subdev*` named: `cam-cpas`, `cam-isp`, `cam-cci-driver`,
  `cam-csiphy-driver` ×4, `cam-sensor-driver` ×4, `cam-icp`, `cam-jpeg`,
  `cam-fd`, `cam-lrme`.

camerad drives these with downstream ioctls (`cam_req_mgr`, `cam_isp`,
`cam_sensor` UAPI structs) and uploads IFE/BPS config blobs (`ife.h`,
`bps_blobs.h`, `cdm.cc`). These nodes/structs are created by the downstream
`drivers/media/platform/msm/camera` ("camera_kt"/Spectra) driver shipped in
AGNOS's 4.9 kernel.

## Why mainline can't satisfy this unmodified

Mainline Linux has a `qcom-camss` driver, but it presents a COMPLETELY different
interface: a `media-ctl` graph of `msm_csiphy`/`msm_csid`/`msm_vfe` entities and
plain `/dev/videoN` capture nodes — **no `cam-req-mgr`, no `cam_sync`, no
`cam_isp`/`cam_sensor` UAPI, no ICP/BPS/IPE**. mainline qcom-camss on sdm845 also
only wires the CSID→VFE "RDI"/bayer path; it has no Spectra request manager and
no on-device ISP pipeline (debayer/AWB/tonemap) that produces the YUV frames
camerad's VisionIPC consumers expect.

So there is no mainline kernel config that makes camerad work **byte-for-byte
unmodified**. The downstream ABI does not exist upstream and openpilot is written
directly against it. The realistic paths are below — this needs a decision.
</content>
