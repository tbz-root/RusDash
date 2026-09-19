#pragma once

#include <Geode/Geode.hpp>
#include "../../../../include/main.hpp"
#include <unordered_set>
#include <vector>
#include <algorithm>

using namespace geode::prelude;

#define KEYBOARD_TRIGGER_ID UNIQ_ID("keyboard-trigger")
#define STOP_KEYBOARD_TRIGGER_ID UNIQ_ID("stop-keyboard-trigger")

class KeyboardTriggerData : public CCObject {
public:
    int keyCode = 0;
    int targetGroup = 0;
    bool activateOnHold = true;
    bool wasPressed = false;
    bool enabled = false;

    static KeyboardTriggerData* create() {
        auto ret = new KeyboardTriggerData();
        ret->autorelease();
        return ret;
    }

    std::string toString() const {
        return fmt::format("{},{},{}", keyCode, targetGroup, activateOnHold ? 1 : 0);
    }

    void fromString(std::string const& s) {
        auto parts = string::split(s, ",");
        if (parts.size() >= 1) keyCode     = numFromString<int>(parts[0]).unwrapOr(0);
        if (parts.size() >= 2) targetGroup = numFromString<int>(parts[1]).unwrapOr(0);
        if (parts.size() >= 3) activateOnHold = parts[2] != "0";
        enabled = false;
        wasPressed = false;
    }
};

class StopKeyboardTriggerData : public CCObject {
public:
    int targetGroup = 0;

    static StopKeyboardTriggerData* create() {
        auto ret = new StopKeyboardTriggerData();
        ret->autorelease();
        return ret;
    }

    std::string toString() const { return std::to_string(targetGroup); }
    void fromString(std::string const& s) {
        targetGroup = numFromString<int>(s).unwrapOr(0);
    }
};

inline std::unordered_set<int> g_kbPressedKeys;
inline std::vector<EffectGameObject*> g_kbActiveTriggers;
inline class KeyboardTriggerPopup* g_kbListeningPopup = nullptr;

inline KeyboardTriggerData* kbGetData(GameObject* obj) {
    if (!obj) return nullptr;
    return typeinfo_cast<KeyboardTriggerData*>(obj->getUserObject("kb-data"_spr));
}

inline void kbSetData(GameObject* obj, KeyboardTriggerData* data) {
    if (obj) obj->setUserObject("kb-data"_spr, data);
}

inline KeyboardTriggerData* kbEnsureData(GameObject* obj) {
    auto d = kbGetData(obj);
    if (!d) {
        d = KeyboardTriggerData::create();
        kbSetData(obj, d);
    }
    return d;
}

inline StopKeyboardTriggerData* stopKbGetData(GameObject* obj) {
    if (!obj) return nullptr;
    return typeinfo_cast<StopKeyboardTriggerData*>(obj->getUserObject("stop-kb-data"_spr));
}

inline void stopKbSetData(GameObject* obj, StopKeyboardTriggerData* data) {
    if (obj) obj->setUserObject("stop-kb-data"_spr, data);
}

inline StopKeyboardTriggerData* stopKbEnsureData(GameObject* obj) {
    auto d = stopKbGetData(obj);
    if (!d) {
        d = StopKeyboardTriggerData::create();
        stopKbSetData(obj, d);
    }
    return d;
}

inline void kbUnregister(EffectGameObject* effect) {
    if (!effect) return;
    auto it = std::find(g_kbActiveTriggers.begin(), g_kbActiveTriggers.end(), effect);
    if (it != g_kbActiveTriggers.end()) g_kbActiveTriggers.erase(it);
    if (auto d = kbGetData(effect)) {
        d->enabled = false;
        d->wasPressed = false;
    }
}

inline void kbRegisterActive(EffectGameObject* effect) {
    if (!effect) return;
    auto d = kbEnsureData(effect);
    d->enabled = true;
    d->wasPressed = false;
    if (std::find(g_kbActiveTriggers.begin(), g_kbActiveTriggers.end(), effect)
        == g_kbActiveTriggers.end()) {
        g_kbActiveTriggers.push_back(effect);
    }
    log::info("Keyboard Trigger ENABLED: key={} group={} mode={}",
        d->keyCode, d->targetGroup, d->activateOnHold ? "Hold" : "Press");
}

inline bool kbObjectInGroup(GameObject* obj, int groupId) {
    if (!obj || groupId <= 0) return false;
    if (obj->m_groups) {
        for (auto const& g : *obj->m_groups) {
            if ((int)g == groupId) return true;
        }
    }
    return false;
}

