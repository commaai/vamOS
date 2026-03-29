#include "tl_dev_eeprom.h"

/*for tof camera Begin*/

struct cam_eeprom_list_head cam_eeprom_list_head;

int transmit_sensor_reg_setting_get(struct list_head *reg_settings
		,enum EEPROM_DATA_OP_T type)
{
	if(cam_eeprom_list_head.list_state == true){
		if(type == EEPROM_INIT_DATA ) {
			reg_settings->prev
				= cam_eeprom_list_head.list_head_init.prev;
			reg_settings->next
				= cam_eeprom_list_head.list_head_init.next;
			cam_eeprom_list_head.list_head_init.next->prev
				= reg_settings;
			cam_eeprom_list_head.list_head_init.prev->next
				= reg_settings;
		} else if (type == EEPROM_CONFIG_DATA ) {
			reg_settings->prev
				= cam_eeprom_list_head.list_head_config.prev;
			reg_settings->next
				= cam_eeprom_list_head.list_head_config.next;
			cam_eeprom_list_head.list_head_config.next->prev
				= reg_settings;
			cam_eeprom_list_head.list_head_config.prev->next
				= reg_settings;
		} else if(type == EEPROM_STREAMON_DATA) {
			reg_settings->prev
				= cam_eeprom_list_head.list_head_streamon.prev;
			reg_settings->next
				= cam_eeprom_list_head.list_head_streamon.next;
			cam_eeprom_list_head.list_head_streamon.next->prev
				= reg_settings;
			cam_eeprom_list_head.list_head_streamon.prev->next
				= reg_settings;
		} else if (type == EEPROM_STREAMOFF_DATA){
			reg_settings->prev
				= cam_eeprom_list_head.list_head_streamoff.prev;
			reg_settings->next
				= cam_eeprom_list_head.list_head_streamoff.next;
			cam_eeprom_list_head.list_head_streamoff.next->prev
				= reg_settings;
			cam_eeprom_list_head.list_head_streamoff.prev->next
				= reg_settings;
		} else return -1;
	return 0;
	} else return -1;
}

int transmit_sensor_reg_setting_ret(struct list_head *reg_settings
		,enum EEPROM_DATA_OP_T type)
{
	if(type == EEPROM_INIT_DATA) {
		cam_eeprom_list_head.list_head_init.prev = reg_settings->prev;
		cam_eeprom_list_head.list_head_init.next = reg_settings->next;
		reg_settings->next->prev = &cam_eeprom_list_head.list_head_init;
		reg_settings->prev->next = &cam_eeprom_list_head.list_head_init;
		reg_settings->next = reg_settings;
		reg_settings->prev = reg_settings;
	} else if(type == EEPROM_CONFIG_DATA) {
			cam_eeprom_list_head.list_head_config.prev
				= reg_settings->prev;
			cam_eeprom_list_head.list_head_config.next
				= reg_settings->next;
			reg_settings->next->prev
				= &cam_eeprom_list_head.list_head_config;
			reg_settings->prev->next
				= &cam_eeprom_list_head.list_head_config;
			reg_settings->next = reg_settings;
			reg_settings->prev = reg_settings;
	} else if(type == EEPROM_STREAMON_DATA) {
		cam_eeprom_list_head.list_head_streamon.prev = reg_settings->prev;
		cam_eeprom_list_head.list_head_streamon.next = reg_settings->next;
		reg_settings->next->prev = &cam_eeprom_list_head.list_head_streamon;
		reg_settings->prev->next = &cam_eeprom_list_head.list_head_streamon;
		reg_settings->next = reg_settings;
		reg_settings->prev = reg_settings;
	} else if(type == EEPROM_STREAMOFF_DATA){
		cam_eeprom_list_head.list_head_streamoff.prev = reg_settings->prev;
		cam_eeprom_list_head.list_head_streamoff.next = reg_settings->next;
		reg_settings->next->prev = &cam_eeprom_list_head.list_head_streamoff;
		reg_settings->prev->next = &cam_eeprom_list_head.list_head_streamoff;
		reg_settings->next = reg_settings;
		reg_settings->prev = reg_settings;
	}

	return 0;
}

static struct i2c_settings_list * list_node_create(
		struct list_head *list_head
		,uint32_t data_num)
{
	struct i2c_settings_list *node
		= (struct i2c_settings_list *)kzalloc(
			sizeof(struct i2c_settings_list),GFP_KERNEL);
	if (node != NULL)
		list_add_tail(&(node->list),list_head);
	else
		return NULL;

	if(data_num == 1) {
		node->op_code = CAM_SENSOR_I2C_WRITE_BURST;
	} else if (data_num > 1) {
		node->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
	} else {
		CAM_ERR(CAM_EEPROM,"data_num error");
		return NULL;
	}
	node->i2c_settings.size = data_num;
	node->i2c_settings.reg_setting
		= (struct cam_sensor_i2c_reg_array *)kzalloc(
			sizeof(struct cam_sensor_i2c_reg_array)*data_num,GFP_KERNEL);
	if(node->i2c_settings.reg_setting == NULL) {
		return NULL;
	}
	node->i2c_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	node->i2c_settings.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;

	return node;
}

static TL_E_RESULT create_node_for_seq(uint16_t *pup_data
		,uint32_t data_num)
{
	uint32_t n                = 1;
	uint32_t i                = 0;
	uint32_t temp             = 0;

	struct i2c_settings_list *node = kzalloc(
			sizeof(struct i2c_settings_list),GFP_KERNEL);
	if (node != NULL)
		list_add_tail(&(node->list),&cam_eeprom_list_head.list_head_init);
	else
		return TL_E_ERR_SYSTEM;

	temp = pup_data[n];

	node->i2c_settings.reg_setting
		= (struct cam_sensor_i2c_reg_array *)kzalloc(
			sizeof(struct cam_sensor_i2c_reg_array)*data_num,GFP_KERNEL);
	if(node->i2c_settings.reg_setting == NULL){
		return TL_E_ERR_SYSTEM;
	}

	for(i = 0;i < data_num; i++){
		n += 1;
		node->i2c_settings.reg_setting[i].reg_data = pup_data[n];
		node->i2c_settings.reg_setting[i].reg_addr = temp + i;
	}
	node->i2c_settings.size = data_num;
	node->i2c_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	node->i2c_settings.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	node->op_code = CAM_SENSOR_I2C_WRITE_SEQ;

	return TL_E_SUCCESS;
}

