#include "runtime_dependencies.h"

#include <QDir>
#include <QFileInfo>
#include <QPair>
#include <QStandardPaths>

RuntimeDependencies::RuntimeDependencies(const QString &applicationDirectory)
    : applicationDirectory(applicationDirectory)
{
}

QString RuntimeDependencies::engineDirectory(RuntimeEngine engine) const
{
    QString engineName;
    switch (engine)
    {
        case RuntimeEngine::Waifu2xNcnnVulkan:
            engineName = QStringLiteral("waifu2x-ncnn-vulkan");
            break;
        case RuntimeEngine::SrmdNcnnVulkan:
            engineName = QStringLiteral("srmd-ncnn-vulkan");
            break;
        case RuntimeEngine::RealSrNcnnVulkan:
            engineName = QStringLiteral("realsr-ncnn-vulkan");
            break;
        case RuntimeEngine::RealESRGANNcnnVulkan:
            engineName = QStringLiteral("realesrgan-ncnn-vulkan");
            break;
        case RuntimeEngine::RealCUGANNcnnVulkan:
            engineName = QStringLiteral("realcugan-ncnn-vulkan");
            break;
    }

#ifdef PLATFORM_LINUX
    return QDir(runtimeDirectory()).filePath(engineName);
#else
    return QDir(applicationDirectory).filePath(engineName);
#endif
}

QString RuntimeDependencies::runtimeDirectory() const
{
#ifdef PLATFORM_LINUX
    const QString configured = qEnvironmentVariable("IMAGE2X_RUNTIME_DIRECTORY");
    if (!configured.isEmpty())
    {
        return QDir::cleanPath(configured);
    }
    if (qEnvironmentVariableIsSet("FLATPAK_ID"))
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("dependencies/distributable"));
    }
    return QDir(applicationDirectory).filePath(QStringLiteral("dependencies/distributable"));
#else
    return QDir(applicationDirectory).filePath(QStringLiteral("dependencies/distributable"));
#endif
}

QString RuntimeDependencies::installerScriptPath() const
{
    const QString configured = qEnvironmentVariable("IMAGE2X_RUNTIME_INSTALLER");
    if (!configured.isEmpty())
    {
        return configured;
    }

    const QStringList candidates{
        QDir(applicationDirectory).filePath(QStringLiteral("scripts/install_linux_runtime.sh")),
        QDir(applicationDirectory).filePath(QStringLiteral("../scripts/install_linux_runtime.sh")),
        QDir(applicationDirectory).filePath(
            QStringLiteral("../libexec/image2x/install_linux_runtime.sh")),
    };
    for (const QString &candidate : candidates)
    {
        const QFileInfo info(candidate);
        if (info.isFile() && info.isExecutable())
        {
            return info.absoluteFilePath();
        }
    }
    return QString();
}

QString RuntimeDependencies::proprietaryInstallerScriptPath() const
{
    const QString configured = qEnvironmentVariable("IMAGE2X_PROPRIETARY_INSTALLER");
    if (!configured.isEmpty())
    {
        return configured;
    }

    const QStringList candidates{
        QDir(applicationDirectory).filePath(
            QStringLiteral("scripts/download_non_free_models.sh")),
        QDir(applicationDirectory).filePath(
            QStringLiteral("../scripts/download_non_free_models.sh")),
        QDir(applicationDirectory).filePath(
            QStringLiteral("../libexec/image2x/download_non_free_models.sh")),
    };
    for (const QString &candidate : candidates)
    {
        const QFileInfo info(candidate);
        if (info.isFile() && info.isExecutable())
        {
            return info.absoluteFilePath();
        }
    }
    return QString();
}

QString RuntimeDependencies::proprietaryModelDirectory() const
{
    const QString configured = qEnvironmentVariable("IMAGE2X_PROPRIETARY_MODEL_ROOT");
    if (!configured.isEmpty())
    {
        return QDir::cleanPath(configured);
    }
#ifdef PLATFORM_LINUX
    if (qEnvironmentVariableIsSet("FLATPAK_ID"))
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("dependencies/non-free"));
    }
#endif
    return QDir(applicationDirectory).filePath(QStringLiteral("dependencies/non-free"));
}

bool RuntimeDependencies::hasMissingDistributable() const
{
    const RuntimeEngine engines[] = {
        RuntimeEngine::Waifu2xNcnnVulkan,
        RuntimeEngine::SrmdNcnnVulkan,
        RuntimeEngine::RealSrNcnnVulkan,
        RuntimeEngine::RealESRGANNcnnVulkan,
        RuntimeEngine::RealCUGANNcnnVulkan,
    };
    for (const RuntimeEngine engine : engines)
    {
        if (!isAvailable(engine))
        {
            return true;
        }
    }
    const QList<QPair<QString, QString>> frameEngines{
        {QStringLiteral("rife-ncnn-vulkan"), QStringLiteral("rife-ncnn-vulkan")},
        {QStringLiteral("ifrnet-ncnn-vulkan"), QStringLiteral("ifrnet-ncnn-vulkan")},
        {QStringLiteral("cain-ncnn-vulkan"), QStringLiteral("cain-ncnn-vulkan")},
        {QStringLiteral("dain-ncnn-vulkan"), QStringLiteral("dain-ncnn-vulkan")},
    };
    const QDir root(runtimeDirectory());
    for (const auto &engine : frameEngines)
    {
        if (!QFileInfo(root.filePath(engine.first)).isDir()
            || !QFileInfo(root.filePath(engine.first + QLatin1Char('/') + engine.second)).isExecutable())
        {
            return true;
        }
    }
    return false;
}

