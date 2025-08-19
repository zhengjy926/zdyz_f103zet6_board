/**
  ******************************************************************************
  * @copyright: Copyright To Hangzhou Dinova EP Technology Co.,Ltd
  * @file     : data_manage.c
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
#include "loop_impd.h"
#include "operate_loop.h"
#include "data_mgmt.h"

#define  LOG_TAG             "loop_impd"
#define  LOG_LVL             4
#include "log.h"

#include <string.h>
/*------------------------------ Macro definition -----------------------------*/


/*------------------------------ typedef definition ---------------------------*/


/*------------------------------ variables prototypes -------------------------*/
float RsOhm[2];

static loop_impd_info_t loop_impd_info;

static uint8_t cal_state = 0;  /* 校准状态 */

int impd_ad[150] = {
    138,238,338,436,537,635,733,830,927,1024,1119,1213,1308,1401,1494,
    1586,1677,1769,1858,1948,2038,2127,2215,2301,2388,2474,2559,2644,
    2728,2813,2900
};

float impd_slope[150] = {
    0.003369, 0.002905, 0.002751, 0.002661, 0.002622, 0.002584, 0.002557,
    0.002533 ,0.002515 ,0.002500 ,0.002484 ,0.002468 ,0.002456 ,0.002443 ,
    0.002432 ,0.002420 ,0.002408 ,0.002399 ,0.002387 ,0.002378 ,0.002369 ,
    0.002360 ,0.002351 ,0.002341 ,0.002332 ,0.002323 ,0.002314 ,0.002305 ,
    0.002297 ,0.002289 ,0.002281 
};

/*------------------------------ function prototypes --------------------------*/
static void stimer_callback(void *arg)
{
    operate_loop_send_cmd(dowLoopImpd_HandShake);
}

int loop_impd_init(void)
{
    int ret;
    
    loop_impd_info.m_Lock = 0;
    loop_impd_info.m_Link = 0;
    loop_impd_info.m_LinkMark = 0;
    loop_impd_info.m_OperateMark = 0;
    loop_impd_info.m_Status = 0;
    
    ret = operate_loop_init();
    if (ret != 0) {
        LOG_E("Failed to initialize operate loop: %d\r\n", ret);
        return -1;
    }
    
    stimer_create(&loop_proto.timer, 1000, STIMER_AUTO_RELOAD, stimer_callback, NULL);
    stimer_start(&loop_proto.timer);
    
    loop_impd_info.m_Status = 1;
    
    return 0;
}

uint8_t loop_impd_get_cal_state(void)
{
    return cal_state;
}

void loop_impd_set_cal_state(uint8_t state)
{
    cal_state = state;
}

void loop_impd_handshake(const uint8_t *data, uint16_t len)
{
    if (loop_impd_info.m_Link) {
        operate_loop_send_byte(dowLoopImpd_HandShake, ack_Finish);
        loop_impd_info.m_Link = 1; //标记握手成功
    }
}

void loop_impd_get_sw_version(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_sw_version();
    operate_loop_send_string(cmd_Get_SoftwareVersion, (uint8_t*)str, strlen(str)+1);
    LOG_D("SW_VERSION = %s!\r\n", str);
}

void loop_impd_set_hw_version(const uint8_t *data, uint16_t len)
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
    operate_loop_send_byte(cmd_Set_HardwareVersion, ack);
}

void loop_impd_get_hw_version(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_hw_version();
    operate_loop_send_string(cmd_Get_HardwareVersion, (uint8_t*)str, strlen(str)+1);
    LOG_D("HW_VERSION = %s!\r\n", str);
}

void loop_impd_set_serial_num(const uint8_t *data, uint16_t len)
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
    operate_loop_send_byte(cmd_Set_SerialNumber, ack);
}

void loop_impd_get_serial_num(const uint8_t *data, uint16_t len)
{
    const char *str = data_mgmt_get_sn_number();
    operate_loop_send_string(cmd_Get_SerialNumber, (uint8_t*)str, strlen(str)+1);
    LOG_D("SN_NUMBER = %s!\r\n", str);
}

void loop_impd_ctrl_soft_reset(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Soft reset!\r\n");
    operate_loop_send_byte(cmd_Ctrl_SoftReset, ack);
}

void loop_impd_self_check(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Self check!\r\n");
    operate_loop_send_byte(cmd_Ctrl_SelfCheck, ack);
}

void loop_impd_ctrl_lowpower(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl lowpower!\r\n");
    operate_loop_send_byte(cmd_Ctrl_LowPowerMode, ack);
}

void loop_impd_ctrl_iap(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl IAP!\r\n");
    operate_loop_send_byte(cmd_Ctrl_IAP, ack);
}

void loop_impd_ctrl_upload(const uint8_t *data, uint16_t len)
{
    uint8_t ack = 0;
    LOG_D("Ctrl upload!\r\n");
    operate_loop_send_byte(cmd_Ctrl_UploadMode, ack);
}

void loop_impd_task(void)
{
    loop_msg_t msg;
    
    custom_proto_parser(&loop_proto);
    
    if (operate_loop_get_msg(&msg) == 1) {
        if (loop_impd_info.m_Link) {
            switch(msg.id) {
                case dowLoopImpd_HandShake:	//握手
                    operate_loop_send_byte(dowLoopImpd_HandShake, ack_Finish);
                    break;
                case dowLoopImpd_Get_SoftwareVersion:
                    loop_impd_get_sw_version(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Set_HardwareVersion:
                    loop_impd_set_hw_version(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Get_HardwareVersion:
                    loop_impd_get_hw_version(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Set_SerialNumber:
                    loop_impd_set_serial_num(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Get_SerialNumber:
                    loop_impd_get_serial_num(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_SoftReset:
                    loop_impd_ctrl_soft_reset(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_SelfCheck:
                    loop_impd_self_check(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_LowPowerMode:
                    loop_impd_ctrl_lowpower(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_IAP:
                    loop_impd_ctrl_iap(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_UploadMode:
                    loop_impd_ctrl_upload(msg.buf, msg.len);
                    break;
                case dowLoopImpd_Ctrl_Mode:
                    break;
                default:
                    LOG_D("Unknow command!\r\n");
                    break;
            }
        } else {
            if (msg.id == dowLoopImpd_HandShake) {
                stimer_stop(&loop_proto.timer);
                operate_loop_send_byte(dowLoopImpd_HandShake, ack_Finish);
                loop_impd_info.m_Link = 1; //标记握手成功
                LOG_D("Handshake sucessful!\r\n");
            } else {
                LOG_D("Pelease handshake first!\r\n");
            }
        }
    }
}
/*------------------------------ loop_impdlication ----------------------------------*/

/******************************* End Of File ************************************/
