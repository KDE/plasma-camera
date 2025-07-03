// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSize>

#include "plasmacameramanager.h"
#include "plasmacamera/path.h"



PlasmaCameraManager::PlasmaCameraManager(QObject *parent) : QObject(parent)
{
    /*
     * m_session links m_videoInput and m_audioInput to m_recorder
     * QVideoFrameInput m_videoInput will just regurgitate the stream of frames we feed it
     *
     * readyToSendVideoFrame signal is not matched with the recorder fps
     * - connect(&m_videoInput, &QVideoFrameInput::readyToSendVideoFrame, this, &PlasmaCameraManager::processVideoFrame);
     * - with a basic test I got ~200 readyToSendVideoFrames signals in 1 second (but we are recording at 24 fps)
     */
    m_session.setVideoFrameInput(&m_videoInput);
    m_videoFrameTimer.setInterval(static_cast<int>(1000.0f / m_fps));
    connect(&m_videoFrameTimer, &QTimer::timeout, this, &PlasmaCameraManager::processVideoFrame);

    m_format.setFileFormat(QMediaFormat::FileFormat::MPEG4);
    m_format.setVideoCodec(QMediaFormat::VideoCodec::H264);
    m_format.setAudioCodec(QMediaFormat::AudioCodec::MP3);
}

PlasmaCameraManager::~PlasmaCameraManager() = default;



PlasmaCameraManager::Error PlasmaCameraManager::error() const
{
    return m_error;
}

QString PlasmaCameraManager::errorString() const
{
    return m_errorString;
}

bool PlasmaCameraManager::isReadyForCapture() const
{
    return m_readyForCapture;
}

PlasmaCameraManager::FileFormat PlasmaCameraManager::fileFormat() const
{
    return m_fileFormat;
}

QList<PlasmaCameraManager::FileFormat> PlasmaCameraManager::supportedFileFormats() const
{
    return QList<FileFormat>({JPEG} );
}

PlasmaCameraManager::Quality PlasmaCameraManager::quality() const
{
    return m_quality;
}

PlasmaCamera *PlasmaCameraManager::plasmaCamera() const
{
    return m_plasmaCamera;
}

QVideoSink *PlasmaCameraManager::videoSink() const
{
    return m_videoSink;
}

QMediaRecorder *PlasmaCameraManager::recorder() const
{
    return m_recorder;
}

QAudioInput *PlasmaCameraManager::audioInput() const
{
    return m_audioInput;
}

float PlasmaCameraManager::fps() const
{
    return m_fps;
}

int PlasmaCameraManager::captureImage()
{
    m_imageCaptureFileName.clear();
    return captureImageInternal();
}

int PlasmaCameraManager::captureImageToFile(const QString &location)
{
    m_imageCaptureFileName = location;
    return captureImageInternal();
}

void PlasmaCameraManager::setReadyForCapture(const bool ready)
{
    qDebug() << "PlasmaCameraManager::setReadyForCapture" << ready;

    if (m_readyForCapture != ready) {
        m_readyForCapture = ready;
        Q_EMIT readyForCaptureChanged(ready);
    }
}

void PlasmaCameraManager::setFileFormat(const FileFormat fileFormat)
{
    if (m_fileFormat != fileFormat)
    {
        m_fileFormat = fileFormat;
        Q_EMIT fileFormatChanged();
    }
}

void PlasmaCameraManager::setQuality(const Quality quality)
{
    if (m_quality != quality)
    {
        m_quality = quality;
        Q_EMIT qualityChanged();
    }
}

void PlasmaCameraManager::setPlasmaCamera(PlasmaCamera *plasmaCamera)
{
    if (plasmaCamera != m_plasmaCamera)
    {
        if (m_plasmaCamera)
        {
            disconnect(plasmaCamera, &PlasmaCamera::availableChanged, this, &PlasmaCameraManager::setReadyForCapture);
            disconnect(plasmaCamera, &PlasmaCamera::settingsChanged, this, &PlasmaCameraManager::setSettings);
            disconnect(plasmaCamera, &PlasmaCamera::viewFinderFrame, this, &PlasmaCameraManager::processViewfinderFrame);
            disconnect(plasmaCamera, &PlasmaCamera::stillCaptureFrames, this, &PlasmaCameraManager::processCaptureImage);

            disconnect(this, &PlasmaCameraManager::fpsChanged, plasmaCamera, &PlasmaCamera::setFps);
        }

        m_plasmaCamera = plasmaCamera;

        if (m_plasmaCamera)
        {
            connect(plasmaCamera, &PlasmaCamera::availableChanged, this, &PlasmaCameraManager::setReadyForCapture);
            connect(plasmaCamera, &PlasmaCamera::settingsChanged, this, &PlasmaCameraManager::setSettings);
            connect(plasmaCamera, &PlasmaCamera::viewFinderFrame, this, &PlasmaCameraManager::processViewfinderFrame);
            connect(plasmaCamera, &PlasmaCamera::stillCaptureFrames, this, &PlasmaCameraManager::processCaptureImage);

            connect(this, &PlasmaCameraManager::fpsChanged, plasmaCamera, &PlasmaCamera::setFps);
        }

        Q_EMIT plasmaCameraChanged();
    }
}

void PlasmaCameraManager::setVideoSink(QVideoSink *videoSink)
{
    if (m_videoSink != videoSink)
    {
        m_videoSink = videoSink;
        Q_EMIT videoSinkChanged();
    }
}

