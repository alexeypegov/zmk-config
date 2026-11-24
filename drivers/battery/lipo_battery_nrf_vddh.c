/*
 * SPDX-License-Identifier: MIT
 *
 * Battery driver for nice!nano-style boards: reads VDDH like ZMK's
 * zmk,battery-nrf-vddh, but converts voltage to percent differently.
 *
 *  1. Takes several ADC readings per update and keeps the highest, so a
 *     reading that lands on a radio TX peak (voltage sag) is dropped.
 *  2. Smooths the voltage with an exponential moving average.
 *  3. Maps voltage to percent with a LiPo discharge lookup table instead of
 *     ZMK's linear 3.45-4.2 V line.
 *  4. Lets the percentage only fall while on battery and only rise while USB
 *     power is present, so the post-charge surface-charge bounce disappears.
 */

#define DT_DRV_COMPAT lipo_battery_nrf_vddh

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_power.h>

LOG_MODULE_REGISTER(lipo_battery, CONFIG_SENSOR_LOG_LEVEL);

#define VDDHDIV 5
#define EMA_SCALE 16 /* fixed-point scale for the filtered voltage */

static const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));

/* Resting LiPo voltage (mV) -> state of charge (%), highest voltage first. */
static const struct {
    uint16_t mv;
    uint8_t pct;
} lipo_curve[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75}, {3950, 70},
    {3910, 65},  {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45}, {3800, 40}, {3790, 35},
    {3770, 30},  {3750, 25}, {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},  {3400, 0},
};

struct lipo_data {
    struct adc_channel_cfg acc;
    struct adc_sequence as;
    int16_t adc_raw;

    int32_t filtered_mv_x16; /* EMA state, mV * EMA_SCALE */
    uint16_t millivolts;     /* filtered voltage reported on GAUGE_VOLTAGE */
    uint8_t state_of_charge; /* reported percentage */
    bool initialized;
    bool usb_powered;
};

static uint8_t lipo_mv_to_pct(int mv) {
    if (mv >= lipo_curve[0].mv) {
        return 100;
    }

    for (size_t i = 1; i < ARRAY_SIZE(lipo_curve); i++) {
        if (mv >= lipo_curve[i].mv) {
            int hi_mv = lipo_curve[i - 1].mv, lo_mv = lipo_curve[i].mv;
            int hi_pct = lipo_curve[i - 1].pct, lo_pct = lipo_curve[i].pct;

            return lo_pct + (mv - lo_mv) * (hi_pct - lo_pct) / (hi_mv - lo_mv);
        }
    }

    return 0;
}

static bool usb_power_present(void) {
#if NRF_POWER_HAS_USBREG
    return nrf_power_usbregstatus_vbusdet_get(NRF_POWER);
#else
    return false;
#endif
}

static int read_vddh_mv(struct lipo_data *data, int *mv_out) {
    int best_raw = INT32_MIN;

    for (int i = 0; i < CONFIG_LIPO_BATTERY_SAMPLES; i++) {
        int rc = adc_read(adc, &data->as);
        data->as.calibrate = false;

        if (rc != 0) {
            LOG_ERR("Failed to read ADC: %d", rc);
            return rc;
        }

        best_raw = MAX(best_raw, data->adc_raw);
    }

    int32_t val = best_raw;
    int rc = adc_raw_to_millivolts(adc_ref_internal(adc), data->acc.gain, data->as.resolution,
                                   &val);
    if (rc != 0) {
        LOG_ERR("Failed to convert raw ADC to mV: %d", rc);
        return rc;
    }

    *mv_out = val * VDDHDIV;
    return 0;
}

static int lipo_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    if (chan != SENSOR_CHAN_GAUGE_VOLTAGE && chan != SENSOR_CHAN_GAUGE_STATE_OF_CHARGE &&
        chan != SENSOR_CHAN_ALL) {
        return -ENOTSUP;
    }

    struct lipo_data *data = dev->data;
    int mv;
    int rc = read_vddh_mv(data, &mv);
    if (rc != 0) {
        return rc;
    }

    bool usb = usb_power_present();
    bool restart = !data->initialized || usb != data->usb_powered;

    if (restart) {
        /* First reading, or the charger was just plugged in or unplugged:
         * start the filter over from the current voltage. */
        data->filtered_mv_x16 = mv * EMA_SCALE;
    } else {
        data->filtered_mv_x16 +=
            (mv * EMA_SCALE - data->filtered_mv_x16) / (1 << CONFIG_LIPO_BATTERY_EMA_SHIFT);
    }

    data->millivolts = data->filtered_mv_x16 / EMA_SCALE;
    uint8_t pct = lipo_mv_to_pct(data->millivolts);

    if (restart) {
        data->state_of_charge = pct;
    } else if (usb) {
        data->state_of_charge = MAX(data->state_of_charge, pct);
    } else {
        data->state_of_charge = MIN(data->state_of_charge, pct);
    }

    data->initialized = true;
    data->usb_powered = usb;

    LOG_INF("raw %d mV, filtered %d mV => %d%% (reported %d%%, usb %d)", mv, data->millivolts,
            pct, data->state_of_charge, usb);

    return 0;
}

static int lipo_channel_get(const struct device *dev, enum sensor_channel chan,
                            struct sensor_value *val) {
    const struct lipo_data *data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_GAUGE_VOLTAGE:
        val->val1 = data->millivolts / 1000;
        val->val2 = (data->millivolts % 1000) * 1000;
        return 0;
    case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
        val->val1 = data->state_of_charge;
        val->val2 = 0;
        return 0;
    default:
        return -ENOTSUP;
    }
}

static const struct sensor_driver_api lipo_api = {
    .sample_fetch = lipo_sample_fetch,
    .channel_get = lipo_channel_get,
};

static int lipo_init(const struct device *dev) {
    struct lipo_data *data = dev->data;

    if (!device_is_ready(adc)) {
        LOG_ERR("ADC device is not ready %s", adc->name);
        return -ENODEV;
    }

    data->as = (struct adc_sequence){
        .channels = BIT(0),
        .buffer = &data->adc_raw,
        .buffer_size = sizeof(data->adc_raw),
        .oversampling = 4,
        .calibrate = true,
        .resolution = 12,
    };

    data->acc = (struct adc_channel_cfg){
        .gain = ADC_GAIN_1_2,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
        .input_positive = SAADC_CH_PSELN_PSELN_VDDHDIV5,
    };

    return adc_channel_setup(adc, &data->acc);
}

static struct lipo_data lipo_data;

DEVICE_DT_INST_DEFINE(0, lipo_init, NULL, &lipo_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &lipo_api);
