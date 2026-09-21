/*
    Copyright (C) 2026 AIPEAC

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
*/

#include "backend_client.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QStandardPaths>

BackendClient::BackendClient(QObject *parent)
    : QObject(parent)
{
    connect(&backendProcess, &QProcess::started, this, &BackendClient::connectSocket);
    connect(&backendProcess,
            QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this,
            [this](QProcess::ProcessError) {
                emit unavailable(backendProcess.errorString());
            });
    connect(&socket, &QLocalSocket::connected, this, [this] {
        send(QJsonObject{{QStringLiteral("type"), QStringLiteral("hello")},
                         {QStringLiteral("id"), QString::number(nextRequestId++)}});
    });
    connect(&socket, &QLocalSocket::readyRead, this, &BackendClient::readSocket);
    connect(&socket,
            &QLocalSocket::errorOccurred,
            this,
            [this](QLocalSocket::LocalSocketError) {
                emit unavailable(socket.errorString());
            });
}

BackendClient::~BackendClient()
{
    stop();
}

bool BackendClient::start(const QString &executablePath, const QString &applicationDirectory)
{
    if (!QFileInfo(executablePath).isExecutable())
    {
        emit unavailable(QStringLiteral("Rust backend is unavailable: %1").arg(executablePath));
        return false;
    }

    const QString runtimeDirectory = QStandardPaths::writableLocation(
        QStandardPaths::RuntimeLocation);
    const QString socketDirectory = runtimeDirectory.isEmpty()
        ? QDir::tempPath()
        : runtimeDirectory;
    socketPath = QDir(socketDirectory).filePath(
        QStringLiteral("image2x-core-%1.sock").arg(QCoreApplication::applicationPid()));
    QFile::remove(socketPath);

    backendProcess.start(executablePath,
                         {QStringLiteral("--socket"), socketPath,
                          QStringLiteral("--application-directory"), applicationDirectory});
    return true;
}

void BackendClient::stop()
{
    socket.abort();
    if (backendProcess.state() != QProcess::NotRunning)
    {
        backendProcess.terminate();
        if (!backendProcess.waitForFinished(1000))
        {
            backendProcess.kill();
            backendProcess.waitForFinished(1000);
        }
    }
    if (!socketPath.isEmpty())
    {
        QFile::remove(socketPath);
    }
}

bool BackendClient::isConnected() const
{
    return socket.state() == QLocalSocket::ConnectedState;
}

bool BackendClient::hasRuntimeRecords() const
{
    QMutexLocker locker(&runtimeMutex);
    return !runtimeRecords.isEmpty();
}

bool BackendClient::runtimeAvailable(const QString &engine) const
{
    return runtimeRecord(engine).value(QStringLiteral("available")).toBool();
}

QString BackendClient::runtimeDirectory(const QString &engine) const
{
    return runtimeRecord(engine).value(QStringLiteral("directory")).toString();
}

QString BackendClient::runtimeExecutable(const QString &engine) const
{
    return runtimeRecord(engine).value(QStringLiteral("executable")).toString();
}

QStringList BackendClient::runtimeMissing(const QString &engine) const
{
    QStringList missing;
    const QJsonArray values = runtimeRecord(engine).value(QStringLiteral("missing")).toArray();
    for (const QJsonValue &value : values)
    {
        missing.append(value.toString());
    }
    return missing;
}

bool BackendClient::hasModelRecords() const
{
    QMutexLocker locker(&modelMutex);
    return !modelRecords.isEmpty();
}

bool BackendClient::modelAvailable(const QString &model) const
{
    QMutexLocker locker(&modelMutex);
    return modelRecords.value(model).value(QStringLiteral("available")).toBool();
}

QJsonObject BackendClient::runRequestBlocking(const QJsonObject &request, int timeoutMs) const
{
    if (socketPath.isEmpty())
    {
        return QJsonObject{{QStringLiteral("ok"), false},
                           {QStringLiteral("event"), QStringLiteral("error")},
                           {QStringLiteral("data"),
                            QJsonObject{{QStringLiteral("message"),
                                         QStringLiteral("Rust backend socket is unavailable.")}}}};
    }

    QLocalSocket requestSocket;
    requestSocket.connectToServer(socketPath);
    if (!requestSocket.waitForConnected(1000))
    {
        return QJsonObject{{QStringLiteral("ok"), false},
                           {QStringLiteral("event"), QStringLiteral("error")},
                           {QStringLiteral("data"),
                            QJsonObject{{QStringLiteral("message"),
                                         requestSocket.errorString()}}}};
    }

    requestSocket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    if (!requestSocket.waitForBytesWritten(1000))
    {
        return QJsonObject{{QStringLiteral("ok"), false},
                           {QStringLiteral("event"), QStringLiteral("error")},
                           {QStringLiteral("data"),
                            QJsonObject{{QStringLiteral("message"),
                                         requestSocket.errorString()}}}};
    }

    const QString requestId = request.value(QStringLiteral("id")).toString();
    QByteArray pendingData;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs)
    {
        if (!requestSocket.bytesAvailable()
            && !requestSocket.waitForReadyRead(qMax(1, timeoutMs - int(timer.elapsed()))))
        {
            break;
        }
        pendingData.append(requestSocket.readAll());
        const qsizetype newline = pendingData.indexOf('\n');
        if (newline < 0)
        {
            continue;
        }
        const QJsonDocument document = QJsonDocument::fromJson(
            pendingData.left(newline).trimmed());
        if (!document.isObject())
        {
            break;
        }
        const QJsonObject response = document.object();
        if (response.value(QStringLiteral("id")).toString() == requestId)
        {
            return response;
        }
        pendingData.remove(0, newline + 1);
    }

    return QJsonObject{{QStringLiteral("ok"), false},
                       {QStringLiteral("event"), QStringLiteral("error")},
                       {QStringLiteral("data"),
                        QJsonObject{{QStringLiteral("message"),
                                     QStringLiteral("Rust backend request timed out.")}}}};
}

