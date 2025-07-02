// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include "settings.h"

Settings::Settings() {}

Settings::~Settings() {}

void Settings::load(const libcamera::ControlInfoMap &infoMap)
{
    for (const auto &[controlId, controlInfo] : infoMap) {
        qDebug()
            << "\t" << controlId->name() << ":"
            << "min:" << controlInfo.min().toString()
            << " max:" << controlInfo.max().toString()
            << " default:" << controlInfo.def().toString();

        switch (controlId->id()) {
        case libcamera::controls::AE_ENABLE:
            m_aeEnableAvailable = true;
            m_aeEnableDefault = controlInfo.def().get<bool>();
            m_aeEnable = m_aeEnableDefault;
            break;

        case libcamera::controls::EXPOSURE_VALUE:
            m_manualExposureValueAvailable = true;
            m_exposureValueDefault = controlInfo.def().get<float>();
            m_exposureValue = m_exposureValueDefault;
            m_exposureValueMin = controlInfo.min().get<float>();
            m_exposureValueMax = controlInfo.max().get<float>();
            break;

        case libcamera::controls::AF_WINDOWS:
            // TODO: should we check for AF_METERING instead?
            // TODO: how to convert Span<const Rectangle> to QSize?
            m_afWindowAvailable = true;
            break;

        case libcamera::controls::AWB_MODE:
            // TODO: AwbModeEnum
            m_wbModeAvailable = true;
            m_wbModeDefault = controlInfo.def().get<int32_t>();
            m_wbMode = m_wbModeDefault;
            break;

        case libcamera::controls::COLOUR_TEMPERATURE:
            // TODO: colour temperature min and max?
            m_wbTempAvailable = true;
            m_wbTempDefault = controlInfo.def().get<int32_t>();
            m_wbTemp = m_wbTempDefault;
            // wb_temp_min = controlInfo.min().get<int32_t>();
            // wb_temp_max = controlInfo.max().get<int32_t>();
            break;

        case libcamera::controls::EXPOSURE_TIME:
            m_manualExposureTimeAvailable = true;
            m_exposureTimeDefault = controlInfo.def().get<int32_t>();
            m_exposureTime = m_exposureTimeDefault;
            m_exposureTimeMin = controlInfo.min().get<int32_t>();
            m_exposureTimeMax = controlInfo.max().get<int32_t>();
            break;

        case libcamera::controls::ANALOGUE_GAIN:
            m_manualAnalogueGainAvailable = true;
            m_analogueGainDefault = controlInfo.def().get<float>();
            m_analogueGain = m_analogueGainDefault;
            m_analogueGainMin = controlInfo.min().get<float>();
            m_analogueGainMax = controlInfo.max().get<float>();
            break;

        case libcamera::controls::CONTRAST:
            m_manualContrastAvailable = true;
            m_contrastDefault = controlInfo.def().get<float>();
            m_contrast = m_contrastDefault;
            m_contrastMin = controlInfo.min().get<float>();
            m_contrastMax = controlInfo.max().get<float>();
            break;

        case libcamera::controls::SATURATION:
            m_manualSaturationAvailable = true;
            m_saturationDefault = controlInfo.def().get<float>();
            m_saturation = m_saturationDefault;
            m_saturationMin = controlInfo.min().get<float>();
            m_saturationMax = controlInfo.max().get<float>();
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
    return m_aeEnableAvailable;
}

bool Settings::trySetAeEnable(const bool enable)
{
    if (!canSetAeEnable())
    {
        return false;
    }

    if (m_aeEnable != enable) {
        m_aeEnableUseDefault = false;
        m_aeEnable = enable;

        // if set to true, then set both exposure time and analogue gain to auto
        if (m_aeEnable) {
            unSetExposureTime();
            unSetGain();
        }
    }

    return true;
}

void Settings::unSetAeEnable()
{
    m_aeEnableUseDefault = true;
}

bool Settings::getAeEnable() const
{
    if (m_aeEnableUseDefault) {
        return m_aeEnableDefault;
    }

    return m_aeEnable;
}


bool Settings::canSetExposureValue() const
{
    return m_aeEnable && m_manualExposureValueAvailable;
}

bool Settings::trySetExposureValue(const float exposureValue)
{
    if (!canSetExposureValue() || exposureValue < m_exposureValueMin || exposureValue > m_exposureValueMax) {
        return false;
    }

    if (m_exposureValue != exposureValue) {
        m_manualExposureValue = true;
        m_exposureValue = exposureValue;
    }

    return true;
}

bool Settings::isSetExposureValue() const
{
    return m_manualExposureValue;
}

void Settings::unSetExposureValue()
{
    m_manualExposureValue = false;
}

float Settings::getExposureValue() const
{
    if (!isSetExposureValue())
    {
        return m_exposureValueDefault;
    }

    return m_exposureValue;
}

float Settings::minExposureValue() const
{
    return m_exposureValueMin;
}

float Settings::maxExposureValue() const
{
    return m_exposureValueMax;
}


// Auto Focus (AF) Settings
bool Settings::canSetAfWindow() const
{
    return m_afWindowAvailable;
}

bool Settings::trySetAfWindow(const QSize window)
{
    if (!canSetAfWindow())
    {
        return false;
    }

    if (m_afWindowTarget != window) {
        m_afWindowUseDefault = false;
        m_afWindow = true;
        m_afWindowTarget = window;
    }

    return true;
}

void Settings::unSetAfWindow()
{
    m_afWindowUseDefault = true;
    m_afWindow = false;
}

QSize Settings::getAfWindow() const
{
    if (m_afWindowUseDefault) {
        return QSize();
    }

    return m_afWindowTarget;
}


// White Balance (WB) Settings
/*
 * - when we set WB with trySetWbMode or trySetWBTemp we will automatically change wb_auto appropriately
 * - unSetWb, unSetWbMode, unSetWbTemp all do the same thing -> set back to auto WB
 */
void Settings::unSetWb()
{
    m_wbAuto = true;
    m_wbAutoUseDefault = true;
}

bool Settings::wbMode() const
{
    return m_wbAuto;
}

bool Settings::wbTemp() const
{
    return !m_wbAuto;
}

bool Settings::canSetWbMode() const
{
    return m_wbModeAvailable;
}

bool Settings::trySetWbMode(const int wbMode)
{
    if (!canSetWbMode()) {
        return false;
    }

    if (m_wbMode != wbMode) {
        m_wbAuto = true;
        m_wbAutoUseDefault = false;
        m_wbMode = wbMode;
    }

    return true;
}

void Settings::unSetWbMode()
{
    unSetWb();
}

int Settings::getWbMode() const
{
    if (m_wbAutoUseDefault || wbTemp()) {
        return m_wbModeDefault;
    }

    return m_wbMode;
}

bool Settings::canSetWbTemp() const
{
    return m_wbTempAvailable;
}

bool Settings::trySetWbTemp(const int wbTemp)
{
    if (!canSetWbTemp()) {
        return false;
    }

    if (m_wbTemp != wbTemp) {
        m_wbAuto = false;
        m_wbAutoUseDefault = false;
        m_wbTemp = wbTemp;
    }

    return true;
}

void Settings::unSetWbTemp()
{
    unSetWb();
}

int Settings::getWbTemp() const
{
    if (m_wbAutoUseDefault || wbMode()) {
        return m_wbTempDefault;
    }

    return m_wbTemp;
}



// Exposure Analogue Gain (EA) Settings
/*
 * - these settings will affect and be affected by changes in AE enabled
 */
bool Settings::canSetExposureTime() const
{
    return m_manualExposureTimeAvailable;
}

bool Settings::trySetExposureTime(const int exposureTime)
{
    if (!canSetExposureTime())
    {
        return false;
    }

    if (m_exposureValue != exposureTime) {
        m_manualExposureTime = true;
        m_exposureValue = exposureTime;
    }

    return true;
}

bool Settings::isSetExposureTime() const
{
    return m_manualExposureTime;
}

void Settings::unSetExposureTime()
{
    m_manualExposureTime = false;
}

int Settings::getExposureTime() const
{
    if (!m_manualExposureTime) {
        return m_exposureTimeDefault;
    }

    return m_exposureTime;
}

int Settings::minExposureTime() const
{
    return m_exposureTimeMin;
}

int Settings::maxExposureTime() const
{
    return m_exposureTimeMax;
}


bool Settings::canSetGain() const
{
    return m_manualAnalogueGainAvailable;
}

bool Settings::trySetGain(const float iso)
{
    if (!canSetGain()) {
        return false;
    }

    if (m_analogueGain != iso) {
        m_manualAnalogueGain = true;
        m_analogueGain = iso;
    }

    return true;
}

bool Settings::isSetGain() const
{
    return m_manualAnalogueGain;
}

void Settings::unSetGain()
{
    m_manualAnalogueGain = false;
}

float Settings::getGain() const
{
    if (!m_manualAnalogueGain) {
        return m_analogueGainDefault;
    }

    return m_analogueGain;
}

float Settings::minGain() const
{
    return m_analogueGainMin;
}

float Settings::maxGain() const
{
    return m_analogueGainMax;
}


// Color Settings
bool Settings::canSetContrast() const
{
    return m_manualContrastAvailable;
}

bool Settings::trySetContrast(const float contrastValue)
{
    if (!canSetContrast())
    {
        return false;
    }

    if (m_contrast != contrastValue) {
        m_manualContrast = true;
        m_contrast = contrastValue;
    }

    return true;
}

bool Settings::isSetContrast() const
{
    return m_manualContrast;
}

void Settings::unSetContrast()
{
    m_manualContrast = false;
}

float Settings::getContrast() const
{
    if (!m_manualContrast) {
        return m_contrastDefault;
    }

    return m_contrast;
}

float Settings::minContrast() const
{
    return m_contrastMin;
}

float Settings::maxContrast() const
{
    return m_contrastMax;
}


bool Settings::canSetSaturation() const
{
    return m_manualSaturationAvailable;
}

bool Settings::trySetSaturation(const float saturationValue)
{
    if (!canSetSaturation())
    {
        return false;
    }

    if (m_saturation != saturationValue) {
        m_manualSaturation = true;
        m_saturation = saturationValue;
    }

    return true;
}

bool Settings::isSetSaturation() const
{
    return m_manualSaturation;
}

void Settings::unSetSaturation()
{
    m_manualSaturation = false;
}

float Settings::getSaturation() const
{
    if (!m_manualSaturation) {
        return m_saturationDefault;
    }

    return m_saturation;
}

float Settings::minSaturation() const
{
    return m_saturationMin;
}

float Settings::maxSaturation() const
{
    return m_saturationMax;
}
