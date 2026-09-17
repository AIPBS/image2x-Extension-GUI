/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ADDITIONAL PERMISSION under GNU AGPL version 3 section 7:
    As a special exception, the copyright holder of this file gives you
    permission to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell this file without restriction, as though it were licensed
    under a permissive license. This additional permission applies only to
    this specific file and files explicitly marked with this notice.
*/

#include "engine_test_runner.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
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
