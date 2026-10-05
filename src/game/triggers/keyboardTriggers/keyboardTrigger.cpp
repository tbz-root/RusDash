#include <Geode/Geode.hpp>
#include "../../../../include/main.hpp"
#include "../../../../include/impl.hpp"
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include "keyboardTriggerShared.hpp"

using namespace GameObjectsFactory;

class KeyboardTriggerPopup : public Popup {
public:
    EffectGameObject* m_trigger = nullptr;
    Ref<CCLabelBMFont> m_keyLabel;
    Ref<CCLabelBMFont> m_modeLabel;
    Ref<TextInput> m_groupInput;
    bool m_listening = false;
    Ref<CCMenuItemSpriteExtra> m_listenBtn;
    float m_listenDelay = 0.f;

    bool init(EffectGameObject* trigger) {
        if (!Popup::init(300.f, 220.f, "GJ_square01.png")) return false;
        
        m_trigger = trigger;
        this->setTitle("Keyboard Trigger");

        auto data = kbEnsureData(trigger);
        auto menu = CCMenu::create();

        menu->setPosition(m_mainLayer->getContentSize() / 2.f);
        m_mainLayer->addChild(menu);

        auto keyTitle = CCLabelBMFont::create("Key:", "bigFont.fnt");
        keyTitle->setScale(0.4f);
        keyTitle->setPosition({-90.f, 70.f});
        menu->addChild(keyTitle);

        m_keyLabel = CCLabelBMFont::create(kbKeyName(data->keyCode).c_str(), "bigFont.fnt");
        m_keyLabel->setScale(0.45f);
        m_keyLabel->setPosition({20.f, 70.f});
        menu->addChild(m_keyLabel);

        auto listenSpr = ButtonSprite::create("Press key...", "bigFont.fnt", "GJ_button_04.png", 0.5f);
        m_listenBtn = CCMenuItemExt::createSpriteExtra(listenSpr, [this](CCMenuItem*) {
            m_listening = true;
            m_listenDelay = 0.2f;
            g_kbListeningPopup = this;
            if (m_keyLabel) m_keyLabel->setString("...");
            if (m_listenBtn) {
                if (auto spr = typeinfo_cast<ButtonSprite*>(m_listenBtn->getNormalImage()))
                    spr->setString("Waiting...");
            }
        });
        m_listenBtn->setPosition({0.f, 35.f});
        menu->addChild(m_listenBtn);

        auto groupTitle = CCLabelBMFont::create("Group:", "bigFont.fnt");

        groupTitle->setScale(0.4f);
        groupTitle->setPosition({-90.f, -5.f});

        menu->addChild(groupTitle);

        m_groupInput = TextInput::create(100.f, "0", "bigFont.fnt");
        m_groupInput->setCommonFilter(CommonFilter::Uint);
        m_groupInput->setString(std::to_string(data->targetGroup));
        m_groupInput->setPosition({30.f, -5.f});
        m_groupInput->setScale(0.8f);
        m_groupInput->setCallback([this](std::string const& str) {
            if (auto d = kbGetData(m_trigger))
                d->targetGroup = numFromString<int>(str).unwrapOr(0);
        });
        menu->addChild(m_groupInput);

        auto modeTitle = CCLabelBMFont::create("Mode:", "bigFont.fnt");
        modeTitle->setScale(0.4f);
        modeTitle->setPosition({-90.f, -45.f});
        menu->addChild(modeTitle);

        m_modeLabel = CCLabelBMFont::create(data->activateOnHold ? "Hold" : "Press", "bigFont.fnt");
        m_modeLabel->setScale(0.4f);
        m_modeLabel->setPosition({-10.f, -45.f});
        menu->addChild(m_modeLabel);

        auto modeBtn = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Switch", "bigFont.fnt", "GJ_button_05.png", 0.45f),
            [this](CCMenuItem*) {
                if (auto d = kbGetData(m_trigger)) {
                    d->activateOnHold = !d->activateOnHold;
                    if (m_modeLabel) m_modeLabel->setString(d->activateOnHold ? "Hold" : "Press");
                }
            }
        );
        modeBtn->setPosition({70.f, -45.f});
        menu->addChild(modeBtn);

        auto hint = CCLabelBMFont::create(
            "Keys work only AFTER this trigger fires\n"
            "(Controller & system keys blocked)\n"
            "Hold = while held | Press = once",
            "chatFont.fnt"
        );
        hint->setScale(0.36f);
        hint->setAlignment(kCCTextAlignmentCenter);
        hint->setPosition({0.f, -90.f});
        menu->addChild(hint);

        this->schedule(schedule_selector(KeyboardTriggerPopup::updateListen), 0.f);
        return true;
    }

    void onClose(CCObject* sender) override {
        if (g_kbListeningPopup == this) g_kbListeningPopup = nullptr;
        m_listening = false;
        g_kbPressedKeys.clear();
        Popup::onClose(sender);
    }

    void onExit() override {
        if (g_kbListeningPopup == this) g_kbListeningPopup = nullptr;
        m_listening = false;
        Popup::onExit();
    }

    void captureKey(int code) {
        if (!kbIsKeyboardKey(code)) return;
        log::info("Key captured: {} ({})", kbKeyName(code), code);
        if (auto d = kbGetData(m_trigger)) {
            d->keyCode = code;
            if (m_keyLabel) m_keyLabel->setString(kbKeyName(code).c_str());
        }
        m_listening = false;
        g_kbListeningPopup = nullptr;
        if (m_listenBtn) {
            if (auto spr = typeinfo_cast<ButtonSprite*>(m_listenBtn->getNormalImage()))
                spr->setString("Press key...");
        }
    }

    void updateListen(float dt) {
        if (!m_listening) return;
        if (m_listenDelay > 0.f) { m_listenDelay -= dt; return; }
        for (int code : g_kbPressedKeys) {
            if (kbIsKeyboardKey(code)) {
                this->captureKey(code);
                return;
            }
        }
    }

    static KeyboardTriggerPopup* create(EffectGameObject* trigger) {
        auto ret = new KeyboardTriggerPopup();
        if (ret->init(trigger)) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

#include <Geode/modify/CCKeyboardDispatcher.hpp>
class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double time) {
        int code = static_cast<int>(key);
        if (down) g_kbPressedKeys.insert(code);
        else g_kbPressedKeys.erase(code);

