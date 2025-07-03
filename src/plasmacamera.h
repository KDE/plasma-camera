// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <plasmacamera/settings.h>

#include "plasmacamera/worker.h"


/*
 * Camera interfaces with a libcamera camera
 * - directly controls the camera to produce a live feed
 * - switch to still image capture to take a picture
 * - gets settings and applies them
 */

/*
 * TODO: image rotation
 * TODO: Raw mode
 * TODO: video mode
 * TODO: stop and pause methods
 *
 * TODO: dealing with changing camera in settings
 *
 * TODO: instead of plasmacamera calling for updates in capabilities or ... worker should take initiative to report this stuff
 *		- when updating some value plasmacamera emits a signal for what it wants and worker emits a signal for what it did (along with another signal maybe for error)
 *		- as a result set does not actually set anything just trys to set
 *
 * TODO: either pass in a ControlList or a CameraConfig
 *		- the values must be already validated
 *		- just of question of if we want to CameraWorker to convert or Camera
 *		- CameraConfig can also just be key-value pair of a single thing to update
 *			- then only Camera stores a config list and we just update as things change
 *			- make a way to start CameraWorker with a vector of configs
 * TODO:
 * 	- camera landscape mode (lock when recording, rotate image/video)
 */

/**
 * \brief Controller object for the application camera state, used from QML.
 */
class PlasmaCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

    // Camera state
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(QString cameraDevice READ cameraDevice WRITE setCameraDevice NOTIFY cameraDeviceChanged)
    Q_PROPERTY(QCameraFormat cameraFormat READ cameraFormat WRITE setCameraFormat NOTIFY cameraFormatChanged)

    // Focus
    Q_PROPERTY(QSize afWindow READ afWindow WRITE setAfWindow RESET resetAfWindow NOTIFY afWindowChanged)

    // Brightness
    Q_PROPERTY(int iso READ iso WRITE setIso RESET resetIso NOTIFY isoChanged)
    Q_PROPERTY(int exposureTime READ exposureTime WRITE setExposureTime RESET resetExposureTime NOTIFY exposureTimeChanged)
    Q_PROPERTY(float exposureValue READ exposureValue WRITE setExposureValue RESET resetExposureValue NOTIFY exposureValueChanged)

    // White balance
    // Q_PROPERTY(int wbMode READ wbMode WRITE setWbMode RESET resetAwb NOTIFY wbModeChanged)
    Q_PROPERTY(int wbTemp READ wbTemp WRITE setWbTemp RESET resetAwb NOTIFY wbTempChanged)

    // Contrast and saturation
    Q_PROPERTY(int contrast READ contrast WRITE setContrast RESET resetContrast NOTIFY contrastChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation RESET resetSaturation NOTIFY saturationChanged)

public:
    explicit PlasmaCamera(QObject *parent = nullptr);
    ~PlasmaCamera() override;

    /*!
     * Returns whether there is a reported error from the camera backend.
     */
    bool error() const;

    /*!
     * Returns the error string of the reported error from the camera backend.
     */
    QString errorString() const;

    /*!
     * Switches the camera source to the next one.
     * Returns whether it was able to switch the camera successfully.
     */
    Q_INVOKABLE bool switchToNextCamera();

    /*!
     * Starts the camera thread if not already started. This starts the viewfinder
     * for generating previews.
     *
     * If the camera is not ready, it will wait until the state changes before starting.
     */
    Q_INVOKABLE void startCamera();

    /*!
     * Stops the camera thread if not already stopped.
     *
     * If the camera is not yet running, it will wait until the state changes before stopping.
     */
    Q_INVOKABLE void stopCamera();

    /*!
     * Returns all camera devices as a list of ids.
     */
    Q_INVOKABLE QList<QString> getCameraDevicesId() const;

    /*!
     * Returns all camera devices as a list of names.
     */
    Q_INVOKABLE QList<QString> getCameraDevicesName() const;

    /*!
     * Whether the camera has been requested to be active.
     */
    bool isActive() const;

    /*!
     * Whether there is a camera running, and a stream available.
     */
    bool isAvailable() const;

    /*!
     * Returns the name of the current camera device.
     */
    QString cameraDevice() const;

    /*!
     * Returns the camera format of the current camera device.
     */
    QCameraFormat cameraFormat() const;

    /*!
     * Capture an image with the current camera.
     */
    bool captureImage();

    // TODO: get min/max afWindow, iso, et, ev, wm, contrast, satruation?

    // Focus
    QSize afWindow() const;

    // Brightness
    int iso() const;
    int exposureTime() const;
    float exposureValue() const;

    // White balance
    // int wbMode() const;
    int wbTemp() const;

    // Contrast and saturation
    int contrast() const;
    int saturation() const;

    // Camera settings
    Settings settings() const;
    float fps() const;

