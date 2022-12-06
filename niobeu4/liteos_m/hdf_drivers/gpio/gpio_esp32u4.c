/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "los_task.h"
#include "los_event.h"
#include "gpio_types.h"
#include "gpio_core.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "driver/gpio.h"

#define HDF_GPIO_DEBUG
#define HDF_LOG_TAG "ESP32U4_HDF_GPIO"

struct GpioSourceStr {
    char *pin_index;
    char *pull_mode;
    char *drive_capability;
};

struct GpioSourceValue {
    int pin_index;
    int pull_mode;
    int drive_capability;
    int direction;
};

int g_pin_nums = 0; // hcs文件中定义的pin个数

#ifdef GPIO_NUM_MAX
#undef GPIO_NUM_MAX
#endif
struct GpioSourceValue all_gpio_config[GPIO_NUM_MAX]; // 单个GPIO配置，最大支持40个GPIO配置

#define PLATFORM_CONFIG HCS_NODE(HCS_ROOT, platform)
#define PLATFORM_GPIO_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), gpio_config)
#define HDF_GPIO_FIND_SOURCE(node, name)                                                                              \
    do {                                                                                                              \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) {                                                          \
            struct GpioSourceStr cur_gpio_attr = HCS_ARRAYS(HCS_PROP(node, gpio_attr));                               \
            all_gpio_config[g_pin_nums].pin_index = GetPinIndexFromStr(cur_gpio_attr.pin_index);                      \
            all_gpio_config[g_pin_nums].pull_mode = GetPullModeFromStr(cur_gpio_attr.pull_mode);                      \
            all_gpio_config[g_pin_nums].drive_capability = GetDriveCapabilityFromStr(cur_gpio_attr.drive_capability); \
            HDF_LOGI("----- GPIO Index [%d] Config -----", g_pin_nums);                                               \
            HDF_LOGI("pin_index = %s[%d]", cur_gpio_attr.pin_index, all_gpio_config[g_pin_nums].pin_index);           \
            HDF_LOGI("pull_mode = %s[%d]", cur_gpio_attr.pull_mode, all_gpio_config[g_pin_nums].pull_mode);           \
            HDF_LOGI("drive_capability = %s[%d]", cur_gpio_attr.drive_capability,                                     \
                     all_gpio_config[g_pin_nums].drive_capability);                                                   \
            gpio_reset_pin(all_gpio_config[g_pin_nums].pin_index);                                                    \
            gpio_set_pull_mode(all_gpio_config[g_pin_nums].pin_index, all_gpio_config[g_pin_nums].pull_mode);         \
            gpio_set_drive_capability(all_gpio_config[g_pin_nums].pin_index,                                          \
                                      all_gpio_config[g_pin_nums].drive_capability);                                  \
            g_pin_nums++;                                                                                             \
        }                                                                                                             \
    } while (0)

#define DIR_PARAM_OFFSET 1
#define ESP_INTR_FLAG_DEFAULT 0

static unsigned int lastTickCount = 0;
static struct GpioCntlr g_EspGpioCntlr;

static int32_t Esp32u4GpioWrite(struct GpioCntlr *cntlr, uint16_t local, uint16_t val)
{
    gpio_set_level(all_gpio_config[local].pin_index, val);
    return 0;
}

static int32_t Esp32u4GpioRead(struct GpioCntlr *cntlr, uint16_t local, uint16_t *val)
{
    if (val == NULL)
        return -1;
    *val = gpio_get_level(all_gpio_config[local].pin_index);
    return *val;
}

#define FUNC_2 2
static int32_t Esp32u4GpioSetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t dir)
{
    all_gpio_config[local].direction = dir;
    if (dir == GPIO_DIR_OUT) {
        gpio_iomux_out(all_gpio_config[local].pin_index, FUNC_2, 0);
    }
    gpio_set_direction(all_gpio_config[local].pin_index, dir + DIR_PARAM_OFFSET);
    return 0;
}

static int32_t Esp32u4GpioGetDir(struct GpioCntlr *cntlr, uint16_t local, uint16_t *dir)
{
    *dir = all_gpio_config[local].direction;
    return 0;
}

