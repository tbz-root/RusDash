# Script Trigger — Documentation

## Overview

**Script Trigger** lets you run sandboxed scripts inside Geometry Dash levels.
Scripts use a Python-like syntax and the same high-level API (`Object`, `Player`, `wait`, etc.).

> **Note:** Editor preview is not fully supported. Prefer playtesting in PlayLayer.

---

## Execute Script Trigger

Place the blue **Python / Script** trigger from the Triggers tab, then open **Edit Object**.

1. Upload a `.py` file (or paste logic via your external editor and re-upload).
2. Optionally enable **Ignore Timeout** for long-running scripts.
3. Trigger the object in-game (group / spawn / touch as usual).

### Basic example

```python
print("Hello from Script Trigger!")
wait(1.0)
Player.kill()
```

### Death diamond

```python
i = 0
while i < 3:
    Object.move(2, 60, 60, 0.5)
    wait(0.5)
    Object.move(2, 60, -60, 0.5)
    wait(0.5)
    Object.move(2, -60, -60, 0.5)
    wait(0.5)
    Object.move(2, -60, 60, 0.5)
    wait(0.5)
    i = i + 1
    if i > 2:
        warn("last cycle done")
    else:
        print("cycle done")

error("player died")
Player.kill()
```

---

## Conditional Script Trigger

Evaluates a **boolean expression**. Depending on the result it spawns:

- **True Group** — when the expression is `true`
- **False Group** — when the expression is `false` or errors

### Example expression

```python
state.score > 100
```

```python
Player.getX() > 500
```

---

## API reference (common)

### Logging

| Call | Description |
|------|-------------|
| `print(...)` | Log / notification |
| `warn(...)` | Warning |
| `error(...)` | Error |

### Timing

| Call | Description |
|------|-------------|
| `wait(seconds)` | Yield for N seconds (coroutine) |

### Persistent state

| Call | Description |
|------|-------------|
| `state.key = value` | Store value across attempts |
| `clearState()` | Clear all persistent vars |

Values persist across attempts but reset when you leave the level.

### Player

| Call | Description |
|------|-------------|
| `Player.kill()` | Kill the player |
| `Player.getX([p])` | X position (1 or 2) |
| `Player.getY([p])` | Y position |
| `Player.getPosition([p])` | `(x, y)` |
| `Player.flipGravity([p], [noFX])` | Flip gravity |

### Object

| Call | Description |
|------|-------------|
| `Object.move(group, x, y, duration)` | Move group |
| `Object.rotate(group, center, deg, ...)` | Rotate group |
| `Object.scale(group, center, sx, sy, ...)` | Scale group |
| `Object.spawn(group, delay)` | Spawn group |

### Item / UI (if available)

| Call | Description |
|------|-------------|
| `Item.get(...)` / `Item.set(...)` | Item IDs |
| `Popup.show(title, text)` | Show popup |
| `Dialog.show(...)` | Show dialog |

---

## Tips

- Press **Shift+T** to toggle the debug console (if keybind enabled).
- Use `state` for counters, flags, and scores.
- Prefer short scripts; enable **Ignore Timeout** only when needed.
- Test in PlayLayer, not only editor preview.

---

## Support

Original project inspiration: LuaTrigger by OmgRod.  
This port targets **Script Trigger** with Python-oriented workflow.
