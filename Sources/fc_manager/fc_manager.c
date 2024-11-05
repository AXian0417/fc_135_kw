#include "fc_manager.h"
#include "stdarg.h"

typedef void (*status_t)(fuel_cell_t* fc);

static struct
{
    uint8_t status_record[FC_STATUS_MAX];

    fuel_cell_t* first;
    fcmngr_config_t* config;    // TODO
} manager;

//TODO 感覺不用
static void logger(const char* fmt, ...)
{
    uint16_t len;
    char buffer[255] = { 0 };

    if (manager.config->log)
    {
        va_list args;
        va_start(args, fmt);

        len = snprintf(buffer, 255, "[%s]", manager.config->name);
        len += vsnprintf(&buffer[len], (255 - len), fmt, args);

        va_end(args);

        manager.config->log(&buffer[0], len);
    }
}

/*------------------------------------------------ private ------------------------------------------------*/

static void host_light_tower(fuel_cell_t* fc, bool lg, bool ly, bool lr, bool bz)
{
    if (fc->host)
    {
        fc->host->light_tower.lg = lg;
        fc->host->light_tower.ly = ly;
        fc->host->light_tower.lr = lr;
        fc->host->light_tower.bz = bz;
    }
}


/* 等待至少一顆 FC 初始化完成 */
/* 開所有 fc 的 power，並將對應的 host 關聯起來 */
static void fc_init(fuel_cell_t* fc)
{
    enum
    {
        HOST_ON,
        FC_ON,
    };

    if (fc->status_old != fc->status)
    {
        fc->status_old = fc->status;
        fc->time_count = 0;
        fc->step = 0;

        host_light_tower(fc, true, true, true, false);
    }

    switch (fc->step)
    {
        case HOST_ON:
        {
            if (fc->host)
            {
                if (fc->host->connect)
                {
                    fc->step = FC_ON;
                }
            }
            else
            {
                fc->step = FC_ON;
            }
            break;
        }
        case FC_ON:
        {
            break;
        }
    }











    // manager.config->init(fc->handle);

    // manager.status_record[FC_INIT]--;
    // manager.status_record[FC_IDLE]++;

    /* 開所有 fc 的 power，並將對應的 host 關聯起來 */
}

/* 等待啟動(only UI) */
static void fc_idle(fuel_cell_t* fc)
{

}

/* 等待至少一顆 FC 預充完成 */
static void fc_precharge(fuel_cell_t* fc)
{

}

/* 至少一顆 FC 正在運轉中 */
static void fc_running(fuel_cell_t* fc)
{

}

/* 所有 FC 執行關機(按鍵) */
/* 單一 FC 執行關機(UI) */
static void fc_shutdown(fuel_cell_t* fc)
{

}


static void fc_error(fuel_cell_t* fc)
{

}

/*------------------------------------------------ public ------------------------------------------------*/

#if 0

bool fcmngr_init(fcmngr_config_t* config)
{
    uint8_t i;

    if (config != 0)
    {
        manager.config = config;

        for (i = 0; i < FC_STATUS_MAX; i++)
        {
            manager.status_record[i] = 0;
        }

        return true;
    }

    return false;
}

bool fcmngr_fc_add(fuel_cell_t* fc)
{
    if ((manager.first != 0) &&
        (fc != 0) &&
        (fc != manager.first))
    {
        fuel_cell_t* p = manager.first;

        while (p->next)
        {
            if (p->next == fc)
            {
                /* 不能重複新增相同的節點 */
                return false;
            }

            p = p->next;
        }

        p->next = fc;
        manager.status_record[FC_INIT]++;

        return true;
    }

    return false;
}

#endif



#if 0

bool fcmngr_init(fuel_cell_t* fc, fcmngr_config_t* config)
{
    uint8_t i;

    if ((manager.first == 0) &&
        (fc != 0) &&
        (config != 0))
    {
        manager.first = fc;
        manager.config = config;

        manager.status_record[FC_INIT] = 1;

        for (i = FC_IDLE; i < FC_STATUS_MAX; i++)
        {
            manager.status_record[i] = 0;
        }

        return true;
    }

    return false;
}

bool fcmngr_fc_add(fuel_cell_t* fc)
{
    if ((manager.first != 0) &&
        (fc != 0) &&
        (fc != manager.first))
    {
        fuel_cell_t* p = manager.first;

        while (p->next)
        {
            if (p->next == fc)
            {
                /* 不能重複新增相同的節點 */
                return false;
            }

            p = p->next;
        }

        p->next = fc;
        manager.status_record[FC_INIT]++;

        return true;
    }

    return false;
}

#endif

void fcmngr_host_receive(fuel_cell_t* fc, uint8_t* data)
{

}



void fcmngr_task(void)
{
    const status_t status[FC_STATUS_MAX] =
    {
        [FC_INIT] = fc_init,
        [FC_IDLE] = fc_idle,
        [FC_PRECHARGE] = fc_precharge,
        [FC_RUNNING] = fc_running,
        [FC_SHUTDOWN] = fc_shutdown,
        [FC_ERROR] = fc_error,
    };

    fuel_cell_t* fc = manager.first;

    /* 單一燃料電池狀態控制 */
    while (fc)
    {
        if (fc->status < FC_STATUS_MAX)
        {
            status[fc->status](fc);
        }

        fc = fc->next;
    }
}
