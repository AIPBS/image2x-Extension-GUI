QT += core gui
CONFIG += console c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += ../image2x-Extension-QT

SOURCES += \
    image2x_model_tests.cpp \
    ../image2x-Extension-QT/engine_test_runner.cpp \
    ../image2x-Extension-QT/model_test_matrix.cpp

HEADERS += \
    ../image2x-Extension-QT/engine_test_runner.h \
    ../image2x-Extension-QT/model_test_matrix.h

TARGET = image2x-model-tests
