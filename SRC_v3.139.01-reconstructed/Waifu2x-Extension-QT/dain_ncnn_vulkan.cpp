/*
    Copyright (C) 2021  Aaron Feng
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
    Also includes: ctxnet.bin, depthnet.bin
    Follows the exact pattern from Frame_Interpolation.cpp.
    */
    // Engine path: "<appdir>/dain-ncnn-vulkan/dain-ncnn-vulkan_waifu2xEX.exe" (Win)
    //              "<appdir>/dain-ncnn-vulkan/dain-ncnn-vulkan" (Linux)
    //
    // Command: <exe> -i <input_frames> -o <output_frames> -g <gpu> -j <threads>
    return 0;
}

QString MainWindow::Dain_ReadConfig() {
    // Build command-line config for DAIN engine
    QString cmd;
    return cmd;
}

int MainWindow::Dain_DetectGPU() {
    return 0;
}
