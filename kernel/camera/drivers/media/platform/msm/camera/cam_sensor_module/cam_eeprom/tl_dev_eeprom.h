/********************************************************************/
/**
 * @file    tl_dev_eeprom.h
 * @brief   load EEPROM for device control
 * @copyright    Thundersoft Corporation.
 */
/********************************************************************/

#ifndef H_TL_DEV_EEPROM
#define H_TL_DEV_EEPROM

/*--------------------------------------------------------------------
    include headers
--------------------------------------------------------------------*/
#include "tl_dev_sensor_config.h"
#include "cam_eeprom_dev.h"
#include "cam_eeprom_core.h"
#include "cam_sensor_core.h"
/*--------------------------------------------------------------------
    definitions
--------------------------------------------------------------------*/
/**
 * @enum    TL_E_BOOL
 * @brief   Value of boolean.
 */
typedef enum {
  TL_E_FALSE = 0,       /*!< false */
  TL_E_TRUE,            /*!< true */
} TL_E_BOOL;

typedef enum {
    TL_E_MODE_0,        /*!< mode 0 */
    TL_E_MODE_1,        /*!< mode 1 */
    TL_E_MODE_MAX,      /*!< Maximum of range mode */
} TL_E_MODE;


/**
 * @enum     TL_E_RESULT
 * @brief    Return value of functions.
*/
typedef enum {
  TL_E_SUCCESS = 0,      /*!< success. */
  TL_E_ERR_PARAM,        /*!< bad parameter */
  TL_E_ERR_SYSTEM,       /*!< system error */
  TL_E_ERR_STATE,        /*!< state error */
  TL_E_ERR_TIMEOUT,      /*!< timeout error */
  TL_E_ERR_EMPTY,        /*!< buffer empty error */
  TL_E_ERR_NOT_SUPPORT   /*!< not supported function */
} TL_E_RESULT;


/*------------------------*/
/* Common area            */
/*------------------------*/
/* common : camera type information */
typedef struct {
    char        mod_name[32];       /* name of camera module */
    char        afe_name[32];       /* name of AFE */
    char        sns_name[32];       /* name of image sensor */
    char        lns_name[32];       /* name of lens */
} tl_dev_rom_cam_type;

/* common : camera information */
typedef struct {
    uint16_t    sno_l;              /* number of serial lower[15:0] */
    uint16_t    map_ver;            /* eeprom map version */
    uint16_t    sno_u;              /* number of serial upper[31:16] */
    uint16_t    ajust_date;         /* date of adjustment */
    uint16_t    ajust_no;           /* number of adjustment */
    uint16_t    mod_type1;          /* module type 1 */
    uint16_t    mod_type2;          /* module type 2 */
    uint16_t    afe_ptn_id;         /* AFE pattern ID */
    uint16_t    opt_axis_center_h;  /* optical axis center (horizonal) [pixel] */
    uint16_t    opt_axis_center_v;  /* optical axis center (vertical)  [pixel] */
    uint16_t    fov_h;              /* horizontal angle of FOV */
    uint16_t    fov_v;              /* vertical angle of FOV */
    uint16_t    pixel_pitch;        /* hundredfold of pixel pitch[um] */
    uint16_t    focal_len;          /* hundredfold of focal length[mm]  */
    uint16_t    enable_mode;        /* enable mode flag */
    uint16_t    tal_mode;           /* TAL mode flag */
    uint16_t    pup_size;           /* size of Power Up Sequence */
    uint16_t    check_sum;          /* check sum */
} tl_dev_rom_cam_info;

/* common : lens parameters */
typedef struct {
    int64_t     planer_prm[4];      /* planer parameters */
    int64_t     distortion_prm[4];  /* distortion parameters */
} tl_dev_rom_lens;

/* common : Depth shading information */
typedef struct {
    uint16_t    offset[192];        /* shading setting */
    uint16_t    shd;                /* shading setting */
    uint16_t    x0;                 /* shading setting */
    uint16_t    xpwr[4];            /* shading setting */
    uint16_t    y0;                 /* shading setting */
    uint16_t    ypwr[3];            /* shading setting */
} tl_dev_rom_shading;

/* common : dfct */
typedef struct {
    uint16_t    dfct_pix_th_tbl[12];/* dfct look up table value */
    uint16_t    dfct[2];            /* dfct setting */
} tl_dev_rom_dfct;

