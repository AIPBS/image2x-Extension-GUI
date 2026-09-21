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

#ifndef IMAGE2X_BACKEND_CLIENT_H
#define IMAGE2X_BACKEND_CLIENT_H

#include <QJsonObject>
#include <QLocalSocket>
#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QString>
#include <QStringList>

struct BackendProcessResult
{
    bool started = false;
    bool finished = false;
    int exitCode = -1;
    bool timedOut = false;
    bool outputValid = false;
    QByteArray standardOutput;
    QByteArray standardError;
    QString diagnostic;

    bool succeeded() const
    {
        return started && finished && exitCode == 0 && !timedOut && outputValid;
    }
};

class BackendClient final : public QObject
{
    Q_OBJECT

public:
    explicit BackendClient(QObject *parent = nullptr);
    ~BackendClient() override;

    bool start(const QString &executablePath, const QString &applicationDirectory);
    void stop();
    void listModels();
    void validateRuntime();
    bool isConnected() const;
    bool hasRuntimeRecords() const;
    bool runtimeAvailable(const QString &engine) const;
    QString runtimeDirectory(const QString &engine) const;
    QString runtimeExecutable(const QString &engine) const;
    QStringList runtimeMissing(const QString &engine) const;
    bool hasModelRecords() const;
    bool modelAvailable(const QString &model) const;
    QJsonObject runRequestBlocking(const QJsonObject &request, int timeoutMs) const;
    BackendProcessResult runCommandBlocking(const QString &program,
                                            const QStringList &arguments,
                                            const QString &workingDirectory,
                                            const QString &outputPath,
                                            int timeoutMs) const;
    BackendProcessResult runImageJobBlocking(const QString &engine,
                                             const QString &model,
                                             const QString &inputPath,
                                             const QStringList &arguments,
                                             const QString &outputPath,
                                             int timeoutMs) const;

signals:
    void ready();
    void runtimeReady();
    void modelsReady();
    void eventReceived(const QJsonObject &event);
    void unavailable(const QString &message);

private slots:
    void connectSocket();
    void readSocket();

private:
    void send(const QJsonObject &request);
    QJsonObject runtimeRecord(const QString &engine) const;

    QProcess backendProcess;
    QLocalSocket socket;
    QString socketPath;
    QByteArray pendingData;
    quint64 nextRequestId = 1;
    mutable QMutex runtimeMutex;
    QMap<QString, QJsonObject> runtimeRecords;
    mutable QMutex modelMutex;
    QMap<QString, QJsonObject> modelRecords;
};

#endif
