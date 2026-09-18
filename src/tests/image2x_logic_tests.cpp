/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#include "compatibility_presentation.h"
#include "engine_test_runner.h"
#include "model_test_matrix.h"
#include "runtime_dependencies.h"
#include "ui_routing.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class Image2xLogicTests : public QObject
{
    Q_OBJECT

private slots:
    void deviceClassification()
    {
        QVERIFY(EngineTestRunner::isSoftwareDevice(QStringLiteral("llvmpipe (LLVM 15.0.7)")));
        QVERIFY(EngineTestRunner::isSoftwareDevice(QStringLiteral("lavapipe")));
        QVERIFY(!EngineTestRunner::isSoftwareDevice(QStringLiteral("NVIDIA GeForce RTX")));
    }

    void compatibilityPresentation()
    {
        const CompatibilityPairState gpuPass = CompatibilityPresentation::pairState(true, false);
        QVERIFY(gpuPass.gpuChecked);
        QVERIFY(!gpuPass.cpuChecked);
        QVERIFY(!gpuPass.cpuEnabled);

        const CompatibilityPairState gpuFailCpuPass = CompatibilityPresentation::pairState(false, true);
        QVERIFY(!gpuFailCpuPass.gpuChecked);
        QVERIFY(gpuFailCpuPass.cpuChecked);
        QVERIFY(gpuFailCpuPass.cpuEnabled);
    }

    void compatibilityTabRouting()
    {
        QTabWidget tabs;
        QWidget first;
        QWidget compatibility;
        tabs.addTab(&first, QStringLiteral("Home"));
        tabs.addTab(&compatibility, QStringLiteral("Compatibility"));
        QCOMPARE(UiRouting::compatibilityTabIndex(&tabs, &compatibility), 1);
    }

    void modelMatrixCoverage()
    {
        const QList<ModelTestCase> publicCases = publicModelTestMatrix();
        QCOMPARE(publicCases.size(), 33);
        QVERIFY(!proprietaryModelTestMatrix().isEmpty());
        for (const ModelTestCase &testCase : publicCases)
        {
            QVERIFY2(!testCase.name.isEmpty(), "model test name must not be empty");
            QVERIFY2(!testCase.engine.isEmpty(), "model test engine must not be empty");
            QVERIFY2(!testCase.model.isEmpty(), "model test model must not be empty");
        }
    }

    void runtimeAvailabilityReportsMissingFiles()
    {
        QTemporaryDir temporaryDirectory;
        QVERIFY(temporaryDirectory.isValid());
        const RuntimeDependencies dependencies(temporaryDirectory.path());
        QVERIFY(!dependencies.isAvailable(RuntimeEngine::Waifu2xNcnnVulkan));
        QVERIFY(!dependencies.missingFiles(RuntimeEngine::Waifu2xNcnnVulkan).isEmpty());
    }

    void outputValidation()
    {
        QTemporaryDir temporaryDirectory;
        QVERIFY(temporaryDirectory.isValid());
        const QString path = QDir(temporaryDirectory.path()).filePath(QStringLiteral("output.png"));
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(Qt::white);
        QVERIFY(image.save(path));
        QVERIFY(EngineTestRunner::isValidImage(path));
    }

    void missingExecutableIsReported()
    {
        EngineTestRequest request;
        request.name = QStringLiteral("missing");
        request.executable = QStringLiteral("/does/not/exist/image2x-engine");
        request.outputPath = QStringLiteral("/tmp/image2x-missing-output.png");
        const EngineTestResult result = EngineTestRunner::run(request);
        QVERIFY(!result.passed);
        QVERIFY(!result.started);
        QVERIFY(!result.diagnostic.isEmpty());
    }
};

QTEST_MAIN(Image2xLogicTests)
#include "image2x_logic_tests.moc"
