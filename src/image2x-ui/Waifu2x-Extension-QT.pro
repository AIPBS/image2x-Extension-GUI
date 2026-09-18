#    Copyright (C) 2026  AIPEAC
#
#    This file is part of Waifu2x-Extension-GUI Reconstructed.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published
#    by the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
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
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += \
    app \
    presentation \
    widgets \
    media \
    engine_legacy \
    ipc \
    services \
    test_support

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
    app/main.cpp \
    app/mainwindow.cpp \
    ipc/backend_client.cpp \
    media/AnimatedPNG.cpp \
    media/Frame_Interpolation.cpp \
    media/gif.cpp \
    media/image.cpp \
    media/video.cpp \
    presentation/CompatibilityTest.cpp \
    presentation/CustomResolution.cpp \
    presentation/Current_File_Progress.cpp \
    presentation/Finish_Action.cpp \
    presentation/files.cpp \
    presentation/Right-click_Menu.cpp \
    presentation/settings.cpp \
    presentation/table.cpp \
    services/Donate.cpp \
    services/Web_Activities.cpp \
    services/checkupdate.cpp \
    services/runtime_dependencies.cpp \
    widgets/SystemTrayIcon.cpp \
    widgets/progressBar.cpp \
    widgets/textBrowser.cpp \
    widgets/topsupporterslist.cpp \
    engine_legacy/Anime4K.cpp \
    engine_legacy/cain_ncnn_vulkan.cpp \
    engine_legacy/dain_ncnn_vulkan.cpp \
    engine_legacy/ifrnet_ncnn_vulkan.cpp \
    engine_legacy/nvidia_maxine.cpp \
    engine_legacy/realcugan_ncnn_vulkan.cpp \
    engine_legacy/realesrgan_ncnn_vulkan.cpp \
    engine_legacy/realsr_ncnn_vulkan.cpp \
    engine_legacy/rife_v4_ncnn_vulkan.cpp \
    engine_legacy/rtx_superres.cpp \
    engine_legacy/srmd-cuda.cpp \
    engine_legacy/srmd_ncnn_vulkan.cpp \
    engine_legacy/waifu2x.cpp \
    engine_legacy/waifu2x_caffe.cpp \
    engine_legacy/waifu2x_converter.cpp \
    engine_legacy/waifu2x_ncnn_vulkan.cpp \
    test_support/engine_test_runner.cpp \
    test_support/model_test_matrix.cpp

HEADERS += \
    app/mainwindow.h \
    ipc/backend_client.h \
    presentation/compatibility_presentation.h \
    presentation/ui_routing.h \
    widgets/topsupporterslist.h \
    services/runtime_dependencies.h \
    engine_legacy/engine_registry.h \
    engine_legacy/platform_compat.h \
    test_support/engine_test_runner.h \
    test_support/model_test_matrix.h

FORMS += \
    app/mainwindow.ui \
    widgets/topsupporterslist.ui

TRANSLATIONS += translations/language_English.ts \
               translations/language_Chinese.ts \
               translations/language_TraditionalChinese.ts

TARGET = image2x-Extension-GUI

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/icon.qrc
