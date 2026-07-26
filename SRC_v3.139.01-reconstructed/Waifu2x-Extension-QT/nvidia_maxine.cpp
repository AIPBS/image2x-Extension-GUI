/*
    Copyright (C) 2021  Aaron Feng

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My Github homepage: https://github.com/AaronFeng753
*/

/*
 * NVIDIA Maxine Video Effects SDK
 *
 * Provides AI-powered video pre/post-processing effects:
 *   - Super Resolution
 *   - Artifact Reduction
 *   - Video Denoising
 *   - Virtual Background
 *
 * Platform support:
 *   - Windows: Spawns VideoEffectsApp_W2xEX.exe from NVIDIA-Maxine/ folder
 *   - Linux / macOS: NOT available (Windows-only SDK)
 *
 * Executable path (Windows):
 *   <appdir>/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe
 *
 * For this reconstructed version:
 *   - Windows: Full QProcess-based spawning of the Maxine executable
 *   - Linux/macOS: Returns an error message that Maxine is Windows-only
 *   - The pushButton_InstallMaxineSDK_RTXSuperRes is shared with RTX Super-Res
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "platform_compat.h"

// ============================================================================
// NVIDIA Maxine — Platform Check
// ============================================================================

/*
 * Check whether NVIDIA Maxine is available on the current platform.
 * Returns true only on Windows (where the SDK is supported).
 */
bool MainWindow::NvidiaMaxine_IsAvailable()
{
#ifdef PLATFORM_WINDOWS
    // On Windows, check if the executable exists
    QString maxinePath = Current_Path + "/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe";
    return QFile::exists(maxinePath);
#else
    return false;
#endif
}

/*
 * Print the platform availability message to the text browser.
 */
void MainWindow::NvidiaMaxine_ShowPlatformMessage()
{
#ifdef PLATFORM_WINDOWS
    QString maxinePath = Current_Path + "/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe";
    if (QFile::exists(maxinePath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("NVIDIA Maxine Video Effects SDK is available. "
               "Executable: [") + maxinePath + "]");
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("NVIDIA Maxine Video Effects SDK executable not found at: [")
            + maxinePath + tr("]. "
               "Please install the SDK using the [Install NVIDIA Broadcast SDK] button "
               "in the RTX Super-Res tab."));
    }
#elif defined(PLATFORM_LINUX)
    emit Send_TextBrowser_NewMessage(
        tr("NVIDIA Maxine Video Effects SDK is NOT available on Linux. "
           "The SDK supports Windows only. "
           "Please use an alternative engine for video processing on this platform."));
#elif defined(PLATFORM_MACOS)
    emit Send_TextBrowser_NewMessage(
        tr("NVIDIA Maxine Video Effects SDK is NOT available on macOS. "
           "The SDK supports Windows only. "
           "Please use an alternative engine for video processing on this platform."));
#else
    emit Send_TextBrowser_NewMessage(
        tr("NVIDIA Maxine Video Effects SDK is NOT available on this platform. "
           "The SDK supports Windows only."));
#endif
}

// ============================================================================
// NVIDIA Maxine — Image Processing
// ============================================================================

/*
 * Process a single image through NVIDIA Maxine Video Effects SDK.
 *
 * On Windows: Spawns the VideoEffectsApp_W2xEX.exe with appropriate arguments.
 * On Linux/macOS: Returns error message.
 */
