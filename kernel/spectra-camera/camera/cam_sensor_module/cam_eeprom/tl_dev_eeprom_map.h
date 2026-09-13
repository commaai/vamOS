/********************************************************************/
/**
 * @file    tl_dev_eeprom_map.h
 * @brief   EEPROM device address mapping
 * @copyright    Thundersoft Corporation.
 */
/********************************************************************/
#ifndef H_TL_DEV_EEPROM_MAP
#define H_TL_DEV_EEPROM_MAP

/*--------------------------------------------------------------------
    include headers
--------------------------------------------------------------------*/

/*--------------------------------------------------------------------
  definitions
  --------------------------------------------------------------------*/
#define TL_EEPROM_SLAVE_ADDR        (0xac)   /* EEPROM slave address */

#define TL_EEPROM_READ_LIMIT        (0x1000U) /* limit of read size */

/* supported EEPROM map vertion */
#define TL_EEPROM_SUPPORT_MAP_VER   (0x31U)   /* v3.1 */

/* supported Virtual Channel */
#define TL_EEPROM_SUPPORT_VC        (0x0044U)


#define TL_EEPROM_ARY_SIZE(ary)     (sizeof((ary)) / sizeof((ary)[0]))

/* module type */
enum {
    TL_E_EEPROM_MOD_TYPE_V4 = 0,    /* V4 */
    TL_E_EEPROM_MOD_TYPE_V4T        /* V4T-EVM */
};



/*---------------------*/
/* EEPROM area mapping */
/*---------------------*/
/* size of common area [byte] */
#define TL_EEPROM_CMN_AREA_SIZE         (912U)
/* size of camera type & info */
#define TL_EEPROM_CMN_CAM_AREA_SIZE     (0x00B0U)

/* size of mode area [byte]   */
#define TL_EEPROM_MODE_AREA_SIZE        (624U)

/* size of Power-Up Sequence area(maximum) [byte]   */
#define TL_EEPROM_PUP_AREA_MAX_SIZE     (4041U)
#define TL_EEPROM_CMN_AREA_MAX_SIZE     (1080U)
#define TL_EEPROM_AFE_ADDR_MAX_SIZE     (5U)


/* start address of common area */
#define TL_EEPROM_CMN_TOP               (0U)
/* start address of common area */
#define TL_EEPROM_MODE_TOP(mode)        (TL_EEPROM_CMN_TOP + TL_EEPROM_CMN_AREA_SIZE + (TL_EEPROM_MODE_AREA_SIZE * (mode)))
/* start address of Power-Up Sequence area */
#define TL_EEPROM_PUP_TOP               (TL_EEPROM_CMN_TOP + TL_EEPROM_CMN_AREA_SIZE + (TL_EEPROM_MODE_AREA_SIZE * TL_E_MODE_MAX))



/* offset for 2byte align */
#define TL_EEPROM_OFFSET(addr)          ((addr) / 2U)
#define TL_EEPROM_OFST_VAL(ptr, addr)   ((ptr)[TL_EEPROM_OFFSET((addr))])



/*---------------------*/
/* common area mapping */
/*---------------------*/
/* offset address from start address of common area */
#define TL_EEPROM_MOD_NAME             (0x0000U)    /* name of camera module */
#define TL_EEPROM_AFE_NAME             (0x0020U)    /* name of AFE */
#define TL_EEPROM_SNS_NAME             (0x0040U)    /* name of image sensor */
#define TL_EEPROM_LNS_NAME             (0x0060U)    /* name of lens */

#define TL_EEPROM_SNO_L                (0x0080U)    /* number of serial lower[15:0] */
#define TL_EEPROM_MAP_VER              (0x0082U)    /* eeprom map version */
#define TL_EEPROM_SNO_U                (0x0084U)    /* number of serial upper[31:16] */
#define TL_EEPROM_AJUST_DATE           (0x0086U)    /* date of adjustment */
#define TL_EEPROM_AJUST_NO             (0x0088U)    /* number of adjustment */
#define TL_EEPROM_MOD_TYPE1            (0x008AU)    /* module type 1 */
#define TL_EEPROM_MOD_TYPE2            (0x008CU)    /* module type 2 */
#define TL_EEPROM_AFE_PTN_ID           (0x008EU)    /* AFE pattern ID */
#define TL_EEPROM_OPT_AXIS_CENTER_H    (0x0090U)    /* optical axis center (horizonal) [pixel] */
#define TL_EEPROM_OPT_AXIS_CENTER_V    (0x0092U)    /* optical axis center (vertical)  [pixel] */
#define TL_EEPROM_FOV_H                (0x0098U)    /* horizontal angle of FOV */
#define TL_EEPROM_FOV_V                (0x009AU)    /* vertical angle of FOV */
#define TL_EEPROM_PIXEL_PITCH          (0x00A0U)    /* hundredfold of pixel pitch[um] */
#define TL_EEPROM_FOCAL_LEN            (0x00A2U)    /* hundredfold of focal length[mm]  */
#define TL_EEPROM_ENABLE_MODE          (0x00A4U)    /* enable mode flag */
#define TL_EEPROM_TAL_MODE             (0x00A8U)    /* TAL mode flag */
#define TL_EEPROM_PUP_SIZE             (0x00AAU)    /* size of Power Up Sequence[byte] */
#define TL_EEPROM_CHECK_SUM            (0x00ACU)    /* check sum */

