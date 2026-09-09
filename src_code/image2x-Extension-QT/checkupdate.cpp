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

    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

    ADDITIONAL PERMISSION under GNU AGPL version 3 section 7:
    As a special exception, the copyright holder of this file gives you
    permission to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell this file without restriction, as though it were licensed
    under a permissive license.
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QVersionNumber>

void MainWindow::on_pushButton_CheckUpdate_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/AIPBS/image2x-Extension-GUI/releases"));
}

int MainWindow::CheckUpadte_Auto()
{
    QString updateType = ui->comboBox_UpdateChannel->currentText();
    QString updateInfoUrl = "https://api.github.com/repos/AIPBS/image2x-Extension-GUI/releases/latest";
    QString updateCacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if(updateCacheDirectory.isEmpty() || !QDir().mkpath(updateCacheDirectory))
    {
        emit Send_TextBrowser_NewMessage(tr("Unable to prepare update information storage."));
        return 0;
    }
    QString updateInfoPath = QDir(updateCacheDirectory).filePath("Update_Info_Github.json");

    emit Send_TextBrowser_NewMessage(tr("Starting to download update information(for auto-check update) from Github."));
    DownloadTo(updateInfoUrl, updateInfoPath);

    QFile updateInfo(updateInfoPath);
    if(!updateInfo.open(QIODevice::ReadOnly))
    {
        emit Send_TextBrowser_NewMessage(tr("Unable to download update information from Github."));
        return 0;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(updateInfo.readAll(), &parseError);
    updateInfo.close();
    QFile::remove(updateInfoPath);
    if(parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        emit Send_TextBrowser_NewMessage(tr("Unable to check for updates! Please check your network or check for updates manually."));
        return 0;
    }

    QJsonObject release = document.object();
    QString latestVersion = release.value("tag_name").toString().trimmed();
    QString changeLog = release.value("body").toString().trimmed();
    if(latestVersion.isEmpty())
    {
        emit Send_TextBrowser_NewMessage(tr("Unable to check for updates! Please check your network or check for updates manually."));
        return 0;
    }

    emit Send_TextBrowser_NewMessage(tr("Successfully downloaded update information from Github."));
    QString latestNumericVersion = latestVersion;
    QString currentNumericVersion = VERSION;
    if(latestNumericVersion.startsWith('v')) latestNumericVersion.remove(0, 1);
    if(currentNumericVersion.startsWith('v')) currentNumericVersion.remove(0, 1);
    if(QVersionNumber::compare(QVersionNumber::fromString(latestNumericVersion),
                               QVersionNumber::fromString(currentNumericVersion)) > 0)
    {
        emit Send_CheckUpadte_NewUpdate(latestVersion, changeLog);
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("No update found, you are using the latest ")+updateType+tr(" version."));
    }
    return 0;
}

int MainWindow::CheckUpadte_NewUpdate(QString update_str,QString Change_log)
{
    QString UpdateType=ui->comboBox_UpdateChannel->currentText();
    if(ui->checkBox_UpdatePopup->isChecked())
    {
        QMessageBox Msg(QMessageBox::Question, QString(tr("New ")+UpdateType+tr(" update available!")), QString(tr("New version: %1\n\nBrief change log:\n%2\n\nDo you wanna update now???")).arg(update_str).arg(Change_log));
        Msg.setIcon(QMessageBox::Information);
        QAbstractButton *pYesBtn = Msg.addButton(QString(tr("YES")), QMessageBox::YesRole);
        Msg.addButton(QString(tr("NO")), QMessageBox::NoRole);
        Msg.exec();
        if(Msg.clickedButton() == pYesBtn)
        {
            QDesktopServices::openUrl(QUrl("https://github.com/AIPBS/image2x-Extension-GUI/releases/tag/"+update_str.trimmed()));
        }
    }
    else
    {
        QString update_msg_str = QString(tr("New ")+UpdateType+tr(" update: %1 is available! Click [Check update] button to download the latest version!")).arg(update_str);
        emit Send_SystemTray_NewMessage(update_msg_str);
        emit Send_TextBrowser_NewMessage(update_msg_str);
    }
    return 0;
}
