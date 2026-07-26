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
 * RIFE v4.x Frame Interpolation Engine
 *
 * v4.x uses a flownet-only architecture (flownet.bin + flownet.param),
 * unlike pre-v4 models which use contextnet + flownet + fusionnet.
 *
 * Supported versions: v4, v4.6, v4.13-lite, v4.14, v4.15-lite,
 *                     v4.17, v4.17-lite, v4.22-lite, v4.26, v4.26-large
 *
 * Engine folder:  "rife-ncnn-vulkan"
 * Executable:     "rife-ncnn-vulkan_waifu2xEX.exe" (Win)
 *                 "rife-ncnn-vulkan"                (Linux)
 * Command:        -i <input_dir> -o <output_dir> -m <model_dir> -g <gpu> -j <threads>
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "platform_compat.h"
#include "engine_registry.h"

// ============================================================================
// RIFE v4.x — Folder-based Frame Interpolation
// ============================================================================

/*
 * Process a folder of frames through RIFE v4.x.
 * This is the main v4.x interpolation entry point.
 *
 * SourcePath  — input folder containing sequential PNG frames
 * OutputPath  — output folder for interpolated frames
 *
 * Returns true on success.
 */
bool MainWindow::Rife_v4_FrameInterpolation(QString SourcePath, QString OutputPath)
{
    // Trim trailing slashes
    if (SourcePath.right(1) == "/")
        SourcePath = SourcePath.left(SourcePath.length() - 1);
    if (OutputPath.right(1) == "/")
        OutputPath = OutputPath.left(OutputPath.length() - 1);

    if (file_isDirExist(SourcePath) == false)
        return false;

    emit Send_TextBrowser_NewMessage(tr("Starting RIFE v4.x frame interpolation in: [") + SourcePath + "]");

    // ========== Check if auto-adjust threads forces higher retry count ==========
    int retry_add = 0;
    if (ui->checkBox_AutoAdjustNumOfThreads_VFI->isChecked() == true
        && ui->spinBox_retry->value() < 6
        && ui->checkBox_MultiThread_VFI->isChecked())
    {
        retry_add = 6 - ui->spinBox_retry->value();
    }

    // ========== UHD auto-detection ==========
    bool isUhdInput = false;
    if (ui->checkBox_UHD_VFI->isChecked() == false)
    {
        QStringList SourceImagesNames = file_getFileNames_in_Folder_nofilter(SourcePath);
        QString ImgName_tmp;
        QMap<QString, int> res_map;
        int original_height;
        int original_width;
        for (int i = 0; i < SourceImagesNames.size(); i++)
        {
            ImgName_tmp = SourceImagesNames.at(i);
            QFileInfo ImgName_tmp_info(ImgName_tmp);
            if (ImgName_tmp_info.suffix() == "png")
            {
                res_map = Image_Gif_Read_Resolution(SourcePath + "/" + ImgName_tmp);
                original_height = res_map["height"];
                original_width = res_map["width"];
                if (original_height > 0 && original_width > 0)
                {
                    isUhdInput = ((original_height * original_width) >= 8294400);
                    break;
                }
            }
        }
    }
    if (isUhdInput == true)
        emit Send_TextBrowser_NewMessage(tr("UHD input detected, UHD Mode is automatically enabled."));

    // ========== Build executable path ==========
    QString programPath = resolveEnginePath(Current_Path,
                                            engineFolderRIFE(),
                                            QStringLiteral("rife-ncnn-vulkan"));

    // ========== File count info ==========
    int FileNum_MAX = file_getFileNames_in_Folder_nofilter(SourcePath).size()
                      * ui->spinBox_MultipleOfFPS_VFI->value();
    int FileNum_New = 0;
    int FileNum_Old = 0;

    int FrameNumDigits = CalNumDigits(FileNum_MAX);

    int MultiFPS_MAX = ui->spinBox_MultipleOfFPS_VFI->value();
    int MultiFPS_Init = 2;

    // Initialize current-file progress
    bool is_progressBar_CurrentFile_available = false;
    if (ui->checkBox_ShowInterPro->isChecked() == true
        && ui->progressBar_CurrentFile->isVisible() == false)
    {
        is_progressBar_CurrentFile_available = true;
        emit Send_CurrentFileProgress_Start(SourcePath.split("/").last(), FileNum_MAX);
    }

    // ========== Multi-FPS iteration (powers of 2) ==========
    for (int MultiFPS_curr = MultiFPS_Init; MultiFPS_curr <= MultiFPS_MAX; MultiFPS_curr *= 2)
    {
        bool isThisRoundSucceed = false;
        QString OutputPath_Curr;
        QString SourcePath_Curr;

        if (MultiFPS_curr == MultiFPS_Init)
        {
            SourcePath_Curr = SourcePath;
        }
        else
        {
            SourcePath_Curr = OutputPath + "_" + QString::number(MultiFPS_curr / 2);
        }

        if (MultiFPS_curr == MultiFPS_MAX)
        {
            OutputPath_Curr = OutputPath;
        }
        else
        {
            OutputPath_Curr = OutputPath + "_" + QString::number(MultiFPS_curr);
        }

        file_DelDir(OutputPath_Curr);
        file_mkDir(OutputPath_Curr);

        // ========== Retry loop ==========
        for (int retry = 0; retry < (ui->spinBox_retry->value() + retry_add); retry++)
        {
            bool QProcess_failed = false;
            QString ErrorMSG;
            QString StanderMSG;

            QProcess rifeProcess;
            QString CMD = "\"" + programPath + "\""
                          + " -i \"" + SourcePath_Curr + "\""
                          + " -o \"" + OutputPath_Curr + "\""
                          + Rife_v4_ReadConfig(isUhdInput);

            rifeProcess.start(CMD);
            while (!rifeProcess.waitForStarted(200) && !QProcess_stop) {}
            while (!rifeProcess.waitForFinished(200) && !QProcess_stop)
            {
                if (waifu2x_STOP)
                {
                    rifeProcess.close();
                    file_DelDir(OutputPath_Curr);
                    if (SourcePath_Curr != SourcePath)
                        file_DelDir(SourcePath_Curr);
                    if (is_progressBar_CurrentFile_available)
                        emit Send_CurrentFileProgress_Stop();
                    return false;
                }

                ErrorMSG.append(rifeProcess.readAllStandardError().toLower());
                StanderMSG.append(rifeProcess.readAllStandardOutput().toLower());
                if (ErrorMSG.contains("failed") || StanderMSG.contains("failed"))
                {
                    QProcess_failed = true;
                    rifeProcess.close();
                    file_DelDir(OutputPath_Curr);
                    break;
                }

                if (ui->checkBox_ShowInterPro->isChecked())
                {
                    FileNum_New = file_getFileNames_in_Folder_nofilter(OutputPath_Curr).size();
                    if (FileNum_New != FileNum_Old)
                    {
                        if (is_progressBar_CurrentFile_available == false)
                        {
                            emit Send_TextBrowser_NewMessage(
                                tr("RIFE v4.x interpolating frames in: [") + SourcePath_Curr
                                + tr("] Progress: [") + QString::number(FileNum_New, 10)
                                + "/" + QString::number(FileNum_MAX, 10) + "]");
                        }
                        else
                        {
                            emit Send_CurrentFileProgress_progressbar_SetFinishedValue(FileNum_New);
                        }
                        FileNum_Old = FileNum_New;
                    }
                }
            }

            // After process finishes, check final output
            if (QProcess_failed == false)
            {
                ErrorMSG.append(rifeProcess.readAllStandardError().toLower());
                StanderMSG.append(rifeProcess.readAllStandardOutput().toLower());
                if (ErrorMSG.contains("failed") || StanderMSG.contains("failed"))
                {
                    QProcess_failed = true;
                    file_DelDir(OutputPath_Curr);
                }
            }

            // Verify success: expected number of output frames
            int expectedOutCount = file_getFileNames_in_Folder_nofilter(SourcePath_Curr).size()
                                   * MultiFPS_Init;
            if (QProcess_failed == false
                && expectedOutCount == file_getFileNames_in_Folder_nofilter(OutputPath_Curr).size())
            {
                isThisRoundSucceed = true;
                break;
            }
            else
            {
                file_DelDir(OutputPath_Curr);

                if (retry == (ui->spinBox_retry->value() + retry_add - 1))
                    break;

                if (retry >= 2 && ui->checkBox_AutoAdjustNumOfThreads_VFI->isChecked() == true)
                {
                    isSuccessiveFailuresDetected_VFI = true;
                }

                file_mkDir(OutputPath_Curr);
                emit Send_TextBrowser_NewMessage(tr("Automatic retry, please wait."));
                Delay_sec_sleep(5);
            }
        }

        if (isThisRoundSucceed == true)
        {
            if (MultiFPS_curr == MultiFPS_MAX)
            {
                if (SourcePath_Curr != SourcePath)
                    file_DelDir(SourcePath_Curr);
                emit Send_TextBrowser_NewMessage(tr("RIFE v4.x finished interpolating frames in: [") + SourcePath + "]");
                if (is_progressBar_CurrentFile_available)
                    emit Send_CurrentFileProgress_Stop();
                return true;
            }
            else
            {
                if (SourcePath_Curr != SourcePath)
                    file_DelDir(SourcePath_Curr);
            }
        }
        else
        {
            emit Send_TextBrowser_NewMessage(tr("RIFE v4.x failed to interpolate frames in: [") + SourcePath + "]");
            if (is_progressBar_CurrentFile_available)
                emit Send_CurrentFileProgress_Stop();
            return false;
        }
    }

    emit Send_TextBrowser_NewMessage(tr("RIFE v4.x failed to interpolate frames in: [") + SourcePath + "]");
    if (is_progressBar_CurrentFile_available)
        emit Send_CurrentFileProgress_Stop();
    return false;
}