static TL_E_RESULT cam_eeprom_list_init_setting_data(
		tl_dev_eeprom_pup *tof_eeprom,
		TL_E_BOOL external_sync)
{
	uint32_t  n                            = 0;
	int16_t   i                            = 0;
	struct    i2c_settings_list *i2c_list;
	uint32_t  data_num;
	uint16_t  pup_size = tof_eeprom->pup_size;
	uint32_t  reg_array_num,addr_temp,n_temp;
	uint16_t  *pup_data = tof_eeprom->pup_data;
	TL_E_RESULT  tl_ret = TL_E_SUCCESS;

	INIT_LIST_HEAD(&cam_eeprom_list_head.list_head_init);

	data_num = pup_data[n];
	if(data_num < 1){
		CAM_ERR(CAM_EEPROM,"have no powerup message");
		return TL_E_ERR_PARAM;
	}
	tl_ret = create_node_for_seq(pup_data,data_num);
	if(tl_ret != TL_E_SUCCESS){
		CAM_ERR(CAM_EEPROM,"SEQ == NULL");
		return TL_E_ERR_PARAM;
	}

	n += data_num + 2; //data_num reg_addr reg_data ...
	while(n < pup_size)
	{
		data_num = pup_data[n];
		if(data_num <= 0){
			CAM_ERR(CAM_EEPROM,"data_num 0");
			return TL_E_ERR_PARAM;
		} else if(data_num > 1){
			n += 1;
			addr_temp = pup_data[n];
			n += 1;
			while(data_num > 0){
				i2c_list = list_node_create(
						&cam_eeprom_list_head.list_head_init,1);
				if(i2c_list == NULL)
					return TL_E_ERR_SYSTEM;
				i2c_list->i2c_settings.reg_setting[0].reg_data
					= pup_data[n];
				i2c_list->i2c_settings.reg_setting[0].reg_addr
					= addr_temp;
				if(addr_temp++ == 0xC08E){
					tof_eeprom->gpo_out_stby_value = pup_data[n];
				}
				n += 1;
				data_num--;
			}
		} else {
			n_temp = n;
			reg_array_num = 1;
			while(1)
			{
				n_temp += 3;
				if(n_temp < pup_size){
					if(pup_data[n_temp] != 1){
						break;
					}
					reg_array_num++;
				} else
					break;
			}
			i2c_list = list_node_create(
					&cam_eeprom_list_head.list_head_init,reg_array_num);
			if(i2c_list == NULL)
				return TL_E_ERR_SYSTEM;
			if(reg_array_num == 1){
				i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
			}
			for(i = 0;i < reg_array_num;i++){
				n += 1;
				i2c_list->i2c_settings.reg_setting[i].reg_addr = pup_data[n];
				n += 1;
				i2c_list->i2c_settings.reg_setting[i].reg_data = pup_data[n];
				if(i2c_list->i2c_settings.reg_setting[i].reg_addr == 0xC08E){
					tof_eeprom->gpo_out_stby_value = pup_data[n];
				}
				n += 1;
			}
		}
	}
	/* IMASK */
	if(external_sync == TL_E_TRUE){
		i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_init,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[0].reg_data
			= 0xFFFB;
		i2c_list->i2c_settings.reg_setting[0].reg_addr
			= 0x7C20;
	}
	return tl_ret;
}

static TL_E_RESULT tl_dev_eeprom_create_burst_list(
		struct list_head *eep_list_head,uint16_t addr,
		uint16_t *data,uint16_t num)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	struct    i2c_settings_list *i2c_list;
	int i = 0;

	if(num == 0 || eep_list_head == NULL){
		tl_ret = TL_E_ERR_SYSTEM;
		return tl_ret;
	}

	for(i = 0 ; i < num; i++){
		i2c_list = list_node_create(eep_list_head,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[0].reg_addr
			= addr++;
		i2c_list->i2c_settings.reg_setting[0].reg_data = data[i];
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_eeprom_create_array_burst_list(
		struct list_head *eep_list_head,uint16_t *addr,
		uint16_t *data,uint16_t num)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	struct    i2c_settings_list *i2c_list;
	int i = 0;

	if(num == 0 || eep_list_head == NULL){
		tl_ret = TL_E_ERR_SYSTEM;
		return tl_ret;
	}

	for(i = 0 ; i < num; i++){
		i2c_list = list_node_create(eep_list_head,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[0].reg_addr
			= addr[i];
		i2c_list->i2c_settings.reg_setting[0].reg_data = data[i];
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_eeprom_create_random_list(
		struct list_head *eep_list_head,uint16_t *addr,
		uint16_t *data,uint16_t num)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	struct    i2c_settings_list *i2c_list;
	int i = 0;

	if(num == 0 || eep_list_head == NULL){
		tl_ret = TL_E_ERR_SYSTEM;
		return tl_ret;
	}

	i2c_list = list_node_create(eep_list_head,num);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	if(num == 1)
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;

	for(i = 0;i < num;i++){
		i2c_list->i2c_settings.reg_setting[i].reg_addr
			= addr[i];
		i2c_list->i2c_settings.reg_setting[i].reg_data = data[i];
	}
	return tl_ret;
}


static TL_E_RESULT tl_dev_afe_write_eeprom_shading(tl_dev_rom_shading *p_shading)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t *write_data = NULL;
	uint16_t data_size;
	uint16_t data_cnt = 0;

	/* write shading offset */
	data_size = (uint16_t)TL_EEPROM_ARY_SIZE(p_shading->offset);
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,TL_AFE_SHD_OFFSET_ADDR,
			p_shading->offset, data_size);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* allocate writing data */
	data_size = (uint16_t)( sizeof(p_shading->shd) + sizeof(p_shading->x0) +
			sizeof(p_shading->xpwr) +
	            sizeof(p_shading->y0) + sizeof(p_shading->ypwr) );
	write_data = (uint16_t *)kzalloc((size_t)data_size,GFP_KERNEL);
	if(write_data == NULL){
		return TL_E_ERR_SYSTEM;
	}

	/* data copy (these data are grouped for consecutive addresses) */
	data_cnt = 0;
	*(write_data + data_cnt) = p_shading->shd;
	data_cnt++;
	*(write_data + data_cnt) = p_shading->x0;
	data_cnt++;
	memcpy((void *)(write_data + data_cnt), (const void *)p_shading->xpwr,
			sizeof(p_shading->xpwr));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_shading->xpwr);
	*(write_data + data_cnt) = p_shading->y0;
	data_cnt++;
	memcpy((void *)(write_data + data_cnt), (const void *)p_shading->ypwr,
			sizeof(p_shading->ypwr));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_shading->ypwr);

	/* write data */
	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_SHD_ADDR, write_data, data_cnt);

	/* free writing data */
	kfree((void *)write_data);

	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_dfct(tl_dev_rom_dfct *p_dfct)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t data_size;

	/* write dfct pixel table */
	data_size = (uint16_t)TL_EEPROM_ARY_SIZE(p_dfct->dfct_pix_th_tbl);
	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_DFCT_PIX_TH_TBL_ADDR, p_dfct->dfct_pix_th_tbl, data_size);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write dfct setting */
	data_size = (uint16_t)TL_EEPROM_ARY_SIZE(p_dfct->dfct);
	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_DFCT_ADDR, p_dfct->dfct, data_size);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}


