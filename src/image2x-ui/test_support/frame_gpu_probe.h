#ifndef IMAGE2X_FRAME_GPU_PROBE_H
#define IMAGE2X_FRAME_GPU_PROBE_H

#include "engine_test_runner.h"
#include <QDir>
#include <QImage>
#include <QTemporaryDir>

namespace FrameGpuProbe
{
class Inputs
{
public:
    explicit Inputs(const QString &directoryTemplate = QDir::tempPath() + QStringLiteral("/image2x-frame-gpu-XXXXXX"))
        : directory(directoryTemplate)
    {
    }

    bool prepare()
    {
        if (!directory.isValid())
        {
            error = QStringLiteral("Could not create temporary frame-GPU probe directory: %1")
                .arg(directory.errorString());
            return false;
        }
        QImage first(32, 32, QImage::Format_RGB32);
        QImage second(32, 32, QImage::Format_RGB32);
        first.fill(Qt::white);
        second.fill(Qt::black);
        if (!first.save(input0(), "PNG") || !second.save(input1(), "PNG")
            || !EngineTestRunner::isValidImage(input0()) || !EngineTestRunner::isValidImage(input1()))
        {
            error = QStringLiteral("Could not save valid PNG inputs for frame-GPU detection.");
            return false;
        }
        error.clear();
        return true;
    }

    QString input0() const { return directory.filePath(QStringLiteral("frame0.png")); }
    QString input1() const { return directory.filePath(QStringLiteral("frame1.png")); }
    QString output() const { return directory.filePath(QStringLiteral("result.png")); }
    QString diagnostic() const { return error; }

private:
    QTemporaryDir directory;
    QString error;
};

struct Mapping
{
    QString engine;
    QString model;
    int tileSize = 0;
};

inline Mapping mapping(int engineIndex)
{
    switch (engineIndex)
    {
        case 0: return {QStringLiteral("rife-ncnn-vulkan"), QStringLiteral("rife-v2.4"), 0};
        case 1: return {QStringLiteral("cain-ncnn-vulkan"), QStringLiteral("cain"), 0};
        case 2: return {QStringLiteral("dain-ncnn-vulkan"), QStringLiteral("best"), 128};
        case 3: return {QStringLiteral("ifrnet-ncnn-vulkan"), QStringLiteral("IFRNet_Vimeo90K"), 0};
        default: return {};
    }
}

inline EngineTestRequest request(const Mapping &mapping, const QString &executable,
                                 const QString &directory, const QString &input0,
                                 const QString &input1, const QString &output, int gpuId)
{
    EngineTestRequest request;
    request.name = mapping.engine + QStringLiteral(" GPU %1").arg(gpuId);
    request.executable = executable;
    request.workingDirectory = directory;
    request.outputPath = output;
    request.timeoutMs = 60000;
    request.device = EngineTestDevice::HardwareGpu;
    request.arguments = QStringList{QStringLiteral("-0"), input0, QStringLiteral("-1"), input1,
                         QStringLiteral("-o"), output, QStringLiteral("-g"), QString::number(gpuId),
                         QStringLiteral("-j"), QStringLiteral("1:1:1"),
                         QStringLiteral("-m"), mapping.model};
    if (mapping.tileSize > 0)
        request.arguments.append({QStringLiteral("-t"), QString::number(mapping.tileSize)});
    return request;
}

inline bool accepted(int gpuId, const EngineTestResult &result, const QString &output)
{
    return gpuId >= 0 && result.passed && result.started && result.finished
        && result.exitCode == 0 && result.outputValid && result.deviceAccepted
        && !result.deviceName.isEmpty() && !EngineTestRunner::isSoftwareDevice(result.deviceName)
        && EngineTestRunner::isValidImage(output);
}
}

#endif
