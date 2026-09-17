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

#ifndef ENGINE_TEST_RUNNER_H
#define ENGINE_TEST_RUNNER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

enum class EngineTestDevice
{
    HardwareGpu,
    SoftwareCpu,
};

struct EngineTestRequest
{
    QString name;
    QString executable;
    QString workingDirectory;
    QStringList arguments;
    QString outputPath;
    int timeoutMs = 30000;
    EngineTestDevice device = EngineTestDevice::HardwareGpu;
};

struct EngineTestResult
{
    bool started = false;
    bool finished = false;
    bool outputValid = false;
    bool deviceAccepted = false;
    bool passed = false;
    int exitCode = -1;
    QString deviceName;
    QString diagnostic;
    QByteArray standardOutput;
    QByteArray standardError;
};

class EngineTestRunner
{
public:
    static EngineTestResult run(const EngineTestRequest &request);
    static bool isSoftwareDevice(const QString &deviceName);
    static bool isValidImage(const QString &path);
    static QString reportedDevice(const QByteArray &standardOutput,
                                  const QByteArray &standardError);
};

#endif // ENGINE_TEST_RUNNER_H
