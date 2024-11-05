#include "fc135kw.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

typedef void (*fc_fun_t)(fc135kw_t* fc);

static void init(fc135kw_t* fc);
static void idle(fc135kw_t* fc);
static void precharge(fc135kw_t* fc);
static void running(fc135kw_t* fc);
static void shutdown(fc135kw_t* fc);
static void error(fc135kw_t* fc);
static void deinit(fc135kw_t* fc);

/**
 * @brief logger
 *
 * @param fc fuel cell 物件
 * @param fmt logger 訊息
 * @param ... 可選參數
 */
static void logger(fc135kw_t* fc, const char* fmt, ...)
{
    uint16_t len;
    char buffer[255] = { 0 };

    assert(fc);

    if (fc->config->log)
    {
        va_list args;
        va_start(args, fmt);

        len = snprintf(buffer, 255, "[%s]", fc->config->name);
        len += vsnprintf(&buffer[len], (255 - len), fmt, args);

        va_end(args);

        fc->config->log(&buffer[0], len);
    }
}

/**
 * @brief 啟動停止控制
 *
 * @param fc fuel cell 物件
 * @param enable true:啟動, false:停止
 * @param deinit 當 enable 為 false 時，決定停止後是否為"去初始化"或"回到待機"
 */
static void start(fc135kw_t* fc, bool enable, bool deinit)
{
    assert(fc);

    fc->flag.bits.deinit = deinit;

    if ((fc->status == FC135KW_ERROR) && deinit)
    {
        /* 在 error 時，用清除錯誤碼來執行停機或重啟 */
        fc->flag.bits.clear_error = 1;
    }

    if (fc->flag.bits.start != enable)
    {
        fc->flag.bits.start = enable;
        fc->flag.bits.emerg_stop = 0;
        logger(fc, "%s\r\n", enable ? "start" : "stop");
    }
}

/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief fuel cell 初始化
 *
 * @param fc fuel cell 物件
 * @param config 配置結構體
 */
void fc135kw_init(fc135kw_t* fc, fc135kw_config_t* config)
{
    assert(fc);
    assert(config);

    fc->config = config;
    fc->flag.all = 0;
    fc->power_setup = 0;
    fc->status = FC135KW_INIT;
    fc->status_old = FC135KW_STATUS_MAX;
}

/**
 * @brief fuel cell 去初始化
 *
 * @param fc fuel cell 物件
 *
 * @note fuel cell 最終會進入完全停機狀態(不上電、沒有 ign)
 */
void fc135kw_deinit(fc135kw_t* fc)
{
    start(fc, false, true);
}

/**
 * @brief fuel cell 狀態
 *
 * @param fc fuel cell 物件
 *
 * @return fc135kw_status_t
 *
 * @note 回傳的是本模組定義的狀態，不是 fuel cell
 *       控制器的本身的狀態
 */
fc135kw_status_t fc135kw_status(fc135kw_t* fc)
{
    assert(fc);

    return fc->status;
}

/**
 * @brief fuel cell 任務
 *
 * @param fc fuel cell 物件
 *
 * @note 內建時間計數器，需以 10ms 間隔時間調用
 */
void fc135kw_task(fc135kw_t* fc)
{
    const fc_fun_t fc_function[FC135KW_STATUS_MAX] =
    {
        [FC135KW_INIT] = init,
        [FC135KW_IDLE] = idle,
        [FC135KW_PRECHARGE] = precharge,
        [FC135KW_RUNNING] = running,
        [FC135KW_SHUTDOWN] = shutdown,
        [FC135KW_ERROR] = error,
        [FC135KW_DEINIT] = deinit,
    };

    assert(fc);

    fc_function[fc->status](fc);

    if (fc->config->light_tower)
    {
        fc->config->light_tower(fc->light_tower);
    }

    if (fc->time_count > 1)
    {
        fc->time_count--;
    }

    if (fc->status > FC135KW_INIT)
    {
        if (fc->update_timeout < 200)
        {
            if (++fc->update_timeout >= 200)
            {
                fc135kw_error_stop(fc);
                logger(fc, "data update timeout\r\n");
            }
        }
    }
    else
    {
        fc->update_timeout = 0;
    }

    if (fc->host_timeout < 200)
    {
        if (++fc->host_timeout >= 200)
        {
            fc135kw_error_stop(fc);
            logger(fc, "host connect timeout\r\n");
        }
    }

    if (fc->config->io)
    {
        fc->config->io(FC135KW_IO_24V_POWER, fc->flag.bits.power_on);
        fc->config->io(FC135KW_IO_IGN, fc->flag.bits.ign_key);
    }
}