void PlasmaCameraManager::setRecorder(QMediaRecorder *recorder)
{
    if (m_recorder != recorder)
    {
        m_recorder = recorder;

        // TODO: move this to a independent function so we can call it whenever we change a value

        // Set the format on the recorder
        recorder->setMediaFormat(m_format);

        // Set the quality
        recorder->setQuality(QMediaRecorder::Quality::NormalQuality);

        // Set the resolution
        // recorder->setVideoResolution(QSize(1280, 720));

        // Set the frame rate
        recorder->setVideoFrameRate(30.0);

        // Attempt to disable hardware acceleration (does it even work?)
        recorder->setProperty("hw-accel", false);

        // Set encoding paramaters
        // recorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);

        // connect(recorder, &QMediaRecorder::errorOccurred,
        //     [](QMediaRecorder::Error error, const QString &errorString) {
        //         qDebug() << "Recording error:" << error << errorString; });
        // recorder->setMetaData()

        m_session.setRecorder(recorder);
        connect(m_recorder, &QMediaRecorder::recorderStateChanged, this, [this]() {
            if (m_recorder->recorderState() == QMediaRecorder::RecordingState) {
                m_videoFrameTimer.start();
            } else {
                m_videoFrameTimer.stop();
            }
        });

        Q_EMIT recorderChanged();
    }
}

void PlasmaCameraManager::setAudioInput(QAudioInput *audioInput)
{
    if (m_audioInput != audioInput)
    {
        m_audioInput = audioInput;
        Q_EMIT audioInputChanged();
    }
}

void PlasmaCameraManager::setFps(const float fps)
{
    if (m_fps != fps)
    {
        m_fps = fps;
        Q_EMIT fpsChanged(fps);
    }
}

void PlasmaCameraManager::setError(const int id, const Error error, const QString& errorString)
{
    m_error = error;
    m_errorString = errorString;

    qWarning() << m_errorString;

    Q_EMIT errorChanged();
    Q_EMIT errorOccurred(id, m_error, m_errorString);
}

void PlasmaCameraManager::unsetError()
{
    m_error = NoError;
    m_errorString.clear();
}

void PlasmaCameraManager::setSettings(const Settings& settings)
{
    m_settings = settings;

    // TODO: get the new values out and set
}

void PlasmaCameraManager::processViewfinderFrame(const QImage &image)
{
    if (image.isNull())
        return;

    // might need to convert to a format compatible with QVideoFrame
    /*
     * if (image.format() != QImage::Format_RGBA8888) {
     *     image = image.convertToFormat(QImage::Format_RGBA8888);
     * }
     */

    m_currentFrame = image;
    m_videoFrame = QVideoFrame(image);

    // if (m_ismobile)
    // {
    //     // TODO: rotation in CameraPage is useless? (set this in libcamera instead)
    //     video_frame.setRotation(QtVideo::Rotation::Clockwise270);
    // }

    // TODO: I'm not sure what this code is actually doing?
    // if(!m_videoFrame.isValid() || !m_videoFrame.map(QVideoFrame::WriteOnly)){
    //     qWarning() << "QVideoFrame is not valid or not writable";
    //     return;
    // }
    // QImage::Format image_format = QVideoFrameFormat::imageFormatFromPixelFormat(m_videoFrame.pixelFormat());
    // if(image_format == QImage::Format_Invalid){
    //     qWarning() << "It is not possible to obtain image format from the pixel format of the videoframe";
    //     return;
    // }
    // m_videoFrame.unmap();

    m_videoSink->setVideoFrame(m_videoFrame);
}

void PlasmaCameraManager::processCaptureImage(const QQueue<QImage> &frames)
{
    // TODO: should this be moved to a different thread (saving an image can be a heavy task)
    // qDebug() << "PlasmaCameraManager::processCaptureImage";

    const QString fileName = PlasmaLibcameraUtils::generateFileName(
        m_imageCaptureFileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));
    QFile file(fileName);

    QImage image = frames.head();

    // if (m_ismobile)
    // {
    //     QTransform transform;
    //     transform.rotate(270);
    //     image = image.transformed(transform);
    // }

    // TODO: set format and quality
    qDebug() << "saving image to " << fileName;
    const bool res = image.save(&file, "JPEG", 50);
    if (!res)
    {
        setError(
            0,
            ResourceError,
            QString::fromStdString("Could not save to file: %1").arg(fileName));
    }

    // TODO: use QImageWriter to write EXIF data
    //  - https://doc.qt.io/qt-6/qimagewriter.html

    Q_EMIT imageSaved(m_imageCaptureNum, fileName);
}

void PlasmaCameraManager::processVideoFrame()
{
    // Whenever m_videoFrame is ready for another frame, send the current one
    // bool res;
    // QMetaObject::invokeMethod(&m_videoInput, "sendVideoFrame", Qt::AutoConnection,
    //     Q_RETURN_ARG(bool, res),
    //     Q_ARG(const QVideoFrame&, m_videoFrame));
    // bool sendVideoFrame(const QVideoFrame &frame);
    // TODO: unsure which is better
    //  - it appears that the phone is having trouble encoding the video
    //  - using cpu capabilities: ARMv8 NEON
    //  - INFO Debayer debayer_cpu:788 Processed 30 frames in 3078733us, 102624 us/frame
    //      - processing 30 frames in 3 seconds (not great?)
    //  - Killed
    //  - ffmpeg sees some kind of error in the produced files so refuses to play them
    // m_videoInput.sendVideoFrame(QVideoFrame(m_currentFrame));
    m_videoInput.sendVideoFrame(m_videoFrame);
}

int PlasmaCameraManager::captureImageInternal()
{
    const int newImageCaptureId = m_imageCaptureNum + 1;

    if (!isReadyForCapture()) {
        setError(
            newImageCaptureId,
            NotReadyError,
            QString::fromStdString("Camera was not ready to take a picture"));
        return newImageCaptureId;
    }

    m_imageCaptureNum = newImageCaptureId;
    m_plasmaCamera->captureImage();

    return newImageCaptureId;
}
