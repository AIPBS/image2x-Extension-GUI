/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#ifndef MODEL_TEST_MATRIX_H
#define MODEL_TEST_MATRIX_H

#include <QList>
#include <QString>
#include <QStringList>

struct ModelTestCase
{
    QString name;
    QString engine;
    QString model;
    int scale = 2;
    int denoise = 0;
    bool proprietary = false;
};

QList<ModelTestCase> publicModelTestMatrix();
QList<ModelTestCase> proprietaryModelTestMatrix();

#endif // MODEL_TEST_MATRIX_H
