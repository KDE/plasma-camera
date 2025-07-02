// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include "worker.h"


Worker::Worker() = default;

Worker::~Worker() = default;

void Worker::init()
{
    // start init after we have moved to the other thread
    m_framePollTimer = new QTimer(this);
    m_framePollTimer->setInterval(1000/24); // manually set 24 fps
    connect(m_framePollTimer, &QTimer::timeout, this, &Worker::requestNextFrame);

    setState(State::Ready);
}

void Worker::shutdown()
{
    if (m_state == State::Running) {
        unsetCamera();
    }

    // terminate the worker
    Q_EMIT finished();
}

void Worker::setCamera(const std::shared_ptr<libcamera::Camera>& camera)
{
    switch (m_state) {
    case State::Ready:
        qDebug() << "starting viewfinder";

        // if state is ready then go to running
        m_camera = camera;
        startViewFinder();

        setState(State::Running);
        setMode(CaptureMode::ViewFinder);
        break;

    case State::Running:
        qDebug() << "restarting viewfinder";
        setMode(CaptureMode::None);

        // if state is running then switch out the camera
        stopViewFinder();
        m_camera = camera;
        startViewFinder();

        setMode(CaptureMode::ViewFinder);
        break;

    default:
        setError(QString::fromStdString("Attempting to set a camera but state is not Ready or Running"));
        // set error
        break;
    }
}

void Worker::unsetCamera()
{
    // we are responsible for cleaning up the camera
    if (m_state == State::Running) {
        setState(State::Stopping);

        stopViewFinder();

        setState(State::Ready);
        setMode(CaptureMode::None);
    }
}

void Worker::capture()
{
    m_stillCaptureFrames.clear();
    setMode(CaptureMode::StillImage);
}

void Worker::setSettings(const Settings& settings)
{
    m_settings = settings;

    // TODO: invoke changes to the requests
}

void Worker::setError(const QString &errorString)
{
    m_error = true;
    m_errorString = errorString;

    qWarning() << m_errorString;

    Q_EMIT errorChanged();
    Q_EMIT errorOccurred(m_errorString);
}

void Worker::setError(const int libcameraError)
{
    m_error = true;
    switch (libcameraError) {
    case -ENOMEM:
        m_errorString = QString::fromStdString("No buffer memory was available to handle the request");
        break;
    case -EACCES:
        m_errorString = QString::fromStdString("The camera is not in a state where it can be configured");
        break;
    case -EBUSY:
        m_errorString = QString::fromStdString("The camera is running and can't be released");
        break;
    case -EXDEV:
        m_errorString = QString::fromStdString("The request does not belong to this camera");
        break;
    case -ENODEV:
        m_errorString = QString::fromStdString("The camera has been disconnected from the system");
        break;
    case -EINVAL:
        m_errorString = QString::fromStdString("The configuration is not valid");
        break;
    default:
        m_errorString = QString::fromStdString("Unknown error: " + std::to_string(libcameraError));
        break;
    }

    qWarning() << m_errorString;

    Q_EMIT errorChanged();
    Q_EMIT errorOccurred(m_errorString);
}

void Worker::unsetError()
{
    m_error = false;
    m_errorString.clear();
}

void Worker::requestNextFrame()
{
    // this code used to be Camera::getViewfinderFrame()
    // how it makes more sense for the camera to continue to get frames even when the frames are not called for
    // - untie the image capture from how fast we can display the frames

     libcamera::Request *request;
     {
         const QMutexLocker lock(&m_freeMutex);
         if (m_freeQueue.isEmpty()) {
             return;
             // return QImage();  // set null image?
         }

         request = m_freeQueue.dequeue();
     }

     request->reuse(libcamera::Request::ReuseBuffers);
     m_camera->queueRequest(request);
}

void Worker::processRequestDataAndEmit()
{
    processRequestData();
    Q_EMIT viewFinderFrame(m_image.copy());

    if (m_mode == CaptureMode::StillImage)
    {
        const QMutexLocker lock(&m_mutex);

        if (m_stillCaptureFrames.length() >= 5)
            return;

        m_stillCaptureFrames.enqueue(m_image.copy());

        if (m_stillCaptureFrames.length() == 5) {
            // qDebug() << "m_stillCaptureFrames" << m_stillCaptureFrames.length();
            setMode(CaptureMode::ViewFinder);
            Q_EMIT stillCaptureFrames(m_stillCaptureFrames);
        }
    }
}



void Worker::startViewFinder()
{
    /*
     * https://libcamera.org/api-html/namespacelibcamera.html#a295d1f5e7828d95c0b0aabc0a8baac03
     * The StillCapture and Viewfinder roles both generate a config that uses the same resolution
     * while the documentation states that the image will be captured in higher quality I prefer
     * the Viewfinder config because it appears that the higher resource usage of StillCapture
     * makes app more unstable (it could also be related to the demands of restarting the camera)
     */
    m_config = m_camera->generateConfiguration({libcamera::StreamRole::Viewfinder});

    // TODO: this should work but doesn't
    //  driver doesn't support it?: https://github.com/raspberrypi/rpicam-apps/issues/149
    // m_config->orientation = libcamera::Orientation::Rotate0Mirror;

    // this does work
    // TODO: allow changing resolution but the camera will restart
    // TODO: it appears I cannot extra the native camera output resolutions... (make some up using percents of the default resolution)
    // m_config->at(0).size = libcamera::Size(100, 100);

    // this doesn't seem to get any useful information
    // libcamera::StreamConfiguration sc = m_config->at(0).formats();
    // qDebug() << sc.size.toString();
    // qDebug() << sc.toString();

    configure();

    m_camera->requestCompleted.connect(this, &Worker::requestComplete);
    int ret = m_camera->start();
    if (ret < 0)
    {
        setError(ret);
        return;
    }

    for (std::unique_ptr<libcamera::Request> &request : m_requests) {
        ret = m_camera->queueRequest(request.get());
        if (ret < 0)
        {
            setError(ret);
            return;
        }
    }

    m_framePollTimer->start();
}

void Worker::stopViewFinder()
{
    m_framePollTimer->stop();

    int ret = m_camera->stop();
    if (ret < 0)
    {
        setError(ret);
        return;
    }
    m_camera->requestCompleted.disconnect(this);

    m_config.reset();

    m_allocator->free(m_stream);
    delete m_allocator;

    m_mappedBuffers.clear();
    m_requests.clear();

    {
        const QMutexLocker lock(&m_freeMutex);
        m_freeQueue.clear();
    }

    /*
     * A CaptureEvent may have been posted before we stopped the camera,
     * but not processed yet. Clear the queue of done buffers to avoid
     * racing with the event handler.
     */
    {
        const QMutexLocker lock(&m_doneMutex);
        m_doneQueue.clear();
    }

    ret = m_camera->release();
    if (ret < 0)
    {
        setError(ret);
        return;
    }
    m_camera.reset();
}

void Worker::setState(const State state)
{
    // handles sending the signals for state changes
    const State previous_state = m_state;

    switch (state)
    {
    case State::None:
        // after initial init it should not be possible to go back to None
        // m_state = State::None;
        break;

    case State::Ready:
        m_state = State::Ready;
        switch (previous_state)
        {
        case State::None:
            Q_EMIT ready();
            break;
        case State::Stopping:
            Q_EMIT ready();
            break;
        default:
            qWarning() << "Transitioned from state other than None or Stopping to Ready";
            break;
        }
        break;

    case State::Running:
        m_state = State::Running;
        switch (previous_state)
        {
        case State::Ready:
            break;
        default:
            qWarning() << "Transitioned from state other than Ready to Running";
            break;
        }
        break;

    case State::Stopping:
        m_state = State::Stopping;
        break;
    }
}

void Worker::setMode(CaptureMode mode)
{
    // Handles sending the signals for state changes
    const CaptureMode previous_mode = m_mode;

    switch (mode) {
    case CaptureMode::None:
        m_mode = CaptureMode::None;
        break;

    case CaptureMode::ViewFinder:
        m_mode = CaptureMode::ViewFinder;
        switch (previous_mode)
        {
        case CaptureMode::None:
            Q_EMIT running();
            break;
        case CaptureMode::StillImage:
            Q_EMIT running();
            break;
        default:
            qWarning() << "Transition from state other than None or StillImage to ViewFinder";
            break;
        }
        break;

    case CaptureMode::StillImage:
        m_mode = CaptureMode::StillImage;
        break;

    case CaptureMode::RawImage:
        m_mode = CaptureMode::RawImage;
        break;

    case CaptureMode::Video:
        m_mode = CaptureMode::Video;
        break;
    }
}

void Worker::configure()
{
    if (!m_config) {
        // setError(CameraError, QString::fromStdString("Config is not set up"));
        return;
    }

    libcamera::StreamConfiguration &config = m_config->at(0);
    qDebug() << "Default camera configuration is: " << config.toString();

    const QList<libcamera::PixelFormat> want_formats = getNativeFormats();
    for (const libcamera::PixelFormat &format : want_formats) {
        qDebug() << "Desired format: " << format.toString();
    }

    std::vector<libcamera::PixelFormat> have_formats = config.formats().pixelformats();
    for (const libcamera::PixelFormat &format : have_formats) {
        qDebug() << "Got format: " << format.toString();
        auto match = std::find_if(have_formats.begin(), have_formats.end(),
                                  [&](const libcamera::PixelFormat& f) {return f == format;});
        if (match != have_formats.end()) {
            config.pixelFormat = format;
            break;
        }
    }

    // TODO: allow user to override config
    //  - https://git.libcamera.org/libcamera/libcamera.git/tree/src/apps/qcam/main_window.cpp#n399

    qDebug() << "Viewfinder configuration is: " << config.toString();

    const libcamera::CameraConfiguration::Status validation = m_config->validate();
    if (validation == libcamera::CameraConfiguration::Adjusted) {
        qInfo() << "Adjusted viewfinder configuration is: " << config.toString();
    }
    if (validation == libcamera::CameraConfiguration::Invalid) {
        // setError(CameraError, QString::fromStdString("Failed to validate camera configuration"));
        return;
    }

    // TODO: it might actually be possible to quick switch the config of the camera
    //  - m_camera->configure can be called when the camera is in the configured state so we don't have to stop all the way
    //  - https://libcamera.org/api-html/classlibcamera_1_1Camera.html#a4c190f4c369628e5cd96770790559a26
    int ret = m_camera->configure(m_config.get());
    if (ret < 0)
    {
        setError(ret);
        return;
    }

    m_stream = m_config->at(0).stream();

    ret = setFormat(
        QSize(static_cast<int>(config.size.width), static_cast<int>(config.size.height)),
        config.pixelFormat,
        config.colorSpace.value_or(libcamera::ColorSpace::Sycc),
        config.stride);
    if (ret < 0)
    {
        setError(ret);
        return;
    }

    ret = createRequests();
    if (ret < 0)
    {
        setError(ret);
        return;
    }
}

