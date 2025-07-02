// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QImage>
#include <QDebug>

#include <libcamera/libcamera.h>


/*
 * Auto Exposure, Gain, and Color (AEGC) Settings
 *
 * Settings is a translation layer between the user and libcamera
 * - libcamera settings don't translate well to options for the user
 * - we also want a way to easily share the settings from plasmacamera and worker
 *
 * From reading over the libcamera source code, they are doing some kind of templating
 * to allow for the different types that setting can take
 * For us, we might be able to do something with C++ std::variant
 * https://en.cppreference.com/w/cpp/utility/variant
 *
 * TODO:
 *  - method to reset all the values
 *  - implementation of set() to modify the controlMap associated with the request
 *  - multiple questions in load()
 *  - converting to EXIF
 *  - plasmacamera.cpp will store the master state and broadcast to worker and manager whenever the settings are changed
 *  - display settings in ui settings
 *  - associate Settings with camera
 *  - saving settings to a file?
 */


class Settings {
public:
    Settings();
    ~Settings();

    void load(const libcamera::ControlInfoMap &infoMap);
    void set(libcamera::ControlList &controlMap);
    void reset();

    // Auto Exposure (AE) Settings
    bool canSetAeEnable() const;
    bool trySetAeEnable(bool enable);
    void unSetAeEnable();
    bool getAeEnable() const;

    bool canSetExposureValue() const;
    bool trySetExposureValue(float exposureValue);
    bool isSetExposureValue() const;
    void unSetExposureValue();
    float getExposureValue() const;
    float minExposureValue() const;
    float maxExposureValue() const;


    // Auto Focus (AF) Settings
    bool canSetAfWindow() const;
    bool trySetAfWindow(QSize window);
    void unSetAfWindow();
    QSize getAfWindow() const;


    // White Balance (WB) Settings
    /*
     * - when we set WB with trySetWbMode or trySetWBTemp, we will automatically change wb_auto appropriately
     * - unSetWb, unSetWbMode, unSetWbTemp all do the same thing -> set back to auto WB
     */
    void unSetWb();
    bool wbMode() const;
    bool wbTemp() const;

    bool canSetWbMode() const;
    bool trySetWbMode(int wbMode);
    void unSetWbMode();
    int getWbMode() const;

    bool canSetWbTemp() const;
    bool trySetWbTemp(int wbTemp);
    void unSetWbTemp();
    int getWbTemp() const;

    // Exposure Analogue Gain (EA) Settings
    /*
     * - these settings will affect and be affected by changes in AE enabled
     */
    bool canSetExposureTime() const;
    bool trySetExposureTime(int exposureTime);
    bool isSetExposureTime() const;
    void unSetExposureTime();
    int getExposureTime() const;
    int minExposureTime() const;
    int maxExposureTime() const;

    bool canSetGain() const;
    bool trySetGain(float iso);
    bool isSetGain() const;
    void unSetGain();
    float getGain() const;
    float minGain() const;
    float maxGain() const;


    // Color Settings
    bool canSetContrast() const;
    bool trySetContrast(float contrastValue);
    bool isSetContrast() const;
    void unSetContrast();
    float getContrast() const;
    float minContrast() const;
    float maxContrast() const;

    bool canSetSaturation() const;
    bool trySetSaturation(float saturationValue);
    bool isSetSaturation() const;
    void unSetSaturation();
    float getSaturation() const;
    float minSaturation() const;
    float maxSaturation() const;

private:
    // Auto Exposure (AE) Settings
    /*
     * TODO FUTURE: set up automatic exposure compensation for selected focus area
     *  - https://github.com/raspberrypi/picamera2/issues/967
     *  - this is controlled by adding more configs to AeMeteringModeEnum
     *  - see 4.6 Example Camera Tuning File and 5.9 AGC/AEC of
     *      - https://datasheets.raspberrypi.com/camera/raspberry-pi-camera-guide.pdf
     *
     * - AeEnable <-
     *  - if set to true, then set both exposure time and analogue gain to auto
     *  - set AE enabled if at least one of exposure time and analogue gain are auto
     *  - set AE disabled if both exposure time and analogue gain are manual
     * - AeMeteringMode
     * - AeConstraintMode
     * - AeExposureMode
     * - AeFlickerMode
     * - AeFlickerPeriod
     *  - for 50Hz mains flicker, the value would be 10000 (corresponding to 100Hz), or 8333 (120Hz) for 60Hz mains flicker
     * - ExposureValue (EV) <-
     *  - can only be applied if AE is enabled
     * - DigitalGain
     *  - we cannot set Digital Gain, but we can see its current value from adjusting Exposure Value
     */
    bool m_aeEnable = true;
    bool m_aeEnableDefault = true;
    bool m_aeEnableUseDefault = true;
    bool m_aeEnableAvailable = false;

