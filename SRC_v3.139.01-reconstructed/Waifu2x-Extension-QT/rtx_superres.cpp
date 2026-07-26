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
 * NVIDIA RTX Super Resolution (RTX VSR)
 *
 * Driver-level AI upscaler for RTX 30/40/50 series GPUs.
 * Requires NVIDIA driver 531.18+ and a supported GPU.
 *
 * This is NOT a spawned QProcess-based engine. Instead, it uses the
 * NVIDIA Video Effects SDK / driver-level API calls.
 *
 * However, for this reconstructed version we implement a stub that:
 *   - Prints a message about the driver/hardware requirements
 *   - Acts as a passthrough (copies input → output without processing)
 *   - Is available in the engine selection combo boxes for UI completeness
 *
 * The actual NVIDIA driver API integration (NvCVImage, NvVFX) would require
 * linking against the NVIDIA Video Effects SDK and is excluded from this
 * reconstruction.
 */

/*
    Copyright (C) 2026  AIPEAC

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

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Based on the original Waifu2x-Extension-GUI by Aaron Feng:
    https://github.com/AaronFeng753/Waifu2x-Extension-GUI
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "platform_compat.h"

// ============================================================================
// RTX Super-Res — Stub: Message about driver requirements
// ============================================================================

/*
 * Print the RTX Super-Res requirement message to the text browser.
 * Called when the user selects RTX Super-Res as the engine.
 */
void MainWindow::RTXSuperRes_ShowRequirementsMessage()
{
    emit Send_TextBrowser_NewMessage(
        tr("NVIDIA RTX Super Resolution (RTX VSR) requires:"));
    emit Send_TextBrowser_NewMessage(
        tr("  - NVIDIA driver version 531.18 or higher"));
    emit Send_TextBrowser_NewMessage(
        tr("  - NVIDIA RTX 30 series, 40 series, or 50 series GPU"));
    emit Send_TextBrowser_NewMessage(
        tr("  - Windows 10/11 (64-bit)"));
    emit Send_TextBrowser_NewMessage(
        tr("This build uses a stub implementation. "
           "Actual RTX Super-Res processing requires linking against the "
           "NVIDIA Video Effects SDK (NvCVImage / NvVFX APIs)."));
}

// ============================================================================
// RTX Super-Res — Image Processing (Stub / Passthrough)
// ============================================================================

/*
 * Process a single image through RTX Super-Res.
 *
 * STUB: Copies the source file to the output path without any actual
 * upscaling. Prints the driver requirement message.
 */
