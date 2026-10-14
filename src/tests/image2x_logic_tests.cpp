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
#include "frame_gpu_probe.h"
#include "model_test_matrix.h"
#include "runtime_dependencies.h"
#include "ui_routing.h"
#include "../image2x-ui/ipc/frame_request.h"

#include <QDir>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class Image2xLogicTests : public QObject
{
    Q_OBJECT

private slots:
    void cainDainFrameRounds_data()
    {
        QTest::addColumn<int>("engineIndex");
        QTest::addColumn<int>("fpsMultiple");
        QTest::addColumn<QString>("engine");
        QTest::addColumn<QString>("model");
        QTest::addColumn<QList<int>>("rounds");
        QTest::newRow("cain-8x") << 1 << 8 << QStringLiteral("cain-ncnn-vulkan")
            << QStringLiteral("cain") << QList<int>{2, 4, 8};
        QTest::newRow("dain-8x") << 2 << 8 << QStringLiteral("dain-ncnn-vulkan")
            << QStringLiteral("best") << QList<int>{8};
        QTest::newRow("dain-3x") << 2 << 3 << QStringLiteral("dain-ncnn-vulkan")
            << QStringLiteral("best") << QList<int>{3};
    }

    void cainDainFrameRounds()
    {
        QFETCH(int, engineIndex);
        QFETCH(int, fpsMultiple);
        QFETCH(QString, engine);
        QFETCH(QString, model);
        QFETCH(QList<int>, rounds);
        QStringList options{QStringLiteral("-g"), QStringLiteral("0"), QStringLiteral("-j"),
                            QStringLiteral("3:3:3"), QStringLiteral("-m"), model};
        if (engineIndex == 2)
            options.append({QStringLiteral("-t"), QStringLiteral("128"), QStringLiteral("-n"),
                            QString::number(3 * fpsMultiple)});
        const FrameRequest::JobSettings settings{engineIndex,
            FrameRequest::modelForEngine(engineIndex, QStringLiteral("unrelated UI model")),
            options, fpsMultiple, 2, true};
        QCOMPARE(settings.engine(), engine);
        QCOMPARE(settings.model, model);
        QList<int> actualRounds;
        for (int multiplier = settings.initialMultiplier(); multiplier <= settings.fpsMultiple; multiplier *= 2)
            actualRounds.append(multiplier);
        QCOMPARE(actualRounds, rounds);
        QCOMPARE(settings.expectedFrames(3), engineIndex == 2 ? 3 * fpsMultiple : 6);
        options[1] = QStringLiteral("1");
        const QStringList arguments = settings.roundArguments(QStringLiteral("/source frames"),
            QStringLiteral("/output"), 8, true);
        QCOMPARE(arguments.mid(6), settings.options);
        QCOMPARE(arguments.at(5), QStringLiteral("%08d.png"));
        QVERIFY(!arguments.contains(QStringLiteral("-u")));
        QCOMPARE(arguments.at(arguments.indexOf(QStringLiteral("-g")) + 1), QStringLiteral("0"));
        if (engineIndex == 2)
        {
            QCOMPARE(arguments.at(arguments.indexOf(QStringLiteral("-t")) + 1), QStringLiteral("128"));
            QCOMPARE(arguments.at(arguments.indexOf(QStringLiteral("-n")) + 1), QString::number(3 * fpsMultiple));
        }
        const QJsonObject request = FrameRequest::make(QStringLiteral("frame-round"), settings.engine(),
            settings.model, QStringLiteral("/source frames"), QStringLiteral("/output"), arguments,
            600000, settings.expectedFrames(3), settings.retryCount);
        QCOMPARE(request.value("engine").toString(), engine);
        QCOMPARE(request.value("model").toString(), model);
        QCOMPARE(request.value("expected_frames").toInt(), settings.expectedFrames(3));
        QCOMPARE(request.value("args").toArray(), QJsonArray::fromStringList(arguments));
        QCOMPARE(FrameRequest::cancel(QStringLiteral("cancel-round"), request.value("id").toString())
                     .value("target_id").toString(), QStringLiteral("frame-round"));
        const BackendProcessResult result = FrameRequest::result(QJsonObject{
            {"ok", true}, {"event", "process_frames"}, {"data", QJsonObject{
                {"started", true}, {"finished", true}, {"exit_code", 0},
                {"timed_out", false}, {"output_valid", true}}}});
        QVERIFY(result.succeeded());
    }

    void frameGpuProbeInputsAreDistinctWritableAndTemporary()
    {
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        QString scanDirectory;
        {
            FrameGpuProbe::Inputs inputs(parent.filePath(QStringLiteral("scan-XXXXXX")));
            QVERIFY2(inputs.prepare(), qPrintable(inputs.diagnostic()));
            QVERIFY(inputs.diagnostic().isEmpty());
            QVERIFY(inputs.input0() != inputs.input1());
            const QImage first(inputs.input0());
            const QImage second(inputs.input1());
            QVERIFY(!first.isNull());
            QVERIFY(!second.isNull());
            QVERIFY(first != second);
            QImageReader reader(inputs.input0());
            QCOMPARE(reader.format(), QByteArray("png"));
            QImageReader secondReader(inputs.input1());
            QCOMPARE(secondReader.format(), QByteArray("png"));
            scanDirectory = QFileInfo(inputs.input0()).absolutePath();
            QCOMPARE(QFileInfo(inputs.input1()).absolutePath(), scanDirectory);
            QCOMPARE(QFileInfo(inputs.output()).absolutePath(), scanDirectory);
            QVERIFY(first.save(inputs.output(), "PNG"));
            QVERIFY(EngineTestRunner::isValidImage(inputs.output()));
            FrameGpuProbe::Inputs another(parent.filePath(QStringLiteral("scan-XXXXXX")));
            QVERIFY(another.prepare());
            QVERIFY(another.input0() != inputs.input0());
        }
        QVERIFY(!QDir(scanDirectory).exists());
        QVERIFY(QDir(parent.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
    }

    void frameGpuProbeInputPreparationFailureIsReported()
    {
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        FrameGpuProbe::Inputs inputs(parent.filePath(QStringLiteral("missing/scan-XXXXXX")));
        QVERIFY(!inputs.prepare());
        QVERIFY(!inputs.diagnostic().isEmpty());
    }

    void frameGpuProbeMapping_data()
    {
        QTest::addColumn<int>("index");
        QTest::addColumn<QString>("engine");
        QTest::addColumn<QString>("model");
        QTest::addColumn<int>("tile");
        QTest::newRow("rife") << 0 << QStringLiteral("rife-ncnn-vulkan") << QStringLiteral("rife-v2.4") << 0;
        QTest::newRow("cain") << 1 << QStringLiteral("cain-ncnn-vulkan") << QStringLiteral("cain") << 0;
        QTest::newRow("dain") << 2 << QStringLiteral("dain-ncnn-vulkan") << QStringLiteral("best") << 128;
        QTest::newRow("ifrnet") << 3 << QStringLiteral("ifrnet-ncnn-vulkan") << QStringLiteral("IFRNet_Vimeo90K") << 0;
    }

    void frameGpuProbeMapping()
    {
        QFETCH(int, index);
        QFETCH(QString, engine);
        QFETCH(QString, model);
        QFETCH(int, tile);
        const FrameGpuProbe::Mapping mapping = FrameGpuProbe::mapping(index);
        QCOMPARE(mapping.engine, engine);
        QCOMPARE(mapping.model, model);
        QCOMPARE(mapping.tileSize, tile);
        const EngineTestRequest request = FrameGpuProbe::request(mapping, QStringLiteral("/runtime/engine"),
            QStringLiteral("/runtime"), QStringLiteral("/input zero.png"), QStringLiteral("/input one.png"),
            QStringLiteral("/output.png"), 1);
        QStringList expected{QStringLiteral("-0"), QStringLiteral("/input zero.png"),
                             QStringLiteral("-1"), QStringLiteral("/input one.png"),
                             QStringLiteral("-o"), QStringLiteral("/output.png"),
                             QStringLiteral("-g"), QStringLiteral("1"),
                             QStringLiteral("-j"), QStringLiteral("1:1:1"),
                             QStringLiteral("-m"), model};
        if (tile > 0) expected.append({QStringLiteral("-t"), QString::number(tile)});
        QCOMPARE(request.arguments, expected);
        QCOMPARE(request.executable, QStringLiteral("/runtime/engine"));
        QCOMPARE(request.workingDirectory, QStringLiteral("/runtime"));
        QCOMPARE(request.outputPath, QStringLiteral("/output.png"));
        QCOMPARE(request.timeoutMs, 60000);
        QCOMPARE(request.device, EngineTestDevice::HardwareGpu);
    }

    void frameGpuProbeMappingIsStable()
    {
        QComboBox engine;
        engine.addItems({QStringLiteral("RIFE"), QStringLiteral("CAIN"),
                         QStringLiteral("DAIN"), QStringLiteral("IFRNet")});
        const FrameGpuProbe::Mapping captured = FrameGpuProbe::mapping(engine.currentIndex());
        engine.setCurrentIndex(2);
        const EngineTestRequest request = FrameGpuProbe::request(captured, QStringLiteral("/rife"),
            QStringLiteral("/runtime"), QStringLiteral("/zero.png"), QStringLiteral("/one.png"),
            QStringLiteral("/result.png"), 2);
        QCOMPARE(captured.engine, QStringLiteral("rife-ncnn-vulkan"));
        QCOMPARE(request.arguments.last(), QStringLiteral("rife-v2.4"));
        QVERIFY(!request.arguments.contains(QStringLiteral("-t")));
        QVERIFY(FrameGpuProbe::mapping(-1).engine.isEmpty());
        QVERIFY(FrameGpuProbe::mapping(4).engine.isEmpty());
    }

    void frameGpuProbeRejectsSoftwareAndInvalidOutput()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString output = directory.filePath(QStringLiteral("result.png"));
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(Qt::white);
        QVERIFY(image.save(output));
        EngineTestResult result;
        result.passed = result.started = result.finished = result.outputValid = result.deviceAccepted = true;
        result.exitCode = 0;
        result.deviceName = QStringLiteral("Example hardware device");
        QVERIFY(FrameGpuProbe::accepted(0, result, output));
        QVERIFY(!FrameGpuProbe::accepted(-1, result, output));
        result.deviceName = QStringLiteral("llvmpipe");
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
        result.deviceName = QStringLiteral("lavapipe");
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
        result.deviceName = QStringLiteral("Example hardware device");
        result.passed = false;
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
        result.passed = true;
        result.deviceAccepted = false;
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
        result.deviceAccepted = true;
        result.started = false;
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
        result.started = true;
        QFile file(output);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("not an image"), qint64(12));
        file.close();
        QVERIFY(!FrameGpuProbe::accepted(0, result, output));
    }

    void frameJobSettingsStayStableAfterWidgetChanges()
    {
        QComboBox engine;
        engine.addItems({QStringLiteral("RIFE"), QStringLiteral("CAIN"),
                         QStringLiteral("DAIN"), QStringLiteral("IFRNet")});
        QComboBox model;
        model.addItems({QStringLiteral("rife-v2.4"), QStringLiteral("IFRNet_Vimeo90K")});
        QSpinBox fps;
        fps.setValue(4);
        QSpinBox retries;
        retries.setValue(2);
        QCheckBox progress;
        progress.setChecked(true);
        QStringList options{QStringLiteral("-x"), QStringLiteral("-j"), QStringLiteral("2:2:2"),
                            QStringLiteral("-m"), model.currentText()};
        const FrameRequest::JobSettings settings{engine.currentIndex(), model.currentText(),
            options, fps.value(), retries.value(), progress.isChecked()};

        engine.setCurrentIndex(3);
        model.setCurrentIndex(1);
        fps.setValue(8);
        retries.setValue(6);
        progress.setChecked(false);
        options[2] = QStringLiteral("8:8:8");
        options[4] = model.currentText();

        QVERIFY(settings.usesRifeBackend());
        QCOMPARE(settings.engineIndex, 0);
        QCOMPARE(settings.model, QStringLiteral("rife-v2.4"));
        QCOMPARE(settings.fpsMultiple, 4);
        QCOMPARE(settings.retryCount, 2);
        QVERIFY(settings.showProgress);
        QCOMPARE(settings.options.at(2), QStringLiteral("2:2:2"));
        QCOMPARE(settings.options.at(4), QStringLiteral("rife-v2.4"));
        const QJsonObject laterPass = FrameRequest::make(QStringLiteral("round-4"),
            QStringLiteral("rife-ncnn-vulkan"), settings.model, QStringLiteral("/output_2"),
            QStringLiteral("/output"), settings.options, 600000, 24, settings.retryCount);
        QCOMPARE(laterPass.value("engine").toString(), QStringLiteral("rife-ncnn-vulkan"));
        QCOMPARE(laterPass.value("model").toString(), QStringLiteral("rife-v2.4"));
        QCOMPARE(laterPass.value("args").toArray(), QJsonArray::fromStringList(settings.options));
        const FrameRequest::JobSettings ifrnet{engine.currentIndex(), model.currentText(),
            options, fps.value(), retries.value(), progress.isChecked()};
        QVERIFY(!ifrnet.usesRifeBackend());
        QCOMPARE(ifrnet.engineIndex, 3);
    }

    void frameCancellationTranslation()
    {
        const QString targetId = FrameRequest::newId();
        const QString cancelId = FrameRequest::newId();
        QVERIFY(targetId != cancelId);
        const QJsonObject request = FrameRequest::cancel(cancelId, targetId);
        QCOMPARE(request.value("type").toString(), QStringLiteral("cancel_frames"));
        QCOMPARE(request.value("id").toString(), cancelId);
        QCOMPARE(request.value("target_id").toString(), targetId);
        QJsonObject response{{"ok", true}, {"event", "cancel_frames"},
                             {"data", QJsonObject{{"cancelled", true}}}};
        QVERIFY(FrameRequest::cancellationAccepted(response));
        response.insert("data", QJsonObject{{"cancelled", false}});
        QVERIFY(!FrameRequest::cancellationAccepted(response));
        response.insert("data", QJsonObject{{"cancelled", true}});
        response.insert("ok", false);
        QVERIFY(!FrameRequest::cancellationAccepted(response));
        response.insert("ok", true);
        response.insert("event", "process_frames");
        QVERIFY(!FrameRequest::cancellationAccepted(response));
    }

    void frameRequestTranslation()
    {
        const QStringList arguments{QStringLiteral("-i"), QStringLiteral("/input with spaces"),
                                    QStringLiteral("-o"), QStringLiteral("/output"),
                                    QStringLiteral("-f"), QStringLiteral("%08d.png"),
                                    QStringLiteral("-m"), QStringLiteral("IFRNet_Vimeo90K"),
                                    QStringLiteral("-g"), QStringLiteral("0,1"),
                                    QStringLiteral("-j"), QStringLiteral("4:2,2:4")};
        const QJsonObject request = FrameRequest::make(QStringLiteral("frames-test"),
            QStringLiteral("IFRNet_Vimeo90K"), QStringLiteral("/input with spaces"),
            QStringLiteral("/output"), arguments, 600000, 24, 2);
        QCOMPARE(request.value("type").toString(), QStringLiteral("process_frames"));
        QCOMPARE(request.value("id").toString(), QStringLiteral("frames-test"));
        QCOMPARE(request.value("engine").toString(), QStringLiteral("ifrnet-ncnn-vulkan"));
        QCOMPARE(request.value("model").toString(), QStringLiteral("IFRNet_Vimeo90K"));
        QCOMPARE(request.value("input_path").toString(), QStringLiteral("/input with spaces"));
        QCOMPARE(request.value("output_path").toString(), QStringLiteral("/output"));
        QCOMPARE(request.value("args").toArray(), QJsonArray::fromStringList(arguments));
        QCOMPARE(request.value("timeout_ms").toInt(), 600000);
        QCOMPARE(request.value("expected_frames").toInt(), 24);
        QCOMPARE(request.value("retry_count").toInt(), 2);
        const QJsonObject explicitRequest = FrameRequest::make(QStringLiteral("frames-test"),
            QStringLiteral("ifrnet-ncnn-vulkan"), QStringLiteral("IFRNet_Vimeo90K"),
            QStringLiteral("/input with spaces"), QStringLiteral("/output"), arguments, 600000, 24, 2);
        QCOMPARE(explicitRequest, request);
    }

    void rifeFrameRequestTranslation()
    {
        const QStringList arguments{QStringLiteral("-i"), QStringLiteral("/input frames"),
                                    QStringLiteral("-o"), QStringLiteral("/output_2"),
                                    QStringLiteral("-f"), QStringLiteral("%08d.png"),
                                    QStringLiteral("-x"), QStringLiteral("-u"),
                                    QStringLiteral("-g"), QStringLiteral("0"),
                                    QStringLiteral("-j"), QStringLiteral("2:2:2"),
                                    QStringLiteral("-m"), QStringLiteral("rife-v2.4")};
        const QJsonObject request = FrameRequest::make(QStringLiteral("rife-round-2"),
            QStringLiteral("rife-ncnn-vulkan"), QStringLiteral("rife-v2.4"),
            QStringLiteral("/input frames"), QStringLiteral("/output_2"), arguments, 600000, 12, 5);
        QCOMPARE(request.value("type").toString(), QStringLiteral("process_frames"));
        QCOMPARE(request.value("id").toString(), QStringLiteral("rife-round-2"));
        QCOMPARE(request.value("engine").toString(), QStringLiteral("rife-ncnn-vulkan"));
        QCOMPARE(request.value("model").toString(), QStringLiteral("rife-v2.4"));
        QCOMPARE(request.value("input_path").toString(), QStringLiteral("/input frames"));
        QCOMPARE(request.value("output_path").toString(), QStringLiteral("/output_2"));
        QCOMPARE(request.value("args").toArray(), QJsonArray::fromStringList(arguments));
        QCOMPARE(request.value("timeout_ms").toInt(), 600000);
        QCOMPARE(request.value("expected_frames").toInt(), 12);
        QCOMPARE(request.value("retry_count").toInt(), 5);
        const QJsonObject cancellation = FrameRequest::cancel(QStringLiteral("cancel-rife"),
            request.value("id").toString());
        QCOMPARE(cancellation.value("target_id").toString(), QStringLiteral("rife-round-2"));
    }

    void frameResultTranslation()
    {
        QJsonObject data{{"started", true}, {"finished", true}, {"exit_code", 0},
                         {"timed_out", false}, {"output_valid", true},
                         {"stdout", "frames complete"}, {"stderr", "device report"},
                         {"diagnostic", "complete"}};
        QJsonObject response{{"ok", true}, {"event", "process_frames"}, {"data", data}};
        BackendProcessResult result = FrameRequest::result(response);
        QVERIFY(result.succeeded());
        QCOMPARE(result.standardOutput, QByteArray("frames complete"));
        QCOMPARE(result.standardError, QByteArray("device report"));
        QCOMPARE(result.diagnostic, QStringLiteral("complete"));
        data.insert("timed_out", true);
        response.insert("data", data);
        QVERIFY(!FrameRequest::result(response).succeeded());
        data.insert("diagnostic", "runtime missing");
        response.insert("data", data);
        response.insert("ok", false);
        QCOMPARE(FrameRequest::result(response).diagnostic, QStringLiteral("runtime missing"));
        response = QJsonObject{{"ok", false}, {"event", "error"},
                               {"data", QJsonObject{{"message", "socket unavailable"}}}};
        result = FrameRequest::result(response);
        QVERIFY(!result.succeeded());
        QCOMPARE(result.diagnostic, QStringLiteral("socket unavailable"));
    }

    void frameOutputRequiresDecoding()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("00000001.png"));
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(Qt::white);
        QVERIFY(image.save(path));
        QVERIFY(FrameRequest::validOutput(directory.path(), 1));
        QVERIFY(!FrameRequest::validOutput(directory.path(), 2));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.resize(24)); // A PNG header is not a decodable frame.
        file.close();
        QVERIFY(!FrameRequest::validOutput(directory.path(), 1));
        QVERIFY(!FrameRequest::validOutput(directory.path(), 0));
    }

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