/**
 * @brief fuel cell 啟動停止控制
 *
 * @param fc fuel cell 物件
 * @param enable true:啟動, false:停止
 */
void fc135kw_start(fc135kw_t* fc, bool enable)
{
    assert(fc);

    if (fc->flag.bits.start != enable)
    {
        start(fc, enable, false);
    }
}

/**
 * @brief fuel cell 清除錯誤
 *
 * @param fc fuel cell 物件
 */
void fc135kw_clear_error(fc135kw_t* fc)
{
    assert(fc);

    if ((fc->status == FC135KW_ERROR) &&
        !fc->flag.bits.clear_error)
    {
        fc->flag.bits.clear_error = 1;
    }
}

/**
 * @brief fuel cell 錯誤停機
 *
 * @param fc fuel cell 物件
 */
void fc135kw_error_stop(fc135kw_t* fc)
{
    assert(fc);

    if (fc->status != FC135KW_ERROR)
    {
        fc135kw_start(fc, false);
        fc->status = FC135KW_ERROR;
        logger(fc, "other error stop\r\n");
    }
}

/**
 * @brief fuel cell 緊急停機
 *
 * @param fc fuel cell 物件
 */
void fc135kw_emerg_stop(fc135kw_t* fc)
{
    assert(fc);

    if (fc->flag.bits.start == 1)
    {
        fc->flag.bits.start = 0;
        fc->flag.bits.emerg_stop = 1;
    }
}

void fc135kw_remove_bubble(fc135kw_t* fc, bool enable)
{
    assert(fc);

    if (fc->status != FC135KW_RUNNING)
    {
        fc->flag.bits.remove_bubble = enable;
    }
}

/**
 * @brief fuel cell 主機連接
 *
 * @param fc fuel cell 物件
 */
void fc135kw_host_connect(fc135kw_t* fc)
{
    assert(fc);

    fc->host_timeout = 0;
}

/**
 * @brief fuel cell 取得要發送給主機的封包
 *
 * @param fc fuel cell 物件
 * @param data 封包數據
 */
void fc135kw_host_packet(fc135kw_t* fc, uint8_t data[8])
{
    assert(fc);
    assert(data);

    fc135kw_data_t* packet = (fc135kw_data_t*)data;

    packet->bytes[0] = fc->status;
    packet->bytes[1] = fc->error;
    packet->bits[2].b0 = fc->flag.bits.start;
    packet->bytes[3] = (uint8_t)(fc->power_setup / 100);
    packet->bytes[4] = 0x00;
    packet->bytes[5] = 0x00;
    packet->bytes[6] = 0x00;

    packet->bits[7].b0 = fc->light_tower.lg;
    packet->bits[7].b1 = fc->light_tower.ly;
    packet->bits[7].b2 = fc->light_tower.lr;
    packet->bits[7].b3 = fc->light_tower.bz;
}

/**
 * @brief fuel cell 指令取得
 *
 * @param fc fuel cell 物件
 * @param type 指令類型
 * @param data 指令內容
 */
void fc135kw_command(fc135kw_t* fc, fc135kw_command_t type, uint8_t data[8])
{
    assert(fc);
    assert(data);

    fc135kw_data_t* command = (fc135kw_data_t*)data;

    switch (type)
    {
        case FC135KW_SET_STATUS:
        {
            command->bits[0].b2 = fc->flag.bits.power_on_sig;
            command->bits[0].b4 = fc->flag.bits.emerg_stop;
            command->bits[0].b5 = fc->flag.bits.start;

            if (fc->status == FC135KW_PRECHARGE)
            {
                fc->power_setup = 2000;
            }

            command->bytes[1] = fc->power_setup & 0xFFU;
            command->bytes[2] = ((fc->power_setup >> 8) & 0xFFU);
            break;
        }
        case FC135KW_DRAIN_VALVE:
        {
            command->lbyte[0] = 0;
            command->lbyte[1] = 0;
            break;
        }
        case FC135KW_REMOVE_BUBBLE:
        {
            command->lbyte[0] = 0;
            command->lbyte[1] = 0;
            command->bits[0].b6 = fc->flag.bits.remove_bubble;
            break;
        }
        default: { break; }
    }
}

