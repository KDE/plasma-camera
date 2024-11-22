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
	// error
    Q_PROPERTY(bool error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

	// camera state
	Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
	Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
	Q_PROPERTY(QString cameraDevice READ cameraDevice WRITE setCameraDevice NOTIFY cameraDeviceChanged)
    Q_PROPERTY(QCameraFormat cameraFormat READ cameraFormat WRITE setCameraFormat NOTIFY cameraFormatChanged)

	// focus
	Q_PROPERTY(QSize afWindow READ afWindow WRITE setAfWindow RESET resetAfWindow NOTIFY afWindowChanged)

	// brightness
	Q_PROPERTY(int iso READ iso WRITE setIso RESET resetIso NOTIFY isoChanged)
	Q_PROPERTY(int exposureTime READ exposureTime WRITE setExposureTime RESET resetExposureTime NOTIFY exposureTimeChanged)
	Q_PROPERTY(float exposureValue READ exposureValue WRITE setExposureValue RESET resetExposureValue NOTIFY exposureValueChanged)

	// white balance
	// Q_PROPERTY(int wbMode READ wbMode WRITE setWbMode RESET resetAwb NOTIFY wbModeChanged)
	Q_PROPERTY(int wbTemp READ wbTemp WRITE setWbTemp RESET resetAwb NOTIFY wbTempChanged)

	// contrast and saturation
	Q_PROPERTY(int contrast READ contrast WRITE setContrast RESET resetContrast NOTIFY contrastChanged)
	Q_PROPERTY(int saturation READ saturation WRITE setSaturation RESET resetSaturation NOTIFY saturationChanged)

	// // zoom
 //    Q_PROPERTY(float minimumZoomFactor READ minimumZoomFactor NOTIFY minimumZoomFactorChanged)
 //    Q_PROPERTY(float maximumZoomFactor READ maximumZoomFactor NOTIFY maximumZoomFactorChanged)
 //    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
	//
	// // flash
 //    Q_PROPERTY(bool flashReady READ isFlashReady NOTIFY flashReady)
 //    Q_PROPERTY(FlashMode flashMode READ flashMode WRITE setFlashMode NOTIFY flashModeChanged)
 //    Q_PROPERTY(TorchMode torchMode READ torchMode WRITE setTorchMode NOTIFY torchModeChanged)

	// camara settings
	/*
	 * TODO:
	 *  - implement setting resolution from multiple options
	 *  - convert orientation to rotation so that when we save the image it is correctly rotated
	 *		- note that libcamera orientation doesn't appear to work (I suspect not implemented for our hardware)
	 *	- implement digital zoom using cropping (unlikely that libcamera camera digital zoom works for our hardware)
	 */
	// TODO: for some reason rotation on libcamera isn't working so we might just have to do it do the QImage before we send it out
	// TODO: changing the resolution of the camera requires a restart (but that is expected so no worries) (just make sure we have a default value so we don't restart the camera)
	// Q_PROPERTY(Settings settings READ settings NOTIFY settingsChanged)
	// Q_PROPERTY(QSize crop READ crop WRITE setCrop NOTIFY cropChanged)
	// Q_PROPERTY(QSize resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
	// Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
	// Q_PROPERTY(float fps READ fps NOTIFY fpsChanged)

public:
	// enum FlashMode {
	// 	FlashOff,
	// 	FlashOn,
	// 	FlashAuto
	// };
	// Q_ENUM(FlashMode)
	//
	// enum TorchMode {
	// 	TorchOff,
	// 	TorchOn,
	// 	TorchAuto
	// };
	// Q_ENUM(TorchMode)
	//
	// enum WhiteBalanceMode {
	// 	Auto = 0,
	// 	Incandescent = 1,
	// 	Tungsten = 2,
	// 	Fluorescent = 3,
	// 	Indoor = 4,
	// 	Daylight = 5,
	// 	Cloudy = 6
	// };
	// Q_ENUM(WhiteBalanceMode)

    explicit PlasmaCamera(QObject *parent = nullptr);
    ~PlasmaCamera() override;

	// error
	bool error() const;
	QString errorString() const;

	// camera state
	Q_INVOKABLE void start();
	Q_INVOKABLE void stop();
    Q_INVOKABLE bool nextCameraSrc();
	Q_INVOKABLE QList<QString> getCameraDevicesId() const;
	Q_INVOKABLE QList<QString> getCameraDevicesName() const;

	bool isActive() const;
	bool isAvailable() const;
    QString cameraDevice() const;
	QCameraFormat cameraFormat() const;

	// photo
	Q_INVOKABLE bool capture();

	// TODO: get min/max afWindow, iso, et, ev, wm, contrast, satruation?

	// focus
	QSize afWindow() const;

	// brightness
	int iso() const;
	int exposureTime() const;
	float exposureValue() const;

	// white balance
	// int wbMode() const;
	int wbTemp() const;

	// contrast and saturation
	int contrast() const;
	int saturation() const;

	// // zoom
	// float minimumZoomFactor() const;
	// float maximumZoomFactor() const;
	// float zoomFactor() const;
	//
	// // flash (not supported by libcamera at the moment)
	// bool isFlashReady() const;
	// FlashMode flashMode() const;
	// TorchMode torchMode() const;

	// camara settings
	Settings settings() const;
	QSize crop() const;
	QSize resolution() const;
	int orientation() const;
	float fps() const;

Q_SIGNALS:
	// error
	void errorChanged();
	void errorOccurred(const QString &errorString);

	// camera state
	void activeChanged(bool newActive);
	void availableChanged(bool newAvailable);
	void cameraDeviceChanged(const QString &cameraDevice);
	void cameraFormatChanged();

	// photos
    /*
     * While it would make the most sense to switch the libcamera config to StillCapture
     * for higher quality capture it requires restarting the camera which is simply too slow
     */
	void viewFinderFrame(const QImage &frame);				// connect to this to get new frames
	void stillCaptureFrames(const QQueue<QImage> &frames);  // connect to this to get captures

	// focus
	void afWindowChanged(const QSize &newAfWindow);

	// brightness
	void isoChanged(int newIso);
	void exposureTimeChanged(int newExposureTime);
	void exposureValueChanged(float newExposureValue);

	// white balance
	// void wbModeChanged(int newWbMode);
	void wbTempChanged(int newWbTemp);

	// contrast and saturation
	void contrastChanged(int newContrast);
	void saturationChanged(int newSaturation);

	// // zoom
	// void minimumZoomFactorChanged(float newMinimumZoomFactor);
	// void maximumZoomFactorChanged(float newMaximumZoomFactor);
	// void zoomFactorChanged(float newZoomFactor);
	//
	// // flash
	// void flashReady(bool newFlashReady);
	// void flashModeChanged();
	// void torchModeChanged();

	// settings
	void settingsChanged(const Settings& settings);
	void cropChanged(QSize crop);
	void resolutionChanged(QSize resolution);
	void orientationChanged(int orientation);
    void fpsChanged(float fps);

public Q_SLOTS:
	// camera state
	void setActive(bool active);
    void setCameraDevice(const QString &cameraDevice);
	void setCameraFormat(const QCameraFormat &cameraFormat);

	// focus
	void setAfWindow(const QSize &afWindow);
	void resetAfWindow();

	// brightness
	void setIso(int iso);
	void setExposureTime(int exposureTime);
	void setExposureValue(float exposureValue);
	void resetIso();
	void resetExposureTime();
	void resetExposureValue();

	// white balance
	// void setWbMode(int wbMode);
	void setWbTemp(int wbTemp);
	void resetAwb();

	// contrast and saturation
	void setContrast(int contrast);
	void setSaturation(int saturation);
	void resetContrast();
	void resetSaturation();

	// // zoom
	// void setZoomFactor(float zoomFactor, float rate = -1.f);  // rate < 0: zoom instantly, rate in power of two per second
	//
	// // flash
	// void setFlashMode(FlashMode flashMode);
	// void setTorchMode(TorchMode torchMode);

	// TODO
	// // settings
	// // TODO: move to plasmacameramanager?
	// void setCrop(const QSize& crop);
	// void setResolution(const QSize& resolution);
	// void setOrientation(int orientation);
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

	// error
	bool m_error = false;
	QString m_errorString;

	// // focus
	// FocusMode m_focusMode = FocusModeAuto;
	// QPointF m_focusPoint = { -1, -1 };
	// QPointF m_customFocusPoint = { -1, -1 };
	// float m_focusDistance = 1.f; // default is to focus as far as possible
	//
	// // zoom
	// float m_minZoomFactor = 1.f;
	// float m_maxZoomFactor = 1.f;
	// float m_zoomFactor = 1.f;
	//
	// // exposure and iso
	// ExposureMode m_exposureMode = ExposureAuto;
	// float m_exposureCompensation = 0.f;
	//
	// float m_exposureTime = -1.f;
	// float m_manualExposureTime = -1.f;
	// int m_minExposureTime = -1.f;
	// int m_maxExposureTime = -1.f;
	//
	// int m_isoSensitivity = -1;
	// int m_manualIsoSensitivity = -1;
	// int m_minIsoSensitivity = -1;
	// int m_maxIsoSensitivity = -1;
	//
	// // flash
	// bool m_flashReady = false;
	// FlashMode m_flashMode = FlashOff;
	// TorchMode m_torchMode = TorchOff;
	//
	// // white balance
	// WhiteBalanceMode m_whiteBalanceMode = WhiteBalanceAuto;
	// int m_colorTemperature = -1;

	// fps
	float m_fps = 30.0f;

	// internals
	// TODO: move items from members to settings
	Settings m_settings;

	void setState(State state);
	bool m_active = false;  // only try to start camera if active
	State m_state = State::None;

	void startCamera();
	void stopCamera();
    std::unique_ptr<libcamera::CameraManager> cm_;
	Worker* camera_worker_;
    QThread* camera_thread_;

	QString m_camera_name;
	std::shared_ptr<libcamera::Camera> m_camera;

    void acquire();
    void release();
	QCameraFormat m_cameraFormat = QCameraFormat();
};
