// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <unistd.h>
#include <sys/mman.h>
#include <plasmacameramanager.h>

#include "plasmacamera.h"



PlasmaCamera::PlasmaCamera(QObject *parent) : QObject(parent)
{
    m_cameraManager = std::make_unique<libcamera::CameraManager>();
    if (const int ret = m_cameraManager->start(); ret != 0) {
        // if there exists two CameraManager we get undefined behavior
        qCritical() << "libcamera manager start failed:" << ret;
        return;
    }

    // initialize camera worker thread
    m_cameraWorker = new Worker();
    m_cameraThread = new QThread();
    m_cameraWorker->moveToThread(m_cameraThread);

    // link the worker with the thread
    connect(m_cameraThread, &QThread::started, m_cameraWorker, &Worker::init);
    connect(m_cameraWorker, &Worker::finished, m_cameraThread, &QThread::quit);
    connect(m_cameraWorker, &Worker::finished, m_cameraWorker, &Worker::deleteLater);
    connect(m_cameraThread, &QThread::finished, m_cameraThread, &QThread::deleteLater);

    // update this state as the worker updates its state
    connect(m_cameraWorker, &Worker::ready, this, [=]() {
        setState(State::Ready);
    });
    connect(m_cameraWorker, &Worker::running, this, [=]() {
        setState(State::Running);
    });

    // resend frames that we received from the camera worker
    connect(m_cameraWorker, &Worker::viewFinderFrame, this, &PlasmaCamera::viewFinderFrame);
    connect(m_cameraWorker, &Worker::stillCaptureFrames, this, &PlasmaCamera::stillCaptureFrames);

    // get errors from the camera worker
    connect(m_cameraWorker, &Worker::errorOccurred, this, &PlasmaCamera::setError);

    // send new settings to the camera worker
    connect(this, &PlasmaCamera::settingsChanged, m_cameraWorker, &Worker::setSettings);

    m_cameraThread->start();
}

PlasmaCamera::~PlasmaCamera()
{
    // optional
    stop();

    // set the thread to quit then wait for it to finish
    // shutdown in worker will emit finished() which is connected to quit() in the thread
    // BlockingQueuedConnection allows the Q_EMIT in shutdown to go through while QueuedConnection with wait does not
    QMetaObject::invokeMethod(m_cameraWorker, "shutdown", Qt::BlockingQueuedConnection);
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

    const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();
    if (cameras.empty())
    {
        qDebug() << "Failed to find any cameras";
        return false;
    }

    auto match = std::find_if(cameras.begin(), cameras.end(), [&](const std::shared_ptr<libcamera::Camera> &camera) {
        return camera->id() == m_cameraName;
    });
    if (match == cameras.end())
    {
        qDebug() << "Failed to find desired camera: " << m_cameraName;
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
    for (const auto &camera : m_cameraManager->cameras()) {
        devices.push_back( QString::fromStdString(camera->id()) );
    }

    return devices;
}

QList<QString> PlasmaCamera::getCameraDevicesName() const
{
    // camera names are more human-readable strings
    QList<QString> devices;
    for (const auto &camera : m_cameraManager->cameras()) {
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
    return m_cameraName;
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
    QMetaObject::invokeMethod(m_cameraWorker, "capture", Qt::QueuedConnection);

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

float PlasmaCamera::fps() const
{
    return m_fps;
}

void PlasmaCamera::setActive(const bool active)
{
    if (isActive() != active) {
        m_active = active;

        if (active) {
            // if we are currently not in Ready then wait until we setState to Ready to start the camera
            if (m_state != State::Ready) {
                return;
            }

            startCamera();

        } else {
            // if we are currently not in Running then wait until we setState to Running to stop the camera
            if (m_state != State::Running) {
                return;
            }

            stopCamera();
        }

        // if the state change request m_active cannot be handled right now
        // it will be handled once setState sets the state to the Ready/Running
    }
}

void PlasmaCamera::setCameraDevice(const QString &cameraDevice)
{
    // set the camera name then attempt to use that camera if possible
    if (cameraDevice != m_cameraName) {
        const std::string cameraDeviceString = cameraDevice.toStdString();

        // since m_cameraManager is set up in the class init we should always have it
        const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();
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
        m_cameraName = cameraDevice;

        // only when we are running can we switch cameras
        if (m_state == State::Running)
        {
            // by going to the ready state if m_active we will start up using m_cameraName
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
    // if the m_cameraName is empty then default to the first one
    if (m_cameraName.isEmpty()) {
        const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();
        if (cameras.empty())
            qDebug() << "Failed to find any cameras";

        // if no camera is set then get the first camera
        for (const auto &camera : cameras)
            qDebug() << "Found camera: " << camera->id();

        m_cameraName = QString::fromStdString(cameras[0]->id());
    }

    m_camera = m_cameraManager->get(m_cameraName.toStdString());
    acquire();

    // only emit that we have changed camera once we
    Q_EMIT cameraDeviceChanged(m_cameraName);

    // we are relying on Worker to free the previous camera
    // this is because there are some things it needs to stop before it can stop the camera
    QMetaObject::invokeMethod(m_cameraWorker, "setCamera", Qt::QueuedConnection, Q_ARG(const std::shared_ptr<libcamera::Camera>, m_camera));
}

void PlasmaCamera::stopCamera()
{
    setState(State::Stopping);

    // clear the current camera
    m_cameraName.clear();
    m_camera.reset();

    // after camera is stopped worker will set this state to ready
    QMetaObject::invokeMethod(m_cameraWorker, "unsetCamera", Qt::QueuedConnection);
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

    // get camera controls
    const libcamera::ControlInfoMap &infoMap = m_camera->controls();
    qInfo() << "Acquired" << infoMap.size() << "controls";

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
    // Actual camera teardown is done in Worker.
    // This is to avoid possible errors if stopping is done out of order.

    m_cameraFormat = QCameraFormat();
}
