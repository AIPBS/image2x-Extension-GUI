QT += core gui testlib widgets
CONFIG += testcase c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += ../image2x-Extension-QT

SOURCES += \
    image2x_logic_tests.cpp \
    ../image2x-Extension-QT/engine_test_runner.cpp \
    ../image2x-Extension-QT/model_test_matrix.cpp \
    ../image2x-Extension-QT/runtime_dependencies.cpp

HEADERS += \
    ../image2x-Extension-QT/engine_test_runner.h \
    ../image2x-Extension-QT/model_test_matrix.h \
    ../image2x-Extension-QT/runtime_dependencies.h \
    ../image2x-Extension-QT/compatibility_presentation.h \
    ../image2x-Extension-QT/ui_routing.h

TARGET = image2x-logic-tests
