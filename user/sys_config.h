/**
  ******************************************************************************
  * @file        : sysconfig.h
  * @author      : ZJY
  * @version     : V1.0
  * @date        : 20xx-xx-xx
  * @brief       : 
  * @attention   : None
  ******************************************************************************
  * @history     :
  *         V1.0 : 1.xxx
  *
  *
  ******************************************************************************
  */
#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/
/* 使用的芯片 */
#define SOC_SERIES_STM32F1

#define USING_RTOS                      0

#define SYS_CONFIG_NAME_MAX             8

/* LOG configure define */
#define USING_LOG                       1
#define LOG_GLOBAL_LVL                  4
#define LOG_USING_TIMESTAMP             1
#define LOG_ASSERT_ENABLE               1
#define LOG_BUF_SIZE                    (256)

/* Kernel configure define */
#define USING_HW_ATOMIC

/* Exported typedef ----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported variable prototypes ----------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SYS_CONFIG_H__ */
