/*
    Copyright (C) 2021  Aaron Feng
    Copyright (C) 2026 AIPEAC

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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "runtime_dependencies.h"
#include <QPlainTextEdit>

void MainWindow::on_pushButton_compatibilityTest_clicked()
{
    ui->tab_Home->setEnabled(0);
    Start_SystemTrayIcon->setEnabled(0);
    ui->tab_EngineSettings->setEnabled(0);
    ui->tab_VideoSettings->setEnabled(0);
    ui->tab_AdditionalSettings->setEnabled(0);
    ui->pushButton_compatibilityTest->setEnabled(0);
    ui->tabWidget->setCurrentIndex(5);
    ui->pushButton_compatibilityTest->setText(tr("Testing, please wait..."));
    Init_progressBar_CompatibilityTest();
    QtConcurrent::run(this, &MainWindow::Waifu2x_Compatibility_Test);
}

int MainWindow::Waifu2x_Compatibility_Test()
{
#ifdef PLATFORM_LINUX
    emit Send_TextBrowser_NewMessage(tr("Compatibility test is ongoing, please wait."));

    isCompatible_Waifu2x_NCNN_Vulkan_NEW = false;
    RuntimeDependencies dependencies(Current_Path);
    const QString appDataDirectory = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    const QString testDirectory = QDir(appDataDirectory).filePath("Compatibility_Test");
    const QString inputPath = testDirectory + "/Compatibility_Test.png";
    const QString outputPath = testDirectory + "/res.png";
    const QString videoPath = testDirectory + "/CompatibilityTest_Video.mp4";
    const QString imageMagickOutputPath = testDirectory + "/convert_res.bmp";
    const QString gifInputPath = testDirectory + "/CompatibilityTest_GIF.gif";
    const QString gifOutputPath = testDirectory + "/CompatibilityTest_GIF_RES.gif";
    const QString apngFrameOnePath = testDirectory + "/frame001.png";
    const QString apngFrameTwoPath = testDirectory + "/frame002.png";
    const QString apngOutputPath = testDirectory + "/CompatibilityTest_APNG.png";
    const QString apngExtractedFramePath = testDirectory + "/1.png";
    const QString soxInputPath = testDirectory + "/CompatibilityTest_Sound.wav";
    const QString soxProfilePath = testDirectory + "/TestTemp_DenoiseProfile.dp";

    const auto runProcess = [](const QString &program, const QStringList &arguments) {
        QProcess process;
        process.start(program, arguments);
        return process.waitForStarted(30000)
            && process.waitForFinished(120000)
            && process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0;
    };
    const auto runProcessInDirectory = [](const QString &program, const QStringList &arguments,
                                          const QString &workingDirectory) {
        QProcess process;
        process.setWorkingDirectory(workingDirectory);
        process.start(program, arguments);
        return process.waitForStarted(30000)
            && process.waitForFinished(120000)
            && process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0;
    };
    const auto missingPackageAdvice = [&](const QString &program, const QString &package) {
        return QStandardPaths::findExecutable(program).isEmpty()
            ? tr("Install package '%1' with your package manager.").arg(package)
            : tr("The installed '%1' package did not pass its functional check.").arg(package);
    };

    const bool testDirectoryReady = !appDataDirectory.isEmpty()
        && QDir().mkpath(testDirectory);
    QFile::remove(outputPath);
    bool inputImageReady = false;
    if (testDirectoryReady)
    {
        QImage inputImage(1, 1, QImage::Format_RGB32);
        inputImage.fill(Qt::white);
        inputImageReady = inputImage.save(inputPath);
    }

    if (!testDirectoryReady)
    {
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with waifu2x-ncnn-vulkan: No. Unable to create the writable compatibility-test directory."));
    }
    else if (!dependencies.isAvailable(RuntimeEngine::Waifu2xNcnnVulkan))
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Compatible with waifu2x-ncnn-vulkan: No. Install the Linux runtime supplied with this application."));
    }
    else if (!inputImageReady)
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan: No. Unable to create the test image."));
    }
    else
    {
        QProcess process;
        process.start(
            dependencies.executable(RuntimeEngine::Waifu2xNcnnVulkan),
            QStringList()
                << "-i" << inputPath
                << "-o" << outputPath
                << "-s" << "2"
                << "-n" << "0"
                << "-t" << "32"
                << "-m" << dependencies.engineDirectory(RuntimeEngine::Waifu2xNcnnVulkan) + "/models-cunet"
                << "-j" << "1:1:1"
                << "-g" << "-1");

        const bool completed = process.waitForStarted(30000)
            && process.waitForFinished(120000)
            && process.exitStatus() == QProcess::NormalExit
            && process.exitCode() == 0
            && QFileInfo(outputPath).size() > 0;
        isCompatible_Waifu2x_NCNN_Vulkan_NEW = completed;

        emit Send_TextBrowser_NewMessage(
            completed
                ? tr("Compatible with waifu2x-ncnn-vulkan: Yes.")
                : tr("Compatible with waifu2x-ncnn-vulkan: No. Check the Vulkan runtime and graphics driver."));
    }
    emit Send_Add_progressBar_CompatibilityTest();

    const QString enginesDirectory = Current_Path + "/dependencies/engines";
    const auto testEngine = [&](const QString &name, const QString &program,
                                const QStringList &arguments, const QString &resultPath) {
        QFile::remove(resultPath);
        QProcess process;
        const bool prerequisitesReady = testDirectoryReady
            && inputImageReady
            && QFileInfo::exists(program);
        bool completed = false;
        QString diagnostics;
        if (prerequisitesReady)
        {
            process.start(program, arguments);
            completed = process.waitForStarted(30000)
                && process.waitForFinished(120000)
                && process.exitStatus() == QProcess::NormalExit
                && process.exitCode() == 0
                && QFileInfo(resultPath).size() > 0;
            diagnostics = QString::fromUtf8(process.readAllStandardError()).trimmed();
            if (diagnostics.isEmpty() && !completed)
            {
                diagnostics = process.errorString();
            }
        }
        else
        {
            diagnostics = tr("The executable, test image, or writable test directory is unavailable.");
        }
        emit Send_TextBrowser_NewMessage(
            completed
                ? tr("Compatible with %1: Yes.").arg(name)
                : tr("Compatible with %1: No. %2").arg(name, diagnostics));
        emit Send_Add_progressBar_CompatibilityTest();
        return completed;
    };
    const auto reportUnavailable = [&](const QString &name, bool &compatible, const QString &advice) {
        compatible = false;
        emit Send_TextBrowser_NewMessage(
            tr("Compatible with %1: No. %2").arg(name, advice));
        emit Send_Add_progressBar_CompatibilityTest();
    };

    const QString realSrDirectory = enginesDirectory + "/realsr-ncnn-vulkan";
    isCompatible_Realsr_NCNN_Vulkan = testEngine(
        "RealSR-NCNN-Vulkan", realSrDirectory + "/realsr-ncnn-vulkan",
        QStringList() << "-i" << inputPath << "-o" << outputPath << "-s" << "4"
                      << "-t" << "32" << "-m" << realSrDirectory + "/models-DF2K",
        outputPath);

    const QString srmdDirectory = enginesDirectory + "/srmd-ncnn-vulkan";
    isCompatible_SRMD_NCNN_Vulkan = testEngine(
        "SRMD-NCNN-Vulkan", srmdDirectory + "/srmd-ncnn-vulkan",
        QStringList() << "-i" << inputPath << "-o" << outputPath << "-s" << "2"
                      << "-n" << "0" << "-t" << "32" << "-m" << srmdDirectory + "/models-srmd",
        outputPath);

    const QString realEsrganDirectory = enginesDirectory + "/realesrgan-ncnn-vulkan";
    isCompatible_RealESRGAN = testEngine(
        "Real-ESRGAN", realEsrganDirectory + "/realesrgan-ncnn-vulkan",
        QStringList() << "-i" << inputPath << "-o" << outputPath << "-s" << "2"
                      << "-n" << "realesrgan-x4plus" << "-t" << "32"
                      << "-m" << realEsrganDirectory + "/models",
        outputPath);

    const QString realCuganDirectory = enginesDirectory + "/realcugan-ncnn-vulkan";
    isCompatible_RealCUGAN = testEngine(
        "Real-CUGAN", realCuganDirectory + "/realcugan-ncnn-vulkan",
        QStringList() << "-i" << inputPath << "-o" << outputPath << "-s" << "2"
                      << "-n" << "0" << "-t" << "32" << "-m" << realCuganDirectory + "/models-se",
        outputPath);

    const QString rifeDirectory = enginesDirectory + "/rife-ncnn-vulkan";
    isCompatible_RifeNcnnVulkan = testEngine(
        "RIFE-NCNN-Vulkan", rifeDirectory + "/rife-ncnn-vulkan",
        QStringList() << "-0" << inputPath << "-1" << inputPath << "-o" << outputPath
                      << "-j" << "1:1:1" << "-m" << rifeDirectory + "/rife-v4.6",
        outputPath);

    const QString cainDirectory = enginesDirectory + "/cain-ncnn-vulkan";
    isCompatible_CainNcnnVulkan = testEngine(
        "CAIN-NCNN-Vulkan", cainDirectory + "/cain-ncnn-vulkan",
        QStringList() << "-0" << inputPath << "-1" << inputPath << "-o" << outputPath
                      << "-j" << "1:1:1" << "-m" << cainDirectory + "/cain",
        outputPath);

    const QString dainDirectory = enginesDirectory + "/dain-ncnn-vulkan";
    isCompatible_DainNcnnVulkan = testEngine(
        "DAIN-NCNN-Vulkan", dainDirectory + "/dain-ncnn-vulkan",
        QStringList() << "-0" << inputPath << "-1" << inputPath << "-o" << outputPath
                      << "-j" << "1:1:1" << "-m" << dainDirectory + "/best",
        outputPath);

    const QString ifrnetDirectory = enginesDirectory + "/ifrnet-ncnn-vulkan";
    isCompatible_IFRNetNcnnVulkan = testEngine(
        "IFRNet-NCNN-Vulkan", ifrnetDirectory + "/ifrnet-ncnn-vulkan",
        QStringList() << "-0" << inputPath << "-1" << inputPath << "-o" << outputPath
                      << "-j" << "1:1:1" << "-m" << ifrnetDirectory + "/IFRNet_Vimeo90K",
        outputPath);

    reportUnavailable("waifu2x-ncnn-vulkan (FP16)", isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P,
                      tr("No separate Linux runtime is bundled; use the Latest option."));
    reportUnavailable("waifu2x-ncnn-vulkan (legacy)", isCompatible_Waifu2x_NCNN_Vulkan_OLD,
                      tr("No separate Linux runtime is bundled; use the Latest option."));
    reportUnavailable("SRMD-CUDA", isCompatible_SRMD_CUDA,
                      tr("This requires an upstream CUDA runtime; it is not an apt package."));
    reportUnavailable("waifu2x-converter", isCompatible_Waifu2x_Converter,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("Anime4K (CPU)", isCompatible_Anime4k_CPU,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("Anime4K (GPU)", isCompatible_Anime4k_GPU,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("waifu2x-caffe (CPU)", isCompatible_Waifu2x_Caffe_CPU,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("waifu2x-caffe (GPU)", isCompatible_Waifu2x_Caffe_GPU,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("waifu2x-caffe (cuDNN)", isCompatible_Waifu2x_Caffe_cuDNN,
                      tr("No Linux runtime is bundled for this option."));
    reportUnavailable("RTX Super Resolution", isCompatible_RTXSuperRes,
                      tr("This requires NVIDIA driver support; it is not an app package."));
    reportUnavailable("NVIDIA Maxine", isCompatible_NvidiaMaxine,
                      tr("This requires the NVIDIA Video Effects SDK; it is not an apt package."));
    QImage apngFrameOne(1, 1, QImage::Format_RGB32);
    QImage apngFrameTwo(1, 1, QImage::Format_RGB32);
    apngFrameOne.fill(Qt::white);
    apngFrameTwo.fill(Qt::black);
    const bool apngFramesReady = testDirectoryReady
        && apngFrameOne.save(apngFrameOnePath)
        && apngFrameTwo.save(apngFrameTwoPath);
    const QString apngAdvice = QStandardPaths::findExecutable("apngasm").isEmpty()
        || QStandardPaths::findExecutable("apngdis").isEmpty()
        ? tr("Install packages 'apngasm' and 'apngdis' with your package manager.")
        : tr("The installed APNG tools did not pass their functional check.");
    isCompatible_APNG = apngFramesReady
        && runProcess("apngasm", QStringList() << apngOutputPath << apngFrameOnePath << "-l1")
        && runProcessInDirectory("apngdis", QStringList() << apngOutputPath, testDirectory)
        && QFileInfo(apngExtractedFramePath).size() > 0;
    emit Send_TextBrowser_NewMessage(
        isCompatible_APNG
            ? tr("Compatible with APNG Tools: Yes.")
            : tr("Compatible with APNG Tools: No. %1").arg(apngAdvice));
    emit Send_Add_progressBar_CompatibilityTest();

    QFile::remove(outputPath);
    QFile::remove(videoPath);
    QFile::remove(imageMagickOutputPath);
    QFile::remove(gifInputPath);
    QFile::remove(gifOutputPath);
    QFile::remove(apngFrameOnePath);
    QFile::remove(apngFrameTwoPath);
    QFile::remove(apngOutputPath);
    QFile::remove(apngExtractedFramePath);
    QFile::remove(soxInputPath);
    QFile::remove(soxProfilePath);

    if (testDirectoryReady)
    {
        isCompatible_FFmpeg = runProcess(
            "ffmpeg",
            QStringList()
                << "-hide_banner" << "-loglevel" << "error" << "-y"
                << "-f" << "lavfi" << "-i" << "testsrc=size=16x16:rate=1"
                << "-t" << "0.1" << "-pix_fmt" << "yuv420p" << videoPath)
            && QFileInfo(videoPath).size() > 0;
        emit Send_TextBrowser_NewMessage(
            isCompatible_FFmpeg
                ? tr("Compatible with FFmpeg: Yes.")
                : tr("Compatible with FFmpeg: No. %1").arg(missingPackageAdvice("ffmpeg", "ffmpeg")));
        emit Send_Add_progressBar_CompatibilityTest();

        QProcess ffprobeProcess;
        ffprobeProcess.start(
            "ffprobe",
            QStringList()
                << "-v" << "error" << "-show_entries" << "format=duration"
                << "-of" << "default=noprint_wrappers=1:nokey=1" << videoPath);
        isCompatible_FFprobe = ffprobeProcess.waitForStarted(30000)
            && ffprobeProcess.waitForFinished(120000)
            && ffprobeProcess.exitStatus() == QProcess::NormalExit
            && ffprobeProcess.exitCode() == 0
            && !ffprobeProcess.readAllStandardOutput().trimmed().isEmpty();
        emit Send_TextBrowser_NewMessage(
            isCompatible_FFprobe
                ? tr("Compatible with FFprobe: Yes.")
                : tr("Compatible with FFprobe: No. %1").arg(missingPackageAdvice("ffprobe", "ffmpeg")));
        emit Send_Add_progressBar_CompatibilityTest();

        isCompatible_ImageMagick = inputImageReady
            && runProcess("convert", QStringList() << inputPath << imageMagickOutputPath)
            && QFileInfo(imageMagickOutputPath).size() > 0;
        emit Send_TextBrowser_NewMessage(
            isCompatible_ImageMagick
                ? tr("Compatible with ImageMagick: Yes.")
                : tr("Compatible with ImageMagick: No. %1").arg(missingPackageAdvice("convert", "imagemagick")));
        emit Send_Add_progressBar_CompatibilityTest();

        QFile gifInput(gifInputPath);
        const bool gifInputReady = gifInput.open(QIODevice::WriteOnly)
            && gifInput.write(QByteArray::fromHex(
                "47494638396101000100800000ffffff00000021f90401000000002c00000000010001000002024401003b")) > 0;
        gifInput.close();
        isCompatible_Gifsicle = gifInputReady
            && runProcess("gifsicle", QStringList() << "-O3" << "-i" << gifInputPath << "-o" << gifOutputPath)
            && QFileInfo(gifOutputPath).size() > 0;
        emit Send_TextBrowser_NewMessage(
            isCompatible_Gifsicle
                ? tr("Compatible with Gifsicle: Yes.")
                : tr("Compatible with Gifsicle: No. %1").arg(missingPackageAdvice("gifsicle", "gifsicle")));
        emit Send_Add_progressBar_CompatibilityTest();

        const bool soxInputReady = runProcess(
            "sox", QStringList() << "-n" << soxInputPath << "synth" << "0.1" << "sine" << "440");
        isCompatible_SoX = soxInputReady
            && runProcess("sox", QStringList() << soxInputPath << "-n" << "noiseprof" << soxProfilePath)
            && QFileInfo(soxProfilePath).size() > 0;
        emit Send_TextBrowser_NewMessage(
            isCompatible_SoX
                ? tr("Compatible with SoX: Yes.")
                : tr("Compatible with SoX: No. %1").arg(missingPackageAdvice("sox", "sox")));
        emit Send_Add_progressBar_CompatibilityTest();
    }
    else
    {
        isCompatible_FFmpeg = false;
        isCompatible_FFprobe = false;
        isCompatible_ImageMagick = false;
        isCompatible_Gifsicle = false;
        isCompatible_SoX = false;
    }

    emit Send_TextBrowser_NewMessage(tr("Compatibility test results summary:"));
    const auto reportSummary = [&](const QString &name, bool compatible) {
        emit Send_TextBrowser_NewMessage(
            tr("%1: %2").arg(name, compatible ? tr("Yes") : tr("No")));
    };
    reportSummary("waifu2x-ncnn-vulkan (Latest)", isCompatible_Waifu2x_NCNN_Vulkan_NEW);
    reportSummary("waifu2x-ncnn-vulkan (FP16)", isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P);
    reportSummary("waifu2x-ncnn-vulkan (Legacy)", isCompatible_Waifu2x_NCNN_Vulkan_OLD);
    reportSummary("SRMD-NCNN-Vulkan", isCompatible_SRMD_NCNN_Vulkan);
    reportSummary("waifu2x-converter", isCompatible_Waifu2x_Converter);
    reportSummary("Anime4K (CPU)", isCompatible_Anime4k_CPU);
    reportSummary("Anime4K (GPU)", isCompatible_Anime4k_GPU);
    reportSummary("FFmpeg", isCompatible_FFmpeg);
    reportSummary("FFprobe", isCompatible_FFprobe);
    reportSummary("ImageMagick", isCompatible_ImageMagick);
    reportSummary("Gifsicle", isCompatible_Gifsicle);
    reportSummary("SoX", isCompatible_SoX);
    reportSummary("waifu2x-caffe (CPU)", isCompatible_Waifu2x_Caffe_CPU);
    reportSummary("waifu2x-caffe (GPU)", isCompatible_Waifu2x_Caffe_GPU);
    reportSummary("waifu2x-caffe (cuDNN)", isCompatible_Waifu2x_Caffe_cuDNN);
    reportSummary("RealSR-NCNN-Vulkan", isCompatible_Realsr_NCNN_Vulkan);
    reportSummary("RIFE-NCNN-Vulkan", isCompatible_RifeNcnnVulkan);
    reportSummary("CAIN-NCNN-Vulkan", isCompatible_CainNcnnVulkan);
    reportSummary("DAIN-NCNN-Vulkan", isCompatible_DainNcnnVulkan);
    reportSummary("Real-ESRGAN", isCompatible_RealESRGAN);
    reportSummary("Real-CUGAN", isCompatible_RealCUGAN);
    reportSummary("IFRNet-NCNN-Vulkan", isCompatible_IFRNetNcnnVulkan);
    reportSummary("RTX Super Resolution", isCompatible_RTXSuperRes);
    reportSummary("NVIDIA Maxine", isCompatible_NvidiaMaxine);
    reportSummary("APNG Tools", isCompatible_APNG);
    if (!isCompatible_Waifu2x_NCNN_Vulkan_NEW)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install the Linux waifu2x runtime supplied with this application."));
    }
    if (!isCompatible_FFmpeg || !isCompatible_FFprobe)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install package 'ffmpeg' with your package manager."));
    }
    if (!isCompatible_ImageMagick)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install package 'imagemagick' with your package manager."));
    }
    if (!isCompatible_Gifsicle)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install package 'gifsicle' with your package manager."));
    }
    if (!isCompatible_SoX)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install package 'sox' with your package manager."));
    }
    if (!isCompatible_APNG)
    {
        emit Send_TextBrowser_NewMessage(tr(
            "Install guidance: install packages 'apngasm' and 'apngdis' with your package manager."));
    }

    QFile::remove(videoPath);
    QFile::remove(inputPath);
    QFile::remove(imageMagickOutputPath);
    QFile::remove(gifInputPath);
    QFile::remove(gifOutputPath);
    QFile::remove(apngFrameOnePath);
    QFile::remove(apngFrameTwoPath);
    QFile::remove(apngOutputPath);
    QFile::remove(apngExtractedFramePath);
    QFile::remove(soxInputPath);
    QFile::remove(soxProfilePath);
    emit Send_TextBrowser_NewMessage(tr("Compatibility test is complete!"));
    emit Send_SystemTray_NewMessage(tr("Compatibility test is complete!"));
    emit Send_Waifu2x_Compatibility_Test_finished();
    return 0;
#else

    emit Send_TextBrowser_NewMessage(tr("Compatibility test is ongoing, please wait."));
    //===============
    QString InputPath = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString OutputPath = Current_Path + "/Compatibility_Test/res.png";
    QFile::remove(OutputPath);
    //==========================================
    //         waifu2x-ncnn-vulkan 最新版
    //==========================================
    QString Waifu2x_folder_path = Current_Path + "/waifu2x-ncnn-vulkan";
    QString program = Waifu2x_folder_path + "/waifu2x-ncnn-vulkan_waifu2xEX.exe";
    QString model_path = Waifu2x_folder_path+"/models-upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_vulkan = new QProcess();
    QString cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 2 -n 0 -t 32 -m " + "\"" + model_path + "\"" + " -j 1:1:1";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_vulkan->start(cmd);
        if(Waifu2x_vulkan->waitForStarted(30000))
        {
            while(!Waifu2x_vulkan->waitForFinished(100)&&!QProcess_stop) {}
        }
        QString ErrorMSG = Waifu2x_vulkan->readAllStandardError().toLower();
        QString StanderMSG = Waifu2x_vulkan->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(Latest Version): Yes"));
        isCompatible_Waifu2x_NCNN_Vulkan_NEW=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(Latest Version): No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_Waifu2x_NCNN_Vulkan_NEW=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //     waifu2x-ncnn-vulkan 自己编译的老版本
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-ncnn-vulkan-old";
    program = Waifu2x_folder_path + "/waifu2x-ncnn-vulkan_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models-upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_vulkan_old = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 2 -n 0 -t 32 -m " + "\"" + model_path + "\"" + " -j 1:1:1";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_vulkan_old->start(cmd);
        if(Waifu2x_vulkan_old->waitForStarted(30000))
        {
            while(!Waifu2x_vulkan_old->waitForFinished(100)&&!QProcess_stop) {}
        }
        QString ErrorMSG = Waifu2x_vulkan_old->readAllStandardError().toLower();
        QString StanderMSG = Waifu2x_vulkan_old->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(Old Version): Yes"));
        isCompatible_Waifu2x_NCNN_Vulkan_OLD=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(Old Version): No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_Waifu2x_NCNN_Vulkan_OLD=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //     waifu2x-ncnn-vulkan 20200414(fp16p)
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-ncnn-vulkan";
    program = Waifu2x_folder_path + "/waifu2x-ncnn-vulkan-fp16p_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models-upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_vulkan_fp16p = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 2 -n 0 -t 32 -m " + "\"" + model_path + "\"" + " -j 1:1:1";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_vulkan_fp16p->start(cmd);
        if(Waifu2x_vulkan_fp16p->waitForStarted(30000))
        {
            while(!Waifu2x_vulkan_fp16p->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = Waifu2x_vulkan_fp16p->readAllStandardError().toLower();
        QString StanderMSG = Waifu2x_vulkan_fp16p->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(20200414(fp16p)): Yes"));
        isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-ncnn-vulkan(20200414(fp16p)): No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            waifu2x-converter
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-converter";
    program = Waifu2x_folder_path + "/waifu2x-converter-cpp_waifu2xEX.exe";
    model_path= Waifu2x_folder_path + "/models_rgb";
    QString Denoise_cmd = " --noise-level 1 ";
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " --scale-ratio 2" + Denoise_cmd + " --block-size 32 --model-dir " + "\"" + model_path + "\"";
    QProcess *Waifu2x_converter = new QProcess();
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_converter->start(cmd);
        if(Waifu2x_converter->waitForStarted(30000))
        {
            while(!Waifu2x_converter->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-converter: Yes."));
        isCompatible_Waifu2x_Converter=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-converter: No. [Advice: Install or reinstall OpenCL runtime.]"));
        isCompatible_Waifu2x_Converter=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            Anime4k(CPU Mode)
    //==========================================
    Current_Path = qApp->applicationDirPath();
    QString Anime4k_folder_path = Current_Path + "/Anime4K";
    program = Anime4k_folder_path + "/Anime4K_waifu2xEX.exe";
    cmd = "\"" + program + "\" -i \"" + InputPath + "\" -o \"" + OutputPath + "\" -z 2";
    QProcess *Waifu2x_anime4k = new QProcess();
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_anime4k->start(cmd);
        if(Waifu2x_anime4k->waitForStarted(30000))
        {
            while(!Waifu2x_anime4k->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Anime4k(CPU Mode): Yes."));
        isCompatible_Anime4k_CPU=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Anime4k(CPU Mode): No. [Advice: Install or reinstall OpenCL runtime.]"));
        isCompatible_Anime4k_CPU=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            Anime4k(GPU Mode)
    //==========================================
    Current_Path = qApp->applicationDirPath();
    program = Anime4k_folder_path + "/Anime4K_waifu2xEX.exe";
    cmd = "\"" + program + "\" -i \"" + InputPath + "\" -o \"" + OutputPath + "\" -z 2 -q";
    QProcess *Waifu2x_anime4k_gpu = new QProcess();
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_anime4k_gpu->start(cmd);
        if(Waifu2x_anime4k_gpu->waitForStarted(30000))
        {
            while(!Waifu2x_anime4k_gpu->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Anime4k(GPU Mode): Yes."));
        isCompatible_Anime4k_GPU=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Anime4k(GPU Mode): No. [Advice: Install or reinstall OpenCL runtime.]"));
        isCompatible_Anime4k_GPU=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            SRMD-ncnn-vulkan
    //==========================================
    Waifu2x_folder_path = Current_Path + "/srmd-ncnn-vulkan";
    program = Waifu2x_folder_path + "/srmd-ncnn-vulkan_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models-srmd";
    QProcess *SRMD_NCNN_VULKAN = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 2 -n 0 -t 32 -m " + "\"" + model_path + "\"" + " -j 1:1:1";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        SRMD_NCNN_VULKAN->start(cmd);
        if(SRMD_NCNN_VULKAN->waitForStarted(30000))
        {
            while(!SRMD_NCNN_VULKAN->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = SRMD_NCNN_VULKAN->readAllStandardError().toLower();
        QString StanderMSG = SRMD_NCNN_VULKAN->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with srmd-ncnn-vulkan: Yes"));
        isCompatible_SRMD_NCNN_Vulkan=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with srmd-ncnn-vulkan: No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_SRMD_NCNN_Vulkan=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            Waifu2x-Caffe(CPU)
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-caffe";
    program = Waifu2x_folder_path + "/waifu2x-caffe_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models/upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_Caffe_CPU_qprocess = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -p cpu -m noise_scale -s 2 -n 1 -c 32 -b 1 --model_dir " + "\"" + model_path + "\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_Caffe_CPU_qprocess->start(cmd);
        if(Waifu2x_Caffe_CPU_qprocess->waitForStarted(30000))
        {
            while(!Waifu2x_Caffe_CPU_qprocess->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(CPU): Yes."));
        isCompatible_Waifu2x_Caffe_CPU=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(CPU): No."));
        isCompatible_Waifu2x_Caffe_CPU=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            Waifu2x-Caffe(GPU)
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-caffe";
    program = Waifu2x_folder_path + "/waifu2x-caffe_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models/upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_Caffe_GPU_qprocess = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -p gpu -m noise_scale -s 2 -n 1 -c 32 -b 1 --model_dir " + "\"" + model_path + "\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_Caffe_GPU_qprocess->start(cmd);
        if(Waifu2x_Caffe_GPU_qprocess->waitForStarted(30000))
        {
            while(!Waifu2x_Caffe_GPU_qprocess->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(GPU): Yes."));
        isCompatible_Waifu2x_Caffe_GPU=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(GPU): No. [Advice: Install NVIDIA CUDA.]"));
        isCompatible_Waifu2x_Caffe_GPU=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            Waifu2x-Caffe(cuDNN)
    //==========================================
    Waifu2x_folder_path = Current_Path + "/waifu2x-caffe";
    program = Waifu2x_folder_path + "/waifu2x-caffe_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models/upconv_7_anime_style_art_rgb";
    QProcess *Waifu2x_Caffe_cuDNN_qprocess = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -p cudnn -m noise_scale -s 2 -n 1 -c 32 -b 1 --model_dir " + "\"" + model_path + "\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        Waifu2x_Caffe_cuDNN_qprocess->start(cmd);
        if(Waifu2x_Caffe_cuDNN_qprocess->waitForStarted(30000))
        {
            Waifu2x_Caffe_cuDNN_qprocess->waitForFinished(70000);
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(cuDNN): Yes."));
        isCompatible_Waifu2x_Caffe_cuDNN=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with waifu2x-caffe(cuDNN): No. [Advice: Install NVIDIA CUDA and NVIDIA cuDNN.]"));
        isCompatible_Waifu2x_Caffe_cuDNN=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //            RealSR-ncnn-vulkan
    //==========================================
    Waifu2x_folder_path = Current_Path + "/realsr-ncnn-vulkan";
    program = Waifu2x_folder_path + "/realsr-ncnn-vulkan_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/models-DF2K_JPEG";
    QProcess *realsr_ncnn_vulkan_qprocess = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 4 -t 32 -m " + "\"" + model_path + "\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        realsr_ncnn_vulkan_qprocess->start(cmd);
        if(realsr_ncnn_vulkan_qprocess->waitForStarted(30000))
        {
            while(!realsr_ncnn_vulkan_qprocess->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = realsr_ncnn_vulkan_qprocess->readAllStandardError().toLower();
        QString StanderMSG = realsr_ncnn_vulkan_qprocess->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Realsr-ncnn-vulkan: Yes."));
        isCompatible_Realsr_NCNN_Vulkan=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Realsr-ncnn-vulkan: No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_Realsr_NCNN_Vulkan=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                SRMD-CUDA
    //==========================================
    Waifu2x_folder_path = Current_Path + "/srmd-cuda";
    program = Waifu2x_folder_path + "/srmd-cuda_waifu2xEX.exe";
    model_path = Waifu2x_folder_path+"/model";
    QProcess *SRMD_CUDA = new QProcess();
    cmd = "\"" + program + "\"" + " -i " + "\"" + InputPath + "\"" + " -o " + "\"" + OutputPath + "\"" + " -s 2 -n 0 -m \""+model_path+"\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        SRMD_CUDA->start(cmd);
        if(SRMD_CUDA->waitForStarted(30000))
        {
            while(!SRMD_CUDA->waitForFinished(100)&&!QProcess_stop) {}
        }
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with SRMD-CUDA: Yes"));
        isCompatible_SRMD_CUDA=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with SRMD-CUDA: No. [Advice: Install NVIDIA CUDA.]"));
        isCompatible_SRMD_CUDA=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                  FFmpeg
    //==========================================
    QString ffmpeg_VideoPath = Current_Path + "/Compatibility_Test/CompatibilityTest_Video.mp4";
    QString ffmpeg_AudioPath = Current_Path + "/Compatibility_Test/CompatibilityTest_Video_audio.wav";
    QString ffmpeg_path = Current_Path+"/ffmpeg_waifu2xEX.exe";
    QFile::remove(ffmpeg_AudioPath);
    QProcess ffmpeg_QProcess;
    ffmpeg_QProcess.start("\""+ffmpeg_path+"\" -y -i \""+ffmpeg_VideoPath+"\" \""+ffmpeg_AudioPath+"\"");
    if(ffmpeg_QProcess.waitForStarted(30000))
    {
        while(!ffmpeg_QProcess.waitForFinished(100)&&!QProcess_stop) {}
    }
    if(QFile::exists(ffmpeg_AudioPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with FFmpeg: Yes."));
        isCompatible_FFmpeg=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with FFmpeg: No."));
        isCompatible_FFmpeg=false;
    }
    QFile::remove(ffmpeg_AudioPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 FFprobe
    //==========================================
    QString FFprobe_VideoPath = Current_Path + "/Compatibility_Test/CompatibilityTest_Video.mp4";
    //========================= 调用ffprobe读取视频信息 ======================
    QProcess *FFprobe_Get_Duration_process = new QProcess();
    QString FFprobe_cmd = "\""+Current_Path+"/ffprobe_waifu2xEX.exe\" -i \""+FFprobe_VideoPath+"\" -v quiet -print_format ini -show_format";
    FFprobe_Get_Duration_process->start(FFprobe_cmd);
    if(FFprobe_Get_Duration_process->waitForStarted(30000))
    {
        while(!FFprobe_Get_Duration_process->waitForFinished(100)&&!QProcess_stop) {}
    }
    //============= 保存ffprobe输出的ini格式文本 =============
    QString ffprobe_output_str = FFprobe_Get_Duration_process->readAllStandardOutput().toLower();
    //===
    if(ffprobe_output_str.contains("duration="))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with FFprobe: Yes."));
        isCompatible_FFprobe=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with FFprobe: No."));
        isCompatible_FFprobe=false;
    }
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 ImageMagick
    //==========================================
    //convert
    QString convert_InputPath = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString convert_OutputPath = Current_Path + "/Compatibility_Test/convert_res.bmp";
    QString convert_program = Current_Path+"/convert_waifu2xEX.exe";
    QFile::remove(convert_OutputPath);
    QProcess convert_QProcess;
    convert_QProcess.start("\""+convert_program+"\" \""+convert_InputPath+"\" \""+convert_OutputPath+"\"");
    if(convert_QProcess.waitForStarted(30000))
    {
        while(!convert_QProcess.waitForFinished(100)&&!QProcess_stop) {}
    }
    //identify
    QMap<QString,int> res_map_Compatibility_Test = Image_Gif_Read_Resolution(convert_InputPath);
    if(QFile::exists(convert_OutputPath)&&res_map_Compatibility_Test["height"]>0&&res_map_Compatibility_Test["width"]>0)
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with ImageMagick: Yes."));
        isCompatible_ImageMagick=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with ImageMagick: No."));
        isCompatible_ImageMagick=false;
    }
    QFile::remove(convert_OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 Gifsicle
    //==========================================
    QString Gifsicle_InputPath = Current_Path + "/Compatibility_Test/CompatibilityTest_GIF.gif";
    QString Gifsicle_OutputPath = Current_Path + "/Compatibility_Test/CompatibilityTest_GIF_RES.gif";
    QFile::remove(Gifsicle_OutputPath);
    //===
    QString Gifsicle_program = Current_Path+"/gifsicle_waifu2xEX.exe";
    QString Gifsicle_cmd = "\"" + Gifsicle_program + "\"" + " -O3 -i \""+Gifsicle_InputPath+"\" -o \""+Gifsicle_OutputPath+"\"";
    QProcess *Gifsicle_CompressGIF=new QProcess();
    Gifsicle_CompressGIF->start(Gifsicle_cmd);
    if(Gifsicle_CompressGIF->waitForStarted(30000))
    {
        while(!Gifsicle_CompressGIF->waitForFinished(100)&&!QProcess_stop) {}
    }
    //===
    if(QFile::exists(Gifsicle_OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Gifsicle: Yes."));
        isCompatible_Gifsicle=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Gifsicle: No."));
        isCompatible_Gifsicle=false;
    }
    QFile::remove(Gifsicle_OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 SoX
    //==========================================
    QString SoX_InputPath = Current_Path + "/Compatibility_Test/CompatibilityTest_Sound.wav";
    QString SoX_OutputPath = Current_Path + "/Compatibility_Test/TestTemp_DenoiseProfile.dp";
    QFile::remove(SoX_OutputPath);
    //===
    QString SoX_program = Current_Path+"/SoX/sox_waifu2xEX.exe";
    QProcess SoX_QProcess;
    SoX_QProcess.start("\""+SoX_program+"\" \""+SoX_InputPath+"\" -n noiseprof \""+SoX_OutputPath+"\"");
    if(SoX_QProcess.waitForStarted(30000))
    {
        while(!SoX_QProcess.waitForFinished(100)&&!QProcess_stop) {}
    }
    //===
    if(QFile::exists(SoX_OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with SoX: Yes."));
        isCompatible_SoX=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with SoX: No."));
        isCompatible_SoX=false;
    }
    QFile::remove(SoX_OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 Rife-NCNN-Vulkan
    //==========================================
    QProcess *RifeNcnnVulkan_QProcess = new QProcess();
    QString InputPath_RifeNcnnVulkan_0 = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString InputPath_RifeNcnnVulkan_1 = Current_Path + "/Compatibility_Test/Compatibility_Test_1.jpg";
    QFile::remove(OutputPath);
    QString rife_ncnn_vulkan_ProgramPath = Current_Path+"/rife-ncnn-vulkan/rife-ncnn-vulkan_waifu2xEX.exe";
    cmd = "\"" + rife_ncnn_vulkan_ProgramPath + "\"" + " -0 " + "\"" + InputPath_RifeNcnnVulkan_0 + "\"" + " -1 " + "\"" + InputPath_RifeNcnnVulkan_1 + "\" -o " + "\"" + OutputPath + "\"" + " -j 1:1:1 -m \""+Current_Path+"/rife-ncnn-vulkan/rife-v2.4\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        RifeNcnnVulkan_QProcess->start(cmd);
        if(RifeNcnnVulkan_QProcess->waitForStarted(30000))
        {
            while(!RifeNcnnVulkan_QProcess->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = RifeNcnnVulkan_QProcess->readAllStandardError().toLower();
        QString StanderMSG = RifeNcnnVulkan_QProcess->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Rife-NCNN-Vulkan: Yes."));
        isCompatible_RifeNcnnVulkan=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Rife-NCNN-Vulkan: No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_RifeNcnnVulkan=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 Cain-NCNN-Vulkan
    //==========================================
    QProcess *CainNcnnVulkan_QProcess = new QProcess();
    QFile::remove(OutputPath);
    QString cain_ncnn_vulkan_ProgramPath = Current_Path+"/cain-ncnn-vulkan/cain-ncnn-vulkan_waifu2xEX.exe";
    cmd = "\"" + cain_ncnn_vulkan_ProgramPath + "\"" + " -0 " + "\"" + InputPath_RifeNcnnVulkan_0 + "\"" + " -1 " + "\"" + InputPath_RifeNcnnVulkan_1 + "\" -o " + "\"" + OutputPath + "\"" + " -j 1:1:1 -m \""+Current_Path+"/cain-ncnn-vulkan/cain\"";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        CainNcnnVulkan_QProcess->start(cmd);
        if(CainNcnnVulkan_QProcess->waitForStarted(30000))
        {
            while(!CainNcnnVulkan_QProcess->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = CainNcnnVulkan_QProcess->readAllStandardError().toLower();
        QString StanderMSG = CainNcnnVulkan_QProcess->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Cain-NCNN-Vulkan: Yes."));
        isCompatible_CainNcnnVulkan=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with Cain-NCNN-Vulkan: No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_CainNcnnVulkan=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //==========================================
    //                 Dain-NCNN-Vulkan
    //==========================================
    QProcess *dainNcnnVulkan_QProcess = new QProcess();
    QFile::remove(OutputPath);
    QString dain_ncnn_vulkan_ProgramPath = Current_Path+"/dain-ncnn-vulkan/dain-ncnn-vulkan_waifu2xEX.exe";
    cmd = "\"" + dain_ncnn_vulkan_ProgramPath + "\"" + " -0 " + "\"" + InputPath_RifeNcnnVulkan_0 + "\"" + " -1 " + "\"" + InputPath_RifeNcnnVulkan_1 + "\" -o " + "\"" + OutputPath + "\"" + " -j 1:1:1 -m \""+Current_Path+"/dain-ncnn-vulkan/best\" -t 128";
    for(int CompatTest_retry=0; CompatTest_retry<3; CompatTest_retry++)
    {
        dainNcnnVulkan_QProcess->start(cmd);
        if(dainNcnnVulkan_QProcess->waitForStarted(30000))
        {
            while(!dainNcnnVulkan_QProcess->waitForFinished(100)&&!QProcess_stop) {}
        }
        //=========
        QString ErrorMSG = dainNcnnVulkan_QProcess->readAllStandardError().toLower();
        QString StanderMSG = dainNcnnVulkan_QProcess->readAllStandardOutput().toLower();
        if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
        {
            QFile::remove(OutputPath);
            continue;
        }
        //========
        if(QFile::exists(OutputPath))break;
    }
    if(QFile::exists(OutputPath))
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with dain-NCNN-Vulkan: Yes."));
        isCompatible_DainNcnnVulkan=true;
    }
    else
    {
        emit Send_TextBrowser_NewMessage(tr("Compatible with dain-NCNN-Vulkan: No. [Advice: Re-install gpu driver or update it to the latest.]"));
        isCompatible_DainNcnnVulkan=false;
    }
    QFile::remove(OutputPath);
    emit Send_Add_progressBar_CompatibilityTest();
    //=================
    // 杀死滞留的进程
    //=================
    QStringList TaskNameList;
    TaskNameList << "convert_waifu2xEX.exe"<<"ffmpeg_waifu2xEX.exe"<<"ffprobe_waifu2xEX.exe"<<"identify_waifu2xEX.exe"<<"gifsicle_waifu2xEX.exe"<<"waifu2x-ncnn-vulkan_waifu2xEX.exe"
                 <<"waifu2x-ncnn-vulkan-fp16p_waifu2xEX.exe"<<"Anime4K_waifu2xEX.exe"<<"waifu2x-caffe_waifu2xEX.exe"<<"srmd-ncnn-vulkan_waifu2xEX.exe"<<"realsr-ncnn-vulkan_waifu2xEX.exe"
                 <<"waifu2x-converter-cpp_waifu2xEX.exe"<<"sox_waifu2xEX.exe"<<"rife-ncnn-vulkan_waifu2xEX.exe"<<"cain-ncnn-vulkan_waifu2xEX.exe"<<"dain-ncnn-vulkan_waifu2xEX.exe"
                 <<"srmd-cuda_waifu2xEX.exe"<<"apngdis_waifu2xEX.exe"<<"apngasm_waifu2xEX.exe";
    KILL_TASK_QStringList(TaskNameList,true);
    //================
    //测试结束
    //================
    emit Send_TextBrowser_NewMessage(tr("Compatibility test is complete!"));
    emit Send_SystemTray_NewMessage(tr("Compatibility test is complete!"));
    emit Send_Waifu2x_Compatibility_Test_finished();
    return 0;
#endif
}

int MainWindow::Waifu2x_Compatibility_Test_finished()
{
    //更改checkbox状态以显示测试结果
    ui->checkBox_isCompatible_Waifu2x_NCNN_Vulkan_NEW->setChecked(isCompatible_Waifu2x_NCNN_Vulkan_NEW);
    ui->checkBox_isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P->setChecked(isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P);
    ui->checkBox_isCompatible_Waifu2x_NCNN_Vulkan_OLD->setChecked(isCompatible_Waifu2x_NCNN_Vulkan_OLD);
    ui->checkBox_isCompatible_Waifu2x_Converter->setChecked(isCompatible_Waifu2x_Converter);
    ui->checkBox_isCompatible_SRMD_NCNN_Vulkan->setChecked(isCompatible_SRMD_NCNN_Vulkan);
    ui->checkBox_isCompatible_SRMD_CUDA->setChecked(isCompatible_SRMD_CUDA);
    ui->checkBox_isCompatible_Anime4k_CPU->setChecked(isCompatible_Anime4k_CPU);
    ui->checkBox_isCompatible_Anime4k_GPU->setChecked(isCompatible_Anime4k_GPU);
    ui->checkBox_isCompatible_FFmpeg->setChecked(isCompatible_FFmpeg);
    ui->checkBox_isCompatible_FFprobe->setChecked(isCompatible_FFprobe);
    ui->checkBox_isCompatible_ImageMagick->setChecked(isCompatible_ImageMagick);
    ui->checkBox_isCompatible_Gifsicle->setChecked(isCompatible_Gifsicle);
    ui->checkBox_isCompatible_SoX->setChecked(isCompatible_SoX);
    ui->checkBox_isCompatible_Waifu2x_Caffe_CPU->setChecked(isCompatible_Waifu2x_Caffe_CPU);
    ui->checkBox_isCompatible_Waifu2x_Caffe_GPU->setChecked(isCompatible_Waifu2x_Caffe_GPU);
    ui->checkBox_isCompatible_Waifu2x_Caffe_cuDNN->setChecked(isCompatible_Waifu2x_Caffe_cuDNN);
    ui->checkBox_isCompatible_Realsr_NCNN_Vulkan->setChecked(isCompatible_Realsr_NCNN_Vulkan);
    ui->checkBox_isCompatible_RifeNcnnVulkan->setChecked(isCompatible_RifeNcnnVulkan);
    ui->checkBox_isCompatible_CainNcnnVulkan->setChecked(isCompatible_CainNcnnVulkan);
    ui->checkBox_isCompatible_DainNcnnVulkan->setChecked(isCompatible_DainNcnnVulkan);
    ui->checkBox_isCompatible_RealESRGAN->setChecked(isCompatible_RealESRGAN);
    ui->checkBox_isCompatible_RealCUGAN->setChecked(isCompatible_RealCUGAN);
    ui->checkBox_isCompatible_IFRNetNcnnVulkan->setChecked(isCompatible_IFRNetNcnnVulkan);
    ui->checkBox_isCompatible_RTXSuperRes->setChecked(isCompatible_RTXSuperRes);
    ui->checkBox_isCompatible_NvidiaMaxine->setChecked(isCompatible_NvidiaMaxine);
    ui->checkBox_isCompatible_APNG->setChecked(isCompatible_APNG);
    //解除界面管制
    Finish_progressBar_CompatibilityTest();
    ui->tab_Home->setEnabled(1);
    Start_SystemTrayIcon->setEnabled(1);
    ui->tab_EngineSettings->setEnabled(1);
    ui->tab_VideoSettings->setEnabled(1);
    ui->tab_AdditionalSettings->setEnabled(1);
    ui->pushButton_compatibilityTest->setEnabled(1);
    ui->pushButton_compatibilityTest->setText(tr("Start compatibility test"));
    ui->tabWidget->setCurrentIndex(5);
#ifdef PLATFORM_LINUX
    QStringList resultLines;
    resultLines << tr("Compatibility Test Results") << QString();
    const auto addResult = [&](const QString &name, bool compatible, const QString &guidance) {
        resultLines << tr("%1: %2").arg(name, compatible ? tr("Compatible") : tr("Not compatible"));
        if (!compatible && !guidance.isEmpty())
        {
            resultLines << tr("  Action: %1").arg(guidance);
        }
    };
    const auto packageGuidance = [&](const QString &program, const QString &package) {
        return QStandardPaths::findExecutable(program).isEmpty()
            ? tr("Install package '%1' with your package manager.").arg(package)
            : tr("The installed package '%1' failed its functional check.").arg(package);
    };
    addResult("waifu2x-ncnn-vulkan (Latest)", isCompatible_Waifu2x_NCNN_Vulkan_NEW,
              tr("Install the Linux runtime supplied with this application."));
    addResult("waifu2x-ncnn-vulkan (FP16)", isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P,
              tr("No separate Linux runtime is bundled; use Latest."));
    addResult("waifu2x-ncnn-vulkan (Legacy)", isCompatible_Waifu2x_NCNN_Vulkan_OLD,
              tr("No separate Linux runtime is bundled; use Latest."));
    addResult("SRMD-NCNN-Vulkan", isCompatible_SRMD_NCNN_Vulkan,
              tr("No Linux runtime is bundled for this option."));
    addResult("waifu2x-converter", isCompatible_Waifu2x_Converter,
              tr("No Linux runtime is bundled for this option."));
    addResult("Anime4K (CPU)", isCompatible_Anime4k_CPU,
              tr("No Linux runtime is bundled for this option."));
    addResult("Anime4K (GPU)", isCompatible_Anime4k_GPU,
              tr("No Linux runtime is bundled for this option."));
    addResult("FFmpeg", isCompatible_FFmpeg, packageGuidance("ffmpeg", "ffmpeg"));
    addResult("FFprobe", isCompatible_FFprobe, packageGuidance("ffprobe", "ffmpeg"));
    addResult("ImageMagick", isCompatible_ImageMagick, packageGuidance("convert", "imagemagick"));
    addResult("Gifsicle", isCompatible_Gifsicle, packageGuidance("gifsicle", "gifsicle"));
    addResult("SoX", isCompatible_SoX, packageGuidance("sox", "sox"));
    addResult("waifu2x-caffe (CPU)", isCompatible_Waifu2x_Caffe_CPU,
              tr("No Linux runtime is bundled for this option."));
    addResult("waifu2x-caffe (GPU)", isCompatible_Waifu2x_Caffe_GPU,
              tr("No Linux runtime is bundled for this option."));
    addResult("waifu2x-caffe (cuDNN)", isCompatible_Waifu2x_Caffe_cuDNN,
              tr("No Linux runtime is bundled for this option."));
    addResult("RealSR-NCNN-Vulkan", isCompatible_Realsr_NCNN_Vulkan,
              tr("Install the RealSR Linux runtime and models."));
    addResult("RIFE-NCNN-Vulkan", isCompatible_RifeNcnnVulkan,
              tr("Install the RIFE Linux runtime and models."));
    addResult("CAIN-NCNN-Vulkan", isCompatible_CainNcnnVulkan,
              tr("Install the CAIN Linux runtime and models."));
    addResult("DAIN-NCNN-Vulkan", isCompatible_DainNcnnVulkan,
              tr("Install the DAIN Linux runtime and models."));
    addResult("Real-ESRGAN", isCompatible_RealESRGAN,
              tr("Install the Real-ESRGAN Linux runtime and models."));
    addResult("Real-CUGAN", isCompatible_RealCUGAN,
              tr("Install the Real-CUGAN Linux runtime and models."));
    addResult("IFRNet-NCNN-Vulkan", isCompatible_IFRNetNcnnVulkan,
              tr("Install the IFRNet Linux runtime and models."));
    addResult("RTX Super Resolution", isCompatible_RTXSuperRes,
              tr("Install compatible NVIDIA driver support."));
    addResult("NVIDIA Maxine", isCompatible_NvidiaMaxine,
              tr("Install the NVIDIA Video Effects SDK."));
    const QString apngGuidance = QStandardPaths::findExecutable("apngasm").isEmpty()
        || QStandardPaths::findExecutable("apngdis").isEmpty()
        ? tr("Install packages 'apngasm' and 'apngdis' with your package manager.")
        : tr("The installed APNG tools failed their functional check.");
    addResult("APNG Tools", isCompatible_APNG, apngGuidance);
    QDialog *resultsDialog = new QDialog(this);
    resultsDialog->setAttribute(Qt::WA_DeleteOnClose);
    resultsDialog->setWindowTitle(tr("Compatibility Test Results"));
    resultsDialog->resize(780, 700);
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsDialog);
    QPlainTextEdit *resultsText = new QPlainTextEdit(resultsDialog);
    resultsText->setReadOnly(true);
    resultsText->setPlainText(resultLines.join("\n"));
    resultsLayout->addWidget(resultsText);
    QPushButton *closeResultsButton = new QPushButton(tr("Close"), resultsDialog);
    connect(closeResultsButton, &QPushButton::clicked, resultsDialog, &QDialog::accept);
    resultsLayout->addWidget(closeResultsButton, 0, Qt::AlignRight);
    resultsDialog->show();
    resultsDialog->raise();
    resultsDialog->activateWindow();
    if (isCompatible_Waifu2x_NCNN_Vulkan_NEW)
    {
        ui->comboBox_Engine_Image->setCurrentIndex(0);
        on_comboBox_Engine_Image_currentIndexChanged(0);
        ui->comboBox_version_Waifu2xNCNNVulkan->setCurrentIndex(0);
        on_comboBox_version_Waifu2xNCNNVulkan_currentIndexChanged(0);
    }
    return 0;
#endif
    QtConcurrent::run(this, &MainWindow::Play_NFSound);//兼容性测试完成,播放提示音
    /*
    判断是否有必要部件不兼容,如果有则弹出提示
    */
    if(isCompatible_FFmpeg==false||isCompatible_FFprobe==false||isCompatible_ImageMagick==false||isCompatible_Gifsicle==false||isCompatible_SoX==false)
    {
        QMessageBox *MSG = new QMessageBox();
        MSG->setWindowTitle(tr("Notification"));
        MSG->setText(tr("One of the essential plugins is not compatible with your PC, the software may not work normally on your PC.\n\nYou can try to re-install this software, this might solve the problem."));
        MSG->setIcon(QMessageBox::Warning);
        MSG->setModal(true);
        MSG->show();
    }
    /*
    判断插帧引擎是否全部不兼容
    */
    if(isCompatible_RifeNcnnVulkan==false && isCompatible_CainNcnnVulkan==false)
    {
        QMessageBox *MSG = new QMessageBox();
        MSG->setWindowTitle(tr("Notification"));
        MSG->setText(tr("All of the Frame Interpolation engines are not compatible with your PC, so you will not be able to use the Frame Interpolation function."));
        MSG->setIcon(QMessageBox::Warning);
        MSG->setModal(true);
        MSG->show();
    }
    //========== 提示是否需要自动调整引擎设定 ==========
    QMessageBox Msg(QMessageBox::Question, QString(tr("Notification")), QString(tr("Do you need the software to automatically adjust the engine settings for you based on the compatibility test results?")));
    Msg.setIcon(QMessageBox::Information);
    QAbstractButton *pYesBtn = Msg.addButton(QString(tr("Yes")), QMessageBox::YesRole);
    QAbstractButton *pNoBtn = Msg.addButton(QString(tr("No")), QMessageBox::NoRole);
    Msg.exec();
    if (Msg.clickedButton() == pYesBtn)
    {
        /*
        * 协助用户调整引擎设定:
        */
        //插帧引擎
        if(isCompatible_RifeNcnnVulkan==true || isCompatible_CainNcnnVulkan==true || isCompatible_DainNcnnVulkan==true)
        {
            if(isCompatible_RifeNcnnVulkan)
            {
                ui->comboBox_Engine_VFI->setCurrentIndex(0);
                on_comboBox_Engine_VFI_currentIndexChanged(0);
            }
            else
            {
                if(isCompatible_CainNcnnVulkan)
                {
                    ui->comboBox_Engine_VFI->setCurrentIndex(1);
                    on_comboBox_Engine_VFI_currentIndexChanged(0);
                }
                else
                {
                    if(isCompatible_DainNcnnVulkan)
                    {
                        ui->comboBox_Engine_VFI->setCurrentIndex(2);
                        on_comboBox_Engine_VFI_currentIndexChanged(0);
                    }
                }
            }
        }
        //========== 检查waifu2x-ncnn-vulkan 最新版 的兼容性 ===============
        if(isCompatible_Waifu2x_NCNN_Vulkan_NEW==true)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(0);
            ui->comboBox_Engine_GIF->setCurrentIndex(0);
            ui->comboBox_Engine_Video->setCurrentIndex(0);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_version_Waifu2xNCNNVulkan->setCurrentIndex(0);
            on_comboBox_version_Waifu2xNCNNVulkan_currentIndexChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查Waifu2x_Caffe_GPU的兼容性 ===================
        if(isCompatible_Waifu2x_Caffe_GPU)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(4);
            ui->comboBox_Engine_GIF->setCurrentIndex(4);
            ui->comboBox_Engine_Video->setCurrentIndex(4);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_ProcessMode_Waifu2xCaffe->setCurrentIndex(1);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查Waifu2x_Caffe_cuDNN的兼容性 ===================
        if(isCompatible_Waifu2x_Caffe_cuDNN)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(4);
            ui->comboBox_Engine_GIF->setCurrentIndex(4);
            ui->comboBox_Engine_Video->setCurrentIndex(4);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_ProcessMode_Waifu2xCaffe->setCurrentIndex(2);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //========== 检查waifu2x-ncnn-vulkan FP16P 的兼容性 ===============
        if(isCompatible_Waifu2x_NCNN_Vulkan_NEW_FP16P==true)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(0);
            ui->comboBox_Engine_GIF->setCurrentIndex(0);
            ui->comboBox_Engine_Video->setCurrentIndex(0);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_version_Waifu2xNCNNVulkan->setCurrentIndex(1);
            on_comboBox_version_Waifu2xNCNNVulkan_currentIndexChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //========== 检查waifu2x-ncnn-vulkan 老版本 的兼容性 ===============
        if(isCompatible_Waifu2x_NCNN_Vulkan_OLD==true)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(0);
            ui->comboBox_Engine_GIF->setCurrentIndex(0);
            ui->comboBox_Engine_Video->setCurrentIndex(0);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_version_Waifu2xNCNNVulkan->setCurrentIndex(2);
            on_comboBox_version_Waifu2xNCNNVulkan_currentIndexChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查SRMD-NCNN-Vulkan的兼容性 ===================
        if(isCompatible_SRMD_NCNN_Vulkan==true)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(2);
            ui->comboBox_Engine_GIF->setCurrentIndex(2);
            ui->comboBox_Engine_Video->setCurrentIndex(3);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查waifu2x-converter的兼容性 ===================
        if(isCompatible_Waifu2x_Converter==true)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(1);
            ui->comboBox_Engine_GIF->setCurrentIndex(1);
            ui->comboBox_Engine_Video->setCurrentIndex(1);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查Anime4K的兼容性 ===================
        if(isCompatible_Anime4k_GPU==true)
        {
            isShowAnime4kWarning=false;
            ui->comboBox_Engine_Image->setCurrentIndex(3);
            ui->comboBox_Engine_GIF->setCurrentIndex(3);
            ui->comboBox_Engine_Video->setCurrentIndex(2);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //=====
            ui->checkBox_GPUMode_Anime4K->setChecked(1);
            on_checkBox_GPUMode_Anime4K_stateChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        if(isCompatible_Anime4k_CPU==true)
        {
            isShowAnime4kWarning=false;
            ui->comboBox_Engine_Image->setCurrentIndex(3);
            ui->comboBox_Engine_GIF->setCurrentIndex(3);
            ui->comboBox_Engine_Video->setCurrentIndex(2);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //=====
            ui->checkBox_GPUMode_Anime4K->setChecked(0);
            on_checkBox_GPUMode_Anime4K_stateChanged(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查Waifu2x_Caffe_CPU的兼容性 ===================
        if(isCompatible_Waifu2x_Caffe_CPU)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(4);
            ui->comboBox_Engine_GIF->setCurrentIndex(4);
            ui->comboBox_Engine_Video->setCurrentIndex(4);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->comboBox_ProcessMode_Waifu2xCaffe->setCurrentIndex(0);
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查Realsr_NCNN_Vulkan的兼容性 ===================
        if(isCompatible_Realsr_NCNN_Vulkan)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(5);
            ui->comboBox_Engine_GIF->setCurrentIndex(5);
            ui->comboBox_Engine_Video->setCurrentIndex(5);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //======================= 检查 SRMD-CUDA 的兼容性 ===================
        if(isCompatible_SRMD_CUDA)
        {
            ui->comboBox_Engine_Image->setCurrentIndex(6);
            ui->comboBox_Engine_GIF->setCurrentIndex(6);
            ui->comboBox_Engine_Video->setCurrentIndex(6);
            on_comboBox_Engine_Image_currentIndexChanged(0);
            on_comboBox_Engine_GIF_currentIndexChanged(0);
            on_comboBox_Engine_Video_currentIndexChanged(0);
            //====
            ui->tabWidget->setCurrentIndex(1);
            return 0;
        }
        //啥引擎都不兼容,提示用户自行修复兼容性问题
        QMessageBox *MSG_ = new QMessageBox();
        MSG_->setWindowTitle(tr("Notification"));
        MSG_->setText(tr("According to the compatibility test results, all engines are not compatible with your computer. Please try to fix the compatibility issue according to the following tutorial:\n\nFirst, try to update your graphics card driver. For the specific update method, please google it. After updating the graphics card driver, open the graphics card driver to see if the current driver supports vulkan (that is, see if can you check the vulkan Driver version and vulkan API version).\n\nIf the current driver supports vulkan but still can't use waifu2x-ncnn-vulkan normally, then manually download the latest version of the beta driver and install it. And then test again to see if your PC is compatible with waifu2x-ncnn-vulkan.\n\nIf it is still not compatible, please uninstall and reinstall the graphics driver and update the Windows OS."));
        MSG_->setIcon(QMessageBox::Warning);
        MSG_->setModal(true);
        MSG_->show();
        return 0;
    }
    if (Msg.clickedButton() == pNoBtn)
    {
        //提醒用户检查测试结果
        QMessageBox *MSG = new QMessageBox();
        MSG->setWindowTitle(tr("Notification"));
        MSG->setText(tr("The compatibility test has been completed. Please check the test results and configure the engine settings based on the test results."));
        MSG->setIcon(QMessageBox::Information);
        MSG->setModal(true);
        MSG->show();
        return 0;
    }
    //===============
    return 0;
}

//初始化 -兼容性测试进度条
void MainWindow::Init_progressBar_CompatibilityTest()
{
    ui->progressBar_CompatibilityTest->setEnabled(1);
    ui->progressBar_CompatibilityTest->setVisible(1);
    ui->progressBar_CompatibilityTest->setRange(0,26);
    ui->progressBar_CompatibilityTest->setValue(0);
}
//进度+1 -兼容性测试进度条
void MainWindow::Add_progressBar_CompatibilityTest()
{
    int CurrentValue = ui->progressBar_CompatibilityTest->value();
    ui->progressBar_CompatibilityTest->setValue(CurrentValue+1);
}
//兼容性测试完成后的操作 -兼容性测试进度条
void MainWindow::Finish_progressBar_CompatibilityTest()
{
    ui->progressBar_CompatibilityTest->setVisible(0);
}