int MainWindow::NvidiaMaxine_Image(int rowNum, bool ReProcess_MissingAlphaChannel)
{
    Q_UNUSED(ReProcess_MissingAlphaChannel);

    // ========== Platform check ==========
    if (!isPlatformWindows())
    {
        NvidiaMaxine_ShowPlatformMessage();
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        mutex_ThreadNumRunning.lock();
        ThreadNumRunning--;
        mutex_ThreadNumRunning.unlock();
        return 0;
    }

    // ========== Read settings ==========
    bool DelOriginal = (ui->checkBox_DelOriginal->isChecked()
                        || ui->checkBox_ReplaceOriginalFile->isChecked());
    QString OutPutPath_Final;

    // ========== Get source file info ==========
    emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Processing");
    QString SourceFile_fullPath = Table_model_image->item(rowNum, 2)->text();

    if (!QFile::exists(SourceFile_fullPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [File does not exist.]"));
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        mutex_ThreadNumRunning.lock();
        ThreadNumRunning--;
        mutex_ThreadNumRunning.unlock();
        return 0;
    }

    // ========== Check executable ==========
    QString maxineProgramPath = Current_Path + "/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe";
    if (!QFile::exists(maxineProgramPath))
    {
        NvidiaMaxine_ShowPlatformMessage();
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [NVIDIA Maxine executable not found.]"));
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        mutex_ThreadNumRunning.lock();
        ThreadNumRunning--;
        mutex_ThreadNumRunning.unlock();
        return 0;
    }

    // ========== Pre-process to PNG ==========
    QString SourceFile_fullPath_Original = SourceFile_fullPath;
    SourceFile_fullPath = Imgae_PreProcess(SourceFile_fullPath_Original, ReProcess_MissingAlphaChannel);

    // ========== File info ==========
    QFileInfo fileinfo(SourceFile_fullPath);
    QString file_name = file_getBaseName(SourceFile_fullPath);
    QString file_ext = fileinfo.suffix();
    QString file_path = file_getFolderPath(fileinfo);

    // ========== Output path ==========
    QString OutPut_Path = file_path + "/" + file_name
                          + "_maxine_" + file_ext + ".png";

    // ========== Spawn NVIDIA Maxine process ==========
    bool maxine_qprocess_failed = false;
    for (int retry = 0; retry < (ui->spinBox_retry->value() + ForceRetryCount); retry++)
    {
        maxine_qprocess_failed = false;
        QProcess *maxineProc = new QProcess();

        // Build command: <exe> -i <input> -o <output> [--super-res] [--denoise] [--artifact-reduction]
        QString cmd = "\"" + maxineProgramPath + "\""
                      + " -i \"" + SourceFile_fullPath + "\""
                      + " -o \"" + OutPut_Path + "\""
                      + " --super-res"
                      + NvidiaMaxine_ReadSettings();

        maxineProc->start(cmd);
        while (!maxineProc->waitForStarted(100) && !QProcess_stop) {}
        while (!maxineProc->waitForFinished(500) && !QProcess_stop)
        {
            if (waifu2x_STOP)
            {
                maxineProc->close();
                QFile::remove(OutPut_Path);
                if (SourceFile_fullPath_Original != SourceFile_fullPath)
                    QFile::remove(SourceFile_fullPath);
                emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Interrupted");
                mutex_ThreadNumRunning.lock();
                ThreadNumRunning--;
                mutex_ThreadNumRunning.unlock();
                return 0;
            }

            QString ErrorMSG = maxineProc->readAllStandardError().toLower();
            QString StanderMSG = maxineProc->readAllStandardOutput().toLower();
            if (ErrorMSG.contains("failed") || StanderMSG.contains("failed")
                || ErrorMSG.contains("error"))
            {
                maxine_qprocess_failed = true;
                maxineProc->close();
                QFile::remove(OutPut_Path);
                break;
            }
        }
        // ========== After process finished ==========
        if (maxine_qprocess_failed == false)
        {
            QString ErrorMSG = maxineProc->readAllStandardError().toLower();
            QString StanderMSG = maxineProc->readAllStandardOutput().toLower();
            if (ErrorMSG.contains("failed") || StanderMSG.contains("failed")
                || ErrorMSG.contains("error"))
            {
                maxine_qprocess_failed = true;
                QFile::remove(OutPut_Path);
            }
        }
        // ========== Check success ==========
        delete maxineProc;
        if (QFile::exists(OutPut_Path) && !maxine_qprocess_failed)
        {
            break;
        }
        else
        {
            QFile::remove(OutPut_Path);
            if (retry == ui->spinBox_retry->value() + (ForceRetryCount - 1))
                break;
            emit Send_TextBrowser_NewMessage(tr("Automatic retry, please wait."));
            Delay_sec_sleep(5);
        }
    }

    // ========== Check if processing succeeded ==========
    if (!QFile::exists(OutPut_Path))
    {
        if (SourceFile_fullPath_Original != SourceFile_fullPath)
            QFile::remove(SourceFile_fullPath);
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath_Original
            + tr("]. Error: [NVIDIA Maxine failed to process the image.]"));
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        mutex_ThreadNumRunning.lock();
        ThreadNumRunning--;
        mutex_ThreadNumRunning.unlock();
        return 0;
    }

    OutPutPath_Final = OutPut_Path;

    // ========== Save as configured format/quality ==========
    OutPutPath_Final = SaveImageAs_FormatAndQuality(
        SourceFile_fullPath_Original, OutPutPath_Final, false, 0);

    // ========== Check alpha channel ==========
    if (ReProcess_MissingAlphaChannel == false)
    {
        QImage QImage_source(SourceFile_fullPath_Original);
        QImage QImage_Final(OutPutPath_Final);
        if (QImage_source.hasAlphaChannel() == true
            && QImage_Final.hasAlphaChannel() == false
            && ui->checkBox_AutoDetectAlphaChannel->isChecked())
        {
            QFile::remove(OutPutPath_Final);
            emit Send_TextBrowser_NewMessage(
                tr("Since the Alpha channel in the resulting image is lost, "
                   "this image will be reprocessed to fix it: [")
                + SourceFile_fullPath_Original + tr("].\n"
                   "If the reprocessing happens a lot, you should consider "
                   "enable [Always pre-process images with Alpha Channel] "
                   "in [Additional settings]."));
            return NvidiaMaxine_Image(rowNum, true);
        }
    }

    // ========== Cleanup temp files ==========
    if (SourceFile_fullPath_Original != SourceFile_fullPath)
    {
        QFile::remove(SourceFile_fullPath);
        SourceFile_fullPath = SourceFile_fullPath_Original;
    }

    // ========== Delete / replace original ==========
    if (DelOriginal)
    {
        if (ReplaceOriginalFile(SourceFile_fullPath, OutPutPath_Final) == false)
        {
            if (QAction_checkBox_MoveToRecycleBin_checkBox_DelOriginal->isChecked())
                file_MoveToTrash(SourceFile_fullPath);
            else
                QFile::remove(SourceFile_fullPath);
        }
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished, original file deleted");
    }
    else
    {
        emit Send_Table_image_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished");
    }

    // ========== Move to output path ==========
    if (ui->checkBox_OutPath_isEnabled->isChecked())
        MoveFileToOutputPath(OutPutPath_Final, SourceFile_fullPath);

    emit Send_progressbar_Add();

    mutex_ThreadNumRunning.lock();
    ThreadNumRunning--;
    mutex_ThreadNumRunning.unlock();
    return 0;
}