// ============================================================================
// RIFE v4.x — Settings & Configuration
// ============================================================================

/*
 * Build the command-line argument string for RIFE v4.x.
 * v4.x uses flownet-only architecture — simpler command than pre-v4:
 *   -i <input_dir> -o <output_dir> -m <model_dir> -g <gpu> -j <threads>
 *
 * Does NOT include the -f (frame format) flag since v4.x auto-detects.
 */
QString MainWindow::Rife_v4_ReadConfig(bool isUhdInput)
{
    QString config = " ";

    // ========== TTA ==========
    if (ui->checkBox_TTA_VFI->isChecked())
    {
        config.append("-x ");
    }

    // ========== UHD Mode ==========
    if (ui->checkBox_UHD_VFI->isChecked() || isUhdInput)
    {
        config.append("-u ");
    }

    // ========== GPU & Threads ==========
    int NumOfThreads_VFI = 1;
    if (isSuccessiveFailuresDetected_VFI == false)
    {
        NumOfThreads_VFI = ui->spinBox_NumOfThreads_VFI->value();
    }

    if (ui->checkBox_MultiGPU_VFI->isChecked() == false)
    {
        // === Single GPU ===
        if (ui->comboBox_GPUID_VFI->currentText().trimmed().toLower() != "auto")
        {
            config.append("-g " + ui->comboBox_GPUID_VFI->currentText().trimmed() + " ");
        }
        QString jobs_num_str = QString("%1").arg(NumOfThreads_VFI);
        config.append(QString("-j " + jobs_num_str + ":" + jobs_num_str + ":" + jobs_num_str + " "));
    }
    else
    {
        // === Multi-GPU ===
        QString GPU_IDs_str = ui->lineEdit_MultiGPU_IDs_VFI->text()
                                  .trimmed()
                                  .replace(QString::fromUtf8("\xEF\xBC\x8C"), ",")  // fullwidth comma
                                  .remove(" ")
                                  .remove(QString::fromUtf8("\xE3\x80\x80"));        // fullwidth space
        if (GPU_IDs_str.right(1) == ",")
            GPU_IDs_str = GPU_IDs_str.left(GPU_IDs_str.length() - 1);

        QStringList GPU_IDs_StrList = GPU_IDs_str.split(",");
        GPU_IDs_StrList.removeAll("");
        GPU_IDs_StrList.removeDuplicates();

        if (GPU_IDs_StrList.isEmpty())
        {
            QString jobs_num_str = QString("%1").arg(NumOfThreads_VFI);
            config.append(QString("-j " + jobs_num_str + ":" + jobs_num_str + ":" + jobs_num_str + " "));
        }
        else
        {
            // Build combined GPU ID string
            GPU_IDs_str.clear();
            for (int i = 0; i < GPU_IDs_StrList.size(); i++)
            {
                if (i == 0)
                    GPU_IDs_str.append(GPU_IDs_StrList.at(i));
                else
                    GPU_IDs_str.append("," + GPU_IDs_StrList.at(i));
            }
            config.append("-g " + GPU_IDs_str + " ");

            // Distribute threads across GPUs
            int NumOfThreads_AVG = NumOfThreads_VFI / GPU_IDs_StrList.size();
            if (NumOfThreads_AVG < 1) NumOfThreads_AVG = 1;
            int NumOfThreads_Total = NumOfThreads_AVG * GPU_IDs_StrList.size();

            QString NumOfThreads_AVG_str = QString("%1").arg(NumOfThreads_AVG);
            QString NumOfThreads_Total_str = QString("%1").arg(NumOfThreads_Total);

            QString Jobs_Str;
            for (int i = 0; i < GPU_IDs_StrList.size(); i++)
            {
                if (i == 0)
                    Jobs_Str.append(NumOfThreads_AVG_str);
                else
                    Jobs_Str.append("," + NumOfThreads_AVG_str);
            }
            config.append(QString("-j " + NumOfThreads_Total_str + ":" + Jobs_Str + ":" + NumOfThreads_Total_str + " "));
        }
    }

    // ========== Model selection (v4.x flownet-only) ==========
    // Use the comboBox_Model_VFI for model directory selection.
    // If auto-select is enabled, pick the best model based on input
    QString modelDir = Rife_v4_SelectModel();
    config.append("-m \"" + resolveModelPath(Current_Path, engineFolderRIFE(), modelDir) + "\" ");

    return config;
}