    float m_exposureValue = 0.0f; // set range to [-2, 2] with 0.25 steps
    float m_exposureValueMin = -2.0f;
    float m_exposureValueMax = 2.0f;
    float m_exposureValueDefault = 0.0f;
    bool m_manualExposureValue = false;
    bool m_manualExposureValueAvailable = false;

    // Auto Focus (AF) Settings
    /*
     * - AfMode
     *  - AfModeManual: don't perform any scan until a lens position is specified
     *  - AfModeAuto: don't perform any scan until AfTrigger is used
     *  - AfModeContinuous: scan whenever unless AfPause is used
     * - AfRange
     * - AfSpeed
     * - AfMetering <-
     *  - AfMeteringAuto: let AF decide where to measure focus
     *  - AfMeteringWindows: use rectangles defined by AfWindows control to measure focus
     * - AfWindows <-
     *  - focus window must overlap with the rectangle returned by the ScalerCropMaximum property
     *  - might need to perform a rotation depending on orientation of camera
     * - AfTrigger
     * - AfPause
     * - AfState
     */
    QSize m_afWindowTarget;
    bool m_afWindow = false;
    bool m_afWindowUseDefault = true;
    bool m_afWindowAvailable = false;

    // White Balance (WB) Settings
    /*
     * TODO:
     *  - need some method to send back the valid values of wb mode and wb temp
     *
     * - AwbEnable <-
     *  - when set to true, use the AwbMode
     *  - when set to false, use the ColourTemperature
     * - AwbMode <-
     * - ColourTemperature <-
     */
    bool m_wbAuto = true;
    bool m_wbAutoUseDefault = true;

    int m_wbMode = 0;
    int m_wbModeDefault = 0;
    bool m_wbModeAvailable = false;

    int m_wbTemp = 5000;
    int m_wbTempDefault = 5000;
    bool m_wbTempAvailable = false;

    // Exposure Analogue Gain (EA) Settings
    /*
     * TODO: https://libcamera.org/api-html/namespacelibcamera_1_1controls.html#a3194f673027fc5367fc1244008c58949
     *  - when transitions from auto to manual exposure time we need to transition more slowly to avoid flickering
     *
     * - ExposureTimeMode <-
     * - ExposureTime <-
     * - AnalogueGainMode <-
     * - AnalogueGain (ISO) <-
     *  - multiply by 100 to get the ISO (e.g. 1 analogue gain -> 100 ISO)
     * - Brightness
     *  - increase or decrease the brightness of the image
     *  - I think this is to be used when exposure value can't be used (e.g. manual exposure time and analogue gain)
     */
    int m_exposureTime = 25000;
    int m_exposureTimeMin = 300;
    int m_exposureTimeMax = 204700;
    int m_exposureTimeDefault = 25000;
    bool m_manualExposureTime = false;
    bool m_manualExposureTimeAvailable = false;

    float m_analogueGain = 1.0f;
    float m_analogueGainMin = 1.0f;
    float m_analogueGainMax = 4.0f;
    float m_analogueGainDefault = 1.0f;
    bool m_manualAnalogueGain = false;
    bool m_manualAnalogueGainAvailable = false;

    // Color Settings
    /*
     * - Contrast <-
     * - Saturation <-
     * - HdrMode
     */
    float m_contrast = 1.0f;
    float m_contrastMin = 0.5f;
    float m_contrastMax = 1.5f;
    float m_contrastDefault = 1.0f;
    bool m_manualContrast = false;
    bool m_manualContrastAvailable = false;

    float m_saturation = 1.0f;
    float m_saturationMin = 0.0f;
    float m_saturationMax = 2.0f;
    float m_saturationDefault = 1.0f;
    bool m_manualSaturation = false;
    bool m_manualSaturationAvailable = false;

    // Zoom Settings
    /*
     * - ScalerCrop
     *  - for digital zoom
     *  - libcamera documentation says it might not work for all platforms
     *  - might make more sense to implement in Qt by cropping the QImage
     */
};
