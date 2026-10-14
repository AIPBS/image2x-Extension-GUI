#ifndef IMAGE2X_FRAME_REQUEST_H
#define IMAGE2X_FRAME_REQUEST_H

#include "backend_client.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QUuid>

namespace FrameRequest
{
inline QString modelForEngine(int engineIndex, const QString &selectedModel)
{
    if (engineIndex == 1) return QStringLiteral("cain");
    if (engineIndex == 2) return QStringLiteral("best");
    return selectedModel;
}

struct JobSettings
{
    const int engineIndex;
    const QString model;
    const QStringList options;
    const int fpsMultiple;
    const int retryCount;
    const bool showProgress;

    bool usesRifeBackend() const { return engineIndex == 0; }
    QString engine() const
    {
        switch (engineIndex)
        {
            case 0: return QStringLiteral("rife-ncnn-vulkan");
            case 1: return QStringLiteral("cain-ncnn-vulkan");
            case 2: return QStringLiteral("dain-ncnn-vulkan");
            case 3: return QStringLiteral("ifrnet-ncnn-vulkan");
            default: return QString();
        }
    }
    int initialMultiplier() const { return engineIndex == 2 ? fpsMultiple : 2; }
    int expectedFrames(int sourceFrames) const { return sourceFrames * initialMultiplier(); }
    QStringList roundArguments(const QString &input, const QString &output,
                               int frameDigits, bool automaticUhd) const
    {
        QStringList arguments{QStringLiteral("-i"), input, QStringLiteral("-o"), output,
                              QStringLiteral("-f"), QStringLiteral("%0") + QString::number(frameDigits) + QStringLiteral("d.png")};
        arguments.append(options);
        if (usesRifeBackend() && automaticUhd && !arguments.contains(QStringLiteral("-u")))
            arguments.append(QStringLiteral("-u"));
        return arguments;
    }
};

inline QString newId()
{
    return QStringLiteral("frames-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

inline QJsonObject cancel(const QString &id, const QString &targetId)
{
    return QJsonObject{{QStringLiteral("type"), QStringLiteral("cancel_frames")},
                       {QStringLiteral("id"), id},
                       {QStringLiteral("target_id"), targetId}};
}

inline bool cancellationAccepted(const QJsonObject &response)
{
    return response.value(QStringLiteral("ok")).toBool()
        && response.value(QStringLiteral("event")).toString() == QStringLiteral("cancel_frames")
        && response.value(QStringLiteral("data")).toObject()
               .value(QStringLiteral("cancelled")).toBool();
}

inline QJsonObject make(const QString &id, const QString &engine, const QString &model,
                        const QString &inputPath, const QString &outputPath,
                        const QStringList &arguments, int timeoutMs,
                        int expectedFrames, int retryCount)
{
    return QJsonObject{
        {QStringLiteral("type"), QStringLiteral("process_frames")},
        {QStringLiteral("id"), id},
        {QStringLiteral("engine"), engine},
        {QStringLiteral("model"), model},
        {QStringLiteral("input_path"), inputPath},
        {QStringLiteral("output_path"), outputPath},
        {QStringLiteral("args"), QJsonArray::fromStringList(arguments)},
        {QStringLiteral("timeout_ms"), timeoutMs},
        {QStringLiteral("expected_frames"), expectedFrames},
        {QStringLiteral("retry_count"), retryCount},
    };
}

inline QJsonObject make(const QString &id, const QString &model,
                        const QString &inputPath, const QString &outputPath,
                        const QStringList &arguments, int timeoutMs,
                        int expectedFrames, int retryCount)
{
    return make(id, QStringLiteral("ifrnet-ncnn-vulkan"), model, inputPath,
                outputPath, arguments, timeoutMs, expectedFrames, retryCount);
}

inline BackendProcessResult result(const QJsonObject &response)
{
    const QJsonObject data = response.value(QStringLiteral("data")).toObject();
    BackendProcessResult result;
    result.started = data.value(QStringLiteral("started")).toBool();
    result.finished = data.value(QStringLiteral("finished")).toBool();
    result.exitCode = data.value(QStringLiteral("exit_code")).toInt(-1);
    result.timedOut = data.value(QStringLiteral("timed_out")).toBool();
    result.outputValid = data.value(QStringLiteral("output_valid")).toBool();
    result.standardOutput = data.value(QStringLiteral("stdout")).toString().toUtf8();
    result.standardError = data.value(QStringLiteral("stderr")).toString().toUtf8();
    result.diagnostic = data.value(QStringLiteral("diagnostic")).toString();
    if (!response.value(QStringLiteral("ok")).toBool()
        || response.value(QStringLiteral("event")).toString() != QStringLiteral("process_frames"))
    {
        result.outputValid = false;
        const QString message = data.value(QStringLiteral("message")).toString();
        if (!message.isEmpty()) result.diagnostic = message;
        if (result.diagnostic.isEmpty())
            result.diagnostic = QStringLiteral("Invalid Rust frame-processing response.");
    }
    return result;
}

inline bool validOutput(const QString &directory, int expectedFrames)
{
    const QFileInfoList frames = QDir(directory).entryInfoList(QDir::Files);
    if (expectedFrames <= 0 || frames.size() != expectedFrames) return false;
    for (const QFileInfo &frame : frames)
    {
        QImageReader reader(frame.absoluteFilePath());
        if (frame.size() <= 0 || reader.read().isNull()) return false;
    }
    return true;
}
}

#endif