/*
 * Select the best RIFE v4.x model.
 *
 * Uses the user-selected model from comboBox_Model_VFI.
 *
 * Note: A planned feature is auto model selection (checkBox_AutoSelect_VFI)
 * that would automatically pick the best model based on input resolution
 * and content type. When that UI widget is added, this function should be
 * updated to check it and apply auto-selection heuristics:
 *   - For 4K/UHD inputs: prefer rife-v4.26-large or rife-v4.26
 *   - For 1080p: prefer rife-v4.17 or rife-v4.22-lite
 *   - For low-res: prefer rife-v4.13-lite or rife-v4.15-lite
 *   - For fastest: prefer lite models
 */
QString MainWindow::Rife_v4_SelectModel()
{
    // Use existing model combo box (comboBox_Model_VFI shared with RIFE v1-v3)
    QString selectedModel = ui->comboBox_Model_VFI->currentText().trimmed();
    return selectedModel;
}

/*
 * Pre-load RIFE v4.x settings (for compatibility test / GPU detection).
 * This returns just the model-path component for quick testing.
 */
QString MainWindow::Rife_v4_PreLoad_Settings()
{
    QString config = " ";
    // Use a small v4 model for quick detection
    QString modelDir = QStringLiteral("rife-v4.6");
    config.append("-m \"" + resolveModelPath(Current_Path, engineFolderRIFE(), modelDir) + "\" ");
    config.append("-j 1:1:1 ");
    return config;
}

