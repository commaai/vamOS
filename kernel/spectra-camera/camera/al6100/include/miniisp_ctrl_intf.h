
extern int handle_ControlFlowCmd_II(u16 miniisp_op_code, u8 *param);
extern long handle_ControlFlowCmd(unsigned int cmd, unsigned long arg);
// ALTEK_AL6100_CHI >>>
enum miniisp_firmware {
	IQ_CODE,
	DEPTH_CODE,
	OTHER_MAX
};
extern long handle_ControlFlowCmd_CHI(unsigned int cmd, unsigned long arg);
extern void mini_isp_other_drv_open(char *file_name, u8 type);
extern void mini_isp_other_drv_read(struct file *filp, u8 type);
// ALTEK_AL6100_CHI <<<
