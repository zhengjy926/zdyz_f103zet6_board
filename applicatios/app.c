/**
  ******************************************************************************
  * @copyright: Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file     : app.c
  * @author   : ZJY
  * @version  : V1.0
  * @date     : 20xx-xx-xx
  * @brief    : xxx
  *                  1.xx
  *                  2.xx
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
/*------------------------------ include --------------------------------------*/
#include "app.h"

#include "custom_proto.h"
#include "data_mgmt.h"
#include "safety.h"
#include "loop_impd.h"
#include "attach_impd.h"

#define  LOG_TAG             "app"
#define  LOG_LVL             4
#include "log.h"

#include <string.h>

/*------------------------------ Macro definition -----------------------------*/


/*------------------------------ typedef definition ---------------------------*/


/*------------------------------ variables prototypes -------------------------*/
static uint8_t mode = 0;
static uint8_t sub_mode = 0;

/*------------------------------ function prototypes --------------------------*/


/*------------------------------ application ----------------------------------*/
void app_handshake(const uint8_t *data, uint16_t len)
{
    uint8_t module_addr = data[0];
    
//    if (module_addr == LOOP_IMPD_MODULE_ADDR) {
//        custom_proto_send_frame(cmd_HandShake, DEVICE_ADDR, LOOP_IMPD_MODULE_ADDR, NULL, 0);
//    } else if (module_addr == ATTACH_IMPD_MODULE_ADDR) {
//        
//        custom_proto_send_frame(cmd_HandShake, DEVICE_ADDR, ATTACH_IMPD_MODULE_ADDR, NULL, 0);
//    } else {
//        custom_proto_send_frame(cmd_HandShake, DEVICE_ADDR, LOOP_IMPD_CALIB_MODULE_ADDR, NULL, 0);
//    }
}

void app_get_sw_version(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_sw_version();
    custom_proto_send_frame(cmd_Get_SoftwareVersion, 0x03, 0x05, (uint8_t*)str, strlen(str)+1);
    LOG_D("SW_VERSION = %s!\r\n", str);
}

void app_set_hw_version(const uint8_t *data, uint16_t len)
{
    int ret = 0;
    uint8_t ack = 0;
    ret = data_mgmt_set_hw_version((const char*)data, len);
    if (!ret) {
        ack = ack_Finish;
    } else {
        ack = ack_Failure_Unknown;
    }
    LOG_D("ack = %d\r\n", ack);
    custom_proto_send_frame(cmd_Set_HardwareVersion, 0x03, 0x05, &ack, 1);
}

void app_get_hw_version(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_hw_version();
    custom_proto_send_frame(cmd_Get_HardwareVersion, 0x03, 0x05, (uint8_t*)str, strlen(str)+1);
    LOG_D("HW_VERSION = %s!\r\n", str);
}

void app_set_serial_num(const uint8_t *data, uint16_t len)
{
    int ret = 0;
    uint8_t ack = 0;
    ret = data_mgmt_set_sn_number((const char*)data, len);
    if (!ret) {
        ack = ack_Finish;
    } else {
        ack = ack_Failure_Unknown;
    }
    LOG_D("ack = %d\r\n", ack);
    custom_proto_send_frame(cmd_Set_SerialNumber, 0x03, 0x05, &ack, 1);
}

void app_get_serial_num(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_sn_number();
    custom_proto_send_frame(cmd_Get_SerialNumber, 0x03, 0x05, (uint8_t*)str, strlen(str)+1);
    LOG_D("SN_NUMBER = %s!\r\n", str);
}

void app_ctrl_soft_reset(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Soft reset!\r\n");
    custom_proto_send_frame(cmd_Ctrl_SoftReset, 0x03, 0x05, &ack, 1);
}

void app_self_check(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Self check!\r\n");
    custom_proto_send_frame(cmd_Ctrl_SelfCheck, 0x03, 0x05, &ack, 1);
}

void app_ctrl_lowpower(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl lowpower!\r\n");
    custom_proto_send_frame(cmd_Ctrl_LowPowerMode, 0x03, 0x05, &ack, 1);
}

void app_ctrl_iap(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl IAP!\r\n");
    custom_proto_send_frame(cmd_Ctrl_IAP, 0x03, 0x05, &ack, 1);
}

void app_ctrl_upload(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl upload!\r\n");
    custom_proto_send_frame(cmd_Ctrl_UploadMode, 0x03, 0x05, &ack, 1);
}

int app_init(void)
{
    data_mgmt_init();
    loop_impd_init();
    custom_proto_init();
    custom_proto_handshake_start();
    
    return 0;
}

void app_task(void)
{
    custom_proto_task();
    safety_task();
    
    switch(mode) {
        case INIT_MODE:
            mode = NOMAL_MODE;
            break;
        case NOMAL_MODE:
            switch (sub_mode) {
                case LOOP_MODE:
//                    if (loop_get_status)
//                        loop_impd_start();
//                    else
//                        loop_impd_stop();
                    break;
                case ATTACH_MODE:
                    break;
                default:
                    break;
            }
            break;
        case CALIBRATION_MODE:
            break;
        default:
            break;
    }
}
/******************************* End Of File ************************************/