static TL_E_RESULT tl_dev_afe_write_eeprom_timing_mipi(tl_dev_rom_timing_mipi *p_timing_mipi)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t addr[2] = {0};
	uint16_t data[2] = {0};

	addr[0] = TL_AFE_SHP_LOC_ADDR;
	data[0] = p_timing_mipi->shp_loc;
	addr[1] = TL_AFE_SHD_LOC_ADDR;
	data[1] = p_timing_mipi->shd_loc;

	tl_ret = tl_dev_eeprom_create_random_list(&cam_eeprom_list_head.list_head_config,addr,data,2);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_OUTPUT_ADDR,&p_timing_mipi->output,1);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_OUTPUTSEL_ADDR,&p_timing_mipi->output_sel,1);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_VC_ADDR,&p_timing_mipi->vc,1);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_cutout(tl_dev_rom_cutout *p_cutout)
{
	uint16_t addr[1] = {0};
	uint16_t data[1] = {0};

	addr[0] = TL_AFE_GRID3_ADDR;
	data[0] = p_cutout->grid3;

	return tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,1);
}

static TL_E_RESULT tl_dev_afe_write_eeprom_ir(tl_dev_rom_ir *p_ir)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t *write_data = NULL;
	uint16_t data_size;
	uint16_t data_cnt = 0;

	/* allocate writing data */
	data_size = (uint16_t)(sizeof(p_ir->ir1) + sizeof(p_ir->ir_gmm) +
			sizeof(p_ir->ir_gmm_y));
	write_data = (uint16_t *)kzalloc((size_t)data_size,GFP_KERNEL);
	if(write_data == NULL){
		return TL_E_ERR_SYSTEM;
	}

	/* data copy (these data are grouped for consecutive addresses) */
	data_cnt = 0;
	*(write_data + data_cnt) = p_ir->ir1;
	data_cnt++;
	memcpy((void *)(write_data + data_cnt), (const void *)p_ir->ir_gmm,
			sizeof(p_ir->ir_gmm));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_ir->ir_gmm);
	memcpy((void *)(write_data + data_cnt), (const void *)p_ir->ir_gmm_y,
			sizeof(p_ir->ir_gmm_y));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_ir->ir_gmm_y);

	/* write data */
	tl_ret = tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_IR_GAIN_GMM_ADDR, write_data, data_cnt);

	/* free writing data */
	kfree((void *)write_data);

	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_chkr(tl_dev_rom_chkr *p_chkr)
{
	uint16_t write_data[] = {	/* writing data (these data are grouped for consecutive addresses) */
		p_chkr->upprth,
		p_chkr->lwrth,
		p_chkr->start_v,
		p_chkr->start_h,
		p_chkr->size_h,
		p_chkr->upprerr_h,
		p_chkr->upprerr_v,
		p_chkr->lwrerr_h,
		p_chkr->lwrerr_v,
		p_chkr->det_ena,
	};

	return tl_dev_eeprom_create_burst_list(&cam_eeprom_list_head.list_head_config,
			TL_AFE_CHKR_UPPRTH_ADDR, write_data,
			(uint16_t)TL_EEPROM_ARY_SIZE(write_data));
}

