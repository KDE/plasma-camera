// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>
#include <QDebug>
#include <QImage>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QMetaType>
#include <QCameraFormat>

#include <memory>
#include <libcamera/libcamera.h>
#include <libcamera/framebuffer.h>

#include "converter.h"
#include "image.h"
#include "settings.h"

// needed to pass shared_ptr via signals
Q_DECLARE_METATYPE(const std::shared_ptr<libcamera::Camera>)


class Worker : public QObject
{
	Q_OBJECT

public:
	explicit Worker();
	~Worker() override;

Q_SIGNALS:
	void ready();
	void running();
    void finished(); // Emit this signal to shut down this thread

    // Error handling
    void errorChanged();
    void errorOccurred(const QString &errorString);

    // Get supported features
    void cameraFormatChanged();
    void supportedFeaturesChanged();

    // Get frames
    // TODO: https://stackoverflow.com/questions/8455887/stack-object-qt-signal-and-parameter-as-reference
    //		- might not work properly for QQueue unless copy constructor is done recursively
    //		- it looks like slots with queued connection might work really nicely?
    void viewFinderFrame(const QImage &frame); // connect to this to get new frames
    void stillCaptureFrames(const QQueue<QImage> &frames); // connect to this to get captures

public Q_SLOTS:
    // Camera
    void init(); // worker startup
    void shutdown(); // worker shutdown

    // We need to get the shared pointer because of FrameBufferAllocator
    void setCamera(std::shared_ptr<libcamera::Camera> camera);

    /*!
     * Remove the camera and cleanup.
     * \param deleted - Whether the camera object has already been removed from libcamera
     */
    void unsetCamera();

    // Photo capture
    void capture();

    void setSettings(const Settings &settings);

private Q_SLOTS:
    void setError(int libcameraError);
    void setError(const QString &errorString);
    void unsetError();

    // Photo capture
    void requestNextFrame(); // triggered by timer, queues a frame request to libcamera
    void processRequestDataAndEmit(); // triggered when libcamera processes the current data into a QImage and emits

private:
	enum class State
	{
        None, // Init
        Ready, // Worker has started, waiting for camera
        Running, // Camera is active
        SwitchingCamera, // While the camera source is switching
        Stopping, // Shutting down permanently
    };

    enum class CaptureMode {
        None,
        ViewFinder,
        StillImage,
        RawImage,
        Video,
    };

    void startViewFinder();
    void stopViewFinder();

    void setState(State state);
    void setMode(CaptureMode mode);

    void configure();

    static const QList<libcamera::PixelFormat> &getNativeFormats();
    int setFormat(const QSize &size, const libcamera::PixelFormat &format, unsigned int stride);

    /*
     * How to get frames from libcamera
     * 1. Queue requests to libcamera
     * 2. Whenever libcamera finishes a request, it will call the appropriate requestComplete
     * 3. requestComplete will queue that request onto the doneQueue_ then issue newData signal
     * 4. newData will be processed by the appropriate processing method, then buffer will be placed on freeQueue_
     * 5. Once the timer expires, we move the free buffer to the libcamera request queue
     */
    int createRequests();

    void requestComplete(libcamera::Request *request);
    void processRequestData(); // convert buffers to QImage stored at m_image

    bool m_error = false;
    QString m_errorString;

    // viewfinder
    QTimer *m_framePollTimer = nullptr;

    State m_state = State::None;
    CaptureMode m_mode = CaptureMode::None;

    // internals
    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<libcamera::CameraConfiguration> m_config;

    QSize m_size;
    QImage m_image;
    Converter m_converter;
    libcamera::PixelFormat m_format;

    // Queue of images for capturing a photo
    QMutex m_stillCaptureFramesMutex;
    QQueue<QImage> m_stillCaptureFrames;

    libcamera::Stream *m_stream{};
    libcamera::FrameBufferAllocator *m_allocator{};
    std::map<libcamera::FrameBuffer *, std::unique_ptr<Image>> m_mappedBuffers;
    std::vector<std::unique_ptr<libcamera::Request>> m_requests;

    // Queue of finished requests with data, waiting for processing
    QQueue<libcamera::Request *> m_doneQueue;

    // Queue of finished requests that have been processed, and can be reused for new requests
    QQueue<libcamera::Request *> m_freeQueue;

    // We require mutexes because the libcamera process lives on a different thread.
    // We also we don't want to clog up the libcamera thread with the processing.
    QMutex m_doneMutex;
    QMutex m_freeMutex;

    Settings m_settings;
};
