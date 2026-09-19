# RusDash — Windows + Android Python

## Platforms

| Platform | Support |
|----------|---------|
| Windows | Yes — system Python 3.x + pybind11 |
| Android | Yes — prebuilt `libpython` in `third_party/python-android/` |
| Mac / iOS | No |
| Linux native | No native GD (Wine/Proton only) |

## Windows build

1. Install Python 3.11+ **with development headers** (checkbox on Windows installer).
2. Ensure `python` is on PATH.
3. `geode build --ninja`

## Android build

1. Obtain `libpython3.11.so` + headers for **arm64-v8a** (see `third_party/python-android/README.md`).
2. Place them under:

```
third_party/python-android/arm64-v8a/
  include/
  lib/libpython3.11.so
```

3. Provide a minimal stdlib (zip or folder) and either:
   - put extracted modules in `resources/python/stdlib/`, or
   - copy into the mod save dir as documented in code.
4. Build the Android target with Geode the usual way.

Without `libpython3.11.so`, CMake configures with a **warning** and the **link step fails** until the library is present.

## Runtime notes

- One embedded interpreter per process.
- Per-level `state` dict is isolated per `GJBaseGameLayer`.
- `wait()` is still a stub (no cooperative yield yet).
