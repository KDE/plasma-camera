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
    // int minWbTemp() const;
    // int maxWbTemp() const;


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
    bool ae_enable = true;
    bool ae_enable_default = true;
    bool ae_enable_use_default = true;
    bool ae_enable_available = false;

    float exposure_value = 0.0f; // set range to [-2, 2] with 0.25 steps
    float exposure_value_min = -2.0f;
    float exposure_value_max = 2.0f;
    float exposure_value_default = 0.0f;
    bool manual_exposure_value = false;
    bool manual_exposure_value_available = false;


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
    QSize af_window_target;
    bool af_window = false;
    bool af_window_use_default = true;
    bool af_window_available = false;


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
    bool wb_auto = true;
    bool wb_auto_use_default = true;

    int wb_mode = 0;
    int wb_mode_default = 0;
    bool wb_mode_available = false;

    int wb_temp = 5000;
    int wb_temp_default = 5000;
    bool wb_temp_available = false;


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
    int exposure_time = 25000;
    int exposure_time_min = 300;
    int exposure_time_max = 204700;
    int exposure_time_default = 25000;
    bool manual_exposure_time = false;
    bool manual_exposure_time_available = false;

    float analogue_gain = 1.0f;
    float analogue_gain_min = 1.0f;
    float analogue_gain_max = 4.0f;
    float analogue_gain_default = 1.0f;
    bool manual_analogue_gain = false;
    bool manual_analogue_gain_available = false;


    // Color Settings
    /*
     * - Contrast <-
     * - Saturation <-
     * - HdrMode
     */
    float contrast = 1.0f;
    float contrast_min = 0.5f;
    float contrast_max = 1.5f;
    float contrast_default = 1.0f;
    bool manual_contrast = false;
    bool manual_contrast_available = false;

    float saturation = 1.0f;
    float saturation_min = 0.0f;
    float saturation_max = 2.0f;
    float saturation_default = 1.0f;
    bool manual_saturation = false;
    bool manual_saturation_available = false;

    // Zoom Settings
    /*
     * - ScalerCrop
     *  - for digital zoom
     *  - libcamera documentation says it might not work for all platforms
     *  - might make more sense to implement in Qt by cropping the QImage
     */
};
