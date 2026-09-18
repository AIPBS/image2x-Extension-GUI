QT += core gui
CONFIG += console c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += ../image2x-ui

SOURCES += \
    image2x_model_tests.cpp \
    ../image2x-ui/engine_test_runner.cpp \
    ../image2x-ui/model_test_matrix.cpp

HEADERS += \
    ../image2x-ui/engine_test_runner.h \
    ../image2x-ui/model_test_matrix.h

TARGET = image2x-model-tests