int MainWindow::RTXSuperRes_Image(int rowNum, bool ReProcess_MissingAlphaChannel)
{
    Q_UNUSED(ReProcess_MissingAlphaChannel);

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

    // Print the stub warning (only once per session)
    RTXSuperRes_ShowRequirementsMessage();

    // ========== Pre-process to PNG ==========
    QString SourceFile_fullPath_Original = SourceFile_fullPath;
    SourceFile_fullPath = Imgae_PreProcess(SourceFile_fullPath_Original, ReProcess_MissingAlphaChannel);

    // ========== File info ==========
    QFileInfo fileinfo(SourceFile_fullPath);
    QString file_name = file_getBaseName(SourceFile_fullPath);
    QString file_ext = fileinfo.suffix();
    QString file_path = file_getFolderPath(fileinfo);

    // ========== Output path (passthrough naming) ==========
    QString OutPut_Path = file_path + "/" + file_name
                          + "_rtx_superres_" + file_ext + ".png";

    // ========== STUB: Copy through (no actual processing) ==========
    // In a real implementation, this would call NvVFX APIs:
    //   NvCVImage src, dst;
    //   NvVFX_LoadImage(&src, ...);
    //   NvVFX_Run(&src, &dst, params);
    //   NvVFX_SaveImage(&dst, ...);
    if (!QFile::copy(SourceFile_fullPath, OutPut_Path))
    {
        if (SourceFile_fullPath_Original != SourceFile_fullPath)
            QFile::remove(SourceFile_fullPath);

        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath_Original
            + tr("]. Error: [RTX Super-Res stub: unable to copy file.]"));
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
            return RTXSuperRes_Image(rowNum, true);
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
// RTX Super-Res — GIF Processing (Stub / Passthrough)
// ============================================================================

/*
 * Process a GIF through RTX Super-Res.
 *
 * STUB: Splits, copies frames through, and reassembles without upscaling.
 */
int MainWindow::RTXSuperRes_GIF(int rowNum)
{
    // ========== Read settings ==========
    int ScaleRatio = ui->doubleSpinBox_ScaleRatio_gif->value();
    bool DelOriginal = (ui->checkBox_DelOriginal->isChecked()
                        || ui->checkBox_ReplaceOriginalFile->isChecked());
    bool OptGIF = ui->checkBox_OptGIF->isChecked();
    QString OutPutPath_Final;

    // ========== Get source file info ==========
    emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Processing");
    QString SourceFile_fullPath = Table_model_gif->item(rowNum, 2)->text();

    if (!QFile::exists(SourceFile_fullPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [File does not exist.]"));
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // Print stub warning
    RTXSuperRes_ShowRequirementsMessage();

    // ========== File info ==========
    QFileInfo fileinfo(SourceFile_fullPath);
    QString file_name = file_getBaseName(SourceFile_fullPath);
    QString file_ext = fileinfo.suffix();
    QString file_path = file_getFolderPath(fileinfo);
    QString ResGIFPath = file_path + "/" + file_name
                         + "_rtx_superres_" + QString::number(ScaleRatio, 10) + "x.gif";

    // ========== Get GIF duration ==========
    int GIF_Duration = Gif_getDuration(SourceFile_fullPath);
    if (GIF_Duration == 0)
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Can't get Duration value of GIF file.]"));
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Split GIF ==========
    QString SplitFramesFolderPath = file_path + "/" + file_name + "_SplitFrames_W2xEX";
    Gif_splitGif(SourceFile_fullPath, SplitFramesFolderPath);

    QStringList Frame_fileName_list = file_getFileNames_in_Folder_nofilter(SplitFramesFolderPath);
    if (Frame_fileName_list.isEmpty())
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Can't split GIF into frames.]"));
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        file_DelDir(SplitFramesFolderPath);
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Create "scaled" frames folder ==========
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

    // ========== STUB: Copy frames through (no actual upscaling) ==========
    int NumOfSplitFrames = Frame_fileName_list.size();
    for (int i = 0; i < NumOfSplitFrames; i++)
    {
        QString srcFrame = SplitFramesFolderPath + "/" + Frame_fileName_list.at(i);
        QString dstFrame = ScaledFramesFolderPath + "/" + Frame_fileName_list.at(i);
        QFile::copy(srcFrame, dstFrame);
    }

    // ========== Assemble GIF ==========
    Gif_assembleGif(ResGIFPath, ScaledFramesFolderPath, GIF_Duration,
                    false, 0, 0, false, SourceFile_fullPath);

    if (!QFile::exists(ResGIFPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Unable to assemble gif.]"));
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        file_DelDir(SplitFramesFolderPath);
        emit Send_progressbar_Add();
        return 0;
    }
    OutPutPath_Final = ResGIFPath;

    // ========== Optimize GIF ==========
    if (OptGIF)
    {
        QString ResGIFPath_compressed = file_path + "/" + file_name
            + "_rtx_superres_" + QString::number(ScaleRatio, 10) + "x_opt.gif";
        OutPutPath_Final = Gif_compressGif(ResGIFPath, ResGIFPath_compressed);
    }

    // ========== Cleanup cache ==========
    file_DelDir(SplitFramesFolderPath);

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
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished, original file deleted");
    }
    else
    {
        emit Send_Table_gif_ChangeStatus_rowNumInt_statusQString(rowNum, "Finished");
    }

    // ========== Move to output path ==========
    if (ui->checkBox_OutPath_isEnabled->isChecked())
        MoveFileToOutputPath(OutPutPath_Final, SourceFile_fullPath);

    emit Send_progressbar_Add();
    return 0;
}

// ============================================================================
// RTX Super-Res — Video Processing (Stub / Passthrough)
// ============================================================================

/*
 * Process a video through RTX Super-Res.
 *
 * STUB: Splits, copies frames through, and reassembles without upscaling.
 */
int MainWindow::RTXSuperRes_Video(int rowNum)
{
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

    // Print stub warning
    RTXSuperRes_ShowRequirementsMessage();

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

    // ========== STUB: Create output frames folder and copy through ==========
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

    int NumOfSplitFrames = Frame_fileName_list.size();
    for (int i = 0; i < NumOfSplitFrames; i++)
    {
        QString srcFrame = SplitFramesFolderPath + "/" + Frame_fileName_list.at(i);
        QString dstFrame = ScaledFramesFolderPath + "/" + Frame_fileName_list.at(i);
        QFile::copy(srcFrame, dstFrame);
    }

    // ========== Assemble video ==========
    QString video_mp4_scaled_fullpath = file_path + "/" + file_name
        + "_rtx_superres_" + file_ext + ".mp4";

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
// RTX Super-Res — Video by Segment Processing (Stub)
// ============================================================================

/*
 * Stub for RTX Super-Res video processing by segment.
 * For this stub, the segment-based processing is identical to the non-segment
 * version since no actual GPU processing occurs.
 */
int MainWindow::RTXSuperRes_Video_BySegment(int rowNum)
{
    // Delegate to the non-segment version for stub behavior
    return RTXSuperRes_Video(rowNum);
}

// ============================================================================
// RTX Super-Res — Compatibility Test
// ============================================================================

/*
 * Compatibility test for RTX Super-Res.
 * Always reports "No" and the driver requirement message, since this is a stub
 * and the real API integration requires NVIDIA Video Effects SDK linking.
 */
bool MainWindow::RTXSuperRes_CompatibilityTest()
{
    emit Send_TextBrowser_NewMessage(
        tr("Testing NVIDIA RTX Super Resolution compatibility, please wait."));

    // RTX Super-Res is a driver-level feature — it can't be tested
    // with a simple QProcess like the other engines.
    // The stub always reports: "Requires NVIDIA driver 531.18+ and RTX 30/40/50 series GPU"

    emit Send_TextBrowser_NewMessage(
        tr("Compatible with NVIDIA RTX Super Resolution: Not tested. "
           "[Requires NVIDIA driver 531.18+ and RTX 30/40/50 series GPU.] "
           "The RTX Super-Res engine uses driver-level APIs and cannot be "
           "tested through standard process execution."));

    emit Send_Add_progressBar_CompatibilityTest();
    return false;
}

// ============================================================================
// RTX Super-Res — Settings / Config
// ============================================================================

/*
 * Read RTX Super-Res settings from the UI and return a config string.
 * This is used for displaying current settings to the user.
 */
QString MainWindow::RTXSuperRes_ReadSettings()
{
    QString config = "RTX Super-Res Settings:\n";
    config += tr("  Driver requirement: NVIDIA 531.18+\n");
    config += tr("  GPU requirement: RTX 30/40/50 series\n");

    if (ui->checkBox_PrioritizeQuality_RTXSuperRes->isChecked())
        config += tr("  Quality mode: Prioritize Quality\n");
    else
        config += tr("  Quality mode: Balanced\n");

    if (ui->checkBox_DisableDenoise_RTXSuperRes->isChecked())
        config += tr("  Denoise: Disabled\n");
    else
        config += tr("  Denoise: Enabled (default)\n");

    return config;
}

// ============================================================================
// RTX Super-Res — UI Slot Methods
// ============================================================================

/*
 * Handle click on the "Install NVIDIA Broadcast SDK (Maxine)" push button.
 * This provides information about where to download the SDK.
 */
void MainWindow::on_pushButton_InstallMaxineSDK_RTXSuperRes_clicked()
{
    QMessageBox *MSG = new QMessageBox();
    MSG->setWindowTitle(tr("Install NVIDIA Broadcast SDK"));
    MSG->setText(
        tr("NVIDIA Maxine Video Effects SDK provides advanced AI-powered "
           "video effects including:\n\n"
           "- Super Resolution\n"
           "- Artifact Reduction\n"
           "- Video Denoising\n"
           "- Virtual Background\n\n"
           "Download from: https://developer.nvidia.com/maxine\n\n"
           "After installation, place the VideoEffectsApp_W2xEX.exe in the "
           "NVIDIA-Maxine subfolder of the application directory."));
    MSG->setIcon(QMessageBox::Information);
    MSG->setModal(true);
    MSG->show();
}

/*
 * Handle state change on checkBox_PrioritizeQuality_RTXSuperRes.
 */
void MainWindow::on_checkBox_PrioritizeQuality_RTXSuperRes_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    if (ui->checkBox_PrioritizeQuality_RTXSuperRes->isChecked())
    {
        emit Send_TextBrowser_NewMessage(
            tr("RTX Super-Res: Quality mode set to [Prioritize Quality]. "
               "Processing will be slower but quality will be higher."));
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("RTX Super-Res: Quality mode set to [Balanced]."));
    }
}

/*
 * Handle state change on checkBox_DisableDenoise_RTXSuperRes.
 */
void MainWindow::on_checkBox_DisableDenoise_RTXSuperRes_stateChanged(int arg1)
{
    Q_UNUSED(arg1);
    if (ui->checkBox_DisableDenoise_RTXSuperRes->isChecked())
    {
        emit Send_TextBrowser_NewMessage(
            tr("RTX Super-Res: Denoising disabled."));
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("RTX Super-Res: Denoising enabled (default)."));
    }
}

/*
 * Handle the isCompatible checkbox for RTX Super-Res.
 * Since this is a stub, it always reads as unchecked.
 */
void MainWindow::on_checkBox_isCompatible_RTXSuperRes_clicked()
{
    // Stub: RTX Super-Res compatibility is never confirmed
    ui->checkBox_isCompatible_RTXSuperRes->setChecked(false);

    QMessageBox *MSG = new QMessageBox();
    MSG->setWindowTitle(tr("RTX Super-Res Compatibility"));
    MSG->setText(
        tr("NVIDIA RTX Super Resolution uses driver-level APIs and cannot "
           "be tested through the standard compatibility test.\n\n"
           "Requirements:\n"
           "  - NVIDIA driver 531.18 or higher\n"
           "  - RTX 30/40/50 series GPU\n"
           "  - Windows 10/11\n\n"
           "If your system meets these requirements, RTX Super-Res should "
           "be available at the driver level."));
    MSG->setIcon(QMessageBox::Information);
    MSG->setModal(true);
    MSG->show();
}
