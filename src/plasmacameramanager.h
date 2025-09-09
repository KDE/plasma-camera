// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <KAboutData>
#include <QOrientationSensor>
#include <QQmlEngine>
#include <QtMultimedia>

#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>

#include "plasmacamera.h"

/*!
 * \brief Manages the libcamera camera controller (similar to QMediaCaptureSession)
 */

class PlasmaCameraManager : public QObject
{
    Q_OBJECT
    // error handling
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

    // preforming an image capture
    Q_PROPERTY(bool readyForCapture READ isReadyForCapture NOTIFY readyForCaptureChanged)

    Q_PROPERTY(QList<FileFormat> supportedFileFormats READ supportedFileFormats CONSTANT)
    Q_PROPERTY(FileFormat fileFormat READ fileFormat WRITE setFileFormat NOTIFY fileFormatChanged)
    Q_PROPERTY(Quality quality READ quality WRITE setQuality NOTIFY qualityChanged)

    // preview and video recording
    Q_PROPERTY(PlasmaCamera *plasmaCamera READ plasmaCamera WRITE setPlasmaCamera NOTIFY plasmaCameraChanged)
    Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(QMediaRecorder *recorder READ recorder WRITE setRecorder NOTIFY recorderChanged)
    Q_PROPERTY(bool audioRecordingEnabled READ audioRecordingEnabled WRITE setAudioRecordingEnabled NOTIFY audioRecordingEnabledChanged)
    Q_PROPERTY(QAudioInput *audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(float videoRecordingFps READ videoRecordingFps WRITE setVideoRecordingFps NOTIFY videoRecordingFpsChanged)
    Q_PROPERTY(Resolution videoResolution READ videoResolution WRITE setVideoResolution NOTIFY videoResolutionChanged)
    Q_PROPERTY(VideoCodec videoCodec READ videoCodec WRITE setVideoCodec NOTIFY videoCodecChanged)

    Q_PROPERTY(bool isRecordingVideo READ isRecordingVideo NOTIFY isRecordingVideoChanged)
    Q_PROPERTY(bool isSavingVideo READ isSavingVideo NOTIFY isSavingVideoChanged)

public:
    explicit PlasmaCameraManager(QObject *parent = nullptr);
    ~PlasmaCameraManager() override;

    enum Error
    {
        NoError,
        NotReadyError,
        ResourceError,
        OutOfSpaceError,
        NotSupportedFeatureError,
        FormatError,
    };
    Q_ENUM(Error)

    enum Quality
    {
        VeryLowQuality = 0,
        LowQuality = 1,
        NormalQuality = 2,
        HighQuality = 3,
        VeryHighQuality = 4,
    };
    Q_ENUM(Quality)

    enum FileFormat {
        UnspecifiedFormat,
        JPEG,
        PNG,
        WebP,
        Tiff,
    };
    Q_ENUM(FileFormat)

    enum Resolution {
        ResolutionAuto = 0,
        Resolution540p = 1,
        Resolution720p = 2,
        Resolution1080p = 3,
        Resolution1440p = 4,
        Resolution2160p = 5
    };
    Q_ENUM(Resolution)

    enum VideoCodec {
        H264 = 0,
        H265 = 1,
        MPEG2 = 2
    };
    Q_ENUM(VideoCodec)

    Error error() const;
    QString errorString() const;

    /*!
     * Whether an image is currently being captured (for photo).
     */
    bool isReadyForCapture() const;

    QList<FileFormat> supportedFileFormats() const;
    FileFormat fileFormat() const;
    Quality quality() const;

    PlasmaCamera *plasmaCamera() const;
    QVideoSink *videoSink() const;
    QMediaRecorder *recorder() const;
    QAudioInput *audioInput() const;
    float videoRecordingFps() const;
    Resolution videoResolution() const;
    VideoCodec videoCodec() const;

    bool audioRecordingEnabled() const;
    void setAudioRecordingEnabled(bool enabled);

    bool isRecordingVideo() const;
    bool isSavingVideo() const;

    /*!
     * Start a new video recording.
     */
    Q_INVOKABLE void startRecordingVideo();

    /*!
     * Stop the current video recording, and save to disk.
     */
    Q_INVOKABLE void stopRecordingVideo();

Q_SIGNALS:
    void errorChanged();
    void errorOccurred(int id, Error error, const QString &errorString);

    void readyForCaptureChanged(bool ready);
    void metaDataChanged();

    void fileFormatChanged();
    void qualityChanged();

    void plasmaCameraChanged();
    void videoSinkChanged();
    void recorderChanged();
    void audioInputChanged();
    void videoRecordingFpsChanged(float fps);
    void videoResolutionChanged();
    void videoCodecChanged();
    void imageSaved(int id, const QString &fileName);

    void audioRecordingEnabledChanged();
    void isRecordingVideoChanged();
    void isSavingVideoChanged();

    void framesDropped();

public Q_SLOTS:
    /*!
     * Take a photo with the camera.
     */
    int captureImage();

    /*!
     * Take a photo with the camera, and store it at a specific location.
     */
    int captureImageToFile(const QString &location = QString());

    void setReadyForCapture(bool ready);

    void setFileFormat(FileFormat fileFormat);
    void setQuality(Quality quality);
    void setPlasmaCamera(PlasmaCamera *plasmaCamera);
    void setVideoSink(QVideoSink *videoSink);
    void setRecorder(QMediaRecorder *recorder);
    void setAudioInput(QAudioInput *audioInput);
    void setVideoRecordingFps(float fps);
    void setVideoResolution(Resolution resolution);
    void setVideoCodec(VideoCodec codec);

private Q_SLOTS:
    // error handling
    void setError(int id, Error error, const QString &errorString);
    void unsetError();

    // settings
    void updateRecorderSettings();
    void setSettings(const Settings &settings);

    // image processing
    void processViewfinderFrame(const QImage &image);
    void processCaptureImage(const QQueue<QImage> &frames);
    void processVideoFrame();
    void updateFromOrientationSensor();

    void setIsSavingVideo(bool isSavingVideo);

private:
    /*!
     * Loop over the list of available audio input devices and set it to be the
     * audio input for video recording.
     *
     * Returns whether an audio input was able to be found and set.
     */
    bool findAndSetDefaultRecordingDevice();

    /*!
     * Writes EXIF data based on the current camera state to the given fileName.
     */
    void writeExifData(const QString &fileName);

    void setAudioRecordingEnabledInternal(bool enabled);

    int captureImageInternal();

    /*!
     * The amount of degrees to rotate the final output frame by.
     * This adds the camera orientation and the device orientation degrees together.
     */
    float outputOrientationDegrees() const;

    // error handling
    Error m_error;
    QString m_errorString;

    // preforming an image capture
    bool m_readyForCapture = false;

    // saving to file
    FileFormat m_fileFormat;
    Quality m_quality = Quality::NormalQuality;
    QList<FileFormat> m_supportedFileFormats;

    // preview and video recording
    PlasmaCamera *m_plasmaCamera = nullptr;

    QImage m_currentFrame;
    QVideoFrame m_videoFrame;
    QVideoSink *m_videoSink = nullptr;
    int m_frameRecordingCount = 0;
    bool m_frameRecorded = false;

    QMediaCaptureSession m_session;
    QTimer m_videoFrameTimer;
    QMediaFormat m_format;
    QVideoFrameInput m_videoInput;
    QMediaRecorder *m_recorder = nullptr;
    QAudioInput *m_audioInput = nullptr;
    float m_videoRecordingFps = 24.0f;
    bool m_audioRecordingEnabled = true;
    Resolution m_videoResolution = Resolution::ResolutionAuto;
    VideoCodec m_videoCodec = VideoCodec::H264;

    // Device orientation for correct rotation
    QOrientationSensor m_orientationSensor;
    float m_orientationSensorDegrees = 0.0f;

    // internal
    Settings m_settings;

    int m_imageCaptureNum = 0;
    QString m_imageCaptureFileName;

    bool m_isSavingVideo = false;
    bool m_droppedFrames = false;
};
