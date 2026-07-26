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

    ===================================================================
    Engine model registry header.
    Maps all 179+ model weights to engine configurations.

    IMPORTANT — LICENSE BOUNDARIES (see MODELS.md for full details):
      - Models marked PROPRIETARY below are custom-trained by Aaron Feng.
        They are NOT open-source. Do NOT distribute them.
      - Models marked OPEN-SOURCE are from upstream projects (MIT/BSD).
        They can be freely redistributed.
    ===================================================================
*/

#ifndef ENGINE_REGISTRY_H
#define ENGINE_REGISTRY_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QList>

// ============================================================================
// Real-ESRGAN model registry
// ============================================================================

struct RealESRGANModel
{
    QString name;        // Display name (also used as model subdirectory on disk)
    QString paramFile;   // .param filename
    QString binFile;     // .bin filename
    QString style;       // "2D" (anime) or "3D" (photo/real-world)
    int     scaleFactor; // 2, 3, or 4
    bool    isCustom;    // W2xEX custom model?
};

inline QList<RealESRGANModel> getRealESRGANModels()
{
    return {
        // ═══════════════════════════════════════════════════════════
        // ⚠️  PROPRIETARY — Aaron Feng custom-trained models.
        //     Do NOT distribute. See MODELS.md for details.
        //     These are referenced but NOT included in this source tree.
        // ═══════════════════════════════════════════════════════════
        {QStringLiteral("Anime-HQ-W4xEX"),
         QStringLiteral("Anime-HQ-W4xEX.param"),
         QStringLiteral("Anime-HQ-W4xEX.bin"),
         QStringLiteral("2D"), 4, true},

        {QStringLiteral("AnimeVideo-MiniV1.8-W2xEX"),
         QStringLiteral("AnimeVideo-MiniV1.8-W2xEX.param"),
         QStringLiteral("AnimeVideo-MiniV1.8-W2xEX.bin"),
         QStringLiteral("2D"), 2, true},

        {QStringLiteral("Omni-TurboV1.5-W2xEX"),
         QStringLiteral("Omni-TurboV1.5-W2xEX.param"),
         QStringLiteral("Omni-TurboV1.5-W2xEX.bin"),
         QStringLiteral("2D"), 2, true},

        {QStringLiteral("Omni-MiniV2-W2xEX"),
         QStringLiteral("Omni-MiniV2-W2xEX.param"),
         QStringLiteral("Omni-MiniV2-W2xEX.bin"),
         QStringLiteral("2D"), 2, true},

        {QStringLiteral("Omni-Smallv2-W2xEX"),
         QStringLiteral("Omni-Smallv2-W2xEX.param"),
         QStringLiteral("Omni-Smallv2-W2xEX.bin"),
         QStringLiteral("2D"), 2, true},

        {QStringLiteral("Photo-HQ-W4xEX"),
         QStringLiteral("Photo-HQ-W4xEX.param"),
         QStringLiteral("Photo-HQ-W4xEX.bin"),
         QStringLiteral("3D"), 4, true},

        {QStringLiteral("Photo-Small-W2xEX"),
         QStringLiteral("Photo-Small-W2xEX.param"),
         QStringLiteral("Photo-Small-W2xEX.bin"),
         QStringLiteral("3D"), 2, true},

        {QStringLiteral("Universal-FastV2-W2xEX"),
         QStringLiteral("Universal-FastV2-W2xEX.param"),
         QStringLiteral("Universal-FastV2-W2xEX.bin"),
         QStringLiteral("3D"), 2, true},

        {QStringLiteral("Photo-Conservative-x4"),
         QStringLiteral("Photo-Conservative-x4.param"),
         QStringLiteral("Photo-Conservative-x4.bin"),
         QStringLiteral("3D"), 4, true},

        // ---- Standard Real-ESRGAN models ----
        {QStringLiteral("realesr-animevideov3-x2"),
         QStringLiteral("realesr-animevideov3-x2.param"),
         QStringLiteral("realesr-animevideov3-x2.bin"),
         QStringLiteral("2D"), 2, false},

        {QStringLiteral("realesr-animevideov3-x3"),
         QStringLiteral("realesr-animevideov3-x3.param"),
         QStringLiteral("realesr-animevideov3-x3.bin"),
         QStringLiteral("2D"), 3, false},

        {QStringLiteral("realesr-animevideov3-x4"),
         QStringLiteral("realesr-animevideov3-x4.param"),
         QStringLiteral("realesr-animevideov3-x4.bin"),
         QStringLiteral("2D"), 4, false},

        {QStringLiteral("realesrgan-x4plus-anime"),
         QStringLiteral("realesrgan-x4plus-anime.param"),
         QStringLiteral("realesrgan-x4plus-anime.bin"),
         QStringLiteral("2D"), 4, false},

        {QStringLiteral("RealESRGANv2-animevideo-xsx2"),
         QStringLiteral("RealESRGANv2-animevideo-xsx2.param"),
         QStringLiteral("RealESRGANv2-animevideo-xsx2.bin"),
         QStringLiteral("2D"), 2, false},

        {QStringLiteral("RealESRGANv2-animevideo-xsx4"),
         QStringLiteral("RealESRGANv2-animevideo-xsx4.param"),
         QStringLiteral("RealESRGANv2-animevideo-xsx4.bin"),
         QStringLiteral("2D"), 4, false},

        {QStringLiteral("realesrgan-x4plus"),
         QStringLiteral("realesrgan-x4plus.param"),
         QStringLiteral("realesrgan-x4plus.bin"),
         QStringLiteral("3D"), 4, false},

        {QStringLiteral("realesr-general-x4v3"),
         QStringLiteral("realesr-general-x4v3.param"),
         QStringLiteral("realesr-general-x4v3.bin"),
         QStringLiteral("3D"), 4, false},

        {QStringLiteral("realesr-general-wdn-x4v3"),
         QStringLiteral("realesr-general-wdn-x4v3.param"),
         QStringLiteral("realesr-general-wdn-x4v3.bin"),
         QStringLiteral("3D"), 4, false},
    };
}

