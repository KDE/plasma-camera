// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings.h"

Settings::Settings() {}

Settings::~Settings() {}

void Settings::load(const libcamera::ControlInfoMap &infoMap)
{
    for (const auto &[controlId, controlInfo] : infoMap)
    {
        qDebug()
            << "\t" << controlId->name() << ":"
            << "min:" << controlInfo.min().toString()
            << " max:" << controlInfo.max().toString()
            << " default:" << controlInfo.def().toString();

        switch (controlId->id())
        {
        case libcamera::controls::AE_ENABLE:
            ae_enable_available = true;
            ae_enable_default = controlInfo.def().get<bool>();
            ae_enable = ae_enable_default;
            break;

        case libcamera::controls::EXPOSURE_VALUE:
            manual_exposure_value_available = true;
            exposure_value_default = controlInfo.def().get<float>();
            exposure_value = exposure_value_default;
            exposure_value_min = controlInfo.min().get<float>();
            exposure_value_max = controlInfo.max().get<float>();
            break;

        case libcamera::controls::AF_WINDOWS:
            // TODO: should we check for AF_METERING instead?
            // TODO: how to convert Span<const Rectangle> to QSize?
            af_window_available = true;
            break;

        case libcamera::controls::AWB_MODE:
            // TODO: AwbModeEnum
            wb_mode_available = true;
            wb_mode_default = controlInfo.def().get<int32_t>();
            wb_mode = wb_mode_default;
            break;

        case libcamera::controls::COLOUR_TEMPERATURE:
            // TODO: colour temperature min and max?
            wb_temp_available = true;
            wb_temp_default = controlInfo.def().get<int32_t>();
            wb_temp = wb_temp_default;
            // wb_temp_min = controlInfo.min().get<int32_t>();
            // wb_temp_max = controlInfo.max().get<int32_t>();
            break;

        case libcamera::controls::EXPOSURE_TIME:
            manual_exposure_time_available = true;
            exposure_time_default = controlInfo.def().get<int32_t>();
            exposure_time = exposure_time_default;
            exposure_time_min = controlInfo.min().get<int32_t>();
            exposure_time_max = controlInfo.max().get<int32_t>();
            break;

        case libcamera::controls::ANALOGUE_GAIN:
            manual_analogue_gain_available = true;
            analogue_gain_default = controlInfo.def().get<float>();
            analogue_gain = analogue_gain_default;
            analogue_gain_min = controlInfo.min().get<float>();
            analogue_gain_max = controlInfo.max().get<float>();
            break;

        case libcamera::controls::CONTRAST:
            manual_contrast_available = true;
            contrast_default = controlInfo.def().get<float>();
            contrast = contrast_default;
            contrast_min = controlInfo.min().get<float>();
            contrast_max = controlInfo.max().get<float>();
            break;

        case libcamera::controls::SATURATION:
            manual_saturation_available = true;
            saturation_default = controlInfo.def().get<float>();
            saturation = saturation_default;
            saturation_min = controlInfo.min().get<float>();
            saturation_max = controlInfo.max().get<float>();
            break;

        case libcamera::controls::HDR_MODE:
            // break;

        case libcamera::controls::SCALER_CROP:
            break;

        default:
            qInfo() << "\t Unknown control: " << controlId->name() << " id: " << controlId->id();
            break;
        }
    }
}

void Settings::set(libcamera::ControlList &controlMap)
{
    // TODO: use the settings to modify the ControlList
    //  - add option to force writing default values to the controlMap
}

void Settings::reset()
{
    unSetAeEnable();
    unSetExposureValue();
    unSetAfWindow();
    unSetWb();
    unSetContrast();
    unSetSaturation();
}


// Auto Exposure (AE) Settings
bool Settings::canSetAeEnable() const
{
    return ae_enable_available;
}

bool Settings::trySetAeEnable(const bool enable)
{
    if (!canSetAeEnable())
    {
        return false;
    }

    if (ae_enable != enable)
    {
        ae_enable_use_default = false;
        ae_enable = enable;

        // if set to true, then set both exposure time and analogue gain to auto
        if (ae_enable)
        {
            unSetExposureTime();
            unSetGain();
        }
    }

    return true;
}

void Settings::unSetAeEnable()
{
    ae_enable_use_default = true;
}

bool Settings::getAeEnable() const
{
    if (ae_enable_use_default)
    {
        return ae_enable_default;
    }

    return ae_enable;
}


bool Settings::canSetExposureValue() const
{
    return ae_enable && manual_exposure_value_available;
}

bool Settings::trySetExposureValue(const float exposureValue)
{
    if (!canSetExposureValue() ||
        exposureValue < exposure_value_min ||
        exposureValue > exposure_value_max)
    {
        return false;
    }

    if (exposure_value != exposureValue)
    {
        manual_exposure_value = true;
        exposure_value = exposureValue;
    }

    return true;
}

bool Settings::isSetExposureValue() const
{
    return manual_exposure_value;
}

void Settings::unSetExposureValue()
{
    manual_exposure_value = false;
}

float Settings::getExposureValue() const
{
    if (!isSetExposureValue())
    {
        return exposure_value_default;
    }

    return exposure_value;
}

float Settings::minExposureValue() const
{
    return exposure_value_min;
}

float Settings::maxExposureValue() const
{
    return exposure_value_max;
}


// Auto Focus (AF) Settings
bool Settings::canSetAfWindow() const
{
    return af_window_available;
}

