#!/usr/bin/env bash
set -euo pipefail

readonly ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
readonly BUILD_DIR="${ROOT_DIR}/build"
readonly BOOKSIM_BIN="${ROOT_DIR}/3rdpart/booksim2/src/booksim"

die() {
  echo "package-macos: 错误: $*" >&2
  exit 1
}

[[ "$(uname -s)" == "Darwin" ]] || die "仅支持在 macOS 上运行"

command -v cmake >/dev/null 2>&1 || die "未找到 cmake"
command -v ditto >/dev/null 2>&1 || die "未找到 ditto（请安装 Xcode 命令行工具）"

if [[ -z "${QT_SDK_DIR:-}" ]]; then
  if command -v brew >/dev/null 2>&1; then
    QT_SDK_DIR="$(brew --prefix qt 2>/dev/null || true)"
  fi
fi
[[ -n "${QT_SDK_DIR:-}" && -d "$QT_SDK_DIR" ]] ||
  die "请设置 QT_SDK_DIR 为 Qt 6 安装根目录（例如 Homebrew: export QT_SDK_DIR=\"\$(brew --prefix qt)\")"

readonly MACDEPLOYQT="${QT_SDK_DIR}/bin/macdeployqt"
[[ -x "$MACDEPLOYQT" ]] || die "未找到可执行的 macdeployqt: $MACDEPLOYQT"

if command -v ninja >/dev/null 2>&1; then
  readonly CMAKE_GENERATOR="Ninja"
else
  readonly CMAKE_GENERATOR="Unix Makefiles"
fi

# 与 make/Darwin.mk 中 cdb 一致：Release + Qt 路径
cmake -B "$BUILD_DIR" -G "$CMAKE_GENERATOR" \
  -DQT_SDK_DIR="$QT_SDK_DIR" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" --config Release --parallel
cmake --build "$BUILD_DIR" --target booksim2 --config Release

[[ -f "$BOOKSIM_BIN" && -x "$BOOKSIM_BIN" ]] || die "booksim 未生成: $BOOKSIM_BIN"

APP_BUNDLE=""
if [[ -d "${BUILD_DIR}/BookCanvas.app" ]]; then
  APP_BUNDLE="${BUILD_DIR}/BookCanvas.app"
elif [[ -d "${BUILD_DIR}/Release/BookCanvas.app" ]]; then
  APP_BUNDLE="${BUILD_DIR}/Release/BookCanvas.app"
else
  die "未找到 BookCanvas.app（请先确认 CMake 配置与生成器）"
fi

readonly MACOS_DIR="${APP_BUNDLE}/Contents/MacOS"
[[ -d "$MACOS_DIR" ]] || die "无效的 .app 结构: $MACOS_DIR"

cp -f "$BOOKSIM_BIN" "${MACOS_DIR}/booksim"
chmod +x "${MACOS_DIR}/booksim"

# 将 Qt 框架打入包内；booksim 一并声明，便于解析其依赖的非系统库（若有）
"$MACDEPLOYQT" "$APP_BUNDLE" \
  -executable="${MACOS_DIR}/booksim" \
  -verbose=1

VER="$(git -C "$ROOT_DIR" describe --tags --always --dirty 2>/dev/null || echo "unknown")"
VER_SAFE="$(printf '%s' "$VER" | tr '/:' '--')"
ARCH="$(uname -m)"
readonly OUT_ZIP="${BUILD_DIR}/BookCanvas-${VER_SAFE}-macOS-${ARCH}.zip"

rm -f "$OUT_ZIP"
# 使用 ditto 保留符号链接与 Mac 资源分支，便于分发
ditto -c -k --sequesterRsrc --keepParent "$APP_BUNDLE" "$OUT_ZIP"

echo "已生成: $OUT_ZIP"