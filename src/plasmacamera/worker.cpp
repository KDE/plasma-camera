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
    if (m_state == State::Running)
    {
        unsetCamera();
    }

    // terminate the worker
    Q_EMIT finished();
}

void Worker::setCamera(const std::shared_ptr<libcamera::Camera>& camera)
{
    switch (m_state)
    {
    case State::Ready:
        qDebug() << "starting viewfinder";

        // if state is ready then go to running
        camera_ = camera;
        startViewFinder();

        setState(State::Running);
        setMode(CaptureMode::ViewFinder);
        break;

    case State::Running:
        qDebug() << "restarting viewfinder";
        setMode(CaptureMode::None);

        // if state is running then switch out the camera
        stopViewFinder();
        camera_ = camera;
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
    if (m_state == State::Running)
    {
        setState(State::Stopping);

        stopViewFinder();

        setState(State::Ready);
        setMode(CaptureMode::None);
    }
}

void Worker::capture()
{
    stillCaptureFrames_.clear();
    setMode(CaptureMode::StillImage);
}

void Worker::setSettings(const Settings& settings)
{
    settings_ = settings;

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
    switch (libcameraError)
    {
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
         const QMutexLocker lock(&freeMutex_);
         if (freeQueue_.isEmpty())
         {
             return;
             // return QImage();  // set null image?
         }

         request = freeQueue_.dequeue();
     }

     request->reuse(libcamera::Request::ReuseBuffers);
     camera_->queueRequest(request);
}

void Worker::processRequestDataAndEmit()
{
    processRequestData();
    Q_EMIT viewFinderFrame(image_.copy());

    if (m_mode == CaptureMode::StillImage)
    {
        const QMutexLocker lock(&mutex_);

        if (stillCaptureFrames_.length() >= 5)
            return;

        stillCaptureFrames_.enqueue(image_.copy());

        if (stillCaptureFrames_.length() == 5)
        {
            // qDebug() << "stillCaptureFrames_" << stillCaptureFrames_.length();
            setMode(CaptureMode::ViewFinder);
            Q_EMIT stillCaptureFrames(stillCaptureFrames_);
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
    config_ = camera_->generateConfiguration({ libcamera::StreamRole::Viewfinder });

    // TODO: this should work but doesn't
    //  driver doesn't support it?: https://github.com/raspberrypi/rpicam-apps/issues/149
    // config_->orientation = libcamera::Orientation::Rotate0Mirror;

    // this does work
    // TODO: allow changing resolution but the camera will restart
    // TODO: it appears I cannot extra the native camera output resolutions... (make some up using percents of the default resolution)
    // config_->at(0).size = libcamera::Size(100, 100);

    // this doesn't seem to get any useful information
    // libcamera::StreamConfiguration sc = config_->at(0).formats();
    // qDebug() << sc.size.toString();
    // qDebug() << sc.toString();

    configure();

    camera_->requestCompleted.connect(this, &Worker::requestComplete);
    int ret = camera_->start();
    if (ret < 0)
    {
        setError(ret);
        return;
    }

    for (std::unique_ptr<libcamera::Request> &request : requests_) {
        ret = camera_->queueRequest(request.get());
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

    int ret = camera_->stop();
    if (ret < 0)
    {
        setError(ret);
        return;
    }
    camera_->requestCompleted.disconnect(this);

    config_.reset();

    allocator_->free(stream_);
    delete allocator_;

    mappedBuffers_.clear();
    requests_.clear();

    {
        const QMutexLocker lock(&freeMutex_);
        freeQueue_.clear();
    }

    /*
     * A CaptureEvent may have been posted before we stopped the camera,
     * but not processed yet. Clear the queue of done buffers to avoid
     * racing with the event handler.
     */
    {
        const QMutexLocker lock(&doneMutex_);
        doneQueue_.clear();
    }

    ret = camera_->release();
    if (ret < 0)
    {
        setError(ret);
        return;
    }
    camera_.reset();
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
    // handles sending the signals for state changes
    const CaptureMode previous_mode = m_mode;

    switch (mode)
    {
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
    if (!config_)
    {
        // setError(CameraError, QString::fromStdString("Config is not set up"));
        return;
    }

    libcamera::StreamConfiguration &config = config_->at(0);
    qDebug() << "Default camera configuration is: " << config.toString();

    const QList<libcamera::PixelFormat> want_formats = getNativeFormats();
    for (const libcamera::PixelFormat &format : want_formats)
    {
        qDebug() << "Desired format: " << format.toString();
    }

    std::vector<libcamera::PixelFormat> have_formats = config.formats().pixelformats();
    for (const libcamera::PixelFormat& format : have_formats)
    {
        qDebug() << "Got format: " << format.toString();
        auto match = std::find_if(have_formats.begin(), have_formats.end(),
                                  [&](const libcamera::PixelFormat& f) {return f == format;});
        if (match != have_formats.end())
        {
            config.pixelFormat = format;
            break;
        }
    }



    // TODO: allow user to override config
    //  - https://git.libcamera.org/libcamera/libcamera.git/tree/src/apps/qcam/main_window.cpp#n399



    qDebug() << "Viewfinder configuration is: " << config.toString();

    const libcamera::CameraConfiguration::Status validation = config_->validate();
    if (validation == libcamera::CameraConfiguration::Adjusted)
    {
        qInfo() << "Adjusted viewfinder configuration is: " << config.toString();
    }
    if (validation == libcamera::CameraConfiguration::Invalid)
    {
        // setError(CameraError, QString::fromStdString("Failed to validate camera configuration"));
        return;
    }



    // TODO: it might actually be possible to quick switch the config of the camera
    //  - camera_->configure can be called when the camera is in the configured state so we don't have to stop all the way
    //  - https://libcamera.org/api-html/classlibcamera_1_1Camera.html#a4c190f4c369628e5cd96770790559a26
    int ret = camera_->configure(config_.get());
    if (ret < 0)
    {
        setError(ret);
        return;
    }

    stream_ = config_->at(0).stream();

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
    image_ = QImage();
    if (!getNativeFormats().contains(format))
    {
        if (const int ret = converter_.configure(format, size, stride); ret < 0)
            return ret;

        image_ = QImage(size, QImage::Format_RGB32);
        qDebug() << "Using software format conversion from" << format.toString();
    }

    format_ = format;
    size_ = size;

    return 0;
}

int Worker::createRequests()
{
    qDebug() << "create req" << QThread::currentThread()->currentThreadId();
    // FrameBufferAllocator creates FrameBuffer instances that can be used by libcamera to store frames
    allocator_ = new libcamera::FrameBufferAllocator(camera_);

    int ret = allocator_->allocate(stream_);
    if (ret < 0) {
        qWarning() << "Can't allocate buffers";
        return ret;
    }
    const size_t allocated = allocator_->buffers(stream_).size();
    qDebug() << "Allocated" << allocated << "buffers for stream";

    // using the created buffers we create a mapping from the buffer to an image
    for (const std::unique_ptr<libcamera::FrameBuffer> &buffer : allocator_->buffers(stream_)) {
        // Map memory buffers and cache the mappings
        std::unique_ptr<Image> image =
            Image::fromFrameBuffer(buffer.get(), Image::MapMode::ReadOnly);
        assert(image != nullptr);
        mappedBuffers_[buffer.get()] = std::move(image);

        // Create request from the buffer
        std::unique_ptr<libcamera::Request> request = camera_->createRequest();
        if (!request)
        {
            qWarning() << "Can't create request";
            return ret;
        }
        ret = request->addBuffer(stream_, buffer.get());
        if (ret < 0)
        {
            qWarning() << "Can't set buffer for request";
            return ret;
        }
        qDebug() << "Added new request: " << request->toString();





        // TODO:
        libcamera::ControlList &cl = request->controls();
        // cl.set(libcamera::controls::ColourTemperature, 6000);
        // qInfo() << "Acquired " << cl.size() << " properties";
        // for (const auto &info : cl)
        // {
        //     qInfo() << "\t" << libcamera::controls::controls.at(info.first)->name() << " " << info.second.toString();
        // }
        // //
        // cl.set(libcamera::controls::AE_ENABLE, true);
        // cl.set(libcamera::controls::AeEnable, true);
        // const libcamera::ControlInfoMap &infoMap = camera_->controls();
        // qInfo() << "Acquired " << infoMap.size() << " controls";
        //
        // for (const auto &info : infoMap)
        // {
        //     qInfo() << "\t" << info.first->name() << ": "
        //     << info.second.min().toString() << " to " << info.second.max().toString() << " with default " << info.second.def().toString();
        // }
        //
        // qInfo() << "Acquired " << cl.size() << " properties";
        // for (const auto &info : cl)
        // {
        //     qInfo() << "\t" << libcamera::controls::controls.at(info.first)->name() << " " << info.second.toString();
        // }




        requests_.push_back(std::move(request));
    }

    return 0;
}

void Worker::requestComplete(libcamera::Request *request)
{
    if (request->status() == libcamera::Request::RequestCancelled)
        return;

    {
        const QMutexLocker lock(&doneMutex_);
        doneQueue_.enqueue(request);
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
        const QMutexLocker lock(&doneMutex_);
        if (doneQueue_.isEmpty())
            return;
        request = doneQueue_.dequeue();
    }

    if (!request->buffers().count(stream_))
        return;

    libcamera::FrameBuffer *buffer = request->buffers().at(stream_);
    Image *image = mappedBuffers_[buffer].get();

    size_t size = buffer->metadata().planes()[0].bytesused;
    if (getNativeFormats().contains(format_))
    {
        image_ = QImage(
            image->data(0).data(),
            size_.width(),
            size_.height(),
            static_cast<qsizetype>(size) / size_.height(),
            nativeFormats[format_]);

    } else {
        converter_.convert(image, size, &image_);
    }

    {
        const QMutexLocker lock(&freeMutex_);
        freeQueue_.enqueue(request);
    }
}





// void Worker::stopViewFinder()
// {
//     // qDebug() << QThread::currentThread()->currentThreadId();
//
//     // TODO: how to handle if a capture is being performed but we want to stop the camera?
//     //   may need to handle stopping in different way depending on previous camera state
//     setState(State::Stopping);
//
//     m_framePollTimer->stop();
//
//     // TODO: it appears that calling camera_->stop() crashs the thread somehow
//     //      so it appears we need to go back to plasmacamera to deal with stopping and releasing of cameras
//     // TODO: everything is actually properly stopped when we try to switch the camera (it appears it doesn't like being shutdown??)
//
//     // TODO: my suspicion is that maybe we need to stop calling for new images before we stop
//     //  - idea 1: the queue is overflowing (we are not properly pacing the new image calls)
//     //  - idea 2: not enough time between stopping the poll timer and attempting to stop the camera
//     //  - idea 3: we need to clear out some shared pointers
//     //  - idea 4: cm_ is never stopped
//     //  - https://git.libcamera.org/libcamera/libcamera.git/tree/src/libcamera/device_enumerator.cpp?id=5154e14b3e682eb0ad313d060ae12f95357f3a07#n161
//     //   - error message is not clear but is just saying that media device is still busy
//     //   - this is occurring in the destructor of DeviceEnumerator (wtf, error in destructor???)
//     //   - https://git.libcamera.org/libcamera/libcamera.git/tree/src/libcamera/log.cpp?id=67e791373de781a68889c8b56c7e18f3f27bd52e#n690
//     //   - not really the actual error, as is just a log message
//     //   - nvm log message could be the issue
//
//     // TODO: I found that issue
//     //   Deleting a QObject while pending events are waiting to be delivered can cause a crash.
//     //   You must not delete the QObject directly if it exists in a different thread than the one currently executing.
//     //   Use deleteLater() instead, which will cause the event loop to delete the object after all pending events have been delivered to it.
//     // TODO: shutdown methods that call
//
//     // camera->stop();
//     // allocator->free(stream);
//     // delete allocator;
//     // camera->release();
//     // camera.reset();
//     // cm->stop();
//     // TODO: shared_ptr is not thread safe
//     //  accessing the same instance from multiple threads is not allowed
//     //  however using two instances of shared_ptr that both point to the same object is find (as long as the object is thread safe)
//
//     int ret = camera_->stop();
//     if (ret < 0)
//     {
//         setError(ret);
//         return;
//     }
//     camera_->requestCompleted.disconnect(this);
//
//     config_.reset();
//
//     allocator_->free(stream_);
//     delete allocator_;
//
//     mappedBuffers_.clear();
//     requests_.clear();
//
//     {
//         const QMutexLocker lock(&freeMutex_);
//         freeQueue_.clear();
//     }
//     qDebug() << "a";
//
//     /*
//      * A CaptureEvent may have been posted before we stopped the camera,
//      * but not processed yet. Clear the queue of done buffers to avoid
//      * racing with the event handler.
//      */
//     {
//         const QMutexLocker lock(&doneMutex_);
//         doneQueue_.clear();
//     }
//
//     ret = camera_->release();
//     if (ret < 0)
//     {
//         setError(ret);
//         return;
//     }
//     camera_.reset();
//
//     m_state = State::Ready;
//     m_mode = CaptureMode::None;
// }

// bool Worker::startViewFinder()
// {
//     if (m_state != State::Acquired)
//     {
//         // qInfo() << "Camera state is" << m_state << "while should be Acquired, unable to start viewfinder";
//         return false;
//     }
//
//     /*
//      * https://libcamera.org/api-html/namespacelibcamera.html#a295d1f5e7828d95c0b0aabc0a8baac03
//      * The StillCapture and Viewfinder roles both generate a config that uses the same resolution
//      * while the documentation states that the image will be captured in higher quality I prefer
//      * the Viewfinder config because it appears that the higher resource usage of StillCapture
//      * makes app more unstable (it could also be related to the demands of restarting the camera)
//      */
//     config_ = camera_->generateConfiguration({ libcamera::StreamRole::Viewfinder });
//     configure();
//
//     camera_->requestCompleted.connect(this, &Worker::requestComplete);
//     // camera_->requestCompleted.connect(this, &Camera::viewfinderRequestComplete);
//
//     for (std::unique_ptr<libcamera::Request> &request : requests_) {
//         int ret = camera_->queueRequest(request.get());
//         if (ret < 0)
//         {
//             setError(ret);
//             return false;
//         }
//     }
//     m_mode = CaptureMode::ViewFinder;
//     m_framePollTimer->start();
//     return true;
// }

// bool Worker::stopViewFinder()
// {
//     // TODO: how to handle if a capture is being performed but we want to stop the camera?
//
//     m_framePollTimer->stop();
//
//     int ret = camera_->stop();
//     if (ret < 0)
//     {
//         setError(ret);
//         return false;
//     }
//     m_state = State::Acquired;
//
//     camera_->requestCompleted.disconnect(this);
//
//     config_.reset();
//
//     delete allocator_;
//
//     mappedBuffers_.clear();
//     requests_.clear();
//
//     {
//         const QMutexLocker lock(&freeMutex_);
//         freeQueue_.clear();
//     }
//
//     /*
//      * A CaptureEvent may have been posted before we stopped the camera,
//      * but not processed yet. Clear the queue of done buffers to avoid
//      * racing with the event handler.
//      */
//     {
//         const QMutexLocker lock(&doneMutex_);
//         doneQueue_.clear();
//     }
//     m_mode = CaptureMode::None;
//
//     return true;
// }

// Camera::Error Worker::error() const
// {
//     return m_error;
// }

// QString Worker::errorString() const
// {
//     return QString::fromStdString("Test Success!");
//     return m_errorString;
// }

// Worker::Features Worker::supportedFeatures() const
// {
//     return m_features;
// }

// QCameraFormat Worker::cameraFormat() const
// {
//     return m_cameraFormat;
// }

// void Worker::setCameraFormat(const QCameraFormat& cameraFormat)
// {
//     if (m_cameraFormat != cameraFormat)
//     {
//         m_cameraFormat = cameraFormat;
//         Q_EMIT cameraFormatChanged();
//     }
// }

// bool Worker::isActive() const
// {
//     return m_state == State::Acquired
//         || m_state == State::Configured
//         || m_state == State::Running;
// }

// bool Worker::isAvailable() const
// {
//     return m_state == State::Running;
// }
// QImage PlasmaCamera::getViewfinderFrame()
// {
//      libcamera::Request *request;
//      {
//          const QMutexLocker lock(&freeMutex_);
//          if (freeQueue_.isEmpty())
//          {
//              return QImage();  // set null image?
//          }
//
//          request = freeQueue_.dequeue();
//      }
//
//      request->reuse(libcamera::Request::ReuseBuffers);
//      camera_->queueRequest(request);
//
//      return viewfinderFrame_;
// }

// QQueue<QImage> PlasmaCamera::getCaptureFrames()
// {
//     // TODO: currently this trusts that this was called after the captureCompelete signal is emitted
//     //      do we need to any sort of handling for when this isn't that case?
//     return stillCaptureFrames_;
// }

// QImage *Camera::getCurViewfinderFrame()
// {
//     libcamera::Request *request;
//     {
//         const QMutexLocker lock(&freeMutex_);
//         if (freeQueue_.isEmpty())
//             return nullptr;
//         request = freeQueue_.dequeue();
//     }
//
//     request->reuse(libcamera::Request::ReuseBuffers);
//     camera_->queueRequest(request);
//
//     return &viewfinderFrame_;
// }

// void Camera::capture()
// {
//     stopViewFinder();
//     startCaptureImage();
// }

// QQueue<QImage> *Camera::getStillCaptureFrames()
// {
//     return &stillCaptureFrames_;
// }

// void Worker::setActive(bool active)
// {
//     if (m_active != active)
//     {
//         if ((active && acquire() && startViewFinder()) ||
//             (!active && stopViewFinder() && release()))
//         {
//             // TODO: depending on capture mode need to stop viewfinder (or not stop viewfinder)
//             m_active = active;
//             Q_EMIT activeChanged(active);
//         }
//     }
// }

// void PlasmaCamera::release()
// {
//     // this will hot handle telling working to stop using camera
//     // TODO: do we want to try to completely stop using current camera before starting switching to other camera
//
//     // TODO: state change
//     int ret = m_camera->release();
//     if (ret < 0)
//     {
//         // TODO
//         // setError(ret);
//         return;
//     }
//     m_camera.reset();
// }

// void PlasmaCamera::start()
// {
//     setActive(true);
//     // if (m_state != State::Ready)
//     //     return;
//     //
//     // // acquire the first camera and start streaming
//     // const std::vector<std::shared_ptr<libcamera::Camera>> cameras = cm_->cameras();
//     // if (cameras.empty())
//     // {
//     //     qDebug() << "Failed to find any cameras";
//     //     // setError();
//     //     return;
//     // }
//     // for (const auto &camera : cameras)
//     //     qDebug() << "Found camera: " << camera->id();
//     //
//     // m_camera_name = QString::fromStdString(cameras[0]->id());
//     // setCameraDevice(m_camera_name);
//
//     // TODO: delete
//     // qDebug() << "Using first camera: " << m_camera_name;
//     //
//     // // do we need to emit the camera changed signal since the thread was created during init?
//     // m_camera = cm_->get(m_camera_name.toStdString());
//     //
//     // acquire(); // acquire the current camera_device
//     //
//     // // TODO: this should only occur if we are confident it works and that we are actually giving it a new camera
//     // QMetaObject::invokeMethod(camera_worker_, "setCamera", Qt::QueuedConnection,
//     //     Q_ARG(const std::shared_ptr<libcamera::Camera>, m_camera));
//
//     // m_state = State::Acquired;
// }

// void PlasmaCamera::stop()
// {
//     setActive(false);
//     // // TODO: switch case fall through behaviour
//     //
//     // Q_EMIT activeChanged(false);
//     // Q_EMIT availableChanged(false);
//     // if (m_state == State::Ready)
//     // {
//     //     m_state = State::Stopping;
//     //
//     // } else if (m_state == State::Acquired)
//     // {
//     //     m_state = State::Stopping;
//     //
//     // } else if (m_state == State::Running)
//     // {
//     //     m_state = State::Stopping;
//     //
//     // }
//     //
//     // // m_camera->stop();
//     //
//     // qDebug() << QThread::currentThread()->currentThreadId();
//     // QMetaObject::invokeMethod(camera_worker_, "stop", Qt::QueuedConnection);
//     //
// }
