// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <unistd.h>
#include <sys/mman.h>
#include <plasmacameramanager.h>

#include "plasmacamera.h"



PlasmaCamera::PlasmaCamera(QObject *parent) : QObject(parent)
{
    cm_ = std::make_unique<libcamera::CameraManager>();
    if (const int ret = cm_->start(); ret != 0)
    {
        // if there exists two CameraManager we get undefined behavior
        qCritical() << "libcamera manager start failed:" << ret;
        return;
    }

    // initialize camera worker thread
    camera_worker_ = new Worker();
    camera_thread_ = new QThread();
    camera_worker_->moveToThread(camera_thread_);

    // link the worker with the thread
    connect(camera_thread_, &QThread::started, camera_worker_, &Worker::init);
    connect(camera_worker_, &Worker::finished, camera_thread_, &QThread::quit);
    connect(camera_worker_, &Worker::finished, camera_worker_, &Worker::deleteLater);
    connect(camera_thread_, &QThread::finished, camera_thread_, &QThread::deleteLater);

    // update this state as the worker updates its state
    connect(camera_worker_, &Worker::ready, this, [=]() { setState(State::Ready); });
    connect(camera_worker_, &Worker::running, this, [=]() { setState(State::Running); });

    // resend frames that we received from the camera worker
    connect(camera_worker_, &Worker::viewFinderFrame, this, &PlasmaCamera::viewFinderFrame);
    connect(camera_worker_, &Worker::stillCaptureFrames, this, &PlasmaCamera::stillCaptureFrames);

    // get errors from the camera worker
    connect(camera_worker_, &Worker::errorOccurred, this, &PlasmaCamera::setError);

    // send new settings to the camera worker
    connect(this, &PlasmaCamera::settingsChanged, camera_worker_, &Worker::setSettings);

    camera_thread_->start();
}

PlasmaCamera::~PlasmaCamera()
{
    // optional
    stop();

    // set the thread to quit then wait for it to finish
    // shutdown in worker will emit finished() which is connected to quit() in the thread
    // BlockingQueuedConnection allows the Q_EMIT in shutdown to go through while QueuedConnection with wait does not
    QMetaObject::invokeMethod(camera_worker_, "shutdown", Qt::BlockingQueuedConnection);
}

bool PlasmaCamera::error() const
{
    return m_error;
}

QString PlasmaCamera::errorString() const
{
    return m_errorString;
}

void PlasmaCamera::start()
{
    setActive(true);
}

void PlasmaCamera::stop()
{
    setActive(false);
}

bool PlasmaCamera::nextCameraSrc()
{
    if (m_state != State::Running)
    {
        return false;
    }

    const std::vector<std::shared_ptr<libcamera::Camera>> cameras = cm_->cameras();
    if (cameras.empty())
    {
        qDebug() << "Failed to find any cameras";
        return false;
    }

    auto match = std::find_if(cameras.begin(), cameras.end(),
        [&](const std::shared_ptr<libcamera::Camera> &camera)
        {
            return camera->id() == m_camera_name;
        });
    if (match == cameras.end())
    {
        qDebug() << "Failed to find desired camera: " << m_camera_name;
        for (const auto &camera : cameras)
            qDebug() << "Have camera: " << camera->id();

        return false;
    }

    // cyclic increment
    ++match;
    if (match == cameras.end())
    {
        match = cameras.begin();
    }

    setCameraDevice( QString::fromStdString((*match)->id()) );
    return true;
}

QList<QString> PlasmaCamera::getCameraDevicesId() const
{
    // camera ids are unique strings to identify the camera devices
    QList<QString> devices;
    for (const auto &camera : cm_->cameras())
    {
        devices.push_back( QString::fromStdString(camera->id()) );
    }

    return devices;
}

QList<QString> PlasmaCamera::getCameraDevicesName() const
{
    // camera names are more human-readable strings
    QList<QString> devices;
    for (const auto &camera : cm_->cameras())
    {
        devices.push_back( QString::fromStdString(*camera->properties().get(libcamera::properties::Model)) );
    }

    return devices;
}

bool PlasmaCamera::isActive() const
{
    return m_state == State::Running || m_state == State::Busy;
}

bool PlasmaCamera::isAvailable() const
{
    return m_state == State::Running;
}

QString PlasmaCamera::cameraDevice() const
{
    return m_camera_name;
}

