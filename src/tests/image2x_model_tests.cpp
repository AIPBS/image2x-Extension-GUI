/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#include "engine_test_runner.h"
#include "model_test_matrix.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
struct TestRecord
{
    QString name;
    QString device;
    EngineTestResult result;
};

QString engineDirectory(const QString &applicationDirectory, const QString &engine)
{
    return QDir(applicationDirectory).filePath(QStringLiteral("dependencies/engines/%1").arg(engine));
}

QString executablePath(const QString &directory, const QString &engine)
{
    return QDir(directory).filePath(engine);
}

QStringList argumentsFor(const ModelTestCase &testCase,
                         const QString &inputPath,
                         const QString &outputPath)
{
    if (testCase.engine == QStringLiteral("realesrgan-ncnn-vulkan"))
    {
        return QStringList() << "-i" << inputPath << "-o" << outputPath
                             << "-s" << QString::number(testCase.scale)
                             << "-n" << testCase.model << "-t" << "32"
                             << "-m" << "models" << "-g" << "0";
    }
    if (testCase.engine == QStringLiteral("realsr-ncnn-vulkan"))
    {
        return QStringList() << "-i" << inputPath << "-o" << outputPath
                             << "-s" << QString::number(testCase.scale)
                             << "-t" << "32" << "-m" << testCase.model << "-g" << "0";
    }
    return QStringList() << "-i" << inputPath << "-o" << outputPath
                         << "-s" << QString::number(testCase.scale)
                         << "-n" << QString::number(testCase.denoise)
                         << "-t" << "32" << "-m" << testCase.model << "-g" << "0";
}

TestRecord runCase(const ModelTestCase &testCase,
                   const QString &applicationDirectory,
                   const QString &modelRoot,
                   const QString &inputPath,
                   const QString &outputDirectory,
                   EngineTestDevice device)
{
    const QString packagedDirectory = engineDirectory(applicationDirectory, testCase.engine);
    QString workingDirectory = packagedDirectory;
    QString modelArgumentRoot;
    if (testCase.proprietary)
    {
        const QString externalDirectory = engineDirectory(modelRoot, testCase.engine);
        if (QDir(externalDirectory).exists())
        {
            workingDirectory = externalDirectory;
        }
        modelArgumentRoot = QStringLiteral("models");
    }

    ModelTestCase effectiveCase = testCase;
    if (testCase.proprietary)
    {
        effectiveCase.model = testCase.model;
    }
    QStringList arguments = argumentsFor(effectiveCase, inputPath,
                                         QDir(outputDirectory).filePath(testCase.name + ".png"));
    if (testCase.proprietary)
    {
        const int modelIndex = arguments.indexOf(QStringLiteral("-m"));
        if (modelIndex >= 0 && modelIndex + 1 < arguments.size())
        {
            arguments[modelIndex + 1] = modelArgumentRoot;
        }
    }

    EngineTestRequest request;
    request.name = testCase.name;
    request.executable = executablePath(packagedDirectory, testCase.engine);
    request.workingDirectory = workingDirectory;
    request.arguments = arguments;
    request.outputPath = QDir(outputDirectory).filePath(testCase.name + ".png");
    request.timeoutMs = 60000;
    request.device = device;
    return TestRecord{testCase.name,
                      device == EngineTestDevice::HardwareGpu ? QStringLiteral("gpu") : QStringLiteral("cpu"),
                      EngineTestRunner::run(request)};
}

QJsonObject recordToJson(const TestRecord &record)
{
    QJsonObject object;
    object.insert(QStringLiteral("name"), record.name);
    object.insert(QStringLiteral("device"), record.device);
    object.insert(QStringLiteral("passed"), record.result.passed);
    object.insert(QStringLiteral("deviceName"), record.result.deviceName);
    object.insert(QStringLiteral("diagnostic"), record.result.diagnostic);
    return object;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("image2x-model-tests"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Run the packaged image2x model matrix."));
    parser.addHelpOption();
    parser.addOption({{"a", "application-directory"}, "Application bundle directory.", "directory"});
    parser.addOption({{"i", "input"}, "Still-image input path.", "file"});
    parser.addOption({{"m", "models"}, "Authorized model root containing proprietary models.", "directory"});
    parser.addOption({{"j", "json"}, "Write JSON results to this path.", "file"});
    parser.addOption({"cpu", "Run software-Vulkan/CPU checks."});
    parser.addOption({"gpu", "Run hardware-GPU checks."});
    parser.addOption({"include-proprietary", "Include authorized proprietary model cases."});
    parser.process(application);

    const QString applicationDirectory = parser.value("application-directory");
    const QString inputOption = parser.value("input");
    if (applicationDirectory.isEmpty() || inputOption.isEmpty()
        || (!parser.isSet("cpu") && !parser.isSet("gpu")))
    {
        QTextStream(stderr) << parser.helpText();
        return 2;
    }
    const QString modelRoot = parser.value("models");
    if (parser.isSet("include-proprietary") && modelRoot.isEmpty())
    {
        QTextStream(stderr) << "--include-proprietary requires --models.\n";
        return 2;
    }

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid())
    {
        QTextStream(stderr) << "Unable to create a temporary model-test directory.\n";
        return 1;
    }
    QList<ModelTestCase> cases = publicModelTestMatrix();
    if (parser.isSet("include-proprietary"))
    {
        cases.append(proprietaryModelTestMatrix());
    }
    QList<TestRecord> records;
    bool allPassed = true;
    for (const ModelTestCase &testCase : cases)
    {
        if (parser.isSet("gpu"))
        {
            const TestRecord record = runCase(testCase, applicationDirectory, modelRoot,
                                              inputOption,
                                              temporaryDirectory.path(), EngineTestDevice::HardwareGpu);
            records.append(record);
            allPassed = allPassed && record.result.passed;
            QTextStream(stdout) << (record.result.passed ? "PASS" : "FAIL")
                                << " gpu " << record.name << " " << record.result.diagnostic << "\n";
        }
        if (parser.isSet("cpu"))
        {
            const TestRecord record = runCase(testCase, applicationDirectory, modelRoot,
                                              inputOption,
                                              temporaryDirectory.path(), EngineTestDevice::SoftwareCpu);
            records.append(record);
            allPassed = allPassed && record.result.passed;
            QTextStream(stdout) << (record.result.passed ? "PASS" : "FAIL")
                                << " cpu " << record.name << " " << record.result.diagnostic << "\n";
        }
    }

    if (parser.isSet("json"))
    {
        QJsonArray jsonRecords;
        for (const TestRecord &record : records)
        {
            jsonRecords.append(recordToJson(record));
        }
        QFile jsonFile(parser.value("json"));
        if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QTextStream(stderr) << "Unable to write JSON results: " << jsonFile.errorString() << "\n";
            return 1;
        }
        jsonFile.write(QJsonDocument(jsonRecords).toJson(QJsonDocument::Indented));
    }
    return allPassed ? 0 : 1;
}
