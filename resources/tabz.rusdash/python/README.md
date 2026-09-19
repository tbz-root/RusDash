# Optional: ship a minimal Python stdlib for Android

On Android, place an extracted stdlib tree here:

```
resources/python/stdlib/
  encodings/
  ...
```

Or follow `third_party/python-android/README.md` and copy into the mod save dir at runtime.

Windows builds use the system Python and do not need this folder.
