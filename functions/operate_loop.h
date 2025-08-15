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
#define OPERATE_LOOP_FRAME_MAX_LEN          (64)
#define OPERATE_LOOP_FRAME_MIN_LEN          (9)

/************通用命令码************/
//系统相关功能：预留0x00~0x2f
#define dowLoopImpd_HandShake               cmd_HandShake
#define dowLoopImpd_Get_SoftwareVersion     cmd_Get_SoftwareVersion
#define dowLoopImpd_Set_HardwareVersion     cmd_Set_HardwareVersion
#define dowLoopImpd_Get_HardwareVersion     cmd_Get_HardwareVersion
#define dowLoopImpd_Set_SerialNumber        cmd_Set_SerialNumber
#define dowLoopImpd_Get_SerialNumber        cmd_Get_SerialNumber
#define dowLoopImpd_Ctrl_SoftReset          cmd_Ctrl_SoftReset
#define dowLoopImpd_Ctrl_SelfCheck          cmd_Ctrl_SelfCheck
#define dowLoopImpd_Ctrl_LowPowerMode       cmd_Ctrl_LowPowerMode
#define dowLoopImpd_Ctrl_IAP                cmd_Ctrl_IAP
#define dowLoopImpd_Ctrl_UploadMode         cmd_Ctrl_UploadMode /*控制上传模式*/
/*上行通用应答命令*/
#define upLoopImpd_UniversalACK             cmd_up_UniversalACK

/*专用命令*/
#define dowLoopImpd_GET_LOOP_IMPD_VALUE     0x30

/*------------------------------ typedef definition --------------------------*/


/*------------------------------ variable declarations -----------------------*/


/*------------------------------ function declarations -----------------------*/


/******************************* End Of File **********************************/

