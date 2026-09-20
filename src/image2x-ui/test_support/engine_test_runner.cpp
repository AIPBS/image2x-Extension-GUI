/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#include "engine_test_runner.h"
#include "../ipc/backend_client.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

bool EngineTestRunner::isSoftwareDevice(const QString &deviceName)
{
    const QString normalized = deviceName.toLower();
    return normalized.contains(QStringLiteral("llvmpipe"))
        || normalized.contains(QStringLiteral("lavapipe"))
        || normalized.contains(QStringLiteral("softpipe"))
        || normalized.contains(QStringLiteral("swiftshader"));
}

QString EngineTestRunner::reportedDevice(const QByteArray &standardOutput,
                                         const QByteArray &standardError)
{
    const QString output = QString::fromUtf8(standardOutput + "\n" + standardError);
    const QRegularExpression devicePattern(QStringLiteral("\\[\\d+\\s+([^\\]]+)\\]\\s+queueC="));
    const QRegularExpressionMatch match = devicePattern.match(output);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

bool EngineTestRunner::isValidImage(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (!fileInfo.isFile() || fileInfo.size() <= 0)
    {
        return false;
    }
    QImage image;
    return image.load(path) && !image.isNull() && image.width() > 0 && image.height() > 0;
}

EngineTestResult EngineTestRunner::run(const EngineTestRequest &request)
{
    EngineTestResult result;
    QFile::remove(request.outputPath);

    const QFileInfo executableInfo(request.executable);
    if (!executableInfo.isFile() || !executableInfo.isExecutable())
    {
        result.diagnostic = QStringLiteral("Executable is unavailable: %1").arg(request.executable);
        return result;
    }

    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (request.device == EngineTestDevice::SoftwareCpu
        && !qEnvironmentVariableIsEmpty("IMAGE2X_SOFTWARE_VULKAN_ICD"))
    {
        environment.insert(QStringLiteral("VK_ICD_FILENAMES"),
                           qEnvironmentVariable("IMAGE2X_SOFTWARE_VULKAN_ICD"));
    }
    process.setProcessEnvironment(environment);
    process.setWorkingDirectory(request.workingDirectory.isEmpty()
                                    ? executableInfo.absolutePath()
                                    : request.workingDirectory);
    process.start(request.executable, request.arguments);
    result.started = process.waitForStarted(10000);
    result.finished = result.started && process.waitForFinished(request.timeoutMs);
    if (!result.finished && process.state() != QProcess::NotRunning)
    {
        process.kill();
        process.waitForFinished(5000);
    }

    result.standardOutput = process.readAllStandardOutput();
    result.standardError = process.readAllStandardError();
    result.exitCode = process.exitCode();
    result.deviceName = reportedDevice(result.standardOutput, result.standardError);
    result.outputValid = isValidImage(request.outputPath);
    const bool softwareDevice = isSoftwareDevice(result.deviceName);
    result.deviceAccepted = !result.deviceName.isEmpty()
        && (request.device == EngineTestDevice::SoftwareCpu ? softwareDevice : !softwareDevice);
    result.passed = result.started
        && result.finished
        && process.exitStatus() == QProcess::NormalExit
        && result.exitCode == 0
        && result.outputValid
        && result.deviceAccepted;

    result.diagnostic.clear();
    if (!result.passed)
    {
        if (!result.started)
        {
            result.diagnostic = process.errorString();
        }
        else if (!result.finished)
        {
            result.diagnostic = QStringLiteral("The engine did not finish before the deadline.");
        }
        else if (process.exitStatus() != QProcess::NormalExit || result.exitCode != 0)
        {
            result.diagnostic = QStringLiteral("The engine exited with code %1.").arg(result.exitCode);
        }
        else if (result.deviceName.isEmpty())
        {
            result.diagnostic = QStringLiteral("The engine did not report a Vulkan device.");
        }
        else if (!result.deviceAccepted)
        {
            result.diagnostic = QStringLiteral("Unexpected Vulkan device: %1").arg(result.deviceName);
        }
        else if (!result.outputValid)
        {
            result.diagnostic = QStringLiteral("The engine did not create a valid output image.");
        }
    }
    return result;
}

EngineTestResult EngineTestRunner::run(const EngineTestRequest &request,
                                       const BackendClient *backendClient)
{
    if (backendClient == nullptr)
    {
        return run(request);
    }

    QJsonArray arguments;
    for (const QString &argument : request.arguments)
    {
        arguments.append(argument);
    }
    const QString device = request.device == EngineTestDevice::SoftwareCpu
        ? QStringLiteral("software_cpu")
        : QStringLiteral("hardware_gpu");
    const QJsonObject requestObject{
        {QStringLiteral("type"), QStringLiteral("test_engine")},
        {QStringLiteral("id"), QStringLiteral("engine-test-%1").arg(QRandomGenerator::global()->generate())},
        {QStringLiteral("program"), request.executable},
        {QStringLiteral("args"), arguments},
        {QStringLiteral("cwd"), request.workingDirectory},
        {QStringLiteral("output_path"), request.outputPath},
        {QStringLiteral("timeout_ms"), request.timeoutMs},
        {QStringLiteral("device"), device},
    };
    const QJsonObject response = backendClient->runRequestBlocking(
        requestObject, request.timeoutMs + 5000);
    EngineTestResult result;
    const QJsonObject data = response.value(QStringLiteral("data")).toObject();
    result.started = data.value(QStringLiteral("started")).toBool();
    result.finished = data.value(QStringLiteral("finished")).toBool();
    result.outputValid = data.value(QStringLiteral("output_valid")).toBool();
    result.deviceAccepted = data.value(QStringLiteral("device_accepted")).toBool();
    result.passed = data.value(QStringLiteral("passed")).toBool();
    result.exitCode = data.value(QStringLiteral("exit_code")).toInt(-1);
    result.deviceName = data.value(QStringLiteral("device_name")).toString();
    result.diagnostic = data.value(QStringLiteral("diagnostic")).toString();
    result.standardOutput = data.value(QStringLiteral("stdout")).toString().toUtf8();
    result.standardError = data.value(QStringLiteral("stderr")).toString().toUtf8();
    if (!response.value(QStringLiteral("ok")).toBool())
    {
        result.passed = false;
        result.diagnostic = response.value(QStringLiteral("data"))
                                .toObject()
                                .value(QStringLiteral("message"))
                                .toString();
    }
    if (result.passed && !isValidImage(request.outputPath))
    {
        result.outputValid = false;
        result.passed = false;
        result.diagnostic = QStringLiteral("The engine did not create a valid output image.");
    }
    return result;
}
