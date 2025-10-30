// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QSize>

#include "camerasettings.h"
#include "plasmacamera/path.h"
#include "plasmacameramanager.h"

#include <exiv2/exiv2.hpp>
#include <memory>

PlasmaCameraManager::PlasmaCameraManager(QObject *parent)
    : QObject(parent)
{
    // Read saved configuration settings from kconfig
    setVideoRecordingFps(CameraSettings::self()->videoRecordingFps());
    setQuality(static_cast<Quality>(CameraSettings::self()->videoRecordingQuality()));
    setVideoResolution(static_cast<Resolution>(CameraSettings::self()->videoResolution()));
    setVideoCodec(static_cast<VideoCodec>(CameraSettings::self()->videoCodec()));
    setFilenamePattern(static_cast<FilenamePattern>(CameraSettings::self()->filenamePattern()));
    setOutputPath(static_cast<OutputPath>(CameraSettings::self()->outputPath()));

    // Set to default audio input
    setAudioRecordingEnabledInternal(true);

    // m_session links m_videoInput and m_audioInput to m_recorder
    m_session.setVideoFrameInput(&m_videoInput);
    connect(&m_videoFrameTimer, &QTimer::timeout, this, &PlasmaCameraManager::processVideoFrame);
    connect(&m_orientationSensor, &QOrientationSensor::readingChanged, this, &PlasmaCameraManager::updateFromOrientationSensor);

    m_orientationSensor.start();

    // Listen to audio inputs changing, and change audio device if needed
    QMediaDevices *mediaDevices = new QMediaDevices(this);
    connect(mediaDevices, &QMediaDevices::audioInputsChanged, this, [this]() {
        if (audioRecordingEnabled()) {
            bool success = findAndSetDefaultRecordingDevice();
            if (!success) {
                setAudioRecordingEnabled(false);
            }
        }
    });
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

PlasmaCameraManager::FilenamePattern PlasmaCameraManager::filenamePattern() const
{
    return m_filenamePattern;
}

PlasmaCameraManager::OutputPath PlasmaCameraManager::outputPath() const
{
    return m_outputPath;
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

PlasmaCameraManager::Resolution PlasmaCameraManager::videoResolution() const
{
    return m_videoResolution;
}

PlasmaCameraManager::VideoCodec PlasmaCameraManager::videoCodec() const
{
    return m_videoCodec;
}

bool PlasmaCameraManager::audioRecordingEnabled() const
{
    return m_audioRecordingEnabled;
}

void PlasmaCameraManager::setAudioRecordingEnabled(bool enabled)
{
    if (enabled == m_audioRecordingEnabled) {
        return;
    }
    setAudioRecordingEnabledInternal(enabled);
}

void PlasmaCameraManager::setAudioRecordingEnabledInternal(bool enabled)
{
    if (enabled) {
        // Try to set the default recording device
        if (!findAndSetDefaultRecordingDevice()) {
            return;
        }
    } else {
        setAudioInput(nullptr);
    }

    m_audioRecordingEnabled = enabled;
    Q_EMIT audioRecordingEnabledChanged();
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

    m_droppedFrames = false; // reset

    // Set the correct rotation metadata prior to recording
    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Orientation, outputOrientationDegrees());
    metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
    m_recorder->setMetaData(metaData);

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

void PlasmaCameraManager::setFilenamePattern(const FilenamePattern filenamePattern)
{
    qDebug() << "PlasmaCameraManager::setFilenamePattern" << filenamePattern;
    if (m_filenamePattern != filenamePattern) {
        m_filenamePattern = filenamePattern;
        CameraSettings::self()->setFilenamePattern(m_filenamePattern);
        Q_EMIT filenamePatternChanged();
    }
}

void PlasmaCameraManager::setOutputPath(const OutputPath outputPath)
{
    qDebug() << "PlasmaCameraManager::setOutputPath" << outputPath;
    if (m_outputPath != outputPath) {
        m_outputPath = outputPath;
        CameraSettings::self()->setOutputPath(m_outputPath);
        Q_EMIT outputPathChanged();
    }
}

void PlasmaCameraManager::setQuality(const Quality quality)
{
    if (m_quality != quality) {
        m_quality = quality;
        Q_EMIT qualityChanged();

        CameraSettings::self()->setVideoRecordingQuality(static_cast<int>(quality));
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

    connect(recorder, &QMediaRecorder::errorOccurred, [this](QMediaRecorder::Error error, const QString &errorString) {
        qDebug() << "Recording error:" << error << errorString;
        setError(0, Error::ResourceError, errorString);
    });

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

    // We use mp4 container with mp3 audio stream for simplicity
    m_format.setFileFormat(QMediaFormat::FileFormat::MPEG4);
    m_format.setAudioCodec(QMediaFormat::AudioCodec::MP3);

    switch (m_videoCodec) {
    case PlasmaCameraManager::MPEG2:
        // For slower devices
        m_format.setVideoCodec(QMediaFormat::VideoCodec::MPEG2);
        break;
    case PlasmaCameraManager::H264:
        // Good for most cases
        m_format.setVideoCodec(QMediaFormat::VideoCodec::H264);
        break;
    case PlasmaCameraManager::H265:
        m_format.setVideoCodec(QMediaFormat::VideoCodec::H265);
        break;
    default:
        m_format.setVideoCodec(QMediaFormat::VideoCodec::H264);
        break;
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
    switch (m_videoResolution) {
    case ResolutionAuto:
        m_recorder->setVideoResolution(QSize());
        break;
    case Resolution540p:
        m_recorder->setVideoResolution(QSize(960, 540));
        break;
    case Resolution720p:
        m_recorder->setVideoResolution(QSize(1280, 720));
        break;
    case Resolution1080p:
        m_recorder->setVideoResolution(QSize(1920, 1080));
        break;
    case Resolution1440p:
        m_recorder->setVideoResolution(QSize(2560, 1440));
        break;
    case Resolution2160p:
        m_recorder->setVideoResolution(QSize(3840, 2160));
        break;
    }

    // Set the frame rate (let recorder decide)
    // m_recorder->setVideoFrameRate(m_videoRecordingFps);
    m_recorder->setVideoFrameRate(0);

    // Set frame timer polling rate
    m_videoFrameTimer.setInterval(static_cast<int>(1000.0f / m_videoRecordingFps));
}

void PlasmaCameraManager::setAudioInput(QAudioInput *audioInput)
{
    if (m_audioInput != audioInput) {
        m_audioInput = audioInput;
        Q_EMIT audioInputChanged();

        m_session.setAudioInput(m_audioInput);
    }
}

bool PlasmaCameraManager::findAndSetDefaultRecordingDevice()
{
    // Initialize default audio input if we are enabling it
    QAudioDevice audioDevice;

    for (const QAudioDevice &device : QMediaDevices::audioInputs()) {
        if (device.isDefault() && device.mode() == QAudioDevice::Input) {
            audioDevice = device;
        }
    }

    if (QMediaDevices::audioInputs().isEmpty() || audioDevice.isNull()) {
        qWarning() << "No audio devices found for recording";
        return false;
    }

    QAudioInput *audioInput = new QAudioInput();
    audioInput->setDevice(std::move(audioDevice));
    setAudioInput(audioInput);

    return true;
}

void PlasmaCameraManager::setVideoRecordingFps(const float fps)
{
    if (m_videoRecordingFps == fps) {
        return;
    }

    m_videoRecordingFps = fps;
    CameraSettings::self()->setVideoRecordingFps(fps);
    Q_EMIT videoRecordingFpsChanged(fps);

    updateRecorderSettings();
}

void PlasmaCameraManager::setVideoResolution(Resolution resolution)
{
    if (m_videoResolution == resolution) {
        return;
    }

    m_videoResolution = resolution;
    CameraSettings::self()->setVideoResolution(static_cast<int>(resolution));
    Q_EMIT videoResolutionChanged();

    // TODO: persist video resolution, but it needs to be per-camera
    updateRecorderSettings();
}

void PlasmaCameraManager::setVideoCodec(VideoCodec codec)
{
    if (m_videoCodec == codec) {
        return;
    }

    m_videoCodec = codec;
    CameraSettings::self()->setVideoCodec(static_cast<int>(codec));
    Q_EMIT videoCodecChanged();

    updateRecorderSettings();
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
    QDateTime now = QDateTime::currentDateTime();
    if ( (m_filenamePattern==1) || (m_filenamePattern==2) ) {
        m_imageCaptureFileName = now.toString(QStringLiteral("yyyyMMddHHmmss"));
    }
    const QString fileName = PlasmaLibcameraUtils::generateFileName(
        m_imageCaptureFileName, QLatin1String("jpg"), m_filenamePattern, m_outputPath);
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
    transform.rotate(outputOrientationDegrees());
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

    // Write EXIF data
    writeExifData(fileName);

    Q_EMIT imageSaved(m_imageCaptureNum, fileName);
}

QString getExifOffsetFormat(int offsetSeconds)
{
    char sign = (offsetSeconds >= 0) ? '+' : '-';
    int absOffsetSeconds = qAbs(offsetSeconds);
    int hours = absOffsetSeconds / 3600;
    int minutes = (absOffsetSeconds % 3600) / 60;

    return QStringLiteral("%1%2:%3").arg(sign).arg(hours, 2, 10, QLatin1Char('0')).arg(minutes, 2, 10, QLatin1Char('0'));
}

void PlasmaCameraManager::writeExifData(const QString &fileName)
{
    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(fileName.toStdString());
        if (!image) {
            return;
        }

        image->readMetadata();

        Exiv2::ExifData &exifData = image->exifData();

        // Write time and date related information
        QDateTime now = QDateTime::currentDateTime();

        std::string exifDateTime = now.toString(QStringLiteral("yyyy:MM:dd HH:mm:ss")).toStdString();
        exifData["Exif.Image.DateTime"] = exifDateTime;
        exifData["Exif.Photo.DateTimeOriginal"] = exifDateTime;
        exifData["Exif.Photo.DateTimeDigitized"] = exifDateTime;

        std::string exifOffset = getExifOffsetFormat(now.offsetFromUtc()).toStdString();
        exifData["Exif.Photo.OffsetTime"] = exifOffset;
        exifData["Exif.Photo.OffsetTimeOriginal"] = exifOffset;
        exifData["Exif.Photo.OffsetTimeDigitized"] = exifOffset;

        image->setExifData(exifData);
        image->writeMetadata();
    } catch (const Exiv2::Error &e) {
        qWarning() << "Error writing EXIF data (exiv2 Error): " << e.what();
        return;
    } catch (const std::exception &e) {
        qWarning() << "Error writing EXIF data: " << e.what();
        return;
    }
}

void PlasmaCameraManager::processVideoFrame()
{
    // Set the time in the video that the frame should show up.
    // This allows for frames to be dropped without impacting video speed.
    // TODO: This causes severe frame corruption on devices that drop frames, how do we make it smoother?
    int frameLengthMicroseconds = qRound(1000.0 * 1000.0 / m_videoRecordingFps);
    m_videoFrame.setStartTime(m_frameRecordingCount * frameLengthMicroseconds);
    m_videoFrame.setEndTime((m_frameRecordingCount + 1) * frameLengthMicroseconds - 1);

    // Attempt to send frame
    bool success = m_videoInput.sendVideoFrame(m_videoFrame);
    if (success) {
        m_frameRecorded = true;
    } else {
        if (!m_droppedFrames) {
            Q_EMIT framesDropped();
            m_droppedFrames = true;
        }
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

float PlasmaCameraManager::outputOrientationDegrees() const
{
    return m_plasmaCamera->softwareRotationDegrees() + m_orientationSensorDegrees;
}