static TL_E_RESULT cam_eeprom_list_common_data_offload(
		tl_dev_rom_common *eep_cmn)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;

	INIT_LIST_HEAD(&cam_eeprom_list_head.list_head_config);

	/* write shading */
	tl_ret = tl_dev_afe_write_eeprom_shading(&eep_cmn->shading);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write dfct */
	tl_ret = tl_dev_afe_write_eeprom_dfct(&eep_cmn->dfct);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write timing_mipi */
	tl_ret = tl_dev_afe_write_eeprom_timing_mipi(&eep_cmn->timing_mipi);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write cutout */
	tl_ret = tl_dev_afe_write_eeprom_cutout(&eep_cmn->cutout);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write ir */
	tl_ret = tl_dev_afe_write_eeprom_ir(&eep_cmn->ir);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write chkr */
	tl_ret = tl_dev_afe_write_eeprom_chkr(&eep_cmn->chkr);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_nlr(tl_dev_rom_nlr *p_nlr)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t *write_data = NULL;
	uint16_t data_size;
	uint16_t data_cnt = 0;

	/* allocate writing data */
	data_size = (uint16_t)(sizeof(p_nlr->offset) + sizeof(p_nlr->x0) +
			sizeof(p_nlr->xpwr));
	write_data = (uint16_t *)kzalloc((size_t)data_size,GFP_KERNEL);
	if(write_data == NULL){
		return TL_E_ERR_SYSTEM;
	}

	/* data copy (these data are grouped for consecutive addresses) */
	data_cnt = 0;
	memcpy((void *)(write_data + data_cnt), (const void *)p_nlr->offset,
			sizeof(p_nlr->offset));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_nlr->offset);
	*(write_data + data_cnt) = p_nlr->x0;
	data_cnt++;
	memcpy((void *)(write_data + data_cnt), (const void *)p_nlr->xpwr,
			sizeof(p_nlr->xpwr));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_nlr->xpwr);

	/* write data */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_NLR_OFFSET_ADDR, write_data, data_cnt);

	/* free writing data */
	kfree((void *)write_data);

	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_depth(
		tl_dev_rom_depth *p_depth)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t write_data[] = {	/* writing data (these data are grouped for consecutive addresses) */
		p_depth->depth2,
		p_depth->depth3,
	};

	uint16_t addr[1] = {TL_AFE_ZERO_OFFSET_ADDR};
	uint16_t data[1] = {p_depth->depth0};

	/* write zero point offset */
	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,1);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write slope */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_DEPTH_SLOPE_ADDR, write_data,
			(uint16_t)TL_EEPROM_ARY_SIZE(write_data));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_mode_timing(
		tl_dev_rom_mode_timing *p_timing)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t	num;

	/* write rate_adjust */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_RATE_ADJUST_ADDR, p_timing->rate_adjust,
			(uint16_t)TL_EEPROM_ARY_SIZE(p_timing->rate_adjust));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write align */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_ALIGN_ADDR, p_timing->align,
			(uint16_t)TL_EEPROM_ARY_SIZE(p_timing->align));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write read_size0-1 */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_READ_SIZE0_ADDR, p_timing->read_size, 2U);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write read_size3-7 */
	num = (uint16_t)TL_EEPROM_ARY_SIZE(p_timing->read_size) - 2U;
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,
			TL_AFE_READ_SIZE3_ADDR, &p_timing->read_size[2], num);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write ROI */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config,TL_AFE_ROI_ADDR,
			p_timing->roi, (uint16_t)TL_EEPROM_ARY_SIZE(p_timing->roi));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_grid(tl_dev_rom_grid *p_grid)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;

	/* write grid */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_GRID_ADDR,
			p_grid->grid, (uint16_t)TL_EEPROM_ARY_SIZE(p_grid->grid));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_raw_nr(
		tl_dev_rom_raw_nr *p_raw_nr)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t write_data[] = {	/* writing data (these data are grouped for consecutive addresses) */
		p_raw_nr->med,
		p_raw_nr->sat_th,
	};

	/* write xpwr */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_RAWNR_XPWR_ADDR,
			p_raw_nr->xpwr, (uint16_t)TL_EEPROM_ARY_SIZE(p_raw_nr->xpwr));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write bl_tbl */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_RAWNR_BLTBL_ADDR,
			p_raw_nr->bl_tbl, (uint16_t)TL_EEPROM_ARY_SIZE(p_raw_nr->bl_tbl));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write med, sat_th */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_RAWNR_MED_ADDR,
			write_data, (uint16_t)TL_EEPROM_ARY_SIZE(write_data));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write bk_tbl */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_RAWNR_BKTBL_ADDR,
			p_raw_nr->bk_tbl, (uint16_t)TL_EEPROM_ARY_SIZE(p_raw_nr->bk_tbl));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_coring(tl_dev_rom_coring *p_coring)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t *write_data = NULL;
	uint16_t data_size;
	uint16_t data_cnt = 0;

	/* allocate writing data */
	data_size = (uint16_t)(sizeof(p_coring->cor) + sizeof(p_coring->corb) +
			sizeof(p_coring->corf));
	write_data = (uint16_t *)kzalloc((size_t)data_size,GFP_KERNEL);
	if(write_data == NULL){
		return TL_E_ERR_SYSTEM;
	}

	/* data copy (these data are grouped for consecutive addresses) */
	data_cnt = 0;
	memcpy((void *)(write_data + data_cnt), (const void *)p_coring->cor,
			sizeof(p_coring->cor));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_coring->cor);
	memcpy((void *)(write_data + data_cnt), (const void *)p_coring->corb,
			sizeof(p_coring->corb));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_coring->corb);
	memcpy((void *)(write_data + data_cnt), (const void *)p_coring->corf,
			sizeof(p_coring->corf));
	data_cnt += (uint16_t)TL_EEPROM_ARY_SIZE(p_coring->corf);

	/* write data */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_CORING_ADDR,
			write_data, data_cnt);

	/* free writing data */
	kfree((void *)write_data);

	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_depth_ctrl(
		tl_dev_rom_depth_ctrl *p_d_ctrl)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t addr[2] = {0};
	uint16_t data[2] = {0};

	addr[0] = TL_AFE_DEPTH1_ADDR;
	data[0] = p_d_ctrl->depth1;
	addr[1] = TL_AFE_CONTROL_ADDR;
	data[1] = p_d_ctrl->control;

	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,2);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_pls_mod(
		tl_dev_rom_pls_mod *p_pls_mod)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t addr[1] = {0};
	uint16_t data[1] = {0};

	addr[0] = TL_AFE_PLS_MOD_CTRL_ADDR;
	data[0] = p_pls_mod->control;

	/* write control */
	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,1);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write val */
	tl_ret = tl_dev_eeprom_create_burst_list(
			&cam_eeprom_list_head.list_head_config, TL_AFE_PLS_MOD_VAL_ADDR,
			p_pls_mod->val, (uint16_t)TL_EEPROM_ARY_SIZE(p_pls_mod->val));
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_default_exp(tl_dev_rom_mode *eep_mode,
		tl_transmit_kernel *tl_sensor_setting)
{
	TL_E_RESULT		tl_ret = TL_E_SUCCESS;
	tl_afe_exp_val	*val = NULL;
	uint16_t		ofst;
	uint16_t		i;
	uint16_t        data_cnt = 0;
	uint16_t        addr[15];
	uint16_t        data[15];

	val = &tl_sensor_setting->p_exp;

	/* write read size2 */
	addr[data_cnt] = TL_AFE_READ_SIZE2_ADDR;
	data[data_cnt] = val->read_size2;
	data_cnt++;
	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	data_cnt = 0;

	/* write ccd dummy transfer */
	if(eep_mode->ccd_addr.addr_num > 0U){
		for(i=0; i<eep_mode->ccd_addr.addr_num; i++){
			addr[data_cnt] = eep_mode->ccd_addr.addr[i];
			if(tl_sensor_setting->external_sync == TL_E_FALSE){
				data[data_cnt] = val->ccd_dummy;
			} else {
				data[data_cnt] = eep_mode->exp_prm.vd_duration +
					val->ccd_dummy;
			}
			data_cnt++;
		}
		tl_ret = tl_dev_eeprom_create_random_list(
				&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
		data_cnt = 0;
	}

	/* write chkr start_v */
	addr[data_cnt] = TL_AFE_CHKR_START_V_ADDR;
	data[data_cnt] = val->chkr_start_v;
	data_cnt++;
	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	data_cnt = 0;

	/* write initial offset of VD */
	if(eep_mode->exp_prm.vd_ini_ofst_adr_num > 0U){
		ofst = tl_sensor_setting->p_exp.ini_ofst;
		for(i=0; i<eep_mode->exp_prm.vd_ini_ofst_adr_num; i++){
			addr[data_cnt] = eep_mode->exp_prm.vd_init_ofst_adr[i];
			data[data_cnt] = ofst;
			data_cnt++;
		}
		tl_ret = tl_dev_eeprom_create_random_list(
				&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
		data_cnt = 0;
	}
	/* write idle */
	if(eep_mode->exp_prm.idle_peri_num > 0){
		for(i = 0;i < eep_mode->exp_prm.idle_peri_num; i++){
			addr[data_cnt] = eep_mode->exp_prm.idle_peri_adr[i];
			data[data_cnt] = val->idle;
			data_cnt++;
		}
		tl_ret = tl_dev_eeprom_create_random_list(
				&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
		data_cnt = 0;
	}

	/* write VD length */
	addr[data_cnt] = eep_mode->exp_prm.vd_reg_adr;
	if(tl_sensor_setting->external_sync == TL_E_FALSE){
		data[data_cnt] = eep_mode->exp_prm.vd_duration - 2U;
	} else {
		data[data_cnt] = eep_mode->exp_prm.vd_duration * 2 - 2U;
	}
	data_cnt++;

	tl_ret = tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* when WDR, set exposure of short and lms */
	if((eep_mode->info.tof_mode_flag & TL_AFE_WDR) != 0U){
		/* write exposure(short) */
		if(eep_mode->exp_addr.short_num > 0U){
			for(i=0; i<(uint16_t)eep_mode->exp_addr.short_num; i++){
				addr[data_cnt] = eep_mode->exp_addr.short_addr[i];
				data[data_cnt] = val->short_val;
				data_cnt++;
			}
			tl_ret = tl_dev_eeprom_create_random_list(
					&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
			if(tl_ret != TL_E_SUCCESS){
				return tl_ret;
			}
			data_cnt = 0;
		}
		/* write exposure(lms) */
		if(eep_mode->exp_addr.lms_num > 0U){
			for(i=0; i<(uint16_t)eep_mode->exp_addr.lms_num; i++){
				addr[data_cnt] = eep_mode->exp_addr.lms_addr[i];
				data[data_cnt] = val->lms_val;
				data_cnt++;
			}
			tl_ret = tl_dev_eeprom_create_random_list(
					&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
			if(tl_ret != TL_E_SUCCESS){
				return tl_ret;
			}
			data_cnt = 0;
			}
	}

	/* write exposure(long) */
	if(eep_mode->exp_addr.long_num > 0U){
		for(i=0; i<(uint16_t)eep_mode->exp_addr.long_num; i++){
			addr[data_cnt]  = eep_mode->exp_addr.long_addr[i];
			data[data_cnt]  = val->long_val;
			data_cnt++;
		}
		tl_ret = tl_dev_eeprom_create_random_list(
				&cam_eeprom_list_head.list_head_config,addr,data,data_cnt);
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
		data_cnt = 0;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_write_eeprom_mode(
		tl_dev_rom_mode *eep_mode,tl_transmit_kernel *tl_sensor_setting)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;

	if(eep_mode == NULL){	return TL_E_ERR_PARAM; 	}

	/* write nlr */
	tl_ret = tl_dev_afe_write_eeprom_nlr(&eep_mode->nlr);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write zero point offset & slope */
	tl_ret = tl_dev_afe_write_eeprom_depth(&eep_mode->depth);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write mode timing */
	tl_ret = tl_dev_afe_write_eeprom_mode_timing(&eep_mode->timing);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write grid */
	tl_ret = tl_dev_afe_write_eeprom_grid(&eep_mode->grid);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write RAW-NR */
	tl_ret = tl_dev_afe_write_eeprom_raw_nr(&eep_mode->raw_nr);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write coring */
	tl_ret = tl_dev_afe_write_eeprom_coring(&eep_mode->coring);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write depth control */
	tl_ret = tl_dev_afe_write_eeprom_depth_ctrl(&eep_mode->depth_ctrl);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write pls mod */
	tl_ret = tl_dev_afe_write_eeprom_pls_mod(&eep_mode->pls_mod);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}
	/* write default exposure */
	tl_ret = tl_dev_afe_write_default_exp(eep_mode,tl_sensor_setting);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	return TL_E_SUCCESS;
}

static TL_E_RESULT tl_dev_afe_disable_tal(void)
{
	uint16_t addr[2] = {0};
	uint16_t data[2] = {0};

	addr[0] = TL_AFE_TAL_DETECTOR_EN_ADDR;
	data[0] = 0U;
	addr[1] = TL_AFE_TAL_EN_ADDR;
	data[1] = 0U;

	return tl_dev_eeprom_create_array_burst_list(
			&cam_eeprom_list_head.list_head_config,addr,data,2);
}

static uint16_t tl_dev_afe_calc_ld_setting(uint16_t ld_flag)
{
	uint16_t unLdFlag = 0x0010;

	if ( (ld_flag & 0x0001U) == 1U){
		unLdFlag |= 0x0001U;	//LD1
	}
	if (((ld_flag & 0x0010U) >> 4U) == 1U){
		unLdFlag |= 0x0002U;	//LD2
	}
	if (((ld_flag & 0x0100U) >> 8U) == 1U){
		unLdFlag |= 0x0004U;	//LD3
	}
	if (((ld_flag & 0x1000U) >> 12U) == 1U){
		unLdFlag |= 0x0008U;	//LD4
	}
	return unLdFlag;
}

static TL_E_RESULT tl_dev_afe_write_ld_setting(uint16_t ld_setting)
{
	uint16_t addr[2] = {0};
	uint16_t data[2] = {0};

	addr[0] = TL_AFE_LDPOSBLKOUTEN_ADDR;
	data[0] = ld_setting;
	addr[1] = TL_AFE_LDNEGBLKOUTEN_ADDR;
	data[1] = ld_setting;

	return tl_dev_eeprom_create_array_burst_list(
			&cam_eeprom_list_head.list_head_config,addr,data,2);
}

static TL_E_RESULT tl_dev_afe_set_image_type(TL_E_IMAGE_TYPE image_type)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
//	uint16_t data_size;
	uint16_t data[1] = {0};

	switch(image_type){
	case TL_E_IMAGE_TYPE_VGA_DEPTH_QVGA_IR_BG:
	case TL_E_IMAGE_TYPE_QVGA_DEPTH_IR_BG:
		data[0] = TL_AFE_OUTPUT_TYPE_DEPTH_IRBG_VAL;
		/* set output system to Depth + IR/BG output */
		tl_ret = tl_dev_eeprom_create_burst_list(
				&cam_eeprom_list_head.list_head_config,
				TL_AFE_OUTPUT_TYPE_ADDR,data,1);
		break;

	case TL_E_IMAGE_TYPE_VGA_DEPTH_IR:
	case TL_E_IMAGE_TYPE_VGA_IR_QVGA_DEPTH:
		data[0] = TL_AFE_OUTPUT_TYPE_DEPTH_IR_VAL;
		/* set output system to Depth + IR output */
		tl_ret = tl_dev_eeprom_create_burst_list(
				&cam_eeprom_list_head.list_head_config,
				TL_AFE_OUTPUT_TYPE_ADDR,data,1);
		break;

	case TL_E_IMAGE_TYPE_VGA_IR_BG:
		data[0] = TL_AFE_OUTPUT_TYPE_BG_IR_VAL;
		/* set output system to IR + BG output */
		tl_ret = tl_dev_eeprom_create_burst_list(
				&cam_eeprom_list_head.list_head_config,
				TL_AFE_OUTPUT_TYPE_ADDR,data,1);
		break;
	default:
		tl_ret = TL_E_ERR_SYSTEM;
		break;
	}

	return tl_ret;
}

static TL_E_RESULT tl_dev_afe_set_mode_id(TL_E_MODE mode)
{
	uint16_t addr[1] = {0};
	uint16_t data[1] = {0};

	addr[0] = TL_AFE_MODE_ADDR;
	switch(mode){
	case TL_E_MODE_0:
		data[0] = TL_AFE_MODE0_VAL;
		break;
	case TL_E_MODE_1:
		data[0] = TL_AFE_MODE1_VAL;
		break;
	default:
		return TL_E_ERR_PARAM;
	}

	return tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,1);
}

static TL_E_RESULT tl_dev_afe_enable_tal(uint16_t tal_mode_flag,
		uint16_t ld_setting)
{
	uint16_t unData2 = ld_setting | (uint16_t)(ld_setting << 8U);
	uint16_t addr[2] = {0};
	uint16_t data[2] = {0};

	addr[0] = TL_AFE_TAL_EN_ADDR;
	addr[1] = TL_AFE_TAL_DETECTOR_EN_ADDR;
	data[1] = ld_setting;


	if((tal_mode_flag & 0x0001U) == 0x0001U){
		/* LD rising edge TAL-valid */
	} else {
		/* LD rising edge TAL-invalid */
		unData2 &= 0xFFF0U;
	}
	if((tal_mode_flag & 0x0002U) == 0x0002U){
		/* LD falling edge TAL-valid */
	} else {
		/* LD falling edge TAL-invalid */
		unData2 &= 0xF0FFU;
	}
	if((tal_mode_flag & 0x0004U) == 0x0004U){
		/* SUB rising edge TAL-valid */
	} else {
		/* SUB rising edge TAL-invalid */
		unData2 &= 0x0FFFU;
	}
	if((tal_mode_flag & 0x0008U) == 0x0008U){
		/* SUB falling edge TAL-valid */
	} else {
		/* SUB falling edge TAL-invalid */
		unData2 &= 0xFF0FU;
	}
	data[0] = unData2;

	return tl_dev_eeprom_create_random_list(
			&cam_eeprom_list_head.list_head_config,addr,data,2);
}

static TL_E_RESULT cam_eeprom_list_mode_data_offload(
		tl_dev_eeprom *dev_eeprom,TL_E_MODE mode,
		tl_transmit_kernel *tl_sensor_setting)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	uint16_t tal = 0, ld_setting = 0;

	if(dev_eeprom == NULL){        return TL_E_ERR_PARAM;  }
	if(tl_sensor_setting == NULL){ return TL_E_ERR_PARAM;  }
	if(mode < TL_E_MODE_0){        return TL_E_ERR_PARAM;	}
	if(mode > TL_E_MODE_MAX){      return TL_E_ERR_PARAM;	}

	tal = dev_eeprom->cmn.cam_info.tal_mode;

	if(tal != 0U){
		tl_ret = tl_dev_afe_disable_tal();
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
	}
	tl_ret = tl_dev_afe_write_eeprom_mode(
			&dev_eeprom->mode[mode],tl_sensor_setting);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write LD setting */
	ld_setting = tl_dev_afe_calc_ld_setting(
			dev_eeprom->mode[mode].info.ld_flag);
	tl_ret = tl_dev_afe_write_ld_setting(ld_setting);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* set output image according to the dev->image_type */
	tl_ret =  tl_dev_afe_set_image_type(
			tl_sensor_setting->image_type_output_sel);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	/* write mode value */
	tl_ret = tl_dev_afe_set_mode_id(mode);
	if(tl_ret != TL_E_SUCCESS){
		return tl_ret;
	}

	if(tal != 0U){	/* when use tal, enable tal control */
		tl_ret = tl_dev_afe_enable_tal(tal, ld_setting);
		if(tl_ret != TL_E_SUCCESS){
			return tl_ret;
		}
	}
	return tl_ret;
}

int cam_eeprom_free_list_head(enum cam_eeprom_free cmd)
{
	struct i2c_settings_list *i2c_list = NULL;
	struct i2c_settings_list *i2c_next = NULL;
	switch(cmd){
	case LIST_HEAD_ALL:
	case LIST_HEAD_STREAMOFF:
		if(cam_eeprom_list_head.list_head_streamoff.next != NULL
				|| cam_eeprom_list_head.list_head_streamoff.next
				!= &(cam_eeprom_list_head.list_head_streamoff))
		{
			list_for_each_entry_safe(i2c_list, i2c_next,
				&(cam_eeprom_list_head.list_head_streamoff), list) {
				kfree(i2c_list->i2c_settings.reg_setting);
				list_del(&(i2c_list->list));
				kfree(i2c_list);
			}
		}

	case LIST_HEAD_STREAMON:
		if(cam_eeprom_list_head.list_head_streamon.next != NULL
				|| cam_eeprom_list_head.list_head_streamon.next
				!= &(cam_eeprom_list_head.list_head_streamon))
		{
			list_for_each_entry_safe(i2c_list, i2c_next,
				&(cam_eeprom_list_head.list_head_streamon), list) {
				kfree(i2c_list->i2c_settings.reg_setting);
				list_del(&(i2c_list->list));
				kfree(i2c_list);
			}
		}
	case LIST_HEAD_RESOLUTION:
		if(cam_eeprom_list_head.list_head_config.next != NULL
				|| cam_eeprom_list_head.list_head_config.next
				!= &(cam_eeprom_list_head.list_head_config))
		{
			list_for_each_entry_safe(i2c_list, i2c_next,
				&(cam_eeprom_list_head.list_head_config), list) {
				kfree(i2c_list->i2c_settings.reg_setting);
				list_del(&(i2c_list->list));
				kfree(i2c_list);
			}
		}
	case LIST_HEAD_INITIAL:
		if(cam_eeprom_list_head.list_head_init.next != NULL
				|| cam_eeprom_list_head.list_head_init.next
				!= &(cam_eeprom_list_head.list_head_init))
		{
			list_for_each_entry_safe(i2c_list, i2c_next,
				&(cam_eeprom_list_head.list_head_init), list) {
				kfree(i2c_list->i2c_settings.reg_setting);
				list_del(&(i2c_list->list));
				kfree(i2c_list);
			}
		}
		break;
	default:
		CAM_ERR(CAM_EEPROM,"free list error/no this list");
		break;
	}
	i2c_list = NULL;
	i2c_next = NULL;
	cam_eeprom_list_head.list_state = false;
	return 0;
}

static TL_E_RESULT cam_eeprom_streamon_camera(
		uint16_t *gpo_out_stby_value,
		uint16_t *control_value,
		tl_transmit_kernel *tl_sensor_setting)
{
	int i = 0;
	bool external_sync = tl_sensor_setting->external_sync;
	struct i2c_settings_list *i2c_list;

	*gpo_out_stby_value = (*gpo_out_stby_value & 0xFFBFU) | 0x0040U;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_data = *gpo_out_stby_value;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xc08e;
	i2c_list->i2c_settings.delay = 3;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	*gpo_out_stby_value = (*gpo_out_stby_value & 0xFFFDU) | 0x0002U;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xc08e;
	i2c_list->i2c_settings.reg_setting[i].reg_data = *gpo_out_stby_value;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	*control_value = (*control_value & 0xFFFEU) | 0x0001U;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC300;
	i2c_list->i2c_settings.reg_setting[i].reg_data = *control_value;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

#if 0	/* i2c-debug */
	if(external_sync == TL_E_TRUE){
		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamon,15);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
	} else {
		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamon,16);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
	}

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C0;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x001C;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C3;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x001C;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D7;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0000;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D5;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0002;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4DA;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0001;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4F0;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0000;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0003;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0001;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0000;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0030;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0010;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0000;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC423;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0080;

	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC431;
	i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0080;

	if(external_sync == TL_E_TRUE){
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C20;
		i2c_list->i2c_settings.reg_setting[i++].reg_data = 0xFFEB;
	} else {
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x4001;
		i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0007;
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C22;
		i2c_list->i2c_settings.reg_setting[i++].reg_data = 0x0004;
	}
#else
	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C0;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x001C;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C3;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x001C;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D7;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D5;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0002;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4DA;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0001;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4F0;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0003;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0001;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0030;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0010;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC423;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0080;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamon,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC431;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0080;
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

	if(external_sync == TL_E_TRUE){
		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamon,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C20;
		i2c_list->i2c_settings.reg_setting[i].reg_data = 0xFFEB;
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
	} else {
		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamon,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x4001;
		i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0007;
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;

		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamon,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C22;
		i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0004;
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
	}
	i2c_list->i2c_settings.delay = 100;
#endif
	return TL_E_SUCCESS;
}

