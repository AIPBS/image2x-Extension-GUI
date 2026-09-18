QT += core gui testlib widgets
CONFIG += testcase c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += ../image2x-ui

SOURCES += \
    image2x_logic_tests.cpp \
    ../image2x-ui/engine_test_runner.cpp \
    ../image2x-ui/model_test_matrix.cpp \
    ../image2x-ui/runtime_dependencies.cpp

HEADERS += \
    ../image2x-ui/engine_test_runner.h \
    ../image2x-ui/model_test_matrix.h \
    ../image2x-ui/runtime_dependencies.h \
    ../image2x-ui/compatibility_presentation.h \
    ../image2x-ui/ui_routing.h

TARGET = image2x-logic-tests
