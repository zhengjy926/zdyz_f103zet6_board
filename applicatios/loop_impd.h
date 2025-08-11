/**
  ******************************************************************************
  * @copyright: Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file     : xx.h
  * @author   : ZJY
  * @version  : V1.0
  * @date     : 20xx-xx-xx
  * @brief    : xxx
  *                   1.xx
  *                   2.xx
  *
  * @attention: None
  ******************************************************************************
  * @history  : 
  *      V1.0 : 1.xxx
  *
  *
  *     
  ******************************************************************************
  */
/*------------------------------ include -------------------------------------*/
#include "sys_def.h"

/*------------------------------ Macro definition ----------------------------*/
#define T_ID                        0x01	/* 设备通信ID */

#define T_CtrlToolingCali           0x40	/* 阻抗校准开关 */
#define T_GetImpValue       		0x41	/* 校准时设置实际阻抗值 */
#define T_PrintfImpValue       		0x42	/* 控制串口打印实际阻抗值 */
#define T_ImpCaliStatus			    0x43	/* 校准状态 */
#define T_Update_IMP_AD			    0x44	/* 上传AD值 */


#define T_TransferLenMax            30      /* PC通信最大数据长度 */
#define T_TransferLenMin            5  	    /* PC通信最大数据长度 */

#define dowM_CtrlUploadImp          0x10	    /* 主动上传阻抗信息 */
#define dowM_CtrlImpSwitch          0x20	    /* 阻抗开关 */
#define dowM_CtrlRelaySwitch        0x30	    /* 继电器开关、多区域放电 */
#define dowM_GetRelayState          0x31	    /* 读取继电器开关状态 */

/***************上行命令**********************/
/*下位机主动上发类*/
#define upM_AutoUploadImp           0x50	    /* 自动上传阻抗数据 */

#define M_TransferLenMax            30          /* PC通信最大数据长度 */
#define M_TransferLenMin            5  	        /* PC通信最大数据长度 */

/*------------------------------ typedef definition --------------------------*/


/*------------------------------ variable declarations -----------------------*/


/*------------------------------ function declarations -----------------------*/


/******************************* End Of File **********************************/

