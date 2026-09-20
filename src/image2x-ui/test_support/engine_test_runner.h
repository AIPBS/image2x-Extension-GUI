/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#ifndef ENGINE_TEST_RUNNER_H
#define ENGINE_TEST_RUNNER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class BackendClient;

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
    static EngineTestResult run(const EngineTestRequest &request,
                                const BackendClient *backendClient);
    static bool isSoftwareDevice(const QString &deviceName);
    static bool isValidImage(const QString &path);
    static QString reportedDevice(const QByteArray &standardOutput,
                                  const QByteArray &standardError);
};

#endif // ENGINE_TEST_RUNNER_H