#define TL_EEPROM_PLANER_PRM           (0x00B0U)    /* planer parameters */
#define TL_EEPROM_DISTORTION_PRM       (0x00D0U)    /* distortion parameters */

#define TL_EEPROM_SHD_OFFSET           (0x0150U)    /* shading setting */
#define TL_EEPROM_SHD                  (0x02D0U)    /* shading setting */
#define TL_EEPROM_SHD_X0               (0x02D2U)    /* shading setting */
#define TL_EEPROM_SHD_XPWR             (0x02D4U)    /* shading setting */
#define TL_EEPROM_SHD_Y0               (0x02DCU)    /* shading setting */
#define TL_EEPROM_SHD_YPWR             (0x02DEU)    /* shading setting */

#define TL_EEPROM_DFCT_PIX_TH_TBL      (0x02F0U)    /* dfct look up table value */
#define TL_EEPROM_DFCT                 (0x0308U)    /* dfct setting */

#define TL_EEPROM_SHP_LOC              (0x030CU)    /* SHP edge locations */
#define TL_EEPROM_SHD_LOC              (0x030EU)    /* SHD edge location */
#define TL_EEPROM_OUTPUT               (0x0310U)    /* AFE output setting */
#define TL_EEPROM_OUTPUT_SEL           (0x0312U)    /* image output setting(default) */
#define TL_EEPROM_VC                   (0x0314U)    /* MIPI virtual channel setting */

#define TL_EEPROM_GRID3                (0x0320U)    /* cut out */

#define TL_EEPROM_IR1                  (0x0322U)    /* IR gain */
#define TL_EEPROM_IR_GMM               (0x0324U)    /* IR gamma */
#define TL_EEPROM_IR_GMM_Y             (0x032AU)    /* IR gamma */

#define TL_EEPROM_UPPRTH               (0x033CU)    /* houndstooth detection setting */
#define TL_EEPROM_LWRTH                (0x033EU)    /* houndstooth detection setting */
#define TL_EEPROM_START_V              (0x0340U)    /* houndstooth detection setting */
#define TL_EEPROM_START_H              (0x0342U)    /* houndstooth detection setting */
#define TL_EEPROM_SIZE_H               (0x0344U)    /* houndstooth detection setting */
#define TL_EEPROM_UPPRERR_H            (0x0346U)    /* houndstooth detection setting */
#define TL_EEPROM_UPPRERR_V            (0x0348U)    /* houndstooth detection setting */
#define TL_EEPROM_LWRERR_H             (0x034AU)    /* houndstooth detection setting */
#define TL_EEPROM_LWRERR_V             (0x034CU)    /* houndstooth detection setting */
#define TL_EEPROM_DET_ENA              (0x034EU)    /* houndstooth detection setting */


/*---------------------*/
/* mode area mapping   */
/*---------------------*/
/* offset address from start address of mode0 area */
#define TL_EEPROM_TOF_MODE_FLAG        (0x0000U)    /* TOF mode */
#define TL_EEPROM_LD_FLAG              (0x0002U)    /* number of LD */
#define TL_EEPROM_RANGE_NEAR_LIMIT     (0x0004U)    /* near distance */
#define TL_EEPROM_RANGE_FAR_LIMIT      (0x0006U)    /* far distance */
#define TL_EEPROM_DEPTH_UNIT           (0x0008U)    /* unit of depth */

#define TL_EEPROM_EXP_MAX              (0x000CU)    /* exposure max value */
#define TL_EEPROM_EXP_DEFAULT          (0x000EU)    /* exposure default value */

#define TL_EEPROM_AE_DISTANCE          (0x0010U)    /* distance */
#define TL_EEPROM_AE_LUMINE_NUM        (0x0012U)    /* number of lumine */
#define TL_EEPROM_AE_SLOPE             (0x0014U)    /* slope correction */
#define TL_EEPROM_AE_ZOFST             (0x0016U)    /* zero point offset */

#define TL_EEPROM_EXP_ADDR_NUM         (0x0040U)    /* number of <long><short><lms> address */
#define TL_EEPROM_EXP_ADDR_LONG        (0x0042U)    /* address of long */
#define TL_EEPROM_EXP_ADDR_SHORT       (0x0060U)    /* address of short */
#define TL_EEPROM_EXP_ADDR_LMS         (0x0068U)    /* address of lms */