#define SHAKE_COUNT 10
static void gpio_interrupt_callback_func(VOID *arg)
{
    unsigned int tickCount = osKernelGetTickCount();
    unsigned int count = tickCount - lastTickCount;
    lastTickCount = tickCount;
    if (count > SHAKE_COUNT) { // 消抖
        uint16_t *gpio_num = (uint16_t *)arg;
        GpioCntlrIrqCallback(&g_EspGpioCntlr, *gpio_num); // 调用用户注册的回调函数
    }
    return;
}

static int32_t Esp32u4GpioSetIrq(struct GpioCntlr *cntlr, uint16_t local, uint16_t mode)
{
    int interrupt_type = GPIO_INTR_DISABLE;
    uint32_t *irq_args = (uint32_t *)malloc(sizeof(uint32_t));
    switch (mode) {
        case OSAL_IRQF_TRIGGER_NONE:
            interrupt_type = GPIO_INTR_DISABLE;
            break;
        case OSAL_IRQF_TRIGGER_RISING:
            interrupt_type = GPIO_INTR_POSEDGE;
            break;
        case OSAL_IRQF_TRIGGER_FALLING:
            interrupt_type = GPIO_INTR_NEGEDGE;
            break;
        case OSAL_IRQF_TRIGGER_RISING | OSAL_IRQF_TRIGGER_FALLING:
            interrupt_type = GPIO_INTR_ANYEDGE;
            break;
        case OSAL_IRQF_TRIGGER_HIGH:
            interrupt_type = GPIO_INTR_HIGH_LEVEL;
            break;
        case OSAL_IRQF_TRIGGER_LOW:
            interrupt_type = GPIO_INTR_LOW_LEVEL;
            break;
        default:
            HDF_LOGE("Unknow interrupt mode! (mode = %d)", mode);
            break;
    }
#ifdef HDF_GPIO_DEBUG
    HDF_LOGE("---> Set %d interrupt mode %d!", all_gpio_config[local].pin_index, interrupt_type);
#endif
    gpio_set_intr_type(all_gpio_config[local].pin_index, interrupt_type); // 设置中断类型
    if (irq_args != NULL) {
        *irq_args = (uint32_t)local;
        int ret = gpio_isr_handler_add(all_gpio_config[local].pin_index,
                                       gpio_interrupt_callback_func, irq_args); // 使能中断
        if (ret == -1) {
            HDF_LOGE("GpioDevEnableIrq failed!\n");
        }
    } else {
        HDF_LOGE("irq_args malloc failed!");
        return HDF_FAILURE;
    }
    return 0;
}

static int32_t Esp32u4GpioUnsetIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    gpio_set_intr_type(all_gpio_config[local].pin_index, GPIO_INTR_DISABLE); // 设置取消中断设置
    gpio_isr_handler_remove(all_gpio_config[local].pin_index);
    return 0;
}

static int32_t Esp32u4GpioEnableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    gpio_intr_enable(all_gpio_config[local].pin_index);
    return HDF_SUCCESS;
}

static int32_t Esp32u4GpioDisableIrq(struct GpioCntlr *cntlr, uint16_t local)
{
    gpio_intr_disable(all_gpio_config[local].pin_index);
    return 0;
}

struct GpioMethod g_GpioCntlrMethod = {
    .request = NULL,
    .release = NULL,
    .write = Esp32u4GpioWrite,
    .read = Esp32u4GpioRead,
    .setDir = Esp32u4GpioSetDir,
    .getDir = Esp32u4GpioGetDir,
    .toIrq = NULL,
    .setIrq = Esp32u4GpioSetIrq,
    .unsetIrq = Esp32u4GpioUnsetIrq,
    .enableIrq = Esp32u4GpioEnableIrq,
    .disableIrq = Esp32u4GpioDisableIrq,
};

