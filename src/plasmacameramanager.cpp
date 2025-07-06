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
    connect(&m_videoFrameTimer, &QTimer::timeout, this, &PlasmaCameraManager::processVideoFrame);
    connect(&m_orientationSensor, &QOrientationSensor::readingChanged, this, &PlasmaCameraManager::updateFromOrientationSensor);

    m_orientationSensor.start();

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

float PlasmaCameraManager::videoRecordingFps() const
{
    return m_videoRecordingFps;
}

bool PlasmaCameraManager::isRecordingVideo() const
{
    if (!m_recorder) {
        return false;
    }
    return m_recorder->recorderState() == QMediaRecorder::RecordingState && !m_isSavingVideo;
}

bool PlasmaCameraManager::isSavingVideo() const
{
    return m_isSavingVideo;
}

void PlasmaCameraManager::startRecordingVideo()
{
    if (!m_recorder) {
        return;
    }
    m_recorder->record();
}

void PlasmaCameraManager::stopRecordingVideo()
{
    if (!m_recorder) {
        return;
    }
    m_recorder->stop();
    setIsSavingVideo(true);

    m_videoFrameTimer.stop();
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
    if (m_fileFormat != fileFormat) {
        m_fileFormat = fileFormat;
        Q_EMIT fileFormatChanged();
    }
}

void PlasmaCameraManager::setQuality(const Quality quality)
{
    if (m_quality != quality) {
        m_quality = quality;
        Q_EMIT qualityChanged();

        updateRecorderSettings();
    }
}

void PlasmaCameraManager::setPlasmaCamera(PlasmaCamera *plasmaCamera)
{
    if (plasmaCamera != m_plasmaCamera) {
        if (m_plasmaCamera) {
            disconnect(plasmaCamera, &PlasmaCamera::availableChanged, this, &PlasmaCameraManager::setReadyForCapture);
            disconnect(plasmaCamera, &PlasmaCamera::settingsChanged, this, &PlasmaCameraManager::setSettings);
            disconnect(plasmaCamera, &PlasmaCamera::viewFinderFrame, this, &PlasmaCameraManager::processViewfinderFrame);
            disconnect(plasmaCamera, &PlasmaCamera::stillCaptureFrames, this, &PlasmaCameraManager::processCaptureImage);
        }

        m_plasmaCamera = plasmaCamera;

        if (m_plasmaCamera) {
            connect(plasmaCamera, &PlasmaCamera::availableChanged, this, &PlasmaCameraManager::setReadyForCapture);
            connect(plasmaCamera, &PlasmaCamera::settingsChanged, this, &PlasmaCameraManager::setSettings);
            connect(plasmaCamera, &PlasmaCamera::viewFinderFrame, this, &PlasmaCameraManager::processViewfinderFrame);
            connect(plasmaCamera, &PlasmaCamera::stillCaptureFrames, this, &PlasmaCameraManager::processCaptureImage);
        }

        Q_EMIT plasmaCameraChanged();
    }
}

void PlasmaCameraManager::setVideoSink(QVideoSink *videoSink)
{
    if (m_videoSink != videoSink) {
        m_videoSink = videoSink;
        Q_EMIT videoSinkChanged();
    }
}

void PlasmaCameraManager::setRecorder(QMediaRecorder *recorder)
{
    if (m_recorder == recorder) {
        return;
    }

    m_recorder = recorder;
    updateRecorderSettings();

    // TODO: Set metadata
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

        // Once the recorder changes to any state, it no longer is saving video
        // - Saving video happens in the gap between calling m_recorder->stop() and stopping state
        setIsSavingVideo(false);

        Q_EMIT isRecordingVideoChanged();
    });

    Q_EMIT recorderChanged();
}

void PlasmaCameraManager::updateRecorderSettings()
{
    if (!m_recorder) {
        return;
    }

    // Set the format on the recorder
    m_recorder->setMediaFormat(m_format);

    // Set the quality
    switch (m_quality) {
    case PlasmaCameraManager::VeryLowQuality:
        m_recorder->setQuality(QMediaRecorder::Quality::VeryLowQuality);
        break;
    case PlasmaCameraManager::LowQuality:
        m_recorder->setQuality(QMediaRecorder::Quality::LowQuality);
        break;
    case PlasmaCameraManager::NormalQuality:
        m_recorder->setQuality(QMediaRecorder::Quality::NormalQuality);
        break;
    case PlasmaCameraManager::HighQuality:
        m_recorder->setQuality(QMediaRecorder::Quality::HighQuality);
        break;
    case PlasmaCameraManager::VeryHighQuality:
        m_recorder->setQuality(QMediaRecorder::Quality::VeryHighQuality);
        break;
    }

    // Set the resolution
    // m_recorder->setVideoResolution(QSize(1280, 720));

    // Set the frame rate
    m_recorder->setVideoFrameRate(m_videoRecordingFps);

    // Set encoding paramaters
    // m_recorder->setEncodingMode(QMediaRecorder::ConstantQualityEncoding);

    // Set frame timer polling rate
    m_videoFrameTimer.setInterval(static_cast<int>(1000.0f / m_videoRecordingFps));
}

