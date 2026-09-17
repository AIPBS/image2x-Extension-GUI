#!/usr/bin/env bash
# Install every public Linux runtime in the v1 still-image matrix.
set -euo pipefail

[[ "${1:-}" != "" && "${1:-}" != "-h" && "${1:-}" != "--help" ]] || {
    printf 'Usage: %s <application-directory>\n' "$0"
    exit 2
}

APPLICATION_DIRECTORY="$(realpath "$1")"
ENGINES_DIRECTORY="${APPLICATION_DIRECTORY}/dependencies/engines"
WORK_DIRECTORY="$(mktemp -d)"
trap 'rm -rf "$WORK_DIRECTORY"' EXIT
MODEL_ARCHIVE="w2x-models-v3.139.01-open-source.tar.gz"
MODEL_URL="https://github.com/AIPBS/image2x-Extension-GUI/releases/download/open-model-set-3.139.01/${MODEL_ARCHIVE}"
MODEL_SHA256="0fcf7a934977959523a0ea02ec88d707cabd7e96d52407b4a472775ba0e925e3"
MODEL_DIRECTORY="${WORK_DIRECTORY}/open-source-models"

for command in curl sha256sum unzip tar; do
    command -v "$command" >/dev/null 2>&1 || {
        printf 'Required command is unavailable: %s\n' "$command" >&2
        exit 1
    }
done

[[ -d "$APPLICATION_DIRECTORY" ]] || {
    printf 'Application directory does not exist: %s\n' "$APPLICATION_DIRECTORY" >&2
    exit 1
}

mkdir -p "$MODEL_DIRECTORY"
if [[ -n "${MODEL_SOURCE_DIRECTORY:-}" ]]; then
    cp -a "${MODEL_SOURCE_DIRECTORY}/." "$MODEL_DIRECTORY"
else
    MODEL_ARCHIVE_PATH="${WORK_DIRECTORY}/${MODEL_ARCHIVE}"
    printf 'Downloading public model bundle...\n'
    curl --fail --location --retry 3 --progress-bar --output "$MODEL_ARCHIVE_PATH" "$MODEL_URL"
    printf '%s  %s\n' "$MODEL_SHA256" "$MODEL_ARCHIVE_PATH" | sha256sum --check --status
    tar -xzf "$MODEL_ARCHIVE_PATH" -C "$MODEL_DIRECTORY" --strip-components=1
fi

merge_models() {
    local engine="$1"
    local source_directory="${MODEL_DIRECTORY}/${engine}"
    local target_directory="${ENGINES_DIRECTORY}/${engine}"
    if [[ -d "$source_directory" ]]; then
        cp -a "$source_directory/." "$target_directory/"
    fi
}

