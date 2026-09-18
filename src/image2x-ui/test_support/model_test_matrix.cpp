/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

*/

#include "model_test_matrix.h"

QList<ModelTestCase> publicModelTestMatrix()
{
    QList<ModelTestCase> cases;
    cases << ModelTestCase{QStringLiteral("waifu2x-cunet"), QStringLiteral("waifu2x-ncnn-vulkan"), QStringLiteral("models-cunet")}
          << ModelTestCase{QStringLiteral("waifu2x-upconv-anime"), QStringLiteral("waifu2x-ncnn-vulkan"), QStringLiteral("models-upconv_7_anime_style_art_rgb")}
          << ModelTestCase{QStringLiteral("waifu2x-upconv-photo"), QStringLiteral("waifu2x-ncnn-vulkan"), QStringLiteral("models-upconv_7_photo")}
          << ModelTestCase{QStringLiteral("srmd"), QStringLiteral("srmd-ncnn-vulkan"), QStringLiteral("models-srmd")}
          << ModelTestCase{QStringLiteral("realsr-df2k"), QStringLiteral("realsr-ncnn-vulkan"), QStringLiteral("models-DF2K"), 4}
          << ModelTestCase{QStringLiteral("realsr-df2k-jpeg"), QStringLiteral("realsr-ncnn-vulkan"), QStringLiteral("models-DF2K_JPEG"), 4};

    const QStringList realEsrganModels = {
        QStringLiteral("realesr-animevideov3-x2"),
        QStringLiteral("realesr-animevideov3-x3"),
        QStringLiteral("realesr-animevideov3-x4"),
        QStringLiteral("realesrgan-x4plus-anime"),
        QStringLiteral("RealESRGANv2-animevideo-xsx2"),
        QStringLiteral("RealESRGANv2-animevideo-xsx4"),
        QStringLiteral("realesrgan-x4plus"),
        QStringLiteral("realesr-general-x4v3"),
        QStringLiteral("realesr-general-wdn-x4v3"),
    };
    for (const QString &model : realEsrganModels)
    {
        const int scale = model.contains(QStringLiteral("x2")) ? 2
            : model.contains(QStringLiteral("x3")) ? 3 : 4;
        cases.append(ModelTestCase{QStringLiteral("realesrgan-") + model,
                                   QStringLiteral("realesrgan-ncnn-vulkan"),
                                   model, scale});
    }

    const QList<ModelTestCase> realCuganCases = {
        {QStringLiteral("realcugan-se-up2x-conservative"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 2, -1},
        {QStringLiteral("realcugan-se-up2x-denoise1x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 2, 1},
        {QStringLiteral("realcugan-se-up2x-denoise2x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 2, 2},
        {QStringLiteral("realcugan-se-up2x-denoise3x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 2, 3},
        {QStringLiteral("realcugan-se-up2x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 2, 0},
        {QStringLiteral("realcugan-se-up3x-conservative"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 3, -1},
        {QStringLiteral("realcugan-se-up3x-denoise3x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 3, 3},
        {QStringLiteral("realcugan-se-up3x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 3, 0},
        {QStringLiteral("realcugan-se-up4x-conservative"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 4, -1},
        {QStringLiteral("realcugan-se-up4x-denoise3x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 4, 3},
        {QStringLiteral("realcugan-se-up4x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-se"), 4, 0},
        {QStringLiteral("realcugan-pro-up2x-conservative"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 2, -1},
        {QStringLiteral("realcugan-pro-up2x-denoise3x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 2, 3},
        {QStringLiteral("realcugan-pro-up2x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 2, 0},
        {QStringLiteral("realcugan-pro-up3x-conservative"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 3, -1},
        {QStringLiteral("realcugan-pro-up3x-denoise3x"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 3, 3},
        {QStringLiteral("realcugan-pro-up3x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-pro"), 3, 0},
        {QStringLiteral("realcugan-nose-up2x-no-denoise"), QStringLiteral("realcugan-ncnn-vulkan"), QStringLiteral("models-nose"), 2, 0},
    };
    cases.append(realCuganCases);

    return cases;
}

QList<ModelTestCase> proprietaryModelTestMatrix()
{
    const QList<ModelTestCase> cases = {
        {QStringLiteral("Anime-HQ-W4xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Anime-HQ-W4xEX"), 4, 0, true},
        {QStringLiteral("AnimeVideo-MiniV1.8-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("AnimeVideo-MiniV1.8-W2xEX"), 2, 0, true},
        {QStringLiteral("Omni-MiniV2-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Omni-MiniV2-W2xEX"), 2, 0, true},
        {QStringLiteral("Omni-Smallv2-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Omni-Smallv2-W2xEX"), 2, 0, true},
        {QStringLiteral("Omni-TurboV1.5-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Omni-TurboV1.5-W2xEX"), 2, 0, true},
        {QStringLiteral("Photo-HQ-W4xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Photo-HQ-W4xEX"), 4, 0, true},
        {QStringLiteral("Photo-Small-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Photo-Small-W2xEX"), 2, 0, true},
        {QStringLiteral("Universal-FastV2-W2xEX"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Universal-FastV2-W2xEX"), 2, 0, true},
        {QStringLiteral("Photo-Conservative-x4"), QStringLiteral("realesrgan-ncnn-vulkan"), QStringLiteral("Photo-Conservative-x4"), 4, 0, true},
    };
    return cases;
}
