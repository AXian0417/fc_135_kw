#include "toyota60kw.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

typedef void (*fc_fun_t)(tyt60kw_t* fc);

static void init(tyt60kw_t* fc);
static void idle(tyt60kw_t* fc);
static void precharge(tyt60kw_t* fc);
static void running(tyt60kw_t* fc);
static void shutdown(tyt60kw_t* fc);
static void error(tyt60kw_t* fc);
static void deinit(tyt60kw_t* fc);

static void logger(tyt60kw_t* fc, const char* fmt, ...)
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

static void start(tyt60kw_t* fc, bool enable, bool deinit)
{
    assert(fc);

    fc->flag.bits.deinit = deinit;

    if ((fc->status == TYT60KW_ERROR) && deinit)
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

static void cooling(tyt60kw_t* fc)
{
    uint16_t temp;

    assert(fc);

    if (fc->status == TYT60KW_RUNNING)
    {
        /*
            範圍 (55度, 65度) ,轉速(30%, 45%) =  1.5x - 52.5
            轉速上限設 45% 避免電源跟不上
            原公式 -> ((x * (1.5f)) - 52.5f);
        */
        temp = (((uint16_t)fc->w_temp_fco * 3) >> 1) - 52;
        fc->cooling_fan_speed_ps_h = ((temp > 45) ? 45 : (temp < 30) ? 30 : temp);

        /*
            範圍 (20度, 60度) ,轉速(30%, 100%) =  1.75x - 5
            原公式 -> ((x * (1.75f)) - 5.0f);
        */
        temp = (((uint16_t)(fc->pcu_temp * 7) - 20) / 4);
        fc->cooling_fan_speed_ps_l = ((temp > 100) ? 100 : (temp < 30) ? 30 : temp);
    }
    else
    {
        fc->cooling_fan_speed_ps_h = 0;
        fc->cooling_fan_speed_ps_l = 0;
    }
}

/*--------------------------------------------------------------------------------------------------------*/

void tyt60kw_init(tyt60kw_t* fc, tyt60kw_config_t* config)
{
    assert(fc);
    assert(config);

    fc->config = config;
    fc->flag.all = 0;
    fc->power_setup = 0;
    fc->start_command = TYT60KW_OFF;
    fc->status = TYT60KW_INIT;
    fc->status_old = TYT60KW_STATUS_MAX;
}

void tyt60kw_deinit(tyt60kw_t* fc)
{
    start(fc, false, true);
}

// 10ms
void tyt60kw_task(tyt60kw_t* fc)
{
    const fc_fun_t fc_function[TYT60KW_STATUS_MAX] =
    {
        [TYT60KW_INIT] = init,
        [TYT60KW_IDLE] = idle,
        [TYT60KW_PRECHARGE] = precharge,
        [TYT60KW_RUNNING] = running,
        [TYT60KW_SHUTDOWN] = shutdown,
        [TYT60KW_ERROR] = error,
        [TYT60KW_DEINIT] = deinit,
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

    if (fc->flag.bits.ign_key)
    {
        if (fc->update_timeout < 200)
        {
            if (++fc->update_timeout >= 200)
            {
                tyt60kw_error_stop(fc);
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
            tyt60kw_error_stop(fc);
            logger(fc, "host connect timeout\r\n");
        }
    }

    if (++fc->cooling_time >= 100)
    {
        fc->cooling_time = 0;
        cooling(fc);
    }

    if (fc->config->io)
    {
        fc->config->io(TYT60KW_IO_POWER, fc->flag.bits.power_on);
        fc->config->io(TYT60KW_IO_IGN, fc->flag.bits.ign_key);
        fc->config->io(TYT60KW_IO_FAN_H, fc->cooling_fan_speed_ps_h);
        fc->config->io(TYT60KW_IO_FAN_L, fc->cooling_fan_speed_ps_l);
    }
}

tyt60kw_status_t tyt60kw_status(tyt60kw_t* fc)
{
    assert(fc);

    return fc->status;
}

void tyt60kw_start(tyt60kw_t* fc, bool enable)
{
    assert(fc);

    if (fc->flag.bits.start != enable)
    {
        start(fc, enable, false);
    }
}

void tyt60kw_clear_error(tyt60kw_t* fc)
{
    assert(fc);

    if ((fc->status == TYT60KW_ERROR) &&
        !fc->flag.bits.clear_error)
    {
        fc->flag.bits.clear_error = 1;
    }
}

void tyt60kw_error_stop(tyt60kw_t* fc)
{
    assert(fc);

    if (fc->status != TYT60KW_ERROR)
    {
        tyt60kw_start(fc, false);
        fc->status = TYT60KW_ERROR;
        logger(fc, "other error stop\r\n");
    }
}

void tyt60kw_emerg_stop(tyt60kw_t* fc)
{
    assert(fc);

    if (fc->flag.bits.start)
    {
        fc->flag.bits.start = 0;
        fc->flag.bits.emerg_stop = 1;
    }
}

void tyt60kw_host_connect(tyt60kw_t* fc)
{
    assert(fc);

    fc->host_timeout = 0;
}

void tyt60kw_host_packet(tyt60kw_t* fc, uint8_t data[8])
{
    assert(fc);
    assert(data);

    tyt60kw_data_t* packet = (tyt60kw_data_t*)data;

    packet->bytes[0] = fc->status;
    packet->bytes[1] = fc->error;
    packet->bits[2].b0 = fc->flag.bits.start;
    packet->bytes[3] = fc->power_setup;
    packet->bytes[4] = 0x00;
    packet->bytes[5] = 0x00;
    packet->bytes[6] = 0x00;

    packet->bits[7].b0 = fc->light_tower.lg;
    packet->bits[7].b1 = fc->light_tower.ly;
    packet->bits[7].b2 = fc->light_tower.lr;
    packet->bits[7].b3 = fc->light_tower.bz;
}

void tyt60kw_command(tyt60kw_t* fc, tyt60kw_command_t type, uint8_t data[8])
{
    uint16_t temp;

    assert(fc);
    assert(data);

    tyt60kw_data_t* command = (tyt60kw_data_t*)data;

    switch (type)
    {
        case TYT60KW_CMD_04E:
        {
            command->bytes[0] = 0;
            command->bytes[1] = fc->power_max;

            temp = fc->power_setup * 10;

            command->bytes[2] = (uint8_t)((temp >> 8) & 0x0FU);
            command->bits[2].b4 = fc->flag.bits.relay_off;

            command->bytes[3] = (uint8_t)(temp & 0xFFU);
            command->bytes[4] = 0;
            command->bytes[5] = 0;
            command->bytes[6] = ((uint8_t)fc->start_command << 4);
            command->bytes[7] = 0;
            break;
        }
        case TYT60KW_CMD_215:
        {
            command->lbyte[0] = 0;
            command->lbyte[1] = 0;
            break;
        }
        default: { break; }
    }
}

void tyt60kw_update(tyt60kw_t* fc, tyt60kw_update_t type, uint8_t data[8])
{
    assert(fc);
    assert(data);

    tyt60kw_data_t* d = (tyt60kw_data_t*)data;

    switch (type)
    {
        case TYT60KW_UPDATE_050:
        {
            fc->fc_status = (tyt60kw_fc_status_t)((d->bytes[0] & 0x70U) >> 4);
            fc->flag.bits.relay_status = d->bits[0].b7;
            break;
        }
        case TYT60KW_UPDATE_051:
        {
            fc->error = ((d->bytes[5] & 0xC0U) >> 6);
            fc->pcu_voltage = ((d->bytes[6] << 8) | d->bytes[7]) & 0x7FFU;
            break;
        }
        case TYT60KW_UPDATE_222:
        {
            fc->w_temp_fco = d->bytes[0];
            break;
        }
        case TYT60KW_UPDATE_225:
        {
            fc->pcu_temp = d->bytes[1];
            break;
        }
        case TYT60KW_UPDATE_508:
        {
            fc->error_code = d->bytes[7];
            break;
        }

        default: { break; }
    }

    fc->update_timeout = 0;
}

void tyt60kw_power(tyt60kw_t* fc, uint16_t power_kw)
{
    assert(fc);

    if (fc->power_setup != power_kw)
    {
        fc->power_setup = power_kw;
        logger(fc, "set power = %d KW\r\n", power_kw);
    }
}

/*--------------------------------------------------------------------------------------------------------*/

static bool ems(tyt60kw_t* fc)
{
    if (fc->error > 0)
    {
        fc->status = TYT60KW_ERROR;
        logger(fc, "ems error, level: %d\r\n", fc->error);

        return true;
    }

    if (fc->w_temp_fco > 95)
    {
        fc->status = TYT60KW_ERROR;
        logger(fc, "ems error, water temp output too high");

        return true;
    }

    return false;
}

static void init(tyt60kw_t* fc)
{
    enum
    {
        DELAY = 0,
        POWER_ON,
        IGN_ON,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = true;
        fc->light_tower.bz = true;
        fc->time_count = 300;
        fc->step = DELAY;

        logger(fc, "%s\r\n", __func__);
    }

    switch (fc->step)
    {
        default:
        case DELAY:
        {
            if (fc->time_count == 1)
            {
                fc->time_count = 150;
                fc->flag.bits.power_on = 1;
                fc->step = POWER_ON;
                logger(fc, "power on\r\n");
            }
            break;
        }
        case POWER_ON:
        {
            if (fc->time_count == 1)
            {
                fc->time_count = 150;
                fc->flag.bits.ign_key = 1;
                fc->step = IGN_ON;
                logger(fc, "ignition on\r\n");
            }
            break;
        }
        case IGN_ON:
        {
            if (fc->time_count == 1)
            {
                fc->status = TYT60KW_IDLE;
            }
            break;
        }
    }
}

static void idle(tyt60kw_t* fc)
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

    if (fc->flag.bits.start)
    {
        fc->status = TYT60KW_PRECHARGE;
    }
    else if (fc->flag.bits.deinit)
    {
        fc->status = TYT60KW_SHUTDOWN;
    }
}

static void precharge(tyt60kw_t* fc)
{
    enum
    {
        CHECK_PCU_VOLT = 0,
        CHECK_FC_RUN,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;
        fc->step = CHECK_PCU_VOLT;
        fc->time_count = 3000;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        switch (fc->step)
        {
            default:
            case CHECK_PCU_VOLT:
            {
                if (fc->pcu_voltage > 600)
                {
                    logger(fc, "PCU voltage: %d v\r\n", fc->pcu_voltage);
                    fc->start_command = TYT60KW_START;
                    fc->time_count = 3000;
                    fc->step = CHECK_FC_RUN;
                }
                else if (fc->time_count == 1)
                {
                    fc->status = TYT60KW_ERROR;
                    logger(fc, "PCU voltage too low : %d v\r\n", fc->pcu_voltage);
                }
                break;
            }
            case CHECK_FC_RUN:
            {
                if (fc->fc_status == TYT60KW_POWER_SUPPLY)
                {
                    fc->power_max = 70;
                    fc->status = TYT60KW_RUNNING;
                }
                else if (fc->time_count == 1)
                {
                    fc->status = TYT60KW_ERROR;
                    logger(fc, "FC start timeout, now status: %d, error: %d \r\n",
                        fc->fc_status, fc->error);
                }
                break;
            }
        }

        if (!fc->flag.bits.start)
        {
            fc->status = TYT60KW_SHUTDOWN;
        }
    }
}

static void running(tyt60kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = false;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;

        /* 每秒增加，每次增加不超過 10 KW 讓系統有時間反應 */
        fc->time_count = 100;

        logger(fc, "%s\r\n", __func__);
    }

    if (!ems(fc))
    {
        // TODO 功率變載

        if (fc->time_count == 1)
        {
            fc->time_count = 100;
        }

        if (!fc->flag.bits.start)
        {
            fc->status = TYT60KW_SHUTDOWN;
        }
    }
}

static void shutdown(tyt60kw_t* fc)
{
    enum
    {
        LOAD_SHEDDING = 0,
        FC_OFF,
        CHECK_FC_OFF,
        WAIT,
        IGN_OFF,
        POWER_OFF,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = true;
        fc->light_tower.ly = true;
        fc->light_tower.lr = false;
        fc->light_tower.bz = false;
        fc->step = LOAD_SHEDDING;
        fc->time_count = 1;

        logger(fc, "%s\r\n", __func__);
    }

    switch (fc->step)
    {
        default:
        case LOAD_SHEDDING:
        {
            if (fc->power_setup > 0)
            {
                if (fc->time_count == 1)
                {
                    fc->power_setup -= (fc->power_setup >= 5) ? 5 : fc->power_setup;
                    fc->time_count = 150;
                    logger(fc, "load shedding, power: %d \r\n", fc->power_setup);
                }
            }
            else
            {
                fc->step = FC_OFF;
            }
            break;
        }
        case FC_OFF:
        {
            fc->power_max = 0;
            fc->start_command = TYT60KW_STOP;
            fc->time_count = 12000;
            fc->step = CHECK_FC_OFF;
            break;
        }
        case CHECK_FC_OFF:
        {
            if ((fc->fc_status == TYT60KW_STOPPED) ||
                (fc->fc_status == TYT60KW_STOP_FINISH))
            {
                /* relay 狀態為 on，請求控制器關閉(發送 1) */
                fc->flag.bits.relay_off = fc->flag.bits.relay_status;
                logger(fc, "relay off = %d\r\n", fc->flag.bits.relay_off);

                fc->start_command = TYT60KW_OFF;
                fc->time_count = 1000;
                fc->step = WAIT;
            }
            else if (fc->time_count == 1)
            {
                fc->status = TYT60KW_ERROR;
                logger(fc, "FC shutdown timeout, now status: %d \r\n", fc->fc_status);
            }
            break;
        }
        case WAIT:
        {
            if ((fc->fc_status == TYT60KW_STOPPED) ||
                (fc->time_count == 1))
            {
                fc->time_count = 450;
                fc->flag.bits.ign_key = 0;
                fc->flag.bits.relay_off = 0;
                fc->step = IGN_OFF;
                logger(fc, "ignition off\r\n");
            }
            break;
        }
        case IGN_OFF:
        {
            if (fc->time_count == 1)
            {
                if (fc->flag.bits.deinit)
                {
                    fc->time_count = 150;
                    fc->flag.bits.power_on = 0;
                    fc->step = POWER_OFF;
                    logger(fc, "power off\r\n");
                }
                else
                {
                    fc->status = TYT60KW_INIT;
                }
            }
            break;
        }
        case POWER_OFF:
        {
            if (fc->time_count == 1)
            {
                fc->status = TYT60KW_DEINIT;
            }
            break;
        }
    }
}

static void error(tyt60kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->light_tower.lg = false;
        fc->light_tower.ly = false;
        fc->light_tower.lr = true;
        fc->light_tower.bz = true;
        fc->start_command = TYT60KW_OFF;
        fc->flag.bits.relay_off = fc->flag.bits.relay_status;

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
        fc->update_timeout = 0;
        fc->status = !fc->flag.bits.deinit ? TYT60KW_INIT : TYT60KW_DEINIT;
        fc->flag.bits.clear_error = 0;
    }
}

static void deinit(tyt60kw_t* fc)
{
    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;

        logger(fc, "%s\r\n", __func__);
    }

    /* 等待重啟 */
    if (fc->flag.bits.start)
    {
        fc->status = TYT60KW_INIT;
    }
}
