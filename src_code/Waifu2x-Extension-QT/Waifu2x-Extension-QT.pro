#    Copyright (C) 2026  AIPEAC
#
#    This file is part of Waifu2x-Extension-GUI Reconstructed.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published
#    by the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    ADDITIONAL PERMISSION under GNU AGPL version 3 section 7:
#    As a special exception, the copyright holder of this file gives you
#    permission to use, copy, modify, merge, publish, distribute, sublicense,
#    and/or sell this file without restriction, as though it were licensed
#    under a permissive license. This additional permission applies only to
#    this specific file and files explicitly marked with this notice.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#    Based on the original Waifu2x-Extension-GUI by Aaron Feng:
#    https://github.com/AaronFeng753/Waifu2x-Extension-GUI
#
#    Cross-platform build for Waifu2x-Extension-GUI v3.139.01 (Reconstructed)
#
QT       += core gui
QT       += concurrent
QT       += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

# --- Cross-platform detection ---
win32 {
    DEFINES += PLATFORM_WINDOWS
    LIBS += -luser32
    RC_ICONS = icon/icon.ico
}
unix:!macx {
    DEFINES += PLATFORM_LINUX
    CONFIG += link_pkgconfig
    PKGCONFIG += x11
}
macx {
    DEFINES += PLATFORM_MACOS
}

# --- Source files ---
SOURCES += \
    AnimatedPNG.cpp \
    Anime4K.cpp \
    CompatibilityTest.cpp \
    Current_File_Progress.cpp \
    CustomResolution.cpp \
    Donate.cpp \
    Finish_Action.cpp \
    Frame_Interpolation.cpp \
    Right-click_Menu.cpp \
    SystemTrayIcon.cpp \
    Web_Activities.cpp \
    checkupdate.cpp \
    files.cpp \
    gif.cpp \
    image.cpp \
    main.cpp \
    mainwindow.cpp \
    progressBar.cpp \
    realsr_ncnn_vulkan.cpp \
    settings.cpp \
    srmd-cuda.cpp \
    srmd_ncnn_vulkan.cpp \
    table.cpp \
    textBrowser.cpp \
    topsupporterslist.cpp \
    video.cpp \
    waifu2x.cpp \
    waifu2x_caffe.cpp \
    waifu2x_converter.cpp \
    waifu2x_ncnn_vulkan.cpp \
    realesrgan_ncnn_vulkan.cpp \
    realcugan_ncnn_vulkan.cpp \
    rtx_superres.cpp \
    ifrnet_ncnn_vulkan.cpp \
    cain_ncnn_vulkan.cpp \
    dain_ncnn_vulkan.cpp \
    rife_v4_ncnn_vulkan.cpp \
    nvidia_maxine.cpp

HEADERS += \
    mainwindow.h \
    topsupporterslist.h \
    platform_compat.h \
    engine_registry.h

FORMS += \
    mainwindow.ui \
    topsupporterslist.ui

TRANSLATIONS += language_English.ts \
               language_Chinese.ts \
               language_TraditionalChinese.ts

TARGET = image2x-Extension-GUI

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    OtherPic.qrc \
    donate.qrc \
    icon.qrc
