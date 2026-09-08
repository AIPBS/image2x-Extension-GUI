#ifndef RUNTIME_DEPENDENCIES_H
#define RUNTIME_DEPENDENCIES_H

#include <QString>
#include <QStringList>

enum class RuntimeEngine {
    Waifu2xNcnnVulkan,
};

class RuntimeDependencies
{
public:
    explicit RuntimeDependencies(const QString &applicationDirectory);

    QString engineDirectory(RuntimeEngine engine) const;
    QString executable(RuntimeEngine engine) const;
    QStringList missingFiles(RuntimeEngine engine) const;
    bool isAvailable(RuntimeEngine engine) const;
    QString installCommand(RuntimeEngine engine) const;

private:
    QString applicationDirectory;
};

#endif // RUNTIME_DEPENDENCIES_H