#define TL_EEPROM_DMMY_TRNS_NUM        (0x0070U)    /* number of dummy transfer address */
#define TL_EEPROM_DMMY_TRNS_ADR        (0x0072U)    /* address of dummy transfer */

#define TL_EEPROM_VD_INI_OFST          (0x007AU)    /* HD number from VD to exposure start */
#define TL_EEPROM_VD_INI_OFST_ADR_NUM  (0x007CU)    /* number of AFE address to set "VD_INI_OFST" */
#define TL_EEPROM_VD_INIT_OFST_ADR     (0x007EU)    /* address to set "VD_INI_OFST"  */
#define TL_EEPROM_LD_PLS_DUTY          (0x0086U)    /* TOF pulse duty ratio */
#define TL_EEPROM_VD_DURATION          (0x0088U)    /* VD length (HD number within 1 VD) ** this value is orignal VD - 2  */
#define TL_EEPROM_VD_REG_ADR           (0x008AU)    /* addres to set "VD_DURATION" */
#define TL_EEPROM_NUM_CLK_IN_HD        (0x008CU)    /* Number of clocks in 1 HD */
#define TL_EEPROM_BETA_NUM             (0x008EU)    /* Number of light emission and exposure */
#define TL_EEPROM_NUM_HD_IN_READOUT    (0x0090U)    /* Number of HDs in ReadOut */
#define TL_EEPROM_CLK_WIDTH_U          (0x0092U)    /* Reference clock(x1000000) : upper 2byte */
#define TL_EEPROM_CLK_WIDTH_L          (0x0094U)    /* Reference clock(x1000000) : lower 2byte */
#define TL_EEPROM_TOF_EMT_PERIOD_OFST  (0x0096U)    /* Number of HDs for adjusting the light emission exposure period */
#define TL_EEPROM_TOF_SEQ_INI_OFST     (0x0098U)    /* Offset from the positive edge of HD to the start of exposure */

#define TL_EEPROM_NLR_OFFSET           (0x00F0U)    /* nonlinear offset */
#define TL_EEPROM_NLR_X0               (0x0152U)    /* nonlinear setting */
#define TL_EEPROM_NLR_XPWR             (0x0154U)    /* nonlinear setting */

#define TL_EEPROM_DEPTH0               (0x0170U)    /* zero point offset */
#define TL_EEPROM_DEPTH2               (0x0172U)    /* slope correction of depth */
#define TL_EEPROM_DEPTH3               (0x0174U)    /* slope correction of depth */

#define TL_EEPROM_IDLE_PERI_NUM        (0x0176U)    /* */
#define TL_EEPROM_IDLE_PERI_ADR1       (0x0178U)    /* */

#define TL_EEPROM_RATE_ADJUST          (0x0180U)    /* rate adjust */
#define TL_EEPROM_ALIGN                (0x0186U)    /* align */
#define TL_EEPROM_READ_SIZE0           (0x019CU)    /* read size0 */
#define TL_EEPROM_READ_SIZE3           (0x01A0U)    /* read size3 */
#define TL_EEPROM_ROI                  (0x01AAU)    /* ROI */

#define TL_EEPROM_GRID                 (0x01C0U)    /* grid conversion */

#define TL_EEPROM_RAWNR_XPWR           (0x01C6U)    /* Raw-NR setting */
#define TL_EEPROM_RAWNR_BL_TBL         (0x01CCU)    /* Raw-NR setting */
#define TL_EEPROM_RAWNR_MED            (0x01E6U)    /* Raw-NR setting */
#define TL_EEPROM_RAWNR_SAT_TH         (0x01E8U)    /* Raw-NR setting */
#define TL_EEPROM_RAWNR_BK_TBL         (0x01EAU)    /* Raw-NR setting */

#define TL_EEPROM_COR                  (0x0204U)    /* Raw-NR setting */
#define TL_EEPROM_CORB                 (0x020AU)    /* Raw-NR setting */
#define TL_EEPROM_CORF                 (0x0210U)    /* Raw-NR setting */

#define TL_EEPROM_DEPTH1               (0x0218U)    /* method of TOF */
#define TL_EEPROM_CONTROL              (0x021AU)    /* contorl of depth */

#define TL_EEPROM_PLS_MOD_CTRL         (0x021CU)    /* contorl of depth */
#define TL_EEPROM_PLS_MOD_VAL          (0x021EU)    /* contorl of depth */



/*---------------------*/
/* other macro         */
/*---------------------*/
/* size of AE parameters */
#define TL_EEPROM_AE_SIZE              (8U)
/* start addres for AE parameters */
#define TL_EEPROM_AE_OFST(idx, addr)   ((addr) + (TL_EEPROM_AE_SIZE * (idx)))


#endif /* H_TL_DEV_EEPROM_MAP */
