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
#include <QProcess>
#include <QString>

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
    QJsonObject runRequestBlocking(const QJsonObject &request, int timeoutMs) const;

signals:
    void ready();
    void eventReceived(const QJsonObject &event);
    void unavailable(const QString &message);

private slots:
    void connectSocket();
    void readSocket();

private:
    void send(const QJsonObject &request);

    QProcess backendProcess;
    QLocalSocket socket;
    QString socketPath;
    QByteArray pendingData;
    quint64 nextRequestId = 1;
};

#endif