install_archive() {
    local engine="$1"
    local archive="$2"
    local url="$3"
    local sha256="$4"
    local executable="$5"
    shift 5
    local target_directory="${ENGINES_DIRECTORY}/${engine}"
    local archive_path="${WORK_DIRECTORY}/${archive}"
    local extract_directory="${WORK_DIRECTORY}/${engine}"

    if [[ -e "$target_directory" ]]; then
        printf 'Runtime already exists at %s; remove it explicitly before reinstalling.\n' "$target_directory" >&2
        exit 1
    fi

    printf 'Downloading %s...\n' "$archive"
    if [[ -n "${RUNTIME_CACHE_DIRECTORY:-}" && -f "${RUNTIME_CACHE_DIRECTORY}/${archive}" ]]; then
        cp "${RUNTIME_CACHE_DIRECTORY}/${archive}" "$archive_path"
    else
        curl --fail --location --retry 3 --progress-bar --output "$archive_path" "$url"
    fi
    printf '%s  %s\n' "$sha256" "$archive_path" | sha256sum --check --status

    mkdir -p "$extract_directory"
    unzip -q "$archive_path" -d "$extract_directory"
    shopt -s nullglob
    local entries=("${extract_directory}"/*)
    [[ "${#entries[@]}" -eq 1 && -d "${entries[0]}" ]] || {
        printf 'Unexpected archive layout for %s.\n' "$archive" >&2
        exit 1
    }
    [[ -x "${entries[0]}/${executable}" ]] || {
        printf 'Archive %s is missing executable %s.\n' "$archive" "$executable" >&2
        exit 1
    }
    mkdir -p "$ENGINES_DIRECTORY"
    mv "${entries[0]}" "$target_directory"
    merge_models "$engine"
    local model
    for model in "$@"; do
        [[ -d "${target_directory}/${model}" ]] || {
            printf 'Installed runtime %s is missing model directory %s.\n' "$engine" "$model" >&2
            exit 1
        }
    done
    chmod 700 "$target_directory/$executable"
    printf 'Installed %s at %s\n' "$engine" "$target_directory"
}

install_archive \
    waifu2x-ncnn-vulkan \
    waifu2x-ncnn-vulkan-20250915-linux.zip \
    https://github.com/nihui/waifu2x-ncnn-vulkan/releases/download/20250915/waifu2x-ncnn-vulkan-20250915-linux.zip \
    848e0fba55657d34da90b775b8139e9806dc754798b029f95e106ba8850a731f \
    waifu2x-ncnn-vulkan \
    models-cunet models-upconv_7_anime_style_art_rgb models-upconv_7_photo

install_archive \
    srmd-ncnn-vulkan \
    srmd-ncnn-vulkan-20220728-ubuntu.zip \
    https://github.com/nihui/srmd-ncnn-vulkan/releases/download/20220728/srmd-ncnn-vulkan-20220728-ubuntu.zip \
    0020a65e2b4e183ce469c11734656035203cc6597bf42a1277abf0fbef610a6c \
    srmd-ncnn-vulkan models-srmd

install_archive \
    realsr-ncnn-vulkan \
    realsr-ncnn-vulkan-20220728-ubuntu.zip \
    https://github.com/nihui/realsr-ncnn-vulkan/releases/download/20220728/realsr-ncnn-vulkan-20220728-ubuntu.zip \
    6007e55c156a6b4600e569c68818836c22daeac63fdf71db068966b5425ac085 \
    realsr-ncnn-vulkan models-DF2K models-DF2K_JPEG

install_archive \
    realesrgan-ncnn-vulkan \
    realesrgan-ncnn-vulkan-v0.2.0-ubuntu.zip \
    https://github.com/xinntao/Real-ESRGAN-ncnn-vulkan/releases/download/v0.2.0/realesrgan-ncnn-vulkan-v0.2.0-ubuntu.zip \
    d0e8e1cf954f5cde11be4745dd912cc3774bef36f71c5b1cb8f74c4112b6e919 \
    realesrgan-ncnn-vulkan models

install_archive \
    realcugan-ncnn-vulkan \
    realcugan-ncnn-vulkan-20220728-ubuntu.zip \
    https://github.com/nihui/realcugan-ncnn-vulkan/releases/download/20220728/realcugan-ncnn-vulkan-20220728-ubuntu.zip \
    d745174bd04c0232c89d935b74799311008fda06bea4195f61be5f0f3cc087cb \
    realcugan-ncnn-vulkan models-se models-pro models-nose

install_archive \
    rife-ncnn-vulkan \
    rife-ncnn-vulkan-20221029-ubuntu.zip \
    https://github.com/nihui/rife-ncnn-vulkan/releases/download/20221029/rife-ncnn-vulkan-20221029-ubuntu.zip \
    1e2c7ee7fa7daa326542d50622f0afedc80cf6f1858bda411d16385ffa5cdf68 \
    rife-ncnn-vulkan rife-v4.6

install_archive \
    ifrnet-ncnn-vulkan \
    ifrnet-ncnn-vulkan-20220720-ubuntu.zip \
    https://github.com/nihui/ifrnet-ncnn-vulkan/releases/download/20220720/ifrnet-ncnn-vulkan-20220720-ubuntu.zip \
    9c37d4e145fd26d2b03a7d800839f7f1ba0b1324d1c32082923c6926d983281e \
    ifrnet-ncnn-vulkan IFRNet_Vimeo90K

install_archive \
    cain-ncnn-vulkan \
    cain-ncnn-vulkan-20220728-ubuntu.zip \
    https://github.com/nihui/cain-ncnn-vulkan/releases/download/20220728/cain-ncnn-vulkan-20220728-ubuntu.zip \
    a8e6ec971d07c43e93e599a5dd54e205514f740bb69bc6fa3c6bfc077a1b088a \
    cain-ncnn-vulkan cain

install_archive \
    dain-ncnn-vulkan \
    dain-ncnn-vulkan-20220728-ubuntu.zip \
    https://github.com/nihui/dain-ncnn-vulkan/releases/download/20220728/dain-ncnn-vulkan-20220728-ubuntu.zip \
    60defb7d9ff614de8a4e78ee951a8769dac455b00a9bc09f506df665d9302b89 \
    dain-ncnn-vulkan best

printf 'All public Linux runtime bundles are installed under %s.\n' "$ENGINES_DIRECTORY"
