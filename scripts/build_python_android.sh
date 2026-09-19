#!/usr/bin/env bash
# Build embedded CPython for RusDash (Android arm64-v8a)
#
# Requirements (Linux or WSL recommended):
#   - Android NDK r26+ (r27 recommended)
#   - curl, tar, make, cmake (optional), python3 (host)
#
# Usage:
#   export ANDROID_NDK=/path/to/ndk
#   ./scripts/build_python_android.sh
#
# Optional env:
#   PYTHON_VERSION=3.11.11   (default)
#   ANDROID_API=24          (default, GD/Geode-friendly)
#   JOBS=$(nproc)           (parallel make)
#   CLEAN=1                 (wipe build dir first)
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_ROOT="${ROOT}/third_party/python-android/${ABI}"
BUILD_ROOT="${ROOT}/build/python-android"
WORKDIR="${BUILD_ROOT}/src"

PYTHON_VERSION="${PYTHON_VERSION:-3.12.9}"
# e.g. 3.12.9 -> 3.12
PY_MM="${PYTHON_VERSION%.*}"
ANDROID_API="${ANDROID_API:-24}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

# ABI: arm64-v8a (default) or armeabi-v7a (Android32)
ABI="${ANDROID_ABI:-arm64-v8a}"
case "$ABI" in
  arm64-v8a)
    TRIPLE="aarch64-linux-android"
    ;;
  armeabi-v7a)
    TRIPLE="armv7a-linux-androideabi"
    ;;
  *)
    echo "Unsupported ANDROID_ABI=$ABI (use arm64-v8a or armeabi-v7a)" >&2
    exit 1
    ;;
esac

log()  { echo -e "\033[1;32m[python-android]\033[0m $*"; }
warn() { echo -e "\033[1;33m[python-android]\033[0m $*"; }
die()  { echo -e "\033[1;31m[python-android]\033[0m $*" >&2; exit 1; }

# --- NDK ---
if [[ -z "${ANDROID_NDK:-}" ]]; then
  # Common locations
  for c in \
    "${ANDROID_NDK_HOME:-}" \
    "${ANDROID_HOME:+$ANDROID_HOME/ndk-bundle}" \
    "$HOME/Android/Sdk/ndk-bundle" \
    /opt/android-ndk \
    /usr/local/android-ndk
  do
    [[ -n "$c" && -d "$c" ]] && ANDROID_NDK="$c" && break
  done
fi
[[ -n "${ANDROID_NDK:-}" && -d "$ANDROID_NDK" ]] || die "Set ANDROID_NDK to your NDK path (r26+)."

TOOLCHAIN="${ANDROID_NDK}/toolchains/llvm/prebuilt"
# linux-x86_64 | windows-x86_64 | darwin-x86_64
HOST_TAG="$(ls "$TOOLCHAIN" 2>/dev/null | head -1 || true)"
[[ -n "$HOST_TAG" ]] || die "Cannot find NDK prebuilt toolchain under $TOOLCHAIN"
PREBUILT="${TOOLCHAIN}/${HOST_TAG}"
CC="${PREBUILT}/bin/${TRIPLE}${ANDROID_API}-clang"
CXX="${PREBUILT}/bin/${TRIPLE}${ANDROID_API}-clang++"
AR="${PREBUILT}/bin/llvm-ar"
RANLIB="${PREBUILT}/bin/llvm-ranlib"
STRIP="${PREBUILT}/bin/llvm-strip"
[[ -x "$CC" ]] || die "Compiler not found: $CC (check ANDROID_API=${ANDROID_API})"

log "NDK:        $ANDROID_NDK"
log "Host tag:   $HOST_TAG"
log "Python:     $PYTHON_VERSION"
log "API level:  $ANDROID_API"
log "Output:     $OUT_ROOT"

if [[ "${CLEAN:-0}" == "1" ]]; then
  log "Cleaning $BUILD_ROOT"
  rm -rf "$BUILD_ROOT"
fi

mkdir -p "$WORKDIR" "$OUT_ROOT/include" "$OUT_ROOT/lib" "$OUT_ROOT/stdlib"

# --- Download CPython source ---
SRC_DIR="${WORKDIR}/Python-${PYTHON_VERSION}"
TARBALL="${WORKDIR}/Python-${PYTHON_VERSION}.tgz"
URL="https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz"

if [[ ! -d "$SRC_DIR" ]]; then
  if [[ ! -f "$TARBALL" ]]; then
    log "Downloading $URL"
    curl -L --fail -o "$TARBALL" "$URL"
  fi
  log "Extracting..."
  tar -xzf "$TARBALL" -C "$WORKDIR"
fi

# --- Host Python (for build scripts / freeze) ---
HOST_PYTHON="${HOST_PYTHON:-python3}"
command -v "$HOST_PYTHON" >/dev/null || die "Host python3 required"