/* common : AFE timing & mipi setting */
typedef struct {
    uint16_t    shp_loc;            /* SHP edge locations */
    uint16_t    shd_loc;            /* SHD edge location */
    uint16_t    output;             /* AFE output setting */
    uint16_t    output_sel;         /* image output setting(default) */
    uint16_t    vc;                 /* MIPI virtual channel setting */
} tl_dev_rom_timing_mipi;

/* common : cut out */
typedef struct {
    uint16_t    grid3;              /* cut out */
} tl_dev_rom_cutout;

/* common : IR processing */
typedef struct {
    uint16_t    ir1;                /* IR gain */
    uint16_t    ir_gmm[3];          /* IR gamma */
    uint16_t    ir_gmm_y[9];        /* IR gamma */
} tl_dev_rom_ir;

/* common : houndstooth detection */
typedef struct {
    uint16_t    upprth;             /* houndstooth detection setting */
    uint16_t    lwrth;              /* houndstooth detection setting */
    uint16_t    start_v;            /* houndstooth detection setting */
    uint16_t    start_h;            /* houndstooth detection setting */
    uint16_t    size_h;             /* houndstooth detection setting */
    uint16_t    upprerr_h;          /* houndstooth detection setting */
    uint16_t    upprerr_v;          /* houndstooth detection setting */
    uint16_t    lwrerr_h;           /* houndstooth detection setting */
    uint16_t    lwrerr_v;           /* houndstooth detection setting */
    uint16_t    det_ena;            /* houndstooth detection setting */
} tl_dev_rom_chkr;


/* common area informations */
typedef struct {
    tl_dev_rom_cam_type     cam_type;       /* camera type information */
    tl_dev_rom_cam_info     cam_info;       /* camera information  */
    tl_dev_rom_lens         lens;           /* lens parameters */
    tl_dev_rom_shading      shading;        /* Depth shading information */
    tl_dev_rom_dfct         dfct;           /* dfct */
    tl_dev_rom_timing_mipi  timing_mipi;    /* AFE timing & mipi setting */
    tl_dev_rom_cutout       cutout;         /* cut out */
    tl_dev_rom_ir           ir;             /* IR processing */
    tl_dev_rom_chkr         chkr;           /* houndstooth detection */
} tl_dev_rom_common;



/*------------------------*/
/* Mode area              */
/*------------------------*/
/* mode : mode information */
typedef struct {
    uint16_t    tof_mode_flag;      /* TOF mode */
    uint16_t    ld_flag;            /* number of LD */
    uint16_t    range_near_limit;   /* near distance */
    uint16_t    range_far_limit;    /* far distance */
    uint16_t    depth_unit;         /* unit of depth */
    uint16_t    fps;                /* frame rate */
} tl_dev_rom_mode_info;

/* mode : exposure */
typedef struct {
    uint16_t    exp_max;            /* exposure max value */
    uint16_t    exp_default;        /* exposure default value */
} tl_dev_rom_exp;

/* mode : AE parameter */
typedef struct {
    uint16_t    distance;           /* distance */
    uint16_t    lumine_num;         /* number of lumine */
    uint16_t    slope;              /* slope correction */
    int16_t     zofst;              /* zero point offset */
} tl_dev_rom_ae;

/* mode : AFE address of exposure setting */
typedef struct {
    uint8_t     long_num;           /* number of long address */
    uint8_t     short_num;          /* number of short address */
    uint8_t     lms_num;            /* number of lms address */
    uint16_t    long_addr[15];      /* address of long  ... NDR:exposure  WDR:long  */
    uint16_t    short_addr[4];      /* address of short ... NDR:not used  WDR:short */
    uint16_t    lms_addr[4];        /* address of lms   ... NDR:not used  WDR:long-short-1 */
} tl_dev_rom_exp_addr;

/* mode : AFE address of dummy transfer */
typedef struct {
    uint16_t    addr_num;           /* number of address */
    uint16_t    addr[4];            /* address of dummy transfer */
} tl_dev_rom_ccd_addr;

/* mode : Parameters of exposure setting */
typedef struct {
    uint16_t    vd_ini_ofst;        /* HD number from VD to exposure start */
    uint16_t    vd_ini_ofst_adr_num;/* number of AFE address to set "vd_ini_ofst" */
    uint16_t    vd_init_ofst_adr[4];/* address to set "vd_ini_ofst"  */
    uint16_t    ld_pls_duty;        /* TOF pulse duty ratio */
    uint16_t    vd_duration;        /* VD length (HD number within 1 VD) ** this value is orignal VD - 2  */
    uint16_t    vd_reg_adr;         /* addres to set "vd_duration" */
    uint16_t    num_clk_in_hd;      /* Number of clocks in 1 HD */
    uint16_t    beta_num;           /* Number of light emission and exposure */
    uint16_t    num_hd_in_readout;  /* Number of HDs in ReadOut */
    uint16_t    clk_width_u;        /* Reference clock(x1000000) : upper 2byte */
    uint16_t    clk_width_l;        /* Reference clock(x1000000) : lower 2byte */
    uint16_t    tof_emt_period_ofst;/* Number of HDs for adjusting the light emission exposure period */
    uint16_t    tof_seq_ini_ofst;   /* Offset from the positive edge of HD to the start of exposure */
	uint16_t    idle_peri_num;      /* Number of idle*/
	uint16_t    idle_peri_adr[4];   /* Each addr of idle*/
    uint16_t    afe_idle_val[4];
} tl_dev_rom_exp_prm;

/* mode : nonlinear correction */
typedef struct {
    uint16_t    offset[49];         /* nonlinear offset */
    uint16_t    x0;                 /* nonlinear setting */
    uint16_t    xpwr[12];           /* nonlinear setting */
} tl_dev_rom_nlr;

/* mode : depth correction */
typedef struct {
    uint16_t    depth0;              /* zero point offset */
    uint16_t    depth2;              /* slope correction of depth */
    uint16_t    depth3;              /* slope correction of depth */
} tl_dev_rom_depth;

/* mode : AFE timing control */
typedef struct {
    uint16_t    rate_adjust[3];      /* rate adjust */
    uint16_t    align[11];           /* align */
    uint16_t    read_size[7];        /* read size */
    uint16_t    roi[8];              /* ROI */
} tl_dev_rom_mode_timing;

/* mode : grid conversion */
typedef struct {
    uint16_t    grid[3];             /* grid conversion */
} tl_dev_rom_grid;

/* mode : RAW noise reduction */
typedef struct {
    uint16_t    xpwr[3];             /* Raw-NR setting */
    uint16_t    bl_tbl[13];          /* Raw-NR setting */
    uint16_t    med;                 /* Raw-NR setting */
    uint16_t    sat_th;              /* Raw-NR setting */
    uint16_t    bk_tbl[13];          /* Raw-NR setting */
} tl_dev_rom_raw_nr;

/* mode : coring */
typedef struct {
    uint16_t    cor[3];              /* coring setting */
    uint16_t    corb[3];             /* coring setting */
    uint16_t    corf[3];             /* coring setting */
} tl_dev_rom_coring;

/* mode : depth control */
typedef struct {
    uint16_t    depth1;              /* method of TOF */
    uint16_t    control;             /* contorl of depth */
} tl_dev_rom_depth_ctrl;

/* mode : Duty modulation of TOF pulse */
typedef struct {
    uint16_t    control;             /* contorl of TOF pluse */
    uint16_t    val[10];             /* setting value */
} tl_dev_rom_pls_mod;


/* mode area information */
typedef struct {
    tl_dev_rom_mode_info    info;               /* mode information */
    tl_dev_rom_exp          exp;                /* exposure */
    tl_dev_rom_ae           ae[3];              /* AE parameters */
    tl_dev_rom_exp_addr     exp_addr;           /* AFE address of exposure */
    tl_dev_rom_ccd_addr     ccd_addr;           /* AFE address of dummy transfer */
    tl_dev_rom_exp_prm      exp_prm;            /* Parameters of exposure setting */
    tl_dev_rom_nlr          nlr;                /* nonlinear correction */
    tl_dev_rom_depth        depth;              /* depth correction */
    tl_dev_rom_mode_timing  timing;             /* AFE timing control */
    tl_dev_rom_grid         grid;               /* grid conversion */
    tl_dev_rom_raw_nr       raw_nr;             /* RAW noise reduction */
    tl_dev_rom_coring       coring;             /* coring */
    tl_dev_rom_depth_ctrl   depth_ctrl;         /* depth control */
    tl_dev_rom_pls_mod      pls_mod;            /* Duty modulation of TOF pulse */
} tl_dev_rom_mode;