        if (down && !repeat && g_kbListeningPopup && g_kbListeningPopup->m_listening
            && g_kbListeningPopup->m_listenDelay <= 0.f) {
            if (kbIsKeyboardKey(code))
                g_kbListeningPopup->captureKey(code);
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, time);
    }
};

static void registerKeyboardTrigger() {
    auto config = createTriggerConfig(
        KEYBOARD_TRIGGER_ID,
        "keyboardTrigger.png",

        [](EffectGameObject* trigger, GJBaseGameLayer* game, int, gd::vector<int> const*) {
            if (!trigger || !game) return;
            kbRegisterActive(trigger);
        },

        [](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray*) {
            if (!popup || !trigger) return;
            if (auto title = popup->getChildByType<CCLabelBMFont*>(0))
                title->setString("Keyboard Trigger");
            if (auto inf = popup->m_buttonMenu->getChildByType<InfoAlertButton*>(0))
                inf->setVisible(false);

            auto btn = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Edit", "bigFont.fnt", "GJ_button_01.png", 0.6f),
                [trigger = Ref(trigger)](CCMenuItem*) {
                    if (auto p = KeyboardTriggerPopup::create(trigger)) p->show();
                }
            );
            btn->setPosition({0.f, 40.f});
            popup->m_buttonMenu->addChild(btn);
        }
    );

    config->customSetup([](GameObject* obj) {
        auto d = kbEnsureData(obj);
        d->enabled = false;
    });

    config->objectFromVector([](GameObject* obj, gd::vector<gd::string>& p0, gd::vector<void*>&, GJBaseGameLayer*, bool) -> GameObject* {
        auto data = kbEnsureData(obj);
        if (p0.size() > 228 && !p0[228].empty()) {
            data->fromString(ZipUtils::base64URLDecode(p0[228].c_str()));
            log::info("Loaded Keyboard Trigger: key={} ({}) group={} mode={}",
                kbKeyName(data->keyCode), data->keyCode, data->targetGroup,
                data->activateOnHold ? "Hold" : "Press");
        }
        data->enabled = false;
        return obj;
    });

    config->saveString([](std::string str, GameObject* obj, GJBaseGameLayer*) -> gd::string {
        if (auto data = kbGetData(obj)) {
            str += ",228,";
            str += ZipUtils::base64URLEncode(data->toString().c_str()).c_str();
        }
        return gd::string(str.c_str());
    });

    config->resetObject([](GameObject* obj) {
        if (auto e = typeinfo_cast<EffectGameObject*>(obj))
            kbUnregister(e);
    });

    config->registerMe();
    log::info("Keyboard Trigger registered (id={})", KEYBOARD_TRIGGER_ID);
}

#include <Geode/modify/PlayLayer.hpp>

class $modify(KB_PlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        g_kbActiveTriggers.clear();
        g_kbPressedKeys.clear();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void onQuit() {
        g_kbActiveTriggers.clear();
        g_kbPressedKeys.clear();
        PlayLayer::onQuit();
    }
};

#include <Geode/modify/GJBaseGameLayer.hpp>

class $modify(KB_Runtime, GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (g_kbActiveTriggers.empty()) return;

        if (auto pl = typeinfo_cast<PlayLayer*>(this)) {
            if (pl->m_hasCompletedLevel || pl->m_levelEndAnimationStarted) return;
            if (pl->m_player1 && pl->m_player1->m_isDead) return;
        }

        for (auto it = g_kbActiveTriggers.begin(); it != g_kbActiveTriggers.end(); ) {
            auto* trigger = *it;
            if (!trigger || trigger->retainCount() == 0) {
                it = g_kbActiveTriggers.erase(it);
                continue;
            }

            auto* data = kbGetData(trigger);
            if (!data || !data->enabled || data->keyCode == 0 || data->targetGroup <= 0) {
                ++it;
                continue;
            }

            bool pressed = kbIsKeyDown(data->keyCode);

            if (data->activateOnHold) {
                if (pressed) {
                    if (!data->wasPressed) {
                        log::info("HOLD start: {} → group {}", kbKeyName(data->keyCode), data->targetGroup);
                        this->spawnGroup(data->targetGroup, false, 0.0, gd::vector<int>(), -1, -1);
                    }
                    this->toggleGroup(data->targetGroup, true);
                }
                data->wasPressed = pressed;
            } else {
                if (pressed && !data->wasPressed) {
                    log::info("PRESS: {} → group {}", kbKeyName(data->keyCode), data->targetGroup);
                    this->spawnGroup(data->targetGroup, false, 0.0, gd::vector<int>(), -1, -1);
                    this->toggleGroup(data->targetGroup, true);
                }
                data->wasPressed = pressed;
            }
            ++it;
        }
    }
};

$execute {
    registerKeyboardTrigger();
}