# --- Configure & build ---
BUILD_DIR="${BUILD_ROOT}/build-${ABI}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Cross-compile environment
export ANDROID_NDK
export PATH="${PREBUILT}/bin:$PATH"
export CC CXX AR RANLIB
export CFLAGS="-fPIC -O2 -DANDROID -D__ANDROID_API__=${ANDROID_API}"
export CPPFLAGS="-I${PREBUILT}/sysroot/usr/include"
export LDFLAGS="-L${PREBUILT}/sysroot/usr/lib/${TRIPLE}/${ANDROID_API} -llog -ldl -lm"

# Minimal modules suitable for embed (override site, tests, tk, ...)
# Users can expand MODULE_LIST later.
CONFIGURE_FLAGS=(
  --host="${TRIPLE}"
  --build="$(uname -m | sed s/arm64/aarch64/)-linux-gnu"
  --prefix="${BUILD_DIR}/install"
  --enable-shared
  --disable-test-modules
  --without-ensurepip
  --without-doc-strings
  ac_cv_file__dev_ptmx=no
  ac_cv_file__dev_ptc=no
  ac_cv_little_endian_double=yes
)

if [[ ! -f "${SRC_DIR}/configure" ]]; then
  die "configure missing in $SRC_DIR"
fi

if [[ ! -f Makefile ]]; then
  log "Configuring CPython for ${TRIPLE} (API ${ANDROID_API})..."
  # shellcheck disable=SC2086
  "${SRC_DIR}/configure" "${CONFIGURE_FLAGS[@]}"
else
  log "Already configured (delete $BUILD_DIR to reconfigure)"
fi

log "Building (jobs=$JOBS)..."
make -j"$JOBS"

log "Installing to staging..."
make install

LIB_SRC="$(find "${BUILD_DIR}/install/lib" -name "libpython${PY_MM}*.so*" | head -1 || true)"
[[ -n "$LIB_SRC" ]] || die "libpython${PY_MM}.so not found after install"

# --- Install into third_party layout ---
log "Copying artifacts to $OUT_ROOT"

# Shared lib (normalize name)
cp -f "$LIB_SRC" "${OUT_ROOT}/lib/libpython${PY_MM}.so"
if [[ -x "$STRIP" ]]; then
  "$STRIP" --strip-unneeded "${OUT_ROOT}/lib/libpython${PY_MM}.so" || true
fi

# Headers
rm -rf "${OUT_ROOT}/include"
mkdir -p "${OUT_ROOT}/include"
cp -a "${BUILD_DIR}/install/include/python${PY_MM}"*/* "${OUT_ROOT}/include/" 2>/dev/null \
  || cp -a "${BUILD_DIR}/install/include/"* "${OUT_ROOT}/include/"

# Stdlib: pack Lib/ into zip (minimal filter)
STDLIB_SRC="${SRC_DIR}/Lib"
STDLIB_ZIP="${OUT_ROOT}/stdlib/python${PY_MM//./}.zip"
mkdir -p "${OUT_ROOT}/stdlib"
log "Packing minimal stdlib -> $STDLIB_ZIP"

# Exclude bulky / useless-on-Android bits
python3 - << PY
import os, zipfile
from pathlib import Path

src = Path(r"${STDLIB_SRC}")
out = Path(r"${STDLIB_ZIP}")
skip_dirs = {
    "test", "tests", "idlelib", "tkinter", "turtledemo",
    "ctypes/test", "unittest/test", "distutils/tests",
    "lib2to3/tests", "sqlite3/test", "ensurepip",
}
skip_prefixes = tuple(s.replace("\\\\", "/") for s in skip_dirs)

with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED) as zf:
    for path in src.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(src).as_posix()
        if any(rel == p or rel.startswith(p + "/") for p in skip_prefixes):
            continue
        if rel.endswith((".pyc", ".pyo")):
            continue
        zf.write(path, rel)
print("stdlib entries:", len(zf.namelist()) if False else "ok")
print("wrote", out, "size", out.stat().st_size)
PY

# Also extract a copy for resources/python/stdlib optional packaging
EXTRACT_DIR="${ROOT}/resources/python/stdlib"
log "Extracting stdlib snapshot to $EXTRACT_DIR (optional Geode resource)"
rm -rf "$EXTRACT_DIR"
mkdir -p "$EXTRACT_DIR"
python3 - << PY
import zipfile
from pathlib import Path
zipfile.ZipFile(r"${STDLIB_ZIP}").extractall(r"${EXTRACT_DIR}")
print("extracted to", r"${EXTRACT_DIR}")
PY

# Summary
log "Done."
echo
echo "  lib:     ${OUT_ROOT}/lib/libpython${PY_MM}.so"
echo "  headers: ${OUT_ROOT}/include/"
echo "  stdlib:  ${STDLIB_ZIP}"
echo "  res:     ${EXTRACT_DIR}"
echo
echo "Next:"
echo "  1. Point CMake at libpython${PY_MM}.so (already default path for 3.11)."
echo "  2. If you built 3.12+, rename or adjust CMakeLists.txt PY version."
echo "  3. geode build (Android target)."
echo
ls -lh "${OUT_ROOT}/lib/libpython${PY_MM}.so" "${STDLIB_ZIP}"
