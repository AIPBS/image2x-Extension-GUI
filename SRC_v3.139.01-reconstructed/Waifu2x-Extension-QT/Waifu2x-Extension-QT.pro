#    Waifu2x-Extension-GUI - Reconstructed v3.139.01
#    Cross-platform build (Linux + Windows)

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

TARGET = Waifu2x-Extension-GUI

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    OtherPic.qrc \
    donate.qrc \
    icon.qrc
