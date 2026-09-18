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

void BackendClient::validateRuntime()
{
    send(QJsonObject{{QStringLiteral("type"), QStringLiteral("validate_runtime")},
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