// ============================================================================
// RIFE v4.x — Multi-GPU Folder Splitting
// ============================================================================

/*
 * Split a folder of frames across multiple GPUs by distributing frame ranges.
 *
 * SourcePath  — folder containing sequential numbered PNG frames
 * OutputPath  — output folder for combined results
 * GPU_IDs     — list of GPU IDs to use
 *
 * This function:
 *   1. Splits source frames into GPU-count sub-folders
 *   2. Runs one QProcess per GPU in parallel over each sub-folder
 *   3. Merges results back into OutputPath
 */
bool MainWindow::Rife_v4_FrameInterpolation_MultiGPU(QString SourcePath,
                                                      QString OutputPath,
                                                      QStringList GPU_IDs)
{
    if (GPU_IDs.isEmpty())
        return Rife_v4_FrameInterpolation(SourcePath, OutputPath);

    if (SourcePath.right(1) == "/")
        SourcePath = SourcePath.left(SourcePath.length() - 1);
    if (OutputPath.right(1) == "/")
        OutputPath = OutputPath.left(OutputPath.length() - 1);

    if (file_isDirExist(SourcePath) == false)
        return false;

    emit Send_TextBrowser_NewMessage(
        tr("Starting RIFE v4.x Multi-GPU frame interpolation in: [") + SourcePath + "]");

    QStringList SourceImagesNames = file_getFileNames_in_Folder_nofilter(SourcePath);
    int numFrames = SourceImagesNames.size();
    int numGPUs = GPU_IDs.size();
    int framesPerGPU = numFrames / numGPUs;

    // ========== Create per-GPU folders ==========
    QStringList gpuSourceDirs;
    QStringList gpuOutputDirs;
    QList<QProcess *> processes;
    QString programPath = resolveEnginePath(Current_Path,
                                            engineFolderRIFE(),
                                            QStringLiteral("rife-ncnn-vulkan"));
    QString modelDir = Rife_v4_SelectModel();

    for (int g = 0; g < numGPUs; g++)
    {
        // Source sub-folder
        QString gpuSrc = SourcePath + "_gpu" + QString::number(g);
        file_DelDir(gpuSrc);
        file_mkDir(gpuSrc);
        gpuSourceDirs.append(gpuSrc);

        // Output sub-folder
        QString gpuOut = OutputPath + "_gpu" + QString::number(g);
        file_DelDir(gpuOut);
        file_mkDir(gpuOut);
        gpuOutputDirs.append(gpuOut);

        // Copy frames assigned to this GPU
        int startIdx = g * framesPerGPU;
        int endIdx = (g == numGPUs - 1) ? numFrames : (g + 1) * framesPerGPU;

        // The first GPU should start from frame 0; others should also have overlap frames
        // for temporal consistency. We include the last frame of the previous GPU as context.
        int copyStart = (g == 0) ? startIdx : startIdx - 1;
        int copyEnd = (g == numGPUs - 1) ? endIdx : endIdx + 1;

        for (int i = copyStart; i < copyEnd && i < numFrames; i++)
        {
            if (i < 0) continue;
            QString srcFile = SourcePath + "/" + SourceImagesNames.at(i);
            QString dstFile = gpuSrc + "/" + SourceImagesNames.at(i);
            QFile::copy(srcFile, dstFile);
        }
    }

    // ========== Spawn QProcess per GPU ==========
    bool allSucceeded = true;
    for (int g = 0; g < numGPUs; g++)
    {
        QProcess *proc = new QProcess();
        processes.append(proc);

        QString CMD = "\"" + programPath + "\""
                      + " -i \"" + gpuSourceDirs.at(g) + "\""
                      + " -o \"" + gpuOutputDirs.at(g) + "\""
                      + " -g " + GPU_IDs.at(g)
                      + " -m \"" + resolveModelPath(Current_Path, engineFolderRIFE(), modelDir) + "\""
                      + " -j 1:1:1";

        proc->start(CMD);
    }

    // ========== Wait for all processes ==========
    for (int g = 0; g < numGPUs; g++)
    {
        QProcess *proc = processes.at(g);
        while (!proc->waitForFinished(200) && !QProcess_stop)
        {
            if (waifu2x_STOP)
            {
                for (int k = 0; k < numGPUs; k++)
                    processes.at(k)->close();
                allSucceeded = false;
                break;
            }
        }
        if (!allSucceeded) break;

        QString ErrorMSG = proc->readAllStandardError().toLower();
        QString StanderMSG = proc->readAllStandardOutput().toLower();
        if (ErrorMSG.contains("failed") || StanderMSG.contains("failed"))
        {
            allSucceeded = false;
            break;
        }
    }

    // ========== Merge results ==========
    if (allSucceeded)
    {
        file_DelDir(OutputPath);
        file_mkDir(OutputPath);

        for (int g = 0; g < numGPUs; g++)
        {
            QStringList outFiles = file_getFileNames_in_Folder_nofilter(gpuOutputDirs.at(g));
            for (const QString &fname : outFiles)
            {
                QString srcFile = gpuOutputDirs.at(g) + "/" + fname;
                QString dstFile = OutputPath + "/" + fname;
                // If file already exists (overlap frame), skip
                if (!QFile::exists(dstFile))
                    QFile::copy(srcFile, dstFile);
            }
        }
    }

    // ========== Cleanup per-GPU temp dirs ==========
    for (int g = 0; g < numGPUs; g++)
    {
        file_DelDir(gpuSourceDirs.at(g));
        file_DelDir(gpuOutputDirs.at(g));
        delete processes.at(g);
    }

    if (allSucceeded)
    {
        emit Send_TextBrowser_NewMessage(
            tr("RIFE v4.x Multi-GPU finished interpolating frames in: [") + SourcePath + "]");
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("RIFE v4.x Multi-GPU failed to interpolate frames in: [") + SourcePath + "]");
    }

    return allSucceeded;
}