static TL_E_RESULT cam_eeprom_streamoff_camera(
		uint16_t *control_value,
		uint16_t *gpo_out_stby_value,
		tl_transmit_kernel *tl_sensor_setting)
{
	uint16_t i = 0;
	bool external_sync = tl_sensor_setting->external_sync;
	struct i2c_settings_list *i2c_list;

	if(external_sync == TL_E_TRUE){
		i2c_list = list_node_create(
				&cam_eeprom_list_head.list_head_streamoff,1);
		if(i2c_list == NULL)
			return TL_E_ERR_SYSTEM;
		i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C20;
		i2c_list->i2c_settings.reg_setting[i].reg_data = 0xFFFB;
#if 0	/* i2c-debug */
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
		i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif
	}
	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x4001;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0004;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0x7C22;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0004;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC431;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0082;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC423;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC426;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0020;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC427;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0002;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C0;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x003C;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4C3;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x003C;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D5;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0003;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4DA;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0000;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4D7;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0001;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC4F0;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0001;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC300;
	*control_value = (*control_value & 0xFFFE);
	i2c_list->i2c_settings.reg_setting[i].reg_data = *control_value;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC08E;
	*gpo_out_stby_value = (*gpo_out_stby_value & 0xFFFD);
	i2c_list->i2c_settings.reg_setting[i].reg_data = *gpo_out_stby_value;
	i2c_list->i2c_settings.delay = 4;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC08E;
	*gpo_out_stby_value = (*gpo_out_stby_value & 0xFFBF);
	i2c_list->i2c_settings.reg_setting[i].reg_data = *gpo_out_stby_value;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	i2c_list = list_node_create(
			&cam_eeprom_list_head.list_head_streamoff,1);
	if(i2c_list == NULL)
		return TL_E_ERR_SYSTEM;
	i2c_list->i2c_settings.reg_setting[i].reg_addr = 0xC025;
	i2c_list->i2c_settings.reg_setting[i].reg_data = 0x0;
#if 0	/* i2c-debug */
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_RANDOM;
#else
	i2c_list->op_code = CAM_SENSOR_I2C_WRITE_BURST;
#endif

	return TL_E_SUCCESS;
}
static TL_E_RESULT cam_eeprom_list_stream_on(
		uint16_t *gpo_out_stby_value,
		uint16_t *control_value,
		tl_transmit_kernel *tl_sensor_setting)
{
	int tl_ret;
	INIT_LIST_HEAD(&cam_eeprom_list_head.list_head_streamon);
	tl_ret = cam_eeprom_streamon_camera(
			gpo_out_stby_value,control_value,tl_sensor_setting);
	return tl_ret;
}

