# Android embedded CPython for

Geode on Android cannot use the host `find_package(Python3)`.  
You must ship a **prebuilt** `libpython` + stdlib for `arm64-v8a`.

## Expected layout

```
third_party/python-android/
  arm64-v8a/
    include/          # Python.h and headers
    lib/
      libpython3.11.so
    stdlib/
      python311.zip   # or a "lib/" tree of stdlib modules
  README.md
```

## What to put where

| Path | Contents |
|------|----------|
| `arm64-v8a/lib/libpython3.11.so` | Shared CPython built with Android NDK (same STL as Geode if possible) |
| `arm64-v8a/include/` | Headers from that build (`Python.h`, ...) |
| `arm64-v8a/stdlib/python311.zip` | Minimal stdlib zip (at least `encodings`, `codecs`, `io`, `os`, `types`, ...) |

## How to get a build

Options (pick one):

1. **Build CPython with Android NDK**  
   - Official CPython `Android/README.md` (3.11+) or community scripts.  
   - Target API level compatible with Geometry Dash / Geode.  
   - ABI: `arm64-v8a`.

2. **Reuse a known-good prebuilt** from another project (check license).  
   Extract `libpython3.xx.so` + include + stdlib into the layout above.

3. **python-for-android / Chaquopy-style builds** — possible, but heavier; you only need the `.so` + stdlib, not the full packaging stack.

## Runtime (handled in code)

On first run the mod should:

1. Copy/extract `stdlib` into the mod save/cache directory (writable).
2. Set `PYTHONHOME` / `PYTHONPATH` to that directory.
3. Call `Py_Initialize` / `pybind11::initialize_interpreter()`.

See `PythonInterpreter::ensurePython()` in `src/PythonInterpreter.cpp`.

## Size tips

- Strip the `.so` (`llvm-strip`).
- Ship a **minimal** stdlib zip (drop tests, idlelib, tkinter, distutils tests, etc.).
- Prefer one ABI only (`arm64-v8a`) unless you must support 32-bit devices.

## License

CPython is PSF License. Keep attribution if you redistribute binaries.

## Automated build script

From Linux or **WSL**:

```bash
export ANDROID_NDK=$HOME/Android/Sdk/ndk/27.0.12077973   # your NDK
./scripts/build_python_android.sh
```

Optional:

```bash
CLEAN=1 PYTHON_VERSION=3.11.11 ANDROID_API=24 ./scripts/build_python_android.sh
```

On Windows (with WSL + NDK visible from WSL):

```bat
set ANDROID_NDK=/home/you/Android/Sdk/ndk/27.0.12077973
scripts\build_python_android.bat
```

Artifacts land in `third_party/python-android/arm64-v8a/` and a stdlib extract in `resources/python/stdlib/`.