inline void kbStopByGroup(GJBaseGameLayer* layer, int targetGroup) {
    if (targetGroup <= 0) {
        for (auto* t : g_kbActiveTriggers) {
            if (auto d = kbGetData(t)) {
                d->enabled = false;
                d->wasPressed = false;
            }
        }
        g_kbActiveTriggers.clear();
        log::info("Stopped ALL keyboard triggers");
        return;
    }

    for (auto it = g_kbActiveTriggers.begin(); it != g_kbActiveTriggers.end(); ) {
        auto* t = *it;
        if (!t) {
            it = g_kbActiveTriggers.erase(it);
            continue;
        }
        bool match = kbObjectInGroup(t, targetGroup);
        if (match) {
            if (auto d = kbGetData(t)) {
                d->enabled = false;
                d->wasPressed = false;
            }
            log::info("Stopped keyboard trigger in group {}", targetGroup);
            it = g_kbActiveTriggers.erase(it);
        } else {
            ++it;
        }
    }
}

inline bool kbIsBlockedKey(int code) {
    switch (code) {
        case 27: case 44: case 19: case 91: case 92: case 93: case 122: case 123:
            return true;
        default:
            return code == static_cast<int>(KEY_Escape);
    }
}

inline bool kbIsKeyboardKey(int code) {
    if (code == 0 || kbIsBlockedKey(code)) return false;
    switch (code) {
        case 8: case 9: case 13: case 20: case 32:
        case 33: case 34: case 35: case 36:
        case 37: case 38: case 39: case 40:
        case 45: case 46: case 145:
        case 16: case 17: case 18:
        case 160: case 161: case 162: case 163: case 164: case 165:
        case 186: case 187: case 188: case 189: case 190:
        case 191: case 192: case 219: case 220: case 221: case 222:
        case 96: case 97: case 98: case 99: case 100:
        case 101: case 102: case 103: case 104: case 105:
        case 106: case 107: case 109: case 110: case 111:
            return true;
        default: break;
    }
    if (code >= 48 && code <= 57) return true;
    if (code >= 65 && code <= 90) return true;
    if (code >= 97 && code <= 122) return true;
    if (code >= 112 && code <= 121) return true;
    return false;
}

inline bool kbIsKeyDown(int code) {
    if (!kbIsKeyboardKey(code)) return false;
    if (g_kbPressedKeys.count(code)) return true;
#ifdef GEODE_IS_WINDOWS
    if (GetAsyncKeyState(code) & 0x8000) return true;
#endif
    return false;
}

inline std::string kbKeyName(int code) {
    if (code == 0) return "None";
    switch (code) {
        case 8: return "Backspace"; case 9: return "Tab"; case 13: return "Enter";
        case 20: return "Caps Lock"; case 32: return "Space";
        case 33: return "PgUp"; case 34: return "PgDn"; case 35: return "End"; case 36: return "Home";
        case 37: return "Left"; case 38: return "Up"; case 39: return "Right"; case 40: return "Down";
        case 45: return "Insert"; case 46: return "Delete"; case 145: return "Scroll Lock";
        case 16: return "Shift"; case 17: return "Ctrl"; case 18: return "Alt";
        case 160: return "Left Shift"; case 161: return "Right Shift";
        case 162: return "Left Ctrl"; case 163: return "Right Ctrl";
        case 164: return "Left Alt"; case 165: return "Right Alt";
        case 186: return ";"; case 187: return "+"; case 188: return ",";
        case 189: return "-"; case 190: return "."; case 191: return "/";
        case 192: return "~"; case 219: return "["; case 220: return "\\";
        case 221: return "]"; case 222: return "'";
        case 96: return "Num 0"; case 97: return "Num 1"; case 98: return "Num 2";
        case 99: return "Num 3"; case 100: return "Num 4"; case 101: return "Num 5";
        case 102: return "Num 6"; case 103: return "Num 7"; case 104: return "Num 8";
        case 105: return "Num 9"; case 106: return "Num *"; case 107: return "Num +";
        case 109: return "Num -"; case 110: return "Num ."; case 111: return "Num /";
        default:
            if (code >= 48 && code <= 57) return std::string(1, char('0' + (code - 48)));
            if (code >= 65 && code <= 90) return std::string(1, char('A' + (code - 65)));
            if (code >= 97 && code <= 122) return std::string(1, char('A' + (code - 97)));
            if (code >= 112 && code <= 121) return "F" + std::to_string(code - 111);
            return "Key " + std::to_string(code);
    }
}
