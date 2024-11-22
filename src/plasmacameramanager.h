// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QQmlEngine>
#include <KAboutData>
#include <QtMultimedia>

#include <libcamera/libcamera.h>
#include <libcamera/camera_manager.h>

#include "plasmacamera.h"


/*
 * TODO:
 *  - create a worker for file saving (split off image and video saving to a different thread)
 *  - save location is currently ~/Photos (change to ~/Photos/plamsa-camera)
 *  - save location is currently ~/Videos (change to ~/Videos/plamsa-camera)
 *  - allow setting video/audio format
 *  - audio recording when recording video
 *  - ask for permission before using the camera?
 */


/*
 * Manages the libcamera camera controller (similar to QMediaCaptureSession)
 */

class PlasmaCameraManager : public QObject
{
    Q_OBJECT
    // error handling
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

    // preforming an image capture
    Q_PROPERTY(bool readyForCapture READ isReadyForCapture NOTIFY readyForCaptureChanged)

    // saving to file
    Q_PROPERTY(QList<FileFormat> supportedFileFormats READ supportedFileFormats CONSTANT)
    Q_PROPERTY(FileFormat fileFormat READ fileFormat WRITE setFileFormat NOTIFY fileFormatChanged)
    Q_PROPERTY(Quality quality READ quality WRITE setQuality NOTIFY qualityChanged)

    // preview and video recording
  	Q_PROPERTY(PlasmaCamera *plasmaCamera READ plasmaCamera WRITE setPlasmaCamera NOTIFY plasmaCameraChanged)
    Q_PROPERTY(QVideoSink *videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(QMediaRecorder *recorder READ recorder WRITE setRecorder NOTIFY recorderChanged)
    Q_PROPERTY(QAudioInput *audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(float fps READ fps WRITE setFps NOTIFY fpsChanged)

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
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality,
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

    // error handling
    Error error() const;
    QString errorString() const;

    // preforming an image capture
    bool isReadyForCapture() const;

    // saving to file
    QList<FileFormat> supportedFileFormats() const;
    FileFormat fileFormat() const;
    Quality quality() const;

    // preview and video recording
    PlasmaCamera *plasmaCamera() const;
    QVideoSink *videoSink() const;
    QMediaRecorder *recorder() const;
    QAudioInput *audioInput() const;
    float fps() const;

Q_SIGNALS:
    // error handling
    void errorChanged();
    void errorOccurred(int id, Error error, const QString &errorString);

    // preforming an image capture
    void readyForCaptureChanged(bool ready);
    void metaDataChanged();

    // saving to file
    void fileFormatChanged();
    void qualityChanged();

    // preview and video recording
    void plasmaCameraChanged();
    void videoSinkChanged();
    void recorderChanged();
    void audioInputChanged();
    void fpsChanged(float fps);
    void imageSaved(int id, const QString &fileName);

public Q_SLOTS:
    // preforming an image capture
    int capture();
    int captureToFile(const QString &location = QString());
    void setReadyForCapture(bool ready);

    // saving to file
    void setFileFormat(FileFormat fileFormat);
    void setQuality(Quality quality);

    // preview and video recording
    void setPlasmaCamera(PlasmaCamera *plasmaCamera);
    void setVideoSink(QVideoSink *videoSink);
    void setRecorder(QMediaRecorder *recorder);
    void setAudioInput(QAudioInput *audioInput);
    void setFps(float fps);

private Q_SLOTS:
    // error handling
    void setError(int id, Error error, const QString &errorString);
    void unsetError();

    // settings
    void setSettings(const Settings &settings);

    // image processing
    void processViewfinderFrame(const QImage &image);
    void processCaptureImage(const QQueue<QImage> &frames);
    void processVideoFrame();

private:
    // error handling
    Error m_error;
    QString m_errorString;

    // preforming an image capture
    bool m_readyForCapture = false;
    QMediaMetaData m_metaData;

    // saving to file
    FileFormat m_fileFormat;
    Quality m_quality;
    QList<FileFormat> m_supportedFileFormats;

    // preview and video recording
    PlasmaCamera *m_plasmaCamera = nullptr;

    QImage m_currentFrame;
    QVideoFrame m_videoFrame;
    QVideoSink *m_videoSink = nullptr;

    QMediaCaptureSession m_session;
    QTimer m_videoFrameTimer;
    QMediaFormat m_format;
    QVideoFrameInput m_videoInput;
    QMediaRecorder *m_recorder = nullptr;
    QAudioInput *m_audioInput = nullptr;
    float m_fps = 30.0f;

    // internal
    Settings m_settings;

    int captureImage();
    int m_imageCaptureNum = 0;
    QString m_imageCaptureFileName;
};