typedef enum {
	TL_E_IMAGE_TYPE_VGA_DEPTH_QVGA_IR_BG = 1,	/*!< VGA depth image and QVGA IR image, BG data */
	TL_E_IMAGE_TYPE_QVGA_DEPTH_IR_BG,			/*!< QVGA depth image, IR image and BG data */
	TL_E_IMAGE_TYPE_VGA_DEPTH_IR,				/*!< VGA depth image and IR image */
	TL_E_IMAGE_TYPE_VGA_IR_QVGA_DEPTH,			/*!< VGA IR image and QVGA depth image */
	TL_E_IMAGE_TYPE_VGA_IR_BG,					/*!< VGA IR image and BG data */
} TL_E_IMAGE_TYPE;

/*----------------------*/
/* For exposure setting */
/*----------------------*/
typedef struct {
	uint16_t	long_val;		/* number of lumine(long) */
	uint16_t	short_val;		/* number of lumine(short) */
	uint16_t	lms_val;		/* number of lumine(lms) */
	uint16_t	read_size2;		/* when to start output */
	uint16_t	ccd_dummy;		/* ccd dummy transfer */
	uint16_t	chkr_start_v;	        /* Vsync timing */
	uint16_t    idle;
	uint16_t    ini_ofst;
} tl_afe_exp_val;

typedef struct {
    uint16_t           afe_revision;       /* afe_revision:0xC0FF*/
} tl_dev_afe;

typedef struct {
    TL_E_MODE           mode;                   /*!< Ranging mode */
	tl_afe_exp_val      p_exp;         				/* afe exposure value */
	TL_E_IMAGE_TYPE     image_type_output_sel;  	/* image output setting(default) */
	TL_E_BOOL 			external_sync;
} tl_transmit_kernel;

/*------------------------*/
/* ALL                    */
/*------------------------*/
/* EEPROM data for device control */
typedef struct {
    tl_dev_rom_common   cmn;                /* common TOF device info */
    tl_dev_rom_mode     mode[TL_E_MODE_MAX];  /* data of each mode */
    tl_dev_afe          afe_val;            /* afe value */
} tl_dev_eeprom;

typedef struct {
	uint16_t           size;
	uint16_t           addr[TL_EEPROM_AFE_ADDR_MAX_SIZE];     /* 0xc0ff(1) idle(4) */
	uint16_t           data[TL_EEPROM_AFE_ADDR_MAX_SIZE];     /* 0xc0ff(1) idle(4) */
}tl_dev_mode_idle_reg;

typedef struct {
	uint16_t           revision_addr;
	uint16_t           revision_data;
	tl_dev_mode_idle_reg idle_reg[TL_E_MODE_MAX];
}tl_dev_afe_reg;

/*------------------------*/
/* ALL                    */
/*------------------------*/
/* EEPROM data for device control */
typedef struct {
    tl_dev_eeprom                   eeprom;
	tl_dev_afe_reg                  afe_reg;
	tl_transmit_kernel              *tl_sensor_setting;
	struct camera_io_master         io_master_info;
	uint16_t                        p_cmn_mode[TL_EEPROM_CMN_AREA_MAX_SIZE];
	 /* Power-Up Sequenece data */
    uint16_t                        pup_data[TL_EEPROM_PUP_AREA_MAX_SIZE];
    uint16_t                        pup_size;
    uint16_t                        gpo_out_stby_value;
    uint16_t                        control_value;
} tl_dev_eeprom_pup;

tl_dev_eeprom_pup* cam_eeprom_module_offload(struct cam_eeprom_ctrl_t *e_ctrl,uint8_t *mapdata,int cmd);

int cam_eeprom_create_list(tl_dev_eeprom_pup *tof_eeprom,tl_transmit_kernel *tl_sensor_setting);

void tl_dev_fill_afe_reg(struct cam_sensor_ctrl_t *s_ctrl);
void tl_eeprom_create_node(void);
void cam_eeprom_free_kobj(void);
extern int cam_res_mgr_gpio_set_value(unsigned int gpio, int value);
bool tof_sensor_check_sync(void);
void cam_eeprom_set_gpio_value(uint16_t gpio,uint16_t delay);
uint16_t cam_sensor_get_fps(void);

struct kobject * check_kobj(void);

#endif /* H_TL_DEV_EEPROM */
