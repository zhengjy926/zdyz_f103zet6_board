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
int app_init(void)
{
    data_mgmt_init();
    loop_impd_init();
    
    return 0;
}

void app_task(void)
{
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
