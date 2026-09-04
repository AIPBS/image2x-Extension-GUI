#include "runtime_dependencies.h"

#include <QDir>
#include <QFileInfo>

RuntimeDependencies::RuntimeDependencies(const QString &applicationDirectory)
    : applicationDirectory(applicationDirectory)
{
}

QString RuntimeDependencies::engineDirectory(RuntimeEngine engine) const
{
    switch (engine)
    {
        case RuntimeEngine::Waifu2xNcnnVulkan:
#ifdef PLATFORM_LINUX
            return QDir(applicationDirectory).filePath(
                QStringLiteral("dependencies/engines/waifu2x-ncnn-vulkan"));
#else
            return QDir(applicationDirectory).filePath(
                QStringLiteral("waifu2x-ncnn-vulkan"));
#endif
    }

    return QString();
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
    }

    return QString();
}

QStringList RuntimeDependencies::missingFiles(RuntimeEngine engine) const
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

    if (!QFileInfo(QDir(directory).filePath(QStringLiteral("models-cunet"))).isDir())
    {
        missing.append(QDir(directory).filePath(QStringLiteral("models-cunet")));
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
    }

    return QString();
}
