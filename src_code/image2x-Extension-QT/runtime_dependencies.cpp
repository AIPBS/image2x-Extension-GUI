#include "runtime_dependencies.h"

#include <QDir>
#include <QFileInfo>

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
    return QDir(applicationDirectory).filePath(
        QStringLiteral("dependencies/engines/%1").arg(engineName));
#else
    return QDir(applicationDirectory).filePath(engineName);
#endif
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
            return QString();
    }

    return QString();
}
