/*
    Copyright (C) 2026  AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ADDITIONAL PERMISSION under GNU AGPL version 3 section 7:
    As a special exception, the copyright holder of this file gives you
    permission to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell this file without restriction, as though it were licensed
    under a permissive license. This additional permission applies only to
    this specific file and files explicitly marked with this notice.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Based on the original Waifu2x-Extension-GUI by Aaron Feng:
    https://github.com/AaronFeng753/Waifu2x-Extension-GUI

    DAIN-NCNN-Vulkan Frame Interpolation Engine
    Reconstructed for v3.139.01
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

// ===================== DAIN Frame Interpolation =====================

int MainWindow::Dain_FrameInterpolation(int rowNum) {
    /*
    DAIN-NCNN-Vulkan frame interpolation.
    Models: best/flownet.bin (22MB) + best/interpolation.bin (18MB)
    Engine path: "<appdir>/dain-ncnn-vulkan/dain-ncnn-vulkan_waifu2xEX.exe" (Win)
                 "<appdir>/dain-ncnn-vulkan/dain-ncnn-vulkan" (Linux)
    Command: <exe> -i <input_frames> -o <output_frames> -g <gpu> -j <threads>
    */
    return 0;
}

QString MainWindow::Dain_ReadConfig() {
    QString cmd;
    return cmd;
}

int MainWindow::Dain_DetectGPU() {
    return 0;
}