// ============================================================================
// NVIDIA Maxine — Video Processing
// ============================================================================

/*
 * Process a video through NVIDIA Maxine Video Effects SDK.
 *
 * On Windows: Splits video into frames, processes each frame through
 *             VideoEffectsApp_W2xEX.exe, then reassembles.
 * On Linux/macOS: Returns error message.
 */
int MainWindow::NvidiaMaxine_Video(int rowNum)
{
    // ========== Platform check ==========
    if (!isPlatformWindows())
    {
        NvidiaMaxine_ShowPlatformMessage();
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Read settings ==========
    bool DelOriginal = (ui->checkBox_DelOriginal->isChecked()
                        || ui->checkBox_ReplaceOriginalFile->isChecked());

    // ========== Get source file info ==========
    emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Processing");
    QString SourceFile_fullPath = Table_model_video->item(rowNum, 2)->text();

    if (!QFile::exists(SourceFile_fullPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [File does not exist.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Check executable ==========
    QString maxineProgramPath = Current_Path + "/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe";
    if (!QFile::exists(maxineProgramPath))
    {
        NvidiaMaxine_ShowPlatformMessage();
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [NVIDIA Maxine executable not found.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== File info ==========
    QFileInfo fileinfo(SourceFile_fullPath);
    QString file_name = file_getBaseName(SourceFile_fullPath);
    QString file_ext = fileinfo.suffix();
    QString file_path = file_getFolderPath(fileinfo);

    // ========== Convert to MP4 ==========
    QString video_mp4_fullpath = video_To_CFRMp4(SourceFile_fullPath);
    if (!QFile::exists(video_mp4_fullpath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Cannot convert video format to mp4.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    QString AudioPath = file_path + "/Audio_" + file_name + "_" + file_ext + "_W2xEX.wav";
    QString SplitFramesFolderPath = file_path + "/" + file_name + "_" + file_ext + "_SplitFrames_W2xEX";

    // ========== Split video ==========
    file_DelDir(SplitFramesFolderPath);
    file_mkDir(SplitFramesFolderPath);
    QFile::remove(AudioPath);
    video_video2images(video_mp4_fullpath, SplitFramesFolderPath, AudioPath);

    QStringList Frame_fileName_list = file_getFileNames_in_Folder_nofilter(SplitFramesFolderPath);
    if (Frame_fileName_list.isEmpty())
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Unable to split video into pictures.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Create scaled frames folder ==========
    QString ScaledFramesFolderPath = SplitFramesFolderPath + "/ScaledFrames_W2xEX";
    if (file_isDirExist(ScaledFramesFolderPath))
    {
        file_DelDir(ScaledFramesFolderPath);
        file_mkDir(ScaledFramesFolderPath);
    }
    else
    {
        file_mkDir(ScaledFramesFolderPath);
    }

    // ========== Process frames through Maxine ==========
    int NumOfSplitFrames = Frame_fileName_list.size();
    QString maxineSettings = NvidiaMaxine_ReadSettings();
    bool processFailed = false;

    for (int retry = 0; retry < (ui->spinBox_retry->value() + ForceRetryCount); retry++)
    {
        processFailed = false;
        QProcess *maxineProc = new QProcess();

        QString cmd = "\"" + maxineProgramPath + "\""
                      + " -i \"" + SplitFramesFolderPath + "\""
                      + " -o \"" + ScaledFramesFolderPath + "\""
                      + " --super-res"
                      + maxineSettings;

        maxineProc->start(cmd);
        while (!maxineProc->waitForStarted(100) && !QProcess_stop) {}
        while (!maxineProc->waitForFinished(650) && !QProcess_stop)
        {
            if (waifu2x_STOP)
            {
                maxineProc->close();
                emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Interrupted");
                mutex_ThreadNumRunning.lock();
                ThreadNumRunning--;
                mutex_ThreadNumRunning.unlock();
                return 0;
            }

            QString ErrorMSG = maxineProc->readAllStandardError().toLower();
            QString StanderMSG = maxineProc->readAllStandardOutput().toLower();
            if (ErrorMSG.contains("failed") || StanderMSG.contains("failed")
                || ErrorMSG.contains("error"))
            {
                processFailed = true;
                maxineProc->close();
                break;
            }
        }

        if (!processFailed)
        {
            QString ErrorMSG = maxineProc->readAllStandardError().toLower();
            QString StanderMSG = maxineProc->readAllStandardOutput().toLower();
            if (ErrorMSG.contains("failed") || StanderMSG.contains("failed"))
            {
                processFailed = true;
            }
        }

        delete maxineProc;

        // Check success
        if (!processFailed
            && NumOfSplitFrames == file_getFileNames_in_Folder_nofilter(ScaledFramesFolderPath).size())
        {
            break;
        }
        else
        {
            file_DelDir(ScaledFramesFolderPath);
            file_mkDir(ScaledFramesFolderPath);
            if (retry == ui->spinBox_retry->value() + (ForceRetryCount - 1))
                break;
            emit Send_TextBrowser_NewMessage(tr("Automatic retry, please wait."));
            Delay_sec_sleep(5);
        }
    }

    // Check if all frames were processed
    QStringList Frame_fileName_list_scaled = file_getFileNames_in_Folder_nofilter(ScaledFramesFolderPath);
    if (Frame_fileName_list_scaled.size() < NumOfSplitFrames)
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [NVIDIA Maxine failed to process all frames.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Assemble video ==========
    QString video_mp4_scaled_fullpath = file_path + "/" + file_name
        + "_maxine_" + file_ext + ".mp4";

    video_images2video(video_mp4_fullpath, video_mp4_scaled_fullpath,
                       ScaledFramesFolderPath, AudioPath,
                       false, 0, 0, false);

    if (QFile::exists(video_mp4_scaled_fullpath) == false)
    {
        if (waifu2x_STOP)
        {
            emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Interrupted");
            return 0;
        }
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Unable to assemble pictures into videos.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Cleanup cache ==========
    if (ui->checkBox_KeepVideoCache->isChecked() == false)
    {
        file_DelDir(SplitFramesFolderPath);
        QFile::remove(AudioPath);
        if (SourceFile_fullPath != video_mp4_fullpath)
            QFile::remove(video_mp4_fullpath);
    }
    else
    {
        DelOriginal = false;
    }

    // ========== Delete / replace original ==========
    if (DelOriginal)
    {
        if (ReplaceOriginalFile(SourceFile_fullPath, video_mp4_scaled_fullpath) == false)
        {
            if (QAction_checkBox_MoveToRecycleBin_checkBox_DelOriginal->isChecked())
                file_MoveToTrash(SourceFile_fullPath);
            else
                QFile::remove(SourceFile_fullPath);
        }
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished, original file deleted");
    }
    else
    {
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished");
    }

    // ========== Move to output path ==========
    if (ui->checkBox_OutPath_isEnabled->isChecked())
        MoveFileToOutputPath(video_mp4_scaled_fullpath, SourceFile_fullPath);

    emit Send_progressbar_Add();
    return 0;
}

// ============================================================================
// NVIDIA Maxine — Video by Segment Processing
// ============================================================================

/*
 * Stub for NVIDIA Maxine video processing by segment.
 * For now, delegates to the non-segment version.
 */
int MainWindow::NvidiaMaxine_Video_BySegment(int rowNum)
{
    return NvidiaMaxine_Video(rowNum);
}

// ============================================================================
// NVIDIA Maxine — Settings / Config
// ============================================================================

/*
 * Read NVIDIA Maxine settings from UI and return command-line argument string.
 */
QString MainWindow::NvidiaMaxine_ReadSettings()
{
    QString config = " ";

    // Super-resolution (always enabled for this engine)
    config.append("--super-res ");

    // Denoise control
    if (ui->checkBox_DisableDenoise_RTXSuperRes
        && ui->checkBox_DisableDenoise_RTXSuperRes->isChecked())
    {
        config.append("--no-denoise ");
    }
    else
    {
        config.append("--denoise ");
    }

    // Quality preference
    if (ui->checkBox_PrioritizeQuality_RTXSuperRes
        && ui->checkBox_PrioritizeQuality_RTXSuperRes->isChecked())
    {
        config.append("--quality high ");
    }

    // Artifact reduction
    config.append("--artifact-reduction ");

    return config;
}

// ============================================================================
// NVIDIA Maxine — Compatibility Test
// ============================================================================

/*
 * Compatibility test for NVIDIA Maxine.
 * On Windows: Checks if the executable exists and is runnable.
 * On Linux/macOS: Reports as not available.
 */
bool MainWindow::NvidiaMaxine_CompatibilityTest()
{
    emit Send_TextBrowser_NewMessage(
        tr("Testing NVIDIA Maxine Video Effects SDK compatibility, please wait."));

#ifdef PLATFORM_WINDOWS
    QString maxinePath = Current_Path + "/NVIDIA-Maxine/VideoEffectsApp_W2xEX.exe";

    if (!QFile::exists(maxinePath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with NVIDIA Maxine Video Effects SDK: No. "
               "[Executable not found at: ") + maxinePath + tr(".] "
               "Please install the SDK using the [Install NVIDIA Broadcast SDK] button "
               "in the RTX Super-Res tab. "
               "The SDK is available at: https://developer.nvidia.com/maxine"));
        emit Send_Add_progressBar_CompatibilityTest();
        return false;
    }

    // Try a quick test run
    QString InputPath = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString OutputPath = Current_Path + "/Compatibility_Test/res_maxine.png";
    QFile::remove(OutputPath);

    QProcess proc;
    QString cmd = "\"" + maxinePath + "\""
                  + " -i \"" + InputPath + "\""
                  + " -o \"" + OutputPath + "\""
                  + " --super-res";

    for (int retry = 0; retry < 3; retry++)
    {
        proc.start(cmd);
        if (proc.waitForStarted(30000))
        {
            while (!proc.waitForFinished(100) && !QProcess_stop) {}
        }
        if (QFile::exists(OutputPath))
            break;
    }

    if (QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with NVIDIA Maxine Video Effects SDK: Yes."));
        QFile::remove(OutputPath);
        emit Send_Add_progressBar_CompatibilityTest();
        return true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with NVIDIA Maxine Video Effects SDK: No. "
               "[Executable exists but failed to process a test image. "
               "Please ensure the NVIDIA Maxine SDK is properly installed.]"));
        QFile::remove(OutputPath);
        emit Send_Add_progressBar_CompatibilityTest();
        return false;
    }
#else
    // Non-Windows platforms
    emit Send_TextBrowser_NewMessage(
        tr("Compatible with NVIDIA Maxine Video Effects SDK: No. "
           "[The NVIDIA Maxine SDK supports Windows only. "
           "Please use alternative video processing engines on this platform.]"));
    emit Send_Add_progressBar_CompatibilityTest();
    return false;
#endif
}

// ============================================================================
// NVIDIA Maxine — UI Slot Methods
// ============================================================================

/*
 * Handle the isCompatible checkbox for NVIDIA Maxine.
 * On non-Windows platforms, this always shows unchecked and displays a message.
 */
void MainWindow::on_checkBox_isCompatible_NvidiaMaxine_clicked()
{
#ifndef PLATFORM_WINDOWS
    ui->checkBox_isCompatible_NvidiaMaxine->setChecked(false);

    QMessageBox *MSG = new QMessageBox();
    MSG->setWindowTitle(tr("NVIDIA Maxine Compatibility"));
    MSG->setText(
        tr("NVIDIA Maxine Video Effects SDK is only available on Windows.\n\n"
           "Your current platform does not support this engine. "
           "Please use an alternative engine such as:\n"
           "  - waifu2x-ncnn-vulkan\n"
           "  - RealSR-ncnn-vulkan\n"
           "  - Anime4K"));
    MSG->setIcon(QMessageBox::Information);
    MSG->setModal(true);
    MSG->show();
#else
    // On Windows, check if the executable exists
    if (!NvidiaMaxine_IsAvailable())
    {
        ui->checkBox_isCompatible_NvidiaMaxine->setChecked(false);

        QMessageBox *MSG = new QMessageBox();
        MSG->setWindowTitle(tr("NVIDIA Maxine Not Found"));
        MSG->setText(
            tr("NVIDIA Maxine Video Effects SDK executable was not found.\n\n"
               "Please install the SDK:\n"
               "  1. Download from https://developer.nvidia.com/maxine\n"
               "  2. Place VideoEffectsApp_W2xEX.exe in the NVIDIA-Maxine/ folder\n"
               "  3. Re-run the compatibility test\n\n"
               "Or use the [Install NVIDIA Broadcast SDK] button in the RTX Super-Res tab."));
        MSG->setIcon(QMessageBox::Warning);
        MSG->setModal(true);
        MSG->show();
    }
#endif
}
