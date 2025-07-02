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



class PlasmaCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

    // Camera state
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
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

    bool error() const;
    QString errorString() const;

    // Camera state
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool nextCameraSrc();
	Q_INVOKABLE QList<QString> getCameraDevicesId() const;
	Q_INVOKABLE QList<QString> getCameraDevicesName() const;

	bool isActive() const;
	bool isAvailable() const;
    QString cameraDevice() const;
	QCameraFormat cameraFormat() const;

    Q_INVOKABLE bool capture();

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
    void setActive(bool active);
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
		None,		// init
		Ready,		// libcamera manager and worker is started (set by worker)
		Running,	// ready for capture (set by worker)
		Busy,		// currently taking a photo
		Stopping,	// shutting down
	};

    void setState(State state);

    void startCamera();
    void stopCamera();

    void acquire();
    void release();

    bool m_error = false;
    QString m_errorString;

    float m_fps = 30.0f;

    Settings m_settings;

    bool m_active = false; // only try to start camera if active
    State m_state = State::None;

    std::unique_ptr<libcamera::CameraManager> m_cameraManager;
    Worker *m_cameraWorker;
    QThread *m_cameraThread;

    QString m_cameraName;
    std::shared_ptr<libcamera::Camera> m_camera;

    QCameraFormat m_cameraFormat = QCameraFormat();
};
