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


// requires C++20
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
	void finished(); // emit this signal to shut down this thread

	// error handling
	void errorChanged();
	void errorOccurred(const QString &errorString);

	// start camera
	void activeChanged(bool newActive);
	void availableChanged(bool newAvailable);

	// get supported features
	void cameraFormatChanged();
	void supportedFeaturesChanged();

	// get frames
	// TODO: https://stackoverflow.com/questions/8455887/stack-object-qt-signal-and-parameter-as-reference
	//		- might not work properly for QQueue unless copy constructor is done recursively
	//		- it looks like slots with queued connection might work really nicely?
	void viewFinderFrame(const QImage &frame);				// connect to this to get new frames
	void stillCaptureFrames(const QQueue<QImage> &frames);  // connect to this to get captures

	// void gotData();  // new libcamera data stored on doneQueue_

public Q_SLOTS:
	// camera
	void init();		// worker startup
	void shutdown();	// worker shutdown

	// we need to get the shared pointer because of FrameBufferAllocator
	void setCamera(const std::shared_ptr<libcamera::Camera>& camera);
	void unsetCamera();
	// void setCameraFormat(const QCameraFormat& cameraFormat);
	// void setCameraConfig(const std::string& config, int val);

	// photo
	void capture();
	// void startCaptureImage();
	// void startCaptureImageRaw();

	// settings
	void setSettings(const Settings& settings);

private Q_SLOTS:
	// error
	void setError(int libcameraError);
	void setError(const QString &errorString);
	void unsetError();

	// photo
	void requestNextFrame();			// triggered by timer, queues a frame request to libcamera
	void processRequestDataAndEmit();	// triggered when libcamera processes the current data into a QImage and emits

private:
	enum class State
	{
		None,		// init
		Ready,		// worker has started
		Running,	// camera is active
		Stopping,	// shutting down permanently
	};

	enum class CaptureMode
	{
		None,
		ViewFinder,
		StillImage,
		RawImage,
		Video,
	};

	// error
	bool m_error = false;
	QString m_errorString;

	// viewfinder
	void startViewFinder();
	void stopViewFinder();
    QTimer *m_framePollTimer = nullptr;

	// state
	void setState(State state);
	void setMode(CaptureMode mode);
	State m_state = State::None;
	CaptureMode m_mode = CaptureMode::None;

	// internals
	void configure();
    std::shared_ptr<libcamera::Camera> camera_;
    std::unique_ptr<libcamera::CameraConfiguration> config_;

    static const QList<libcamera::PixelFormat> &getNativeFormats();
    int setFormat(
    	const QSize &size,
    	const libcamera::PixelFormat &format,
    	const libcamera::ColorSpace &colorSpace,
    	unsigned int stride);
    QSize size_;
    QImage image_;
    Converter converter_;
    libcamera::PixelFormat format_;

	QMutex mutex_;
	QQueue<QImage> stillCaptureFrames_;

	/*
	 * How to get frames from libcamera
	 * 1. Queue requests to libcamera
	 * 2. Whenever libcamera finishes a request, it will call the appropriate requestComplete
	 * 3. requestComplete will queue that request onto the doneQueue_ then issue newData signal
	 * 4. newData will be processed by the appropriate processing method, then buffer will be placed on freeQueue_
	 * 5. Once the timer expires, we move the free buffer to the libcamera request queue
	 */
	int createRequests();
    libcamera::Stream *stream_{};
    libcamera::FrameBufferAllocator *allocator_{};
    std::map<libcamera::FrameBuffer *, std::unique_ptr<Image>> mappedBuffers_;
	std::vector<std::unique_ptr<libcamera::Request>> requests_;

  	void requestComplete(libcamera::Request *request);
	void processRequestData();  // convert buffers to QImage stored at image_
  	// void viewfinderRequestComplete(libcamera::Request *request);
  	// void stillCaptureRequestComplete(libcamera::Request *request);
	QQueue<libcamera::Request *> doneQueue_;
	QQueue<libcamera::Request *> freeQueue_;

	/*
	 * we require mutexes because the libcamera process lives on a different thread
	 * also we don't want to clog up the libcamera thread with the processing
	 */
	QMutex doneMutex_;
	QMutex freeMutex_;

	// settings
	Settings settings_;
};
