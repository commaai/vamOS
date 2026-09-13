ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_utils
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_cpas/include
#for tof camera Begin#
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_sensor_module/cam_sensor
#for tof camera End#
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_sensor_module/cam_sensor_io
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_sensor_module/cam_sensor_utils
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_req_mgr
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_sensor_module/cam_cci
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_smmu/
ccflags-y += -I$(srctree)/drivers/media/platform/msm/camera/cam_core

obj-$(CONFIG_SPECTRA_CAMERA) += cam_eeprom_dev.o cam_eeprom_core.o cam_eeprom_soc.o
#for tof camera Begin#
obj-$(CONFIG_SPECTRA_CAMERA) += cam_eeprom_offload.o cam_eeprom_toflist.o
#for tof camera End#