// ============================================================================
// RIFE model registry
// ============================================================================

struct RIFEModel
{
    QString dirName;   // Sub-directory name under rife-ncnn-vulkan/
    QString version;   // Human-readable version string
    bool    isLite;    // Lite / smaller variant
    bool    isV4Style; // v4.x uses flownet-only architecture (no contextnet/fusionnet)
};

inline QList<RIFEModel> getRIFEModels()
{
    return {
        // ---- v4.x models (flownet-only architecture) ----
        {QStringLiteral("rife-v4.26-large"), QStringLiteral("4.26-large"), false, true},
        {QStringLiteral("rife-v4.26"),       QStringLiteral("4.26"),       false, true},
        {QStringLiteral("rife-v4.22-lite"),  QStringLiteral("4.22-lite"),  true,  true},
        {QStringLiteral("rife-v4.17"),       QStringLiteral("4.17"),       false, true},
        {QStringLiteral("rife-v4.17-lite"),  QStringLiteral("4.17-lite"),  true,  true},
        {QStringLiteral("rife-v4.15-lite"),  QStringLiteral("4.15-lite"),  true,  true},
        {QStringLiteral("rife-v4.14"),       QStringLiteral("4.14"),       false, true},
        {QStringLiteral("rife-v4.13-lite"),  QStringLiteral("4.13-lite"),  true,  true},
        {QStringLiteral("rife-v4.6"),        QStringLiteral("4.6"),        false, true},
        {QStringLiteral("rife-v4"),          QStringLiteral("4.0"),        false, true},

        // ---- Traditional models (contextnet + flownet + fusionnet) ----
        {QStringLiteral("rife-v3.1"), QStringLiteral("3.1"), false, false},
        {QStringLiteral("rife-v3.0"), QStringLiteral("3.0"), false, false},
        {QStringLiteral("rife-v2.4"), QStringLiteral("2.4"), false, false},
        {QStringLiteral("rife-v2.3"), QStringLiteral("2.3"), false, false},
        {QStringLiteral("rife-v2"),   QStringLiteral("2.0"), false, false},
        {QStringLiteral("rife-HD"),   QStringLiteral("HD"),  false, false},
        {QStringLiteral("rife-UHD"),  QStringLiteral("UHD"), false, false},
        {QStringLiteral("rife-anime"),QStringLiteral("anime"),false, false},
        {QStringLiteral("rife"),      QStringLiteral("1.0"), false, false},
    };
}