/**
 * @brief fuel cell 資料更新
 *
 * @param fc fuel cell 物件
 * @param type 資料類型
 * @param data 資料內容
 */
void fc135kw_update(fc135kw_t* fc, fc135kw_update_t type, uint8_t data[8])
{
    assert(fc);
    assert(data);

    fc135kw_data_t* d = (fc135kw_data_t*)data;

    switch (type)
    {
        case FC135KW_SYS_STATUS:
        {
            if (fc->system_status != d->bytes[0])
            {
                fc->system_status = d->bytes[0];
                logger(fc, "system status = %d\r\n", fc->system_status);
            }
            break;
        }
        case FC135KW_SYS_H2:
        {
            fc->H2_pressure = (d->wbyte[0] / 10);
            fc->H2_src_pressure = (d->wbyte[3] / 10);
            break;
        }
        case FC135KW_SYS_WATER:
        {
            fc->water_temp = ((int8_t)d->bytes[4] - 40);
            fc->water_temp_target = ((int8_t)d->bytes[6] - 40);
            break;
        }
        case FC135KW_SYS_ERROR:
        {
            fc->error = d->bytes[0];
            fc->error_code = (((uint16_t)d->bytes[2] << 8) | d->bytes[1]);
            break;
        }
    }

    fc->update_timeout = 0;
}

/**
 * @brief fuel cell 功率設定
 *
 * @param fc fuel cell 物件
 * @param power_kw 設定目標功率(KW)
 */
void fc135kw_power(fc135kw_t* fc, uint16_t power_kw)
{
    uint16_t temp;

    assert(fc);

    if (power_kw >= 20)
    {
        temp = power_kw * 100;

        if (fc->power_setup != temp)
        {
            fc->power_setup = temp;
            logger(fc, "set power = %d KW\r\n", power_kw);
        }
    }
}

/*--------------------------------------------------------------------------------------------------------*/

/**
 * @brief ems 檢測
 *
 * @param fc fuel cell 物件
 *
 * @return true 發生 ems
 * @return false 未發生 ems
 */
static bool ems(fc135kw_t* fc)
{
    if (fc->error > 1)
    {
        /* FC 控制器通知異常 */
        fc->status = FC135KW_ERROR;
        fc135kw_emerg_stop(fc);
        logger(fc, "ems error, level: %d, code: %d\r\n",
            fc->error, fc->error_code);

        return true;
    }

    if (fc->status == FC135KW_RUNNING)
    {
        /* 檢查氣源壓力 */
        if ((fc->H2_src_pressure < 600) ||
            (fc->H2_src_pressure >= 1000))
        {
            fc->status = FC135KW_ERROR;
            fc135kw_error_stop(fc);
            logger(fc, "H2 source pressure error, pressure: %d kPa\r\n",
                fc->H2_src_pressure);

            return true;
        }
    }

    return false;
}

/**
 * @brief fuel cell 初始化流程
 *
 * @param fc fuel cell 物件
 */
static void init(fc135kw_t* fc)
{
    enum
    {
        DELAY = 0,
        POWER_ON,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = true;
        fc->light_tower.bz = true;
        fc->time_count = 800;
        fc->step = DELAY;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        switch (fc->step)
        {
            default:
            case DELAY:
            {
                /* 延遲執行 */
                if (fc->time_count == 1)
                {
                    fc->time_count = 3000;

                    /* 設定上電 */
                    fc->flag.bits.power_on_sig = 1;
                    fc->flag.bits.power_on = 1;
                    fc->flag.bits.ign_key = 1;
                    fc->step = POWER_ON;
                    logger(fc, "power and ignition on\r\n");
                }
                break;
            }
            case POWER_ON:
            {
                /* 等待 FC 控制器進入狀態 3 */
                if (fc->system_status == 3)
                {
                    fc->status = FC135KW_IDLE;
                }
                else if (fc->time_count == 1)
                {
                    fc->status = FC135KW_ERROR;
                    logger(fc, "power on failed\r\n");
                }
                break;
            }
        }
    }
}