BackendProcessResult BackendClient::runCommandBlocking(const QString &program,
                                                       const QStringList &arguments,
                                                       const QString &workingDirectory,
                                                       const QString &outputPath,
                                                       int timeoutMs) const
{
    QJsonArray jsonArguments;
    for (const QString &argument : arguments)
    {
        jsonArguments.append(argument);
    }
    QJsonObject request{
        {QStringLiteral("type"), QStringLiteral("run")},
        {QStringLiteral("id"), QStringLiteral("run-%1").arg(QRandomGenerator::global()->generate())},
        {QStringLiteral("program"), program},
        {QStringLiteral("args"), jsonArguments},
        {QStringLiteral("cwd"), workingDirectory},
        {QStringLiteral("timeout_ms"), timeoutMs},
    };
    if (!outputPath.isEmpty())
    {
        request.insert(QStringLiteral("output_path"), outputPath);
    }
    const QJsonObject response = runRequestBlocking(request, timeoutMs + 5000);
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
    if (!response.value(QStringLiteral("ok")).toBool())
    {
        result.diagnostic = response.value(QStringLiteral("data"))
                                .toObject()
                                .value(QStringLiteral("message"))
                                .toString();
    }
    return result;
}

BackendProcessResult BackendClient::runImageJobBlocking(const QString &engine,
                                                        const QString &model,
                                                        const QString &inputPath,
                                                        const QStringList &arguments,
                                                        const QString &outputPath,
                                                        int timeoutMs) const
{
    QJsonArray jsonArguments;
    for (const QString &argument : arguments)
    {
        jsonArguments.append(argument);
    }
    const QJsonObject request{
        {QStringLiteral("type"), QStringLiteral("process_image")},
        {QStringLiteral("id"), QStringLiteral("image-%1").arg(QRandomGenerator::global()->generate())},
        {QStringLiteral("engine"), engine},
        {QStringLiteral("model"), model},
        {QStringLiteral("input_path"), inputPath},
        {QStringLiteral("output_path"), outputPath},
        {QStringLiteral("args"), jsonArguments},
        {QStringLiteral("timeout_ms"), timeoutMs},
    };
    const QJsonObject response = runRequestBlocking(request, timeoutMs + 5000);
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
    if (!response.value(QStringLiteral("ok")).toBool())
    {
        result.diagnostic = response.value(QStringLiteral("data"))
                                .toObject()
                                .value(QStringLiteral("message"))
                                .toString();
    }
    return result;
}

void BackendClient::validateRuntime()
{
    send(QJsonObject{{QStringLiteral("type"), QStringLiteral("validate_runtime")},
                     {QStringLiteral("id"), QString::number(nextRequestId++)}});
}

void BackendClient::listModels()
{
    send(QJsonObject{{QStringLiteral("type"), QStringLiteral("list_models")},
                     {QStringLiteral("id"), QString::number(nextRequestId++)}});
}

void BackendClient::connectSocket()
{
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(1000))
    {
        emit unavailable(socket.errorString());
    }
}

void BackendClient::readSocket()
{
    pendingData.append(socket.readAll());
    while (true)
    {
        const qsizetype newline = pendingData.indexOf('\n');
        if (newline < 0)
        {
            return;
        }
        const QByteArray line = pendingData.left(newline).trimmed();
        pendingData.remove(0, newline + 1);
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject())
        {
            emit unavailable(QStringLiteral("Rust backend returned invalid JSON."));
            continue;
        }
        const QJsonObject event = document.object();
        if (event.value(QStringLiteral("event")).toString() == QStringLiteral("runtime"))
        {
            QMap<QString, QJsonObject> records;
            const QJsonArray values = event.value(QStringLiteral("data"))
                                          .toObject()
                                          .value(QStringLiteral("runtimes"))
                                          .toArray();
            for (const QJsonValue &value : values)
            {
                const QJsonObject record = value.toObject();
                const QString engine = record.value(QStringLiteral("engine")).toString();
                if (!engine.isEmpty())
                {
                    records.insert(engine, record);
                }
            }
            {
                QMutexLocker locker(&runtimeMutex);
                runtimeRecords = records;
            }
            emit runtimeReady();
        }
        if (event.value(QStringLiteral("event")).toString() == QStringLiteral("models"))
        {
            QMap<QString, QJsonObject> records;
            const QJsonArray values = event.value(QStringLiteral("data"))
                                          .toObject()
                                          .value(QStringLiteral("models"))
                                          .toArray();
            for (const QJsonValue &value : values)
            {
                const QJsonObject record = value.toObject();
                const QString name = record.value(QStringLiteral("name")).toString();
                if (!name.isEmpty())
                {
                    records.insert(name, record);
                }
            }
            {
                QMutexLocker locker(&modelMutex);
                modelRecords = records;
            }
            emit modelsReady();
        }
        emit eventReceived(event);
        if (event.value(QStringLiteral("event")).toString() == QStringLiteral("hello")
            && event.value(QStringLiteral("ok")).toBool())
        {
            emit ready();
        }
    }
}

void BackendClient::send(const QJsonObject &request)
{
    if (!isConnected())
    {
        return;
    }
    const QByteArray line = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    socket.write(line);
    socket.flush();
}

QJsonObject BackendClient::runtimeRecord(const QString &engine) const
{
    QMutexLocker locker(&runtimeMutex);
    return runtimeRecords.value(engine);
}