Q_SIGNALS:
    void errorChanged();
    void errorOccurred(const QString &errorString);

    // Camera state
    void activeChanged(bool newActive);
    void availableChanged(bool newAvailable);
    void cameraDeviceChanged(const QString &cameraDevice);
    void cameraFormatChanged();

    // Photos
    /*
     * While it would make the most sense to switch the libcamera config to StillCapture
     * for higher quality capture it requires restarting the camera which is simply too slow
     */
    void viewFinderFrame(const QImage &frame); // connect to this to get new frames
    void stillCaptureFrames(const QQueue<QImage> &frames); // connect to this to get captures

    // Focus
    void afWindowChanged(const QSize &newAfWindow);

    // Brightness
    void isoChanged(int newIso);
    void exposureTimeChanged(int newExposureTime);
    void exposureValueChanged(float newExposureValue);

    // White balance
    // void wbModeChanged(int newWbMode);
    void wbTempChanged(int newWbTemp);

    // Contrast and saturation
    void contrastChanged(int newContrast);
    void saturationChanged(int newSaturation);

    // Settings
    void settingsChanged(const Settings &settings);
    void fpsChanged(float fps);

public Q_SLOTS:
    // Camera state
    void setCameraDevice(const QString &cameraDevice);
	void setCameraFormat(const QCameraFormat &cameraFormat);

    // Focus
    void setAfWindow(const QSize &afWindow);
    void resetAfWindow();

    // Brightness
    void setIso(int iso);
    void setExposureTime(int exposureTime);
    void setExposureValue(float exposureValue);
    void resetIso();
    void resetExposureTime();
    void resetExposureValue();

    // White balance
    // void setWbMode(int wbMode);
    void setWbTemp(int wbTemp);
    void resetAwb();

    // Contrast and saturation
    void setContrast(int contrast);
    void setSaturation(int saturation);
    void resetContrast();
    void resetSaturation();

    void setFps(float fps);

private Q_SLOTS:
	void setError(const QString &errorString);
	void unsetError();

private:
	enum class State
	{
        None, // Init
        Ready, // Libcamera manager and worker is started (set by worker)
        Running, // Ready for capture (set by worker)
        Busy, // Currently taking a photo
        Stopping, // Shutting down
    };

    /*!
     * Starts the camera thread if not already started. This starts the viewfinder
     * for generating previews.
     */
    void startCameraInternal();

    /*!
     * Stops the camera thread if not already stopped.
     */
    void stopCameraInternal();

    /*!
     * Set the active state of the camera, and start/stop the camera as needed.
     */
    void setActive(bool active);

    /*!
     * Set the internal state of the camera object.
     */
    void setState(State state);

    /*!
     * Attempt to acquire the current camera (from libcamera).
     */
    void acquire();

    /*!
     * Release the lock on the current camera (in libcamera).
     */
    void release();

    bool m_error = false;
    QString m_errorString;

    float m_fps = 30.0f;

    Settings m_settings;

    // Whether the camera has been requested to be active (see state for the actual camera state).
    bool m_active = false;

    // The current state of the camera.
    State m_state = State::None;

    std::unique_ptr<libcamera::CameraManager> m_cameraManager;
    Worker *m_cameraWorker;
    QThread *m_cameraThread;

    QString m_cameraName;
    std::shared_ptr<libcamera::Camera> m_camera;

    QCameraFormat m_cameraFormat = QCameraFormat();
};