bool RuntimeDependencies::hasMissingProprietaryModels() const
{
    const QStringList requiredFiles{
        QStringLiteral("realesrgan-ncnn-vulkan/models/Anime-HQ-W4xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Anime-HQ-W4xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/AnimeVideo-MiniV1.8-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/AnimeVideo-MiniV1.8-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-MiniV2-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-MiniV2-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-Smallv2-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-Smallv2-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-TurboV1.5-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Omni-TurboV1.5-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-HQ-W4xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-HQ-W4xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-Small-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-Small-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Universal-FastV2-W2xEX.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Universal-FastV2-W2xEX.param"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-Conservative-x4.bin"),
        QStringLiteral("realesrgan-ncnn-vulkan/models/Photo-Conservative-x4.param"),
    };
    const QDir root(proprietaryModelDirectory());
    for (const QString &file : requiredFiles)
    {
        if (!QFileInfo(root.filePath(file)).isFile())
        {
            return true;
        }
    }
    return false;
}

QString RuntimeDependencies::executable(RuntimeEngine engine) const
{
    const QString directory = engineDirectory(engine);

    switch (engine)
    {
        case RuntimeEngine::Waifu2xNcnnVulkan:
#ifdef PLATFORM_LINUX
            return QDir(directory).filePath(QStringLiteral("waifu2x-ncnn-vulkan"));
#else
            return QDir(directory).filePath(
                QStringLiteral("waifu2x-ncnn-vulkan_waifu2xEX.exe"));
#endif
        case RuntimeEngine::SrmdNcnnVulkan:
            return QDir(directory).filePath(QStringLiteral("srmd-ncnn-vulkan"));
        case RuntimeEngine::RealSrNcnnVulkan:
            return QDir(directory).filePath(QStringLiteral("realsr-ncnn-vulkan"));
        case RuntimeEngine::RealESRGANNcnnVulkan:
            return QDir(directory).filePath(QStringLiteral("realesrgan-ncnn-vulkan"));
        case RuntimeEngine::RealCUGANNcnnVulkan:
            return QDir(directory).filePath(QStringLiteral("realcugan-ncnn-vulkan"));
    }

    return QString();
}

QStringList RuntimeDependencies::missingFiles(RuntimeEngine engine,
                                               const QString &modelRelativePath) const
{
    QStringList missing;
    const QString directory = engineDirectory(engine);
    const QString program = executable(engine);

    if (!QFileInfo(directory).isDir())
    {
        missing.append(directory);
    }

    const QFileInfo programInfo(program);
    if (!programInfo.isFile() || !programInfo.isExecutable())
    {
        missing.append(program);
    }

    QStringList requiredModels;
    if (modelRelativePath.isEmpty())
    {
        switch (engine)
        {
            case RuntimeEngine::Waifu2xNcnnVulkan:
                requiredModels << QStringLiteral("models-cunet")
                               << QStringLiteral("models-upconv_7_anime_style_art_rgb")
                               << QStringLiteral("models-upconv_7_photo");
                break;
            case RuntimeEngine::SrmdNcnnVulkan:
                requiredModels << QStringLiteral("models-srmd");
                break;
            case RuntimeEngine::RealSrNcnnVulkan:
                requiredModels << QStringLiteral("models-DF2K")
                               << QStringLiteral("models-DF2K_JPEG");
                break;
            case RuntimeEngine::RealESRGANNcnnVulkan:
                requiredModels << QStringLiteral("models");
                break;
            case RuntimeEngine::RealCUGANNcnnVulkan:
                requiredModels << QStringLiteral("models-se")
                               << QStringLiteral("models-pro")
                               << QStringLiteral("models-nose");
                break;
        }
    }
    else
    {
        requiredModels << modelRelativePath;
    }

    for (const QString &model : requiredModels)
    {
        const QString modelPath = QDir(directory).filePath(model);
        const bool modelDirectoryExists = QFileInfo(modelPath).isDir();
        const bool modelPairExists = QFileInfo(modelPath + QStringLiteral(".param")).isFile()
            && QFileInfo(modelPath + QStringLiteral(".bin")).isFile();
        if (!modelDirectoryExists && !modelPairExists)
        {
            missing.append(modelPath);
        }
    }

    return missing;
}

bool RuntimeDependencies::isAvailable(RuntimeEngine engine) const
{
    return missingFiles(engine).isEmpty();
}

QString RuntimeDependencies::installCommand(RuntimeEngine engine) const
{
    switch (engine)
    {
        case RuntimeEngine::Waifu2xNcnnVulkan:
            return QStringLiteral(
                "./scripts/install_linux_runtime.sh \"%1\"")
                .arg(applicationDirectory);
        case RuntimeEngine::SrmdNcnnVulkan:
        case RuntimeEngine::RealSrNcnnVulkan:
        case RuntimeEngine::RealESRGANNcnnVulkan:
        case RuntimeEngine::RealCUGANNcnnVulkan:
            return QStringLiteral(
                "./scripts/install_linux_runtime.sh \"%1\"")
                .arg(applicationDirectory);
    }

    return QString();
}