/**
 * @brief fuel cell 待機
 *
 * @param fc fuel cell 物件
 */
static void idle(fc135kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = false;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        if (fc->flag.bits.start)
        {
            fc->status = FC135KW_PRECHARGE;
        }
        else if (fc->flag.bits.deinit)
        {
            fc->status = FC135KW_SHUTDOWN;
        }
    }
}

/**
 * @brief fuel cell 預充流程
 *
 * @param fc fuel cell 物件
 */
static void precharge(fc135kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;
        fc->time_count = 200;
        fc->step = 0;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        if (fc->system_status == 6)
        {
            fc->status = FC135KW_RUNNING;
        }
        // TODO 這裡要改 因為冷啟動溫度爬升至少等 20 min
        //else if (fc->time_count == 1) 
        else if (0)
        {
            fc->status = FC135KW_ERROR;
            logger(fc, "precharge failed\r\n");
        }

        if (!fc->flag.bits.start)
        {
            fc->status = FC135KW_SHUTDOWN;
        }
    }
}

/**
 * @brief fuel cell 運行中
 *
 * @param fc fuel cell 物件
 */
static void running(fc135kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = false;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;
        fc->flag.bits.remove_bubble = 0;

        /* 每秒增加，每次增加不超過 10 KW 讓系統有時間反應 */
        fc->time_count = 100;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        // TODO 功率變載

        if (!fc->flag.bits.start)
        {
            fc->status = FC135KW_SHUTDOWN;
        }
    }
}

/**
 * @brief fuel cell 關機流程
 *
 * @param fc fuel cell 物件
 */
static void shutdown(fc135kw_t* fc)
{
    enum
    {
        STOP = 0,
        POWER_OFF,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;
        fc->time_count = 120000;
        fc->step = STOP;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        switch (fc->step)
        {
            default:
            case STOP:
            {
                if (fc->system_status == 3)
                {
                    fc->time_count = 6000;
                    fc->flag.bits.power_on_sig = 0;
                    fc->step = POWER_OFF;
                    logger(fc, "power signal off\r\n");
                }
                else if (fc->time_count == 1)
                {
                    fc->status = FC135KW_ERROR;
                    logger(fc, "shutdown failed\r\n");
                }
                break;
            }
            case POWER_OFF:
            {
                if (fc->system_status == 9)
                {
                    fc->status = !fc->flag.bits.deinit ? FC135KW_INIT : FC135KW_DEINIT;
                }
                else if (fc->time_count == 1)
                {
                    fc->status = FC135KW_ERROR;
                }
                break;
            }
        }
    }
}

/**
 * @brief fuel cell 狀態
 *
 * @param fc fuel cell 物件
 */
static void error(fc135kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = false;
        fc->light_tower.ly = false;
        fc->light_tower.lr = true;
        fc->light_tower.bz = true;
        fc->time_count = 100;

        logger(fc, "%s\r\n", __func__);
    }

    if (fc->time_count == 1)
    {
        fc->light_tower.lr = !fc->light_tower.lr;
        fc->light_tower.bz = !fc->light_tower.bz;
        fc->time_count = 100;
    }

    if (fc->flag.bits.clear_error)
    {
        logger(fc, "clear error\r\n");

        fc->flag.bits.power_on = 0;
        fc->flag.bits.ign_key = 0;

        fc->error = 0;
        fc->error_code = 0;
        fc->status = !fc->flag.bits.deinit ? FC135KW_INIT : FC135KW_DEINIT;
        fc->flag.bits.clear_error = 0;
    }
}

/**
 * @brief fuel cell 錯誤狀態
 *
 * @param fc fuel cell 物件
 */
static void deinit(fc135kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->flag.bits.power_on = 0;
        fc->flag.bits.ign_key = 0;

        logger(fc, "%s\r\n", __func__);
    }

    /* 等待重啟 */
    if (fc->flag.bits.start)
    {
        fc->status = FC135KW_INIT;
    }
}
