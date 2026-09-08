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

    Cross-platform compatibility header.
    Provides platform detection macros, path resolution, and engine path helpers.
*/

#ifndef PLATFORM_COMPAT_H
#define PLATFORM_COMPAT_H

#include <QString>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QCoreApplication>

// Platform detection macros are defined in the .pro file:
//   win32       -> PLATFORM_WINDOWS
//   unix:!macx  -> PLATFORM_LINUX
//   macx        -> PLATFORM_MACOS

#ifdef PLATFORM_WINDOWS
    #define EXE_SUFFIX ".exe"
    #define ENGINE_SUFFIX "_waifu2xEX.exe"
    #define PATH_SEP "\\"
    #include <windows.h>
#elif defined(PLATFORM_LINUX)
    #define EXE_SUFFIX ""
    #define ENGINE_SUFFIX ""
    #define PATH_SEP "/"
    // No windows.h on Linux — provide a cross-platform Sleep replacement
    inline void Sleep(unsigned long ms) { QThread::msleep(ms); }
#elif defined(PLATFORM_MACOS)
    #define EXE_SUFFIX ""
    #define ENGINE_SUFFIX ""
    #define PATH_SEP "/"
    inline void Sleep(unsigned long ms) { QThread::msleep(ms); }
#else
    // Fallback for unknown platforms — treat like Linux
    #define EXE_SUFFIX ""
    #define ENGINE_SUFFIX ""
    #define PATH_SEP "/"
    inline void Sleep(unsigned long ms) { QThread::msleep(ms); }
#endif

/*
 * Resolve the full path to an engine executable.
 *
 * On Windows the executable is named "<exeName>_waifu2xEX.exe".
 * On Linux/macOS it is named "<exeName>" (no suffix).
 *
 *   baseDir    — application directory (Current_Path)
 *   engineName — subfolder name, e.g. "rife-ncnn-vulkan"
 *   exeName    — base executable name, e.g. "rife-ncnn-vulkan"
 *
 * Returns the native-separator full path.
 */
inline QString resolveEnginePath(const QString &baseDir,
                                 const QString &engineName,
                                 const QString &exeName)
{
    QString path = baseDir + "/" + engineName + "/" + exeName + ENGINE_SUFFIX;

    // On non-Windows platforms, also try without ENGINE_SUFFIX
    // (the ENGINE_SUFFIX is empty for Linux/macOS, so this is already correct)
    if (!QFile::exists(path))
    {
        // Try plain exeName as a fallback
        path = baseDir + "/" + engineName + "/" + exeName;
    }

    return QDir::toNativeSeparators(path);
}

/*
 * Resolve the path to a model directory within an engine folder.
 *
 *   baseDir    — application directory (Current_Path)
 *   engineName — subfolder name, e.g. "rife-ncnn-vulkan"
 *   modelDir   — model sub-directory, e.g. "rife-v4.6"
 */
inline QString resolveModelPath(const QString &baseDir,
                                const QString &engineName,
                                const QString &modelDir)
{
    QString path = baseDir + "/" + engineName + "/" + modelDir;
    return QDir::toNativeSeparators(path);
}

/*
 * Return the platform-appropriate application data path.
 * On Windows this is AppData/Roaming; on Linux $HOME/.local/share.
 */
inline QString getAppDataPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

/*
 * Return the application's own directory (equivalent to qApp->applicationDirPath()).
 */
inline QString getAppDir()
{
    return QCoreApplication::applicationDirPath();
}

/*
 * Check whether the current platform is Windows.
 */
inline bool isPlatformWindows()
{
#ifdef PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}

/*
 * Check whether the current platform is Linux.
 */
inline bool isPlatformLinux()
{
#ifdef PLATFORM_LINUX
    return true;
#else
    return false;
#endif
}

/*
 * Check whether the current platform is macOS.
 */
inline bool isPlatformMacOS()
{
#ifdef PLATFORM_MACOS
    return true;
#else
    return false;
#endif
}

/*
 * Return a human-readable platform name.
 */
inline QString platformName()
{
#ifdef PLATFORM_WINDOWS
    return QStringLiteral("Windows");
#elif defined(PLATFORM_LINUX)
    return QStringLiteral("Linux");
#elif defined(PLATFORM_MACOS)
    return QStringLiteral("macOS");
#else
    return QStringLiteral("Unknown");
#endif
}

#endif // PLATFORM_COMPAT_H