/*
 * Return only the v4.x models (isV4Style == true).
 * These models use flownet-only architecture and may have different
 * command-line flags (e.g., no -f format flag needed).
 */
inline QList<RIFEModel> getRIFEv4Models()
{
    QList<RIFEModel> all = getRIFEModels();
    QList<RIFEModel> v4;
    for (const RIFEModel &m : all)
    {
        if (m.isV4Style)
            v4.append(m);
    }
    return v4;
}

/*
 * Return only the traditional (pre-v4) RIFE models.
 */
inline QList<RIFEModel> getRIFETraditionalModels()
{
    QList<RIFEModel> all = getRIFEModels();
    QList<RIFEModel> trad;
    for (const RIFEModel &m : all)
    {
        if (!m.isV4Style)
            trad.append(m);
    }
    return trad;
}

// ============================================================================
// Real-CUGAN model variants
// ============================================================================

inline QStringList getRealCUGANVariants()
{
    return {
        QStringLiteral("models-se"),
        QStringLiteral("models-pro"),
        QStringLiteral("models-nose"),
    };
}

// ============================================================================
// IFRNet model variants
// ============================================================================

inline QStringList getIFRNetModels()
{
    return {
        QStringLiteral("IFRNet_GoPro"),
        QStringLiteral("IFRNet_Vimeo90K"),
        QStringLiteral("IFRNet_S_GoPro"),
        QStringLiteral("IFRNet_S_Vimeo90K"),
        QStringLiteral("IFRNet_L_GoPro"),
        QStringLiteral("IFRNet_L_Vimeo90K"),
    };
}

// ============================================================================
// waifu2x-ncnn-vulkan model registry (for reference)
// ============================================================================

struct Waifu2xNCNNVulkanModel
{
    QString name;
    int     scaleFactor;
    int     noiseLevel;
};

inline QList<Waifu2xNCNNVulkanModel> getWaifu2xNCNNVulkanModels()
{
    return {
        // upconv_7 models
        {QStringLiteral("models-upconv_7_anime_style_art_rgb"), 2, -1},
        {QStringLiteral("models-upconv_7_photo"), 2, -1},
        // cunet models
        {QStringLiteral("models-cunet"), 2, -1},
    };
}

// ============================================================================
// SRMD-ncnn-vulkan model registry
// ============================================================================

inline QStringList getSRMDNCNNVulkanModels()
{
    return {
        QStringLiteral("models-srmd"),
    };
}

// ============================================================================
// Engine path helpers
// ============================================================================

// Engine folder names (relative to application directory)
inline QString engineFolderRIFE()    { return QStringLiteral("rife-ncnn-vulkan"); }
inline QString engineFolderCAIN()    { return QStringLiteral("cain-ncnn-vulkan"); }
inline QString engineFolderDAIN()    { return QStringLiteral("dain-ncnn-vulkan"); }
inline QString engineFolderIFRNet()  { return QStringLiteral("ifrnet-ncnn-vulkan"); }
inline QString engineFolderRealESRGAN() { return QStringLiteral("realesrgan-ncnn-vulkan"); }
inline QString engineFolderRealCUGAN()  { return QStringLiteral("realcugan-ncnn-vulkan"); }
inline QString engineFolderWaifu2x()    { return QStringLiteral("waifu2x-ncnn-vulkan"); }
inline QString engineFolderSRMD()       { return QStringLiteral("srmd-ncnn-vulkan"); }
inline QString engineFolderRealSR()     { return QStringLiteral("realsr-ncnn-vulkan"); }
inline QString engineFolderAnime4K()    { return QStringLiteral("Anime4K"); }

#endif // ENGINE_REGISTRY_H