static int32_t Esp32u4GpioBind(struct HdfDeviceObject *device)
{
    return HDF_SUCCESS;
}
#define NUM_10 10
static int GetPinIndexFromStr(const char *str)
{
    if (str == NULL || strlen(str) < NUM_10) {
        HDF_LOGE("pin index config is illegal!");
        return GPIO_NUM_NC;
    }

    char *ptr_nun = &str[9];
    int level = strlen(ptr_nun); // 获取下标位数
    int pin_index = 0;           // GPIO号
    for (int i = 0; i < level; i++) {
        if (i != 0) {
            pin_index = NUM_10 * pin_index;
        }
        int num = ptr_nun[i] - '0';
        if (num >= NUM_10) { // 字符非法
            HDF_LOGE("pin index config is illegal!");
            return GPIO_NUM_NC;
        }
        pin_index += num;
    }
    return pin_index;
}

static int GetPullModeFromStr(const char *str)
{
    if (!strcmp(str, "GPIO_PULLUP_ONLY")) {
        return GPIO_PULLUP_ONLY;
    } else if (!strcmp(str, "GPIO_PULLDOWN_ONLY")) {
        return GPIO_PULLDOWN_ONLY;
    } else if (!strcmp(str, "GPIO_PULLUP_PULLDOWN")) {
        return GPIO_PULLUP_PULLDOWN;
    }
    return GPIO_FLOATING;
}

static int GetDriveCapabilityFromStr(const char *str)
{
    if (!strcmp(str, "GPIO_DRIVE_CAP_0")) {
        return GPIO_DRIVE_CAP_0;
    } else if (!strcmp(str, "GPIO_DRIVE_CAP_1")) {
        return GPIO_DRIVE_CAP_1;
    } else if (!strcmp(str, "GPIO_DRIVE_CAP_2")) {
        return GPIO_DRIVE_CAP_2;
    } else if (!strcmp(str, "GPIO_DRIVE_CAP_3")) {
        return GPIO_DRIVE_CAP_3;
    } else if (!strcmp(str, "GPIO_DRIVE_CAP_MAX")) {
        return GPIO_DRIVE_CAP_MAX;
    }
    return GPIO_DRIVE_CAP_DEFAULT;
}

static int32_t AttachGpioDevice(struct GpioCntlr *gpioCntlr, struct HdfDeviceObject *device)
{
    int32_t ret;
    if (device == NULL) {
        HDF_LOGE("%s: property is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    HCS_FOREACH_CHILD_VARGS(PLATFORM_GPIO_CONFIG, HDF_GPIO_FIND_SOURCE, device->deviceMatchAttr);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpioCntlr->priv = NULL;
    gpioCntlr->count = g_pin_nums;

    return HDF_SUCCESS;
}

static int32_t Esp32u4GpioInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct GpioCntlr *gpioCntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PlatformDeviceBind(&g_EspGpioCntlr.device, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: bind hdf device failed:%d", __func__, ret);
        return ret;
    }

    gpioCntlr = GpioCntlrFromHdfDev(device);
    if (gpioCntlr == NULL) {
        HDF_LOGE("GpioCntlrFromHdfDev fail\r\n");
        return HDF_DEV_ERR_NO_DEVICE_SERVICE;
    }

    ret = AttachGpioDevice(gpioCntlr, device); /* GpioCntlr add GpioDevice to priv */
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AttachGpioDevice fail\r\n");
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    gpioCntlr->ops = &g_GpioCntlrMethod; /* register callback */
    ret = GpioCntlrAdd(gpioCntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioCntlrAdd fail %d\r\n", gpioCntlr->start);
        return HDF_FAILURE;
    }

    HDF_LOGI("Esp32u4GpioInit success!\r\n");
    return HDF_SUCCESS;
}

static void Esp32u4GpioRelease(struct HdfDeviceObject *device)
{
    printf("========> Func: %s Line:%d\r\n", __FUNCTION__, __LINE__);
}

static const struct HdfDriverEntry GPIO_DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ESP32U4_HDF_GPIO",
    .Bind = Esp32u4GpioBind,
    .Init = Esp32u4GpioInit,
    .Release = Esp32u4GpioRelease,
};

HDF_INIT(GPIO_DriverEntry);