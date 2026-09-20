QT += core gui network
CONFIG += console c++17
TEMPLATE = app

DEFINES += PLATFORM_LINUX
INCLUDEPATH += \
    ../image2x-ui/test_support \
    ../image2x-ui/services \
    ../image2x-ui/presentation \
    ../image2x-ui/engine_legacy

SOURCES += \
    image2x_model_tests.cpp \
    ../image2x-ui/ipc/backend_client.cpp \
    ../image2x-ui/test_support/engine_test_runner.cpp \
    ../image2x-ui/test_support/model_test_matrix.cpp

HEADERS += \
    ../image2x-ui/ipc/backend_client.h \
    ../image2x-ui/test_support/engine_test_runner.h \
    ../image2x-ui/test_support/model_test_matrix.h

TARGET = image2x-model-tests
