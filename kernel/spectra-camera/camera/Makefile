obj-$(CONFIG_SPECTRA_CAMERA) += cam_req_mgr/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_utils/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_core/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_sync/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_smmu/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_cpas/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_cdm/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_isp/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_sensor_module/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_icp/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_jpeg/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_fd/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_lrme/
# al6100 (Altek companion mini-ISP) is excluded from the mainline build: it is
# NOT part of the mici comma four Spectra camera data path (mici uses the SoC's
# own IFE/ICP ISP), it is fully self-contained (no other cam_* subdir references
# it), and its header al6100/include/isp_camera_cmd.h emits unbalanced
# "#pragma pack(1)" that leaks into the kernel headers it transitively includes
# (linux/fs.h), tripping the struct filename alignment static_assert. Off the
# data path, so dropped rather than patched. (ledger: docs/cameras/TASKS.md)
# obj-$(CONFIG_SPECTRA_CAMERA) += al6100/