QCameraFormat PlasmaCamera::cameraFormat() const
{
    return m_cameraFormat;
}

bool PlasmaCamera::capture()
{
    if (m_state != State::Running)
    {
        return false;
    }

    // TODO: some kind of timeout?
    setState(State::Busy);
    QMetaObject::invokeMethod(camera_worker_, "capture", Qt::QueuedConnection);

    return true;
}

QSize PlasmaCamera::afWindow() const
{
    return m_settings.getAfWindow();
}

int PlasmaCamera::iso() const
{
    return static_cast<int>(m_settings.getGain() * 100);
}

int PlasmaCamera::exposureTime() const
{
    return m_settings.getExposureTime();
}

float PlasmaCamera::exposureValue() const
{
    return m_settings.getExposureValue();
}

int PlasmaCamera::wbTemp() const
{
    return m_settings.getWbTemp();
}

int PlasmaCamera::contrast() const
{
    return m_settings.getContrast();
}

int PlasmaCamera::saturation() const
{
    return m_settings.getSaturation();
}

Settings PlasmaCamera::settings() const
{
    return m_settings;
}

QSize PlasmaCamera::crop() const
{
    // TODO
    return QSize();
}

QSize PlasmaCamera::resolution() const
{
    // TODO
    return QSize();
}

int PlasmaCamera::orientation() const
{
    // TODO: rotation counter clockwise
    return 0;
}

// float PlasmaCamera::minimumZoomFactor() const
// {
//     return m_minZoomFactor;
// }
//
// float PlasmaCamera::maximumZoomFactor() const
// {
//     return m_maxZoomFactor;
// }
//
// float PlasmaCamera::zoomFactor() const
// {
//     return m_zoomFactor;
// }
//
// bool PlasmaCamera::isFlashReady() const
// {
//     return m_flashReady;
// }
//
// PlasmaCamera::FlashMode PlasmaCamera::flashMode() const
// {
//     return m_flashMode;
// }
//
// PlasmaCamera::TorchMode PlasmaCamera::torchMode() const
// {
//     return m_torchMode;
// }

float PlasmaCamera::fps() const
{
    return m_fps;
}



void PlasmaCamera::setActive(const bool active)
{
    if (isActive() != active)
    {
        m_active = active;

        if (active)
        {
            // if we are currently not in Ready then wait until we setState to Ready to start the camera
            if (m_state != State::Ready)
                return;

            startCamera();

        } else
        {
            // if we are currently not in Running then wait until we setState to Running to stop the camera
            if (m_state != State::Running)
                return;

            stopCamera();
        }

        // if the state change request m_active cannot be handled right now
        // it will be handled once setState sets the state to the Ready/Running
    }
}

void PlasmaCamera::setCameraDevice(const QString &cameraDevice)
{
    // set the camera name then attempt to use that camera if possible
     if (cameraDevice != m_camera_name)
    {
        const std::string cameraDeviceString = cameraDevice.toStdString();

        // since cm_ is set up in the class init we should always have it
        const std::vector<std::shared_ptr<libcamera::Camera>> cameras = cm_->cameras();
        if (cameras.empty())
            qDebug() << "Failed to find any cameras";

        auto match = std::find_if(cameras.begin(), cameras.end(),
            [&](const std::shared_ptr<libcamera::Camera> &camera) {return camera->id() == cameraDeviceString;});
        if (match == cameras.end())
        {
            qDebug() << "Failed to find desired camera: " << cameraDevice;
            for (const auto &camera : cameras)
                qDebug() << "Have camera: " << camera->id();
            return;
        }

        qDebug() << "Using camera: " << cameraDevice;
        m_camera_name = cameraDevice;

        // only when we are running can we switch cameras
        if (m_state == State::Running)
        {
            // by going to the ready state if m_active we will start up using m_camera_name
            // set state to stopping to emit not active then since we have no clean up, start
            setState(State::Busy);
            startCamera();
        }
    }
}

void PlasmaCamera::setCameraFormat(const QCameraFormat &cameraFormat)
{
    if (cameraFormat != m_cameraFormat)
    {
        m_cameraFormat = cameraFormat;

        Q_EMIT cameraFormatChanged();
    }
}