static const QMap<libcamera::PixelFormat, QImage::Format> nativeFormats
{
    {libcamera::formats::ABGR8888, QImage::Format_RGBX8888},
    {libcamera::formats::XBGR8888, QImage::Format_RGBX8888},
    {libcamera::formats::ARGB8888, QImage::Format_RGB32},
    {libcamera::formats::XRGB8888, QImage::Format_RGB32},
    {libcamera::formats::RGB888, QImage::Format_BGR888},
    {libcamera::formats::BGR888, QImage::Format_RGB888},
    {libcamera::formats::RGB565, QImage::Format_RGB16},
};

const QList<libcamera::PixelFormat>& Worker::getNativeFormats()
{
    static const QList<libcamera::PixelFormat> formats = nativeFormats.keys();
    return formats;
}

int Worker::setFormat(
    const QSize& size,
    const libcamera::PixelFormat& format,
    const libcamera::ColorSpace& colorSpace,
    const unsigned int stride)
{
    m_image = QImage();
    if (!getNativeFormats().contains(format))
    {
        if (const int ret = m_converter.configure(format, size, stride); ret < 0)
            return ret;

        m_image = QImage(size, QImage::Format_RGB32);
        qDebug() << "Using software format conversion from" << format.toString();
    }

    m_format = format;
    m_size = size;

    return 0;
}

int Worker::createRequests()
{
    qDebug() << "create req" << QThread::currentThread()->currentThreadId();

    // FrameBufferAllocator creates FrameBuffer instances that can be used by libcamera to store frames
    m_allocator = new libcamera::FrameBufferAllocator(m_camera);

    int ret = m_allocator->allocate(m_stream);
    if (ret < 0) {
        qWarning() << "Can't allocate buffers";
        return ret;
    }

    const size_t allocated = m_allocator->buffers(m_stream).size();
    qDebug() << "Allocated" << allocated << "buffers for stream";

    // using the created buffers we create a mapping from the buffer to an image
    for (const std::unique_ptr<libcamera::FrameBuffer> &buffer : m_allocator->buffers(m_stream)) {
        // Map memory buffers and cache the mappings
        std::unique_ptr<Image> image =
            Image::fromFrameBuffer(buffer.get(), Image::MapMode::ReadOnly);
        Q_ASSERT(image);

        m_mappedBuffers[buffer.get()] = std::move(image);

        // Create request from the buffer
        std::unique_ptr<libcamera::Request> request = m_camera->createRequest();
        if (!request) {
            qWarning() << "Can't create request";
            return ret;
        }
        ret = request->addBuffer(m_stream, buffer.get());
        if (ret < 0) {
            qWarning() << "Can't set buffer for request";
            return ret;
        }
        qDebug() << "Added new request: " << request->toString();

        m_requests.push_back(std::move(request));
    }

    return 0;
}

void Worker::requestComplete(libcamera::Request *request)
{
    if (request->status() == libcamera::Request::RequestCancelled)
        return;

    {
        const QMutexLocker lock(&m_doneMutex);
        m_doneQueue.enqueue(request);
    }

    // we are currently on the libcamera thread so we need to go back to the Worker thread
    // emits signal to process the request on another thread
    // to avoid clogging up the libcamera thread
    QMetaObject::invokeMethod(this, "processRequestDataAndEmit", Qt::QueuedConnection);
}

void Worker::processRequestData()
{
    libcamera::Request *request;
    {
        const QMutexLocker lock(&m_doneMutex);
        if (m_doneQueue.isEmpty()) {
            return;
        }
        request = m_doneQueue.dequeue();
    }

    if (!request->buffers().count(m_stream))
        return;

    libcamera::FrameBuffer *buffer = request->buffers().at(m_stream);
    Image *image = m_mappedBuffers[buffer].get();

    size_t size = buffer->metadata().planes()[0].bytesused;
    if (getNativeFormats().contains(m_format)) {
        m_image = QImage(image->data(0).data(), m_size.width(), m_size.height(), static_cast<qsizetype>(size) / m_size.height(), nativeFormats[m_format]);
    } else {
        m_converter.convert(image, size, &m_image);
    }

    {
        const QMutexLocker lock(&m_freeMutex);
        m_freeQueue.enqueue(request);
    }
}