static TL_E_RESULT cam_eeprom_list_stream_off(
		uint16_t *control_value,
		uint16_t *gpo_out_stby_value,
		tl_transmit_kernel *tl_sensor_setting)
{
	int tl_ret;
	INIT_LIST_HEAD(&cam_eeprom_list_head.list_head_streamoff);
	tl_ret = cam_eeprom_streamoff_camera(
			control_value,gpo_out_stby_value,tl_sensor_setting);
	return tl_ret;
}

int cam_eeprom_create_list(tl_dev_eeprom_pup *tof_eeprom,
		tl_transmit_kernel *tl_sensor_setting)
{
	TL_E_RESULT tl_ret = TL_E_SUCCESS;
	if(cam_eeprom_list_head.list_state == true){
		cam_eeprom_free_list_head(LIST_HEAD_ALL);
	}
//init data list
	tl_ret = cam_eeprom_list_init_setting_data(
			tof_eeprom,tl_sensor_setting->external_sync);
	if(tl_ret != TL_E_SUCCESS){
		CAM_ERR(CAM_EEPROM,"error create initial list");
		cam_eeprom_free_list_head(LIST_HEAD_INITIAL);
		return -1;
	}

//common data list
	tl_ret = cam_eeprom_list_common_data_offload(&(tof_eeprom->eeprom.cmn));
	if(tl_ret == TL_E_SUCCESS){
//mode data list
		tl_ret = cam_eeprom_list_mode_data_offload(
				&tof_eeprom->eeprom,tl_sensor_setting->mode,tl_sensor_setting);
		if(tl_ret != TL_E_SUCCESS){
			CAM_ERR(CAM_EEPROM,"error create mode list");
			cam_eeprom_free_list_head(LIST_HEAD_RESOLUTION);
			return -1;
		}
	} else {
		CAM_ERR(CAM_EEPROM,"error create common list");
		cam_eeprom_free_list_head(LIST_HEAD_RESOLUTION);
		return -1;
	}

	tl_ret = cam_eeprom_list_stream_on(&(tof_eeprom->gpo_out_stby_value),
			&tof_eeprom->control_value,tl_sensor_setting);
	if(tl_ret != TL_E_SUCCESS){
		CAM_ERR(CAM_EEPROM,"error create stream on list");
		cam_eeprom_free_list_head(LIST_HEAD_STREAMON);
		return -1;
	}
	tl_ret = cam_eeprom_list_stream_off(
			&tof_eeprom->control_value,
			&tof_eeprom->gpo_out_stby_value,tl_sensor_setting);
	if(tl_ret != TL_E_SUCCESS){
		CAM_ERR(CAM_EEPROM,"error create stream off list");
		cam_eeprom_free_list_head(LIST_HEAD_STREAMOFF);
		return -1;
	}
	cam_eeprom_list_head.list_state = true;
	return 0;
}
/*for tof camera End*/