void PlasmaCamera::setAfWindow(const QSize &afWindow)
{
    if (m_settings.canSetAeEnable() && m_settings.trySetAfWindow(afWindow)) {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetAfWindow()
{
    m_settings.unSetAfWindow();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setIso(const int iso)
{
    // TODO: iso vs gain
    if (m_settings.canSetGain() && m_settings.trySetGain(iso / 100.0f))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetIso()
{
    m_settings.unSetGain();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setExposureTime(const int exposureTime)
{
    if (m_settings.canSetExposureTime() && m_settings.trySetExposureTime(exposureTime))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetExposureTime()
{
    m_settings.unSetExposureTime();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setExposureValue(const float exposureValue)
{
    if (m_settings.canSetExposureValue() && m_settings.trySetExposureValue(exposureValue))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetExposureValue()
{
    m_settings.unSetExposureValue();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setWbTemp(const int wbTemp)
{
    if (m_settings.canSetWbTemp() && m_settings.trySetWbTemp(wbTemp))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetAwb()
{
    m_settings.unSetWbTemp();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setContrast(const int contrast)
{
    if (m_settings.canSetContrast() && m_settings.trySetContrast(contrast))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetContrast()
{
    m_settings.unSetContrast();
    Q_EMIT settingsChanged(m_settings);
}

void PlasmaCamera::setSaturation(const int saturation)
{
    if (m_settings.canSetSaturation() && m_settings.trySetSaturation(saturation))
    {
        Q_EMIT settingsChanged(m_settings);
    }
}

void PlasmaCamera::resetSaturation()
{
    m_settings.unSetSaturation();
    Q_EMIT settingsChanged(m_settings);
}

// void PlasmaCamera::setFocusMode(const FocusMode focusMode)
// {
//     if (m_focusMode != focusMode)
//     {
//         m_focusMode = focusMode;
//         Q_EMIT focusModeChanged();
//     }
// }
//
// void PlasmaCamera::setCustomFocusPoint(const QPointF &customFocusPoint)
// {
//     if (m_customFocusPoint != customFocusPoint)
//     {
//         m_customFocusPoint = customFocusPoint;
//         Q_EMIT customFocusPointChanged(customFocusPoint);
//     }
// }
//
// void PlasmaCamera::setFocusDistance(const float focusDistance)
// {
//     if (m_focusDistance != focusDistance)
//     {
//         m_focusDistance = focusDistance;
//         Q_EMIT focusDistanceChanged(focusDistance);
//     }
// }
//
// void PlasmaCamera::setZoomFactor(const float zoomFactor, float rate)
// {
//     if (m_zoomFactor != zoomFactor)
//     {
//         // TODO
//         m_zoomFactor = zoomFactor;
//         Q_EMIT zoomFactorChanged(zoomFactor);
//     }
// }
//
// void PlasmaCamera::setManualExposureTime(const float exposureTime)
// {
//     if (m_manualExposureTime != exposureTime)
//     {
//         m_manualExposureTime = exposureTime;
//         Q_EMIT manualExposureTimeChanged(exposureTime);
//     }
// }
//
// void PlasmaCamera::setExposureCompensation(const float exposureCompensation)
// {
//     if (m_exposureCompensation != exposureCompensation)
//     {
//         m_exposureCompensation = exposureCompensation;
//         Q_EMIT exposureCompensationChanged(exposureCompensation);
//     }
// }
//
// void PlasmaCamera::setExposureMode(const ExposureMode exposureMode)
// {
//     if (m_exposureMode != exposureMode)
//     {
//         m_exposureMode = exposureMode;
//         Q_EMIT exposureModeChanged();
//     }
// }
//
// void PlasmaCamera::setIsoSensitivity(const int isoSensitivity)
// {
//     if (m_isoSensitivity != isoSensitivity)
//     {
//         m_isoSensitivity = isoSensitivity;
//         Q_EMIT isoSensitivityChanged(isoSensitivity);
//     }
// }
//
// void PlasmaCamera::setManualIsoSensitivity(const int manualIsoSensitivity)
// {
//     if (m_manualIsoSensitivity != manualIsoSensitivity)
//     {
//         m_manualIsoSensitivity = manualIsoSensitivity;
//         Q_EMIT manualIsoSensitivityChanged(manualIsoSensitivity);
//     }
// }
//
// void PlasmaCamera::setFlashMode(const FlashMode flashMode)
// {
//     if (m_flashMode != flashMode)
//     {
//         m_flashMode = flashMode;
//         Q_EMIT flashModeChanged();
//     }
// }
//
// void PlasmaCamera::setTorchMode(const TorchMode torchMode)
// {
//     if (m_torchMode != torchMode)
//     {
//         m_torchMode = torchMode;
//         Q_EMIT torchModeChanged();
//     }
// }
//
// void PlasmaCamera::setWhiteBalanceMode(const WhiteBalanceMode whiteBalanceMode)
// {
//     if (m_whiteBalanceMode != whiteBalanceMode)
//     {
//         m_whiteBalanceMode = whiteBalanceMode;
//         Q_EMIT whiteBalanceModeChanged();
//     }
// }
//
// void PlasmaCamera::setColorTemperature(const int colorTemperature)
// {
//     if (m_colorTemperature != colorTemperature)
//     {
//         m_colorTemperature = colorTemperature;
//         Q_EMIT colorTemperatureChanged();
//     }
// }

void PlasmaCamera::setFps(const float fps)
{
    if (m_fps != fps)
    {
        m_fps = fps;
        Q_EMIT fpsChanged(m_fps);
    }
}



void PlasmaCamera::setError(const QString &errorString)
{
    m_error = true;
    m_errorString = errorString;

    qWarning() << m_errorString;

    Q_EMIT errorChanged();
    Q_EMIT errorOccurred(m_errorString);
}

void PlasmaCamera::unsetError()
{
    m_error = false;
    m_errorString.clear();
}



void PlasmaCamera::setState(const State state)
{
    // handles sending the signals for state changes
    // and attempting to change state based on m_active
    const State previous_state = m_state;

    switch (state)
    {
    case State::None:
        // after initial init it should not be possible to go back to None
        // m_state = State::None;
        break;

    case State::Ready:
        m_state = State::Ready;
        if (m_active)
        {
            startCamera();
            break;
        }
        break;

    case State::Running:
        m_state = State::Running;
        if (!m_active)
        {
            stopCamera();
            break;
        }
        switch (previous_state)
        {
        case State::Running:
            break;
        case State::Ready:
            Q_EMIT activeChanged(true);
            Q_EMIT availableChanged(true);
            break;
        case State::Busy:
            Q_EMIT availableChanged(true);
            break;
        default:
            qWarning() << "Transitioned from state other than Running, Ready, or Busy to Running";
        }
        break;

    case State::Busy:
        m_state = State::Busy;
        switch (previous_state)
        {
        case State::Running:
            Q_EMIT availableChanged(false);
            break;
        default:
            qWarning() << "Transitioned from state other than Running to Busy";
        }
        break;

    case State::Stopping:
        m_state = State::Stopping;
        switch (previous_state)
        {
        case State::Ready:
            break;
        case State::Running:
            Q_EMIT activeChanged(false);
            Q_EMIT availableChanged(false);
            break;
        default:
            qWarning() << "Transitioned from state other than Ready or Running to Stopping";
        }
        break;
    }
}

void PlasmaCamera::startCamera()
{
    // if the m_camera_name is empty then default to the first one
    if (m_camera_name.isEmpty())
    {
        const std::vector<std::shared_ptr<libcamera::Camera>> cameras = cm_->cameras();
        if (cameras.empty())
            qDebug() << "Failed to find any cameras";

        // if no camera is set then get the first camera
        for (const auto &camera : cameras)
            qDebug() << "Found camera: " << camera->id();

        m_camera_name = QString::fromStdString(cameras[0]->id());
    }

    m_camera = cm_->get(m_camera_name.toStdString());
    acquire();

    // only emit that we have changed camera once we
    Q_EMIT cameraDeviceChanged(m_camera_name);

    // we are relying on Worker to free the previous camera
    // this is because there are some things it needs to stop before it can stop the camera
    QMetaObject::invokeMethod(camera_worker_, "setCamera", Qt::QueuedConnection,
        Q_ARG(const std::shared_ptr<libcamera::Camera>, m_camera));
}

void PlasmaCamera::stopCamera()
{
    setState(State::Stopping);

    // clear the current camera
    m_camera_name.clear();
    m_camera.reset();

    // after camera is stopped worker will set this state to ready
    QMetaObject::invokeMethod(camera_worker_, "unsetCamera", Qt::QueuedConnection);
}

void PlasmaCamera::acquire()
{
    // TODO:
    //  - acquire should attempt to set up the camera indicated in camera_name_
    //  - it should also get the control info for the camera
    //  - save the previous settings where? or just delete them
    //  - need to reinit everything including requests?
    //  - it should pass the camera_device_ to worker
    //  - what should occur if the worker doesn't accept camera_device_?

    int ret = m_camera->acquire();
    if (ret < 0)
    {
        setError(QString::fromStdString("Failed to acquire camera"));
        m_camera.reset();
        return;
    }


    // TODO: set up m_features with camera

    // "ExposureTime" :  "300"  to  "204700"  with default  "25000"
	// "AeEnable" :  "false"  to  "true"  with default  "true"
	// "AnalogueGain" :  "1.000000"  to  "4.000000"  with default  "1.000000"
	// "Saturation" :  "0.000000"  to  "1.992188"  with default  "1.000000"
	// "Contrast" :  "0.500000"  to  "1.496094"  with default  "1.000000"
	// "Brightness" :  "-1.000000"  to  "0.992188"  with default  "0.000000"

    // AnalogueGain (1, 10) -> ISO (100, 1000) (multiply by 100)
    // DigitalGain -> gain applied digitally (rather than by camera sensor)
    // ExposureTime -> Shutter time in micro-seconds

    // TODO: note that setting settings via requests can be sticky i.e. the settings persist in the camera after the program exists



    // get camera controls
    const libcamera::ControlInfoMap &infoMap = m_camera->controls();
    qInfo() << "Acquired" << infoMap.size() << "controls";

    // for (const auto &[fst, snd] : infoMap)
    // {
    //     qDebug()
    //         << "\t" << fst->name() << ":"
    //         << "min:" << snd.min().toString()
    //         << " max:" << snd.max().toString()
    //         << " default:" << snd.def().toString();
    //
    //     // TODO: check that these controls actually map that what I put here
    //     //  - for now just expose all the controls we get to the end user's setting panel
    //     switch (fst->id())
    //     {
    //     case libcamera::controls::AE_ENABLE:
    //         // if true sets ExposureTimeMode and AnalogueGainMode to auto, if set to false sets them to manual
    //         // TODO
    //         break;
    //
    //     case libcamera::controls::COLOUR_TEMPERATURE:
    //         m_features |= Feature::ColorTemperature;
    //         break;
    //
    //     case libcamera::controls::EXPOSURE_VALUE:
    //         // what is the difference from this and BRIGHTNESS?
    //         m_features |= Feature::ExposureCompensation;
    //         break;
    //
    //     case libcamera::controls::ANALOGUE_GAIN:
    //         m_features |= Feature::IsoSensitivity;
    //         break;
    //
    //     case libcamera::controls::EXPOSURE_TIME:
    //         m_features |= Feature::ManualExposureTime;
    //         break;
    //
    //     case libcamera::controls::AF_WINDOWS:
    //         // TODO: need to convert the focus point to focus window
    //         m_features |= Feature::CustomFocusPoint;
    //         break;
    //
    //     case libcamera::controls::AF_RANGE:
    //         m_features |= Feature::FocusDistance;
    //         break;
    //
    //     case libcamera::controls::SATURATION:
    //         // TODO: ???
    //         break;
    //
    //     case libcamera::controls::CONTRAST:
    //         break;
    //
    //     case libcamera::controls::BRIGHTNESS:
    //         break;
    //
    //     default:
    //         qInfo() << "\t Unknown control: " << fst->name() << " id: " << fst->id();
    //         break;
    //     }
    // }

    // print all possible camera controls
    // qInfo() << "All" << libcamera::controls::controls.size() << "controls";
    // for (const auto &info : libcamera::controls::controls)
    // {
    //     qInfo() << "\t" << info.second->name();
    // }

    // print camera properties
    const libcamera::ControlList &controls = m_camera->properties();
    qInfo() << "Acquired" << controls.size() << "properties";
    for (const auto &info : controls)
    {
        qInfo() << "\t" << libcamera::controls::controls.at(info.first)->name() << " " << info.second.toString();
    }
}

void PlasmaCamera::release()
{
    // actual camera teardown is done in Worker
    // this is to avoid possible errors if stopping is done out of order

    // m_features = Features();
    m_cameraFormat = QCameraFormat();
}