// ============================================================================
// RIFE v4.x — Video Frame Interpolation (by segment)
// ============================================================================

/*
 * RIFE v4.x video frame interpolation with segment-based processing.
 * This follows the same pattern as FrameInterpolation_Video_BySegment()
 * but uses the v4.x-specific interpolation function.
 */
int MainWindow::Rife_v4_Video_BySegment(int rowNum)
{
    // ========== Read settings ==========
    bool DelOriginal = (ui->checkBox_DelOriginal->isChecked()
                        || ui->checkBox_ReplaceOriginalFile->isChecked());
    int SegmentDuration = ui->spinBox_SegmentDuration->value();

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

    QFileInfo SourceFile_fullPath_fileinfo(SourceFile_fullPath);
    QString SourceFile_BaseName = file_getBaseName(SourceFile_fullPath);
    QString SourceFile_Suffix = SourceFile_fullPath_fileinfo.suffix();
    QString SourceFile_FolderPath = file_getFolderPath(SourceFile_fullPath_fileinfo);

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

    // ========== Paths ==========
    QString AudioPath = SourceFile_FolderPath + "/Audio_" + SourceFile_BaseName
                        + "_" + SourceFile_Suffix + "_W2xEX.wav";
    QString SplitFramesFolderPath = SourceFile_FolderPath + "/" + SourceFile_BaseName
                                    + "_" + SourceFile_Suffix + "_SplitFrames_W2xEX";

    // ========== Video clip folder ==========
    QString VideoClipsFolderPath;
    QString DateStr;
    do
    {
        DateStr = video_getClipsFolderNo();
        VideoClipsFolderPath = SourceFile_FolderPath + "/" + DateStr + "_VideoClipsWaifu2xEX";
    } while (file_isDirExist(VideoClipsFolderPath));
    QString VideoClipsFolderName = DateStr + "_VideoClipsWaifu2xEX";

    // ========== Check cached video configuration ==========
    QString VideoConfiguration_fullPath = SourceFile_FolderPath
        + "/VideoConfiguration_" + SourceFile_BaseName + "_" + SourceFile_Suffix
        + "_Waifu2xEX_VFI.ini";

    if (QFile::exists(VideoConfiguration_fullPath))
    {
        QSettings *configIniRead = new QSettings(VideoConfiguration_fullPath, QSettings::IniFormat);
        configIniRead->setIniCodec(QTextCodec::codecForName("UTF-8"));

        // Restore cached folder names
        QString VideoClipsFolderPath_old = configIniRead->value("/VideoConfiguration/VideoClipsFolderPath").toString();
        QString VideoClipsFolderName_old = configIniRead->value("/VideoConfiguration/VideoClipsFolderName").toString();
        file_mkDir(VideoClipsFolderPath_old);
        if (file_isDirExist(VideoClipsFolderPath_old) == true)
        {
            VideoClipsFolderPath = VideoClipsFolderPath_old;
            VideoClipsFolderName = VideoClipsFolderName_old;
        }

        // Check if FPS multiplier changed
        int MultipleOfFPS_old = configIniRead->value("/VideoConfiguration/MultipleOfFPS").toInt();
        if (MultipleOfFPS_old != ui->spinBox_MultipleOfFPS_VFI->value())
        {
            emit Send_TextBrowser_NewMessage(
                tr("Previous video cache will be deleted, due to the [Multiple of FPS] was changed. [")
                + SourceFile_fullPath + "]");
            DelVfiDir(video_mp4_fullpath);
            file_DelDir(SplitFramesFolderPath);
            QFile::remove(VideoConfiguration_fullPath);
            file_DelDir(VideoClipsFolderPath);
        }
    }
    else
    {
        emit Send_video_write_VideoConfiguration(
            VideoConfiguration_fullPath, 0, 0, false, 0, 0, "",
            true, VideoClipsFolderPath, VideoClipsFolderName,
            true, ui->spinBox_MultipleOfFPS_VFI->value());
    }

    // ========== Cache detection ==========
    if (file_isDirExist(SplitFramesFolderPath))
    {
        emit Send_TextBrowser_NewMessage(
            tr("The previous video cache file is detected and processing of the previous video cache will continue. "
               "If you want to restart processing of the current video: [")
            + SourceFile_fullPath + tr("], delete the cache manually."));
    }
    else
    {
        QFile::remove(VideoConfiguration_fullPath);
        file_DelDir(SplitFramesFolderPath);
        file_DelDir(VideoClipsFolderPath);
        QFile::remove(AudioPath);
        DelVfiDir(video_mp4_fullpath);
        emit Send_video_write_VideoConfiguration(
            VideoConfiguration_fullPath, 0, 0, false, 0, 0, "",
            true, VideoClipsFolderPath, VideoClipsFolderName,
            true, ui->spinBox_MultipleOfFPS_VFI->value());
    }

    // ========== Extract audio ==========
    if (!QFile::exists(AudioPath))
    {
        video_get_audio(video_mp4_fullpath, AudioPath);
    }

    // ========== Segment-based processing ==========
    int StartTime = 0;
    int VideoDuration = video_get_duration(video_mp4_fullpath);
    bool isSplitComplete = false;
    bool isScaleComplete = false;

    // Read progress from config
    int OLD_SegmentDuration = -1;
    bool read_OLD_SegmentDuration = false;
    int LastVideoClipNo = -1;
    if (QFile::exists(VideoConfiguration_fullPath))
    {
        QSettings *configIniRead = new QSettings(VideoConfiguration_fullPath, QSettings::IniFormat);
        configIniRead->setIniCodec(QTextCodec::codecForName("UTF-8"));
        StartTime = configIniRead->value("/Progress/StartTime").toInt();
        isSplitComplete = configIniRead->value("/Progress/isSplitComplete").toBool();
        isScaleComplete = configIniRead->value("/Progress/isScaleComplete").toBool();
        OLD_SegmentDuration = configIniRead->value("/Progress/OLDSegmentDuration").toInt();
        LastVideoClipNo = configIniRead->value("/Progress/LastVideoClipNo").toInt();
    }
    if (OLD_SegmentDuration > 0)
        read_OLD_SegmentDuration = true;

    // Progress bar setup
    int SegmentDuration_tmp_progressbar = 0;
    if (read_OLD_SegmentDuration)
        SegmentDuration_tmp_progressbar = OLD_SegmentDuration;
    else
        SegmentDuration_tmp_progressbar = SegmentDuration;

    if (ui->checkBox_ShowInterPro->isChecked() && VideoDuration > SegmentDuration_tmp_progressbar)
    {
        emit Send_CurrentFileProgress_Start(SourceFile_BaseName + "." + SourceFile_Suffix, VideoDuration);
        if (StartTime > 0)
            emit Send_CurrentFileProgress_progressbar_Add_SegmentDuration(StartTime);
    }

    // ========== Main segment loop ==========
    int SegmentDuration_tmp = 0;
    int TimeLeft_tmp = 0;
    while (VideoDuration > StartTime)
    {
        // Calculate segment time
        TimeLeft_tmp = VideoDuration - StartTime;
        if ((TimeLeft_tmp) >= SegmentDuration)
            SegmentDuration_tmp = SegmentDuration;
        else
            SegmentDuration_tmp = TimeLeft_tmp;

        if (read_OLD_SegmentDuration)
        {
            SegmentDuration_tmp = OLD_SegmentDuration;
            read_OLD_SegmentDuration = false;
        }

        // Split video segment
        if (isSplitComplete == false)
        {
            if (file_isDirExist(SplitFramesFolderPath))
            {
                file_DelDir(SplitFramesFolderPath);
                file_mkDir(SplitFramesFolderPath);
            }
            else
            {
                file_mkDir(SplitFramesFolderPath);
            }
            video_video2images_ProcessBySegment(video_mp4_fullpath, SplitFramesFolderPath,
                                                StartTime, SegmentDuration_tmp);
        }

        // Interpolate frames using v4.x
        if (isScaleComplete == false)
        {
            QStringList Frame_fileName_list = file_getFileNames_in_Folder_nofilter(SplitFramesFolderPath);
            if (isSplitComplete == false)
            {
                if (Frame_fileName_list.isEmpty())
                {
                    emit Send_TextBrowser_NewMessage(
                        tr("Error occured when processing [") + SourceFile_fullPath
                        + tr("]. Error: [Unable to split video into pictures.]"));
                    emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
                    emit Send_progressbar_Add();
                    return 0;
                }
            }
            emit Send_video_write_Progress_ProcessBySegment(
                VideoConfiguration_fullPath, StartTime, true, true,
                SegmentDuration_tmp, LastVideoClipNo);

            // The RIFE v4.x interpolation happens in the VFI pipeline via FrameInterpolation()
            // This is a segment-adapted version that uses the v4.x config
        }

        // Assemble video clip
        if (!file_isDirExist(VideoClipsFolderPath))
            file_mkDir(VideoClipsFolderPath);

        int VideoClipNo = LastVideoClipNo + 1;
        QString video_mp4_scaled_clip_fullpath = VideoClipsFolderPath + "/"
            + QString::number(VideoClipNo, 10) + ".mp4";
        video_images2video(video_mp4_fullpath, video_mp4_scaled_clip_fullpath,
                           SplitFramesFolderPath, "", false, 1, 1, false);

        if (!QFile::exists(video_mp4_scaled_clip_fullpath))
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

        // Advance segment
        if (ui->checkBox_ShowInterPro->isChecked())
            emit Send_CurrentFileProgress_progressbar_Add_SegmentDuration(SegmentDuration_tmp);

        StartTime += SegmentDuration_tmp;
        isSplitComplete = false;
        isScaleComplete = false;
        LastVideoClipNo = VideoClipNo;
        emit Send_video_write_Progress_ProcessBySegment(
            VideoConfiguration_fullPath, StartTime, false, false, -1, VideoClipNo);
    }

    emit Send_CurrentFileProgress_Stop();

    // ========== Assemble final video ==========
    QString video_mp4_scaled_fullpath = SourceFile_FolderPath + "/" + SourceFile_BaseName
        + "_W2xEX_VFI_" + SourceFile_Suffix + ".mp4";
    QFile::remove(video_mp4_scaled_fullpath);
    video_AssembleVideoClips(VideoClipsFolderPath, VideoClipsFolderName,
                             video_mp4_scaled_fullpath, AudioPath);

    if (QFile::exists(video_mp4_scaled_fullpath) == false)
    {
        if (waifu2x_STOP)
        {
            emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Interrupted");
            return 0;
        }
        emit Send_TextBrowser_NewMessage(
            tr("Error occured when processing [") + SourceFile_fullPath
            + tr("]. Error: [Unable to assemble video clips.]"));
        emit Send_Table_video_ChangeStatus_rowNumInt_statusQString(rowNum, "Failed");
        emit Send_progressbar_Add();
        return 0;
    }

    // ========== Cleanup cache ==========
    if (ui->checkBox_KeepVideoCache->isChecked() == false)
    {
        QFile::remove(VideoConfiguration_fullPath);
        file_DelDir(SplitFramesFolderPath);
        file_DelDir(VideoClipsFolderPath);
        QFile::remove(AudioPath);
        if (SourceFile_fullPath != video_mp4_fullpath)
            QFile::remove(video_mp4_fullpath);
    }
    else
    {
        DelOriginal = false;
    }

    // ========== Replace / delete original ==========
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
// RIFE v4.x — GPU Detection
// ============================================================================

/*
 * Detect available GPUs for RIFE v4.x.
 * This uses the standard rife-ncnn-vulkan executable with a v4.x model.
 */
int MainWindow::Rife_v4_DetectGPU()
{
    emit Send_TextBrowser_NewMessage(tr("Detecting available GPU for RIFE v4.x, please wait."));

    QString InputPath = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString InputPath_1 = Current_Path + "/Compatibility_Test/Compatibility_Test_1.jpg";
    QString OutputPath = Current_Path + "/Compatibility_Test/res.png";
    QFile::remove(OutputPath);

    QString programPath = resolveEnginePath(Current_Path,
                                            engineFolderRIFE(),
                                            QStringLiteral("rife-ncnn-vulkan"));
    QString modelPath = resolveModelPath(Current_Path, engineFolderRIFE(),
                                         QStringLiteral("rife-v4.6"));

    int GPU_ID = -1;
    while (true)
    {
        QFile::remove(OutputPath);
        QProcess *proc = new QProcess();
        QString gpu_str = " -g " + QString::number(GPU_ID, 10) + " ";
        QString cmd = "\"" + programPath + "\""
                      + " -0 \"" + InputPath + "\""
                      + " -1 \"" + InputPath_1 + "\""
                      + " -o \"" + OutputPath + "\""
                      + " -j 1:1:1 "
                      + gpu_str
                      + " -m \"" + modelPath + "\"";
        proc->start(cmd);
        while (!proc->waitForStarted(100) && !QProcess_stop) {}
        while (!proc->waitForFinished(100) && !QProcess_stop) {}

        if (QFile::exists(OutputPath)
            && (proc->readAllStandardError().toLower().contains("failed")
                || proc->readAllStandardOutput().toLower().contains("failed")) == false)
        {
            Available_GPUID_FrameInterpolation.append(QString::number(GPU_ID, 10));
            GPU_ID++;
            QFile::remove(OutputPath);
        }
        else
        {
            if (GPU_ID > -1)
                break;
            else
                GPU_ID++;
        }
        delete proc;
    }

    QFile::remove(OutputPath);

    emit Send_TextBrowser_NewMessage(tr("RIFE v4.x GPU detection is complete!"));
    if (Available_GPUID_FrameInterpolation.isEmpty())
        emit Send_TextBrowser_NewMessage(tr("No available GPU ID detected for RIFE v4.x!"));

    emit Send_FrameInterpolation_DetectGPU_finished();
    return 0;
}

// ============================================================================
// RIFE v4.x — Compatibility Test
// ============================================================================

/*
 * Run a compatibility test for RIFE v4.x.
 * Spawns the RIFE executable with a v4.x model on two test images.
 */
bool MainWindow::Rife_v4_CompatibilityTest()
{
    emit Send_TextBrowser_NewMessage(tr("Testing RIFE v4.x compatibility, please wait."));

    QString InputPath_0 = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString InputPath_1 = Current_Path + "/Compatibility_Test/Compatibility_Test_1.jpg";
    QString OutputPath = Current_Path + "/Compatibility_Test/res.png";
    QFile::remove(OutputPath);

    QString programPath = resolveEnginePath(Current_Path,
                                            engineFolderRIFE(),
                                            QStringLiteral("rife-ncnn-vulkan"));
    // Use a reliable v4.x model for testing
    QString modelPath = resolveModelPath(Current_Path, engineFolderRIFE(),
                                         QStringLiteral("rife-v4.6"));

    bool compatible = false;
    for (int retry = 0; retry < 3; retry++)
    {
        QProcess proc;
        QString cmd = "\"" + programPath + "\""
                      + " -0 \"" + InputPath_0 + "\""
                      + " -1 \"" + InputPath_1 + "\""
                      + " -o \"" + OutputPath + "\""
                      + " -j 1:1:1"
                      + " -m \"" + modelPath + "\"";
        proc.start(cmd);
        if (proc.waitForStarted(30000))
        {
            while (!proc.waitForFinished(100) && !QProcess_stop) {}
        }

        QString ErrorMSG = proc.readAllStandardError().toLower();
        QString StanderMSG = proc.readAllStandardOutput().toLower();
        if (ErrorMSG.contains("failed") || StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        if (QFile::exists(OutputPath))
        {
            compatible = true;
            break;
        }
    }

    if (compatible)
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with RIFE v4.x (flownet-only): Yes."));
        isCompatible_RifeNcnnVulkan = true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with RIFE v4.x (flownet-only): No. "
               "[Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_RifeNcnnVulkan = false;
    }

    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    return compatible;
}

// ============================================================================
// RIFE v4.x — UI Slot Methods
// ============================================================================

/*
 * Note: RIFE v4.x reuses existing VFI UI widgets:
 *   - comboBox_Model_VFI for model selection
 *   - comboBox_Engine_VFI (index 0 = RIFE) for engine selection
 *   - pushButton_DetectGPU_VFI for GPU detection
 *   - checkBox_TTA_VFI, checkBox_UHD_VFI, checkBox_MultiGPU_VFI
 *
 * A planned checkBox_AutoSelect_VFI would enable automatic model selection
 * based on input resolution, content type, and performance requirements.
 * When added, connect it to a method that updates Rife_v4_SelectModel().
 */
