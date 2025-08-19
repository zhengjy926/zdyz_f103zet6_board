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

/*------------------------------ typedef definition --------------------------*/
typedef struct
{
    uint8_t m_Lock;        //模块锁：0 未锁定；1 锁定
    uint8_t m_Link;        //连接状态：0 未连接；1 连接采集板；2 连接传感器
	uint8_t m_LinkMark;    //链接标记
	uint8_t m_OperateMark; //操作标记
	uint8_t m_Status;      //模块状态标记：0 未就绪；1 就绪状态
		
//	uint8_t m_UploadMode;//数据、信息上传模式标记：0 查询模式；1 自动上报模式
//	
//	uint8_t m_Version_SW[13]; /*模块软件版本信息(ASCII码格式共13字节)：
//	                             X.Y.Z.B
//	                             X：2字节；.字符：1字节；Y：2字节；.字符：1字节；Z：2字节；.字符：1字节；B：4字节*/
//	uint8_t m_Version_HW[13]; /*模块硬件版本信息(ASCII码格式共13字节)：
//	                             X.Y.Z.B
//	                             X：2字节；.字符：1字节；Y：2字节；.字符：1字节；Z：2字节；.字符：1字节；B：4字节*/
//	uint8_t m_SerialNumber[12]; /*模块SN码(ASCII码格式共12字节)：*/
	
//	uint8_t m_Event_InitConfig;//初始化配置事件
//	uint8_t m_Event_Upload_ShowInfo;//更新显示信息事件
	
} loop_impd_info_t;

/*------------------------------ variable declarations -----------------------*/


/*------------------------------ function declarations -----------------------*/
int loop_impd_init(void);
void loop_impd_task(void);
/******************************* End Of File **********************************/