void PlasmaCameraManager::setAudioInput(QAudioInput *audioInput)
{
    if (m_audioInput != audioInput) {
        m_audioInput = audioInput;
        Q_EMIT audioInputChanged();
    }
}

void PlasmaCameraManager::setVideoRecordingFps(const float fps)
{
    if (m_videoRecordingFps != fps) {
        m_videoRecordingFps = fps;
        Q_EMIT videoRecordingFpsChanged(fps);

        updateRecorderSettings();
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
}

void PlasmaCameraManager::setIsSavingVideo(bool isSavingVideo)
{
    if (isSavingVideo == m_isSavingVideo) {
        return;
    }
    m_isSavingVideo = isSavingVideo;
    Q_EMIT isSavingVideoChanged();

    // Since it depends on m_isSavingVideo
    Q_EMIT isRecordingVideoChanged();
}

void PlasmaCameraManager::processViewfinderFrame(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    m_currentFrame = image;
    QVideoFrame frame = QVideoFrame(image);

    // Do software rotation
    // From testing, most drivers don't seem to implement hardware orientation correctly (CameraConfiguration::orientation),
    // it either isn't supported or just crashes. So we will just use software rotation for best hardware compatibility.
    switch (m_plasmaCamera->softwareRotationDegrees()) {
    case 0:
        break;
    case 90:
        frame.setRotation(QtVideo::Rotation::Clockwise90);
        break;
    case 180:
        frame.setRotation(QtVideo::Rotation::Clockwise180);
        break;
    case 270:
        frame.setRotation(QtVideo::Rotation::Clockwise270);
        break;
    }

    // Mirror output if requested
    frame.setMirrored(m_plasmaCamera->mirrorOutput());

    // Add to sink
    m_videoSink->setVideoFrame(frame);

    // Set as video frame for video recording
    m_videoFrame = std::move(frame);
    m_frameRecorded = false;
}

void PlasmaCameraManager::processCaptureImage(const QQueue<QImage> &frames)
{
    const QString fileName = PlasmaLibcameraUtils::generateFileName(
        m_imageCaptureFileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));
    QFile file(fileName);

    QImage image = frames.head();

    // Flip image if requested
    if (m_plasmaCamera->mirrorOutput()) {
        image = image.mirrored(); // Deprecated, replace with image.flipped() once we are on Qt 6.9
    }

    // Do software rotation
    // From testing, most drivers don't seem to implement hardware orientation correctly (CameraConfiguration::orientation),
    // it either isn't supported or just crashes. So we will just use software rotation for best hardware compatibility.
    QTransform transform;
    double degrees = m_plasmaCamera->softwareRotationDegrees() + m_orientationSensorDegrees;
    transform.rotate(degrees);
    image = image.transformed(transform);

    // TODO: set format and quality
    qDebug() << "saving image to " << fileName;
    const bool res = image.save(&file, "JPEG", 50);
    if (!res) {
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
    // Set the time in the video that the frame should show up.
    // This allows for frames to be dropped without impacting video speed.
    // TODO: This causes severe frame corruption on devices that drop frames, so it's disabled for now
    //
    // int frameLengthMicroseconds = qRound(1000.0 * 1000.0 / m_videoRecordingFps);
    // m_videoFrame.setStartTime(m_frameRecordingCount * frameLengthMicroseconds);
    // m_videoFrame.setEndTime((m_frameRecordingCount + 1) * frameLengthMicroseconds);

    // Attempt to send frame
    bool success = m_videoInput.sendVideoFrame(m_videoFrame);
    if (success) {
        m_frameRecorded = true;
    }
    m_frameRecordingCount++;
}

void PlasmaCameraManager::updateFromOrientationSensor()
{
    QOrientationReading *reading = m_orientationSensor.reading();
    if (!reading) {
        return;
    }

    qInfo() << "Orientation sensor reported update:" << reading->orientation();

    switch (reading->orientation()) {
    case QOrientationReading::Orientation::TopUp:
        m_orientationSensorDegrees = 0.0f;
        break;
    case QOrientationReading::Orientation::TopDown:
        m_orientationSensorDegrees = 180.0f;
        break;
    case QOrientationReading::Orientation::LeftUp:
        m_orientationSensorDegrees = 90.0f;
        break;
    case QOrientationReading::Orientation::RightUp:
        m_orientationSensorDegrees = 270.0f;
        break;
    case QOrientationReading::Orientation::FaceUp:
    case QOrientationReading::Orientation::FaceDown:
    case QOrientationReading::Orientation::Undefined:
    default:
        m_orientationSensorDegrees = 0.0f;
        break;
    }
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
