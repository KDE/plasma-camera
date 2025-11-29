// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <unistd.h>
#include <sys/mman.h>
#include <plasmacameramanager.h>

#include "plasmacamera.h"

PlasmaCamera::PlasmaCamera(QObject *parent) : QObject(parent)
{
    // Initialize camera manager
    m_cameraManager = std::make_unique<libcamera::CameraManager>();
    if (const int ret = m_cameraManager->start(); ret != 0) {
        // if there exists two CameraManager we get undefined behavior
        qCritical() << "libcamera manager start failed:" << ret;
        return;
    }

    // Listen to camera added event (on libcamera thread)
    m_cameraManager->cameraAdded.connect(this, [this](auto camera) {
        // Handle on Qt thread
        QMetaObject::invokeMethod(this, "handleCameraAdded", Qt::QueuedConnection, Q_ARG(std::shared_ptr<libcamera::Camera>, camera));
    });

    // Listen to camera removed event (on libcamera thread)
    m_cameraManager->cameraRemoved.connect(this, [this](auto camera) {
        // Handle on Qt thread
        QMetaObject::invokeMethod(this, "handleCameraRemoved", Qt::QueuedConnection, Q_ARG(std::shared_ptr<libcamera::Camera>, camera));
    });

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
    connect(m_cameraWorker, &Worker::ready, this, [this]() {
        setState(State::Ready);
        setBusy(false);
    });
    connect(m_cameraWorker, &Worker::running, this, [this]() {
        setState(State::Running);
        setBusy(false);
    });

    // resend frames that we received from the camera worker
    connect(m_cameraWorker, &Worker::viewFinderFrame, this, &PlasmaCamera::viewFinderFrame);
    connect(m_cameraWorker, &Worker::stillCaptureFrames, this, &PlasmaCamera::stillCaptureFrames);
    connect(m_cameraWorker, &Worker::stillCaptureFrames, this, [this]() {
        // Finished taking image
        setBusy(false);
    });

    // get errors from the camera worker
    connect(m_cameraWorker, &Worker::errorOccurred, this, &PlasmaCamera::setError);

    // send new settings to the camera worker
    connect(this, &PlasmaCamera::settingsChanged, m_cameraWorker, &Worker::setSettings);

    m_cameraThread->start();
}

PlasmaCamera::~PlasmaCamera()
{
    // Cleanup signals
    m_cameraManager->cameraAdded.disconnect();
    m_cameraManager->cameraRemoved.disconnect();

    // optional
    stopCamera();

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

void PlasmaCamera::startCamera()
{
    setActive(true);
}

void PlasmaCamera::stopCamera()
{
    setActive(false);
}

bool PlasmaCamera::switchToNextCamera()
{
    if (m_state != State::Running) {
        return false;
    }

    const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();
    if (cameras.empty()) {
        qDebug() << "Failed to find any cameras";
        return false;
    }

    auto match = std::find_if(cameras.begin(), cameras.end(), [&](const std::shared_ptr<libcamera::Camera> &camera) {
        return camera->id() == m_cameraId;
    });
    if (match == cameras.end()) {
        qDebug() << "Failed to find desired camera: " << m_cameraId;
        for (const auto &camera : cameras) {
            qDebug() << "Have camera: " << camera->id();
        }

        return false;
    }

    // cyclic increment
    ++match;
    if (match == cameras.end()) {
        match = cameras.begin();
    }

    setCameraDevice( QString::fromStdString((*match)->id()) );
    return true;
}

QList<QString> PlasmaCamera::cameraDeviceIds() const
{
    // camera ids are unique strings to identify the camera devices
    QList<QString> devices;
    for (const auto &camera : m_cameraManager->cameras()) {
        devices.push_back( QString::fromStdString(camera->id()) );
    }

    return devices;
}

QList<QString> PlasmaCamera::cameraDeviceNames() const
{
    // camera names are more human-readable strings
    QList<QString> devices;
    for (const auto &camera : m_cameraManager->cameras()) {
        devices.push_back(QString::fromStdString(std::string(*camera->properties().get(libcamera::properties::Model))));
    }

    return devices;
}

bool PlasmaCamera::isAvailable() const
{
    return m_state == State::Running;
}

QString PlasmaCamera::cameraDevice() const
{
    return m_cameraId;
}

QCameraFormat PlasmaCamera::cameraFormat() const
{
    return m_cameraFormat;
}

bool PlasmaCamera::captureImage()
{
    if (m_state != State::Running) {
        return false;
    }

    setBusy(true);
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

bool PlasmaCamera::exposureValueAvailable() const
{
    return m_settings.canSetExposureValue();
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

int PlasmaCamera::softwareRotationDegrees() const
{
    return m_softwareRotationDegrees;
}

bool PlasmaCamera::mirrorOutput() const
{
    return m_mirrorOutput;
}

bool PlasmaCamera::busy() const
{
    return m_busy;
}

void PlasmaCamera::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    Q_EMIT busyChanged();
}

void PlasmaCamera::setActive(const bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;

    if (active) {
        // If we are currently not in Ready, then wait until we setState to Ready to start the camera.
        if (m_state != State::Ready) {
            return;
        }

        startCamera();

    } else {
        // If we are currently not in Running, then wait until we setState to Running to stop the camera.
        if (m_state != State::Running) {
            return;
        }

        stopCamera();
    }

    // If the state change request m_active cannot be handled right now,
    // it will be handled once setState sets the state to the Ready/Running.
}

void PlasmaCamera::setCameraDevice(const QString &cameraDeviceId)
{
    if (cameraDeviceId == m_cameraId) {
        return;
    }

    const std::string cameraDeviceString = cameraDeviceId.toStdString();

    // Since m_cameraManager is set up in the class init we should always have it
    const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();

    auto match = std::find_if(cameras.begin(), cameras.end(), [&](const auto &camera) {
        return camera->id() == cameraDeviceString;
    });
    if (match == cameras.end()) {
        qDebug() << "Failed to find desired camera: " << cameraDeviceId;
        for (const auto &camera : cameras) {
            qDebug() << "Have camera: " << camera->id();
        }

        // Ignore this request if the camera could not be found
        return;
    }

    qDebug() << "Using camera: " << cameraDeviceId;
    m_cameraId = cameraDeviceId;

    startCameraInternal();
}

void PlasmaCamera::setCameraFormat(const QCameraFormat &cameraFormat)
{
    if (cameraFormat != m_cameraFormat) {
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
    if (m_settings.canSetGain() && m_settings.trySetGain(iso / 100.0f)) {
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
    if (m_settings.canSetExposureTime() && m_settings.trySetExposureTime(exposureTime)) {
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
    if (m_settings.canSetExposureValue() && m_settings.trySetExposureValue(exposureValue)) {
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
    if (m_settings.canSetWbTemp() && m_settings.trySetWbTemp(wbTemp)) {
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
    if (m_settings.canSetContrast() && m_settings.trySetContrast(contrast)) {
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
    if (m_settings.canSetSaturation() && m_settings.trySetSaturation(saturation)) {
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
    if (m_fps != fps) {
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

void PlasmaCamera::handleCameraAdded(std::shared_ptr<libcamera::Camera> camera)
{
    Q_UNUSED(camera)

    Q_EMIT cameraDeviceListChanged();

    // Start the camera if we don't have any, but it was requested
    if (!m_camera && m_active) {
        startCameraInternal();
    }
}

void PlasmaCamera::handleCameraRemoved(std::shared_ptr<libcamera::Camera> camera)
{
    Q_EMIT cameraDeviceListChanged();

    // If this is our current camera, disable it
    if (m_camera == camera) {
        stopCameraInternal();

        // Start the next camera (if it exists)
        if (m_active) {
            startCameraInternal();
        }
    }
}

void PlasmaCamera::startCameraInternal()
{
    // If the m_cameraId is empty then default to the first one.
    if (m_cameraId.isEmpty()) {
        const std::vector<std::shared_ptr<libcamera::Camera>> cameras = m_cameraManager->cameras();

        if (cameras.empty()) {
            qDebug() << "Failed to find any cameras";
            m_camera.reset();
            m_cameraId.clear();
            return;
        }

        // if no camera is set then get the first camera
        for (const auto &camera : cameras) {
            qDebug() << "Found camera: " << camera->id();
        }

        m_cameraId = QString::fromStdString(cameras[0]->id());
    }

    // We may call startCameraInternal() multiple times on the same camera, add a check
    auto camera = m_cameraManager->get(m_cameraId.toStdString());
    if (m_camera == camera) {
        qDebug() << "Camera has already been started";
        return;
    }

    m_camera = camera;
    Q_EMIT cameraDeviceChanged(m_cameraId);

    // Attempt to acquire lock on camera
    // TODO: wait until camera becomes available before doing so?
    if (!acquire()) {
        return;
    }

    qDebug() << "Setting camera to" << m_cameraId;
    QMetaObject::invokeMethod(m_cameraWorker, "setCamera", Qt::QueuedConnection, Q_ARG(const std::shared_ptr<libcamera::Camera>, m_camera));
}

void PlasmaCamera::stopCameraInternal()
{
    setState(State::Stopping);

    // Clear the current camera
    m_cameraId.clear();
    m_camera.reset();

    // After the camera is stopped, the worker will set this state to Ready
    QMetaObject::invokeMethod(m_cameraWorker, "unsetCamera", Qt::BlockingQueuedConnection);
}

void PlasmaCamera::setState(const State state)
{
    // handles sending the signals for state changes
    // and attempting to change state based on m_active
    const State previousState = m_state;

    const bool previousAvailable = isAvailable();

    switch (state) {
    case State::None:
        // after initial init it should not be possible to go back to None
        // m_state = State::None;
        break;

    case State::Ready:
        m_state = State::Ready;
        if (m_active) {
            startCameraInternal();
        }
        break;

    case State::Running:
        m_state = State::Running;
        if (!m_active) {
            stopCameraInternal();
            break;
        }
        switch (previousState) {
        case State::Running:
        case State::Ready:
            break;
        default:
            qWarning() << "Transitioned from state other than Running, Ready, to Running";
        }
        break;

    case State::Stopping:
        m_state = State::Stopping;
        switch (previousState) {
        case State::Ready:
        case State::Running:
            break;
        default:
            qWarning() << "Transitioned from state other than Ready or Running to Stopping";
        }
        break;
    }

    if (previousAvailable != isAvailable()) {
        Q_EMIT availableChanged(isAvailable());
    }
}

bool PlasmaCamera::acquire()
{
    if (!m_camera) {
        return false;
    }

    // Attempt to acquire camera
    int ret = m_camera->acquire();
    if (ret < 0) {
        setError(QString::fromStdString("Failed to acquire camera"));
        m_camera.reset();
        return false;
    }

    // List camera properties
    const libcamera::ControlInfoMap &infoMap = m_camera->controls();
    qInfo() << "Acquired" << infoMap.size() << "controls";

    // Print camera properties
    const libcamera::ControlList &properties = m_camera->properties();
    qInfo() << "Acquired" << properties.size() << "properties";
    for (const auto &info : properties) {
        qInfo() << "\t" << properties.idMap()->at(info.first)->name() << " " << info.second.toString();
    }

    // Load camera controls into settings
    // m_camera->controls() is constant through the lifetime of m_camera, so we don't need to reload.
    m_settings.load(m_camera->controls());
    Q_EMIT settingsChanged(m_settings);

    // Find the amount of degrees needed to rotate the camera output by and populate m_softwareRotationDegrees
    std::optional<int> cameraRotation = m_camera->properties().get(libcamera::properties::Rotation);
    qInfo() << "Camera offset orientation:" << cameraRotation.value_or(0);
    switch (cameraRotation.value_or(0)) {
    case 0:
        m_softwareRotationDegrees = 0;
        break;
    case 90:
        m_softwareRotationDegrees = 270;
        break;
    case 180:
        m_softwareRotationDegrees = 180;
        break;
    case 270:
        m_softwareRotationDegrees = 90;
        break;
    }

    // Find whether the camera output needs to be mirrored (ex. selife cam)
    std::optional<int> location = m_camera->properties().get(libcamera::properties::Location);
    m_mirrorOutput = location.value_or(libcamera::properties::CameraLocationExternal) == libcamera::properties::CameraLocationFront;

    return true;
}
