QT += core gui testlib widgets
CONFIG += testcase c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += \
    ../image2x-ui/test_support \
    ../image2x-ui/services \
    ../image2x-ui/presentation \
    ../image2x-ui/engine_legacy

SOURCES += \
    image2x_logic_tests.cpp \
    ../image2x-ui/test_support/engine_test_runner.cpp \
    ../image2x-ui/test_support/model_test_matrix.cpp \
    ../image2x-ui/services/runtime_dependencies.cpp

HEADERS += \
    ../image2x-ui/test_support/engine_test_runner.h \
    ../image2x-ui/test_support/model_test_matrix.h \
    ../image2x-ui/services/runtime_dependencies.h \
    ../image2x-ui/presentation/compatibility_presentation.h \
    ../image2x-ui/presentation/ui_routing.h

TARGET = image2x-logic-tests