bool Settings::trySetAfWindow(const QSize window)
{
    if (!canSetAfWindow())
    {
        return false;
    }

    if (af_window_target != window)
    {
        af_window_use_default = false;
        af_window = true;
        af_window_target = window;
    }

    return true;
}

void Settings::unSetAfWindow()
{
    af_window_use_default = true;
    af_window = false;
}

QSize Settings::getAfWindow() const
{
    if (af_window_use_default)
    {
        return QSize();
    }

    return af_window_target;
}


// White Balance (WB) Settings
/*
 * - when we set WB with trySetWbMode or trySetWBTemp we will automatically change wb_auto appropriately
 * - unSetWb, unSetWbMode, unSetWbTemp all do the same thing -> set back to auto WB
 */
void Settings::unSetWb()
{
    wb_auto = true;
    wb_auto_use_default = true;
}

bool Settings::wbMode() const
{
    return wb_auto;
}

bool Settings::wbTemp() const
{
    return !wb_auto;
}

bool Settings::canSetWbMode() const
{
    return wb_mode_available;
}

bool Settings::trySetWbMode(const int wbMode)
{
    if (!canSetWbMode())
    {
        return false;
    }

    if (wb_mode != wbMode)
    {
        wb_auto = true;
        wb_auto_use_default = false;
        wb_mode = wbMode;
    }

    return true;
}

void Settings::unSetWbMode()
{
    unSetWb();
}

int Settings::getWbMode() const
{
    if (wb_auto_use_default || wbTemp())
    {
        return wb_mode_default;
    }

    return wb_mode;
}

bool Settings::canSetWbTemp() const
{
    return wb_temp_available;
}

bool Settings::trySetWbTemp(const int wbTemp)
{
    if (!canSetWbTemp()) {
        return false;
    }

    if (wb_temp != wbTemp)
    {
        wb_auto = false;
        wb_auto_use_default = false;
        wb_temp = wbTemp;
    }

    return true;
}

void Settings::unSetWbTemp()
{
    unSetWb();
}

int Settings::getWbTemp() const
{
    if (wb_auto_use_default || wbMode())
    {
        return wb_temp_default;
    }

    return wb_temp;
}



// Exposure Analogue Gain (EA) Settings
/*
 * - these settings will affect and be affected by changes in AE enabled
 */
bool Settings::canSetExposureTime() const
{
    return manual_exposure_time_available;
}

bool Settings::trySetExposureTime(const int exposureTime)
{
    if (!canSetExposureTime())
    {
        return false;
    }

    if (exposure_value != exposureTime)
    {
        manual_exposure_time = true;
        exposure_value = exposureTime;
    }

    return true;
}

bool Settings::isSetExposureTime() const
{
    return manual_exposure_time;
}

void Settings::unSetExposureTime()
{
    manual_exposure_time = false;
}

int Settings::getExposureTime() const
{
    if (!manual_exposure_time)
    {
        return exposure_time_default;
    }

    return exposure_time;
}

int Settings::minExposureTime() const
{
    return exposure_time_min;
}

int Settings::maxExposureTime() const
{
    return exposure_time_max;
}


bool Settings::canSetGain() const
{
    return manual_analogue_gain_available;
}

bool Settings::trySetGain(const float iso)
{
    if (!canSetGain())
    {
        return false;
    }

    if (analogue_gain != iso)
    {
        manual_analogue_gain = true;
        analogue_gain = iso;
    }

    return true;
}

bool Settings::isSetGain() const
{
    return manual_analogue_gain;
}

void Settings::unSetGain()
{
    manual_analogue_gain = false;
}

float Settings::getGain() const
{
    if (!manual_analogue_gain)
    {
        return analogue_gain_default;
    }

    return analogue_gain;
}

float Settings::minGain() const
{
    return analogue_gain_min;
}

float Settings::maxGain() const
{
    return analogue_gain_max;
}


// Color Settings
bool Settings::canSetContrast() const
{
    return manual_contrast_available;
}

bool Settings::trySetContrast(const float contrastValue)
{
    if (!canSetContrast())
    {
        return false;
    }

    if (contrast != contrastValue)
    {
        manual_contrast = true;
        contrast = contrastValue;
    }

    return true;
}

bool Settings::isSetContrast() const
{
    return manual_contrast;
}

void Settings::unSetContrast()
{
    manual_contrast = false;
}

float Settings::getContrast() const
{
    if (!manual_contrast)
    {
        return contrast_default;
    }

    return contrast;
}

float Settings::minContrast() const
{
    return contrast_min;
}

float Settings::maxContrast() const
{
    return contrast_max;
}


bool Settings::canSetSaturation() const
{
    return manual_saturation_available;
}

bool Settings::trySetSaturation(const float saturationValue)
{
    if (!canSetSaturation())
    {
        return false;
    }

    if (saturation != saturationValue)
    {
        manual_saturation = true;
        saturation = saturationValue;
    }

    return true;
}

bool Settings::isSetSaturation() const
{
    return manual_saturation;
}

void Settings::unSetSaturation()
{
    manual_saturation = false;
}

float Settings::getSaturation() const
{
    if (!manual_saturation)
    {
        return saturation_default;
    }

    return saturation;
}

float Settings::minSaturation() const
{
    return saturation_min;
}

float Settings::maxSaturation() const
{
    return saturation_max;
}
