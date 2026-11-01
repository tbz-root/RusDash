#include <Geode/Geode.hpp>
#include "../../../../include/main.hpp"
#include "../../../../include/impl.hpp"
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include "keyboardTriggerShared.hpp"

using namespace GameObjectsFactory;

class KeyboardTriggerPopup : public Popup {
public:
    EffectGameObject* m_trigger = nullptr;
    int m_page = 0;
    bool m_listening = false;
    float m_listenDelay = 0.f;

    ScrollLayer* m_scroll = nullptr;
    CCMenu* m_listMenu = nullptr;
    Ref<CCLabelBMFont> m_pageInfo;
    Ref<CCLabelBMFont> m_keyLabel;
    Ref<CCLabelBMFont> m_modeLabel;
    Ref<TextInput> m_groupInput;
    Ref<CCMenuItemSpriteExtra> m_listenBtn;

    bool init(EffectGameObject* trigger) {
        if (!Popup::init(420.f, 280.f, "GJ_square01.png")) return false;

        m_trigger = trigger;
        this->setTitle("Keyboard Trigger");

        auto data = kbEnsureData(trigger);
        data->ensurePages();

        auto win = m_mainLayer->getContentSize();

        CCSize scrollSize = {170.f, 210.f};
        CCPoint scrollPos = {15.f, win.height / 2.f - 115.f};

        auto listBg = NineSlice::create("square02b_001.png");
        listBg->setID("pages-list-bg"_spr);
        listBg->setContentSize(scrollSize);
        listBg->setColor({0, 0, 0});
        listBg->setOpacity(80);
        listBg->setAnchorPoint({0.f, 0.f});
        listBg->setPosition(scrollPos);
        m_mainLayer->addChild(listBg);

        m_scroll = ScrollLayer::create(scrollSize);
        m_scroll->setID("pages-scroll"_spr);
        m_scroll->setPosition(scrollPos);
        m_mainLayer->addChild(m_scroll);

        auto listCtrl = CCMenu::create();
        listCtrl->setID("pages-list-controls"_spr);
        float listCenterX = scrollPos.x + scrollSize.width * 0.5f;
        listCtrl->setPosition({listCenterX, 55.f});
        m_mainLayer->addChild(listCtrl);

        auto plusSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");

        plusSpr->setScale(0.55f);

        auto addBtn = CCMenuItemExt::createSpriteExtra(plusSpr, [this](CCMenuItem*) {
            if (auto d = kbGetData(m_trigger)) {
                d->pages.push_back(KeyboardBinding{});
                m_page = (int)d->pages.size() - 1;
                this->rebuildList();
                this->loadPageToFields();
            }
        });

        addBtn->setID("page-add-btn"_spr);
        addBtn->setPosition({-48.f, 0.f});

        listCtrl->addChild(addBtn);

        auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");

        trashSpr->setScale(0.7f);

        auto delBtn = CCMenuItemExt::createSpriteExtra(trashSpr, [this](CCMenuItem*) {
            if (auto d = kbGetData(m_trigger)) {
                if (d->pages.size() <= 1) return;
                d->pages.erase(d->pages.begin() + m_page);
                if (m_page >= (int)d->pages.size()) m_page = (int)d->pages.size() - 1;
                this->rebuildList();
                this->loadPageToFields();
            }
        });

        delBtn->setID("page-del-btn"_spr);
        delBtn->setPosition({-16.f, 0.f});

        listCtrl->addChild(delBtn);

        auto upSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        upSpr->setScale(0.45f);
        upSpr->setRotation(-90.f);
        upSpr->setFlipX(true);
        auto upBtn = CCMenuItemExt::createSpriteExtra(upSpr, [this](CCMenuItem*) {
            if (m_page <= 0) return;
            if (auto d = kbGetData(m_trigger)) {
                std::swap(d->pages[m_page], d->pages[m_page - 1]);
                m_page--;
                this->rebuildList();
                this->loadPageToFields();
            }
        });
        upBtn->setID("page-up-btn"_spr);
        upBtn->setPosition({16.f, 0.f});
        listCtrl->addChild(upBtn);

        auto downSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        downSpr->setScale(0.45f);
        downSpr->setRotation(-90.f);
        auto downBtn = CCMenuItemExt::createSpriteExtra(downSpr, [this](CCMenuItem*) {
            if (auto d = kbGetData(m_trigger)) {
                if (m_page >= (int)d->pages.size() - 1) return;
                std::swap(d->pages[m_page], d->pages[m_page + 1]);
                m_page++;
                this->rebuildList();
                this->loadPageToFields();
            }
        });
        downBtn->setID("page-down-btn"_spr);
        downBtn->setPosition({48.f, 0.f});
        listCtrl->addChild(downBtn);

        auto editBox = CCNode::create();
        editBox->setID("page-edit-box"_spr);
        editBox->setPosition({300.f, win.height / 2.f});
        m_mainLayer->addChild(editBox);

        m_pageInfo = CCLabelBMFont::create("Page 1", "bigFont.fnt");
        m_pageInfo->setID("page-info-label"_spr);
        m_pageInfo->setScale(0.35f);
        m_pageInfo->setPosition({0.f, 110.f});
        editBox->addChild(m_pageInfo);

        auto keyTitle = CCLabelBMFont::create("Key:", "bigFont.fnt");
        keyTitle->setScale(0.4f);
        keyTitle->setPosition({-90.f, 70.f});
        editBox->addChild(keyTitle);

        m_keyLabel = CCLabelBMFont::create("None", "bigFont.fnt");
        m_keyLabel->setScale(0.45f);
        m_keyLabel->setPosition({20.f, 70.f});
        editBox->addChild(m_keyLabel);

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

        auto editMenu = CCMenu::create();

        editMenu->setPosition({0.f, 0.f});
        editBox->addChild(editMenu);
        editMenu->addChild(m_listenBtn);

        auto groupTitle = CCLabelBMFont::create("Group:", "bigFont.fnt");

        groupTitle->setScale(0.4f);
        groupTitle->setPosition({-90.f, -5.f});
        editBox->addChild(groupTitle);

        m_groupInput = TextInput::create(100.f, "0", "bigFont.fnt");
        m_groupInput->setCommonFilter(CommonFilter::Uint);
        m_groupInput->setPosition({30.f, -5.f});
        m_groupInput->setScale(0.8f);
        m_groupInput->setCallback([this](std::string const& str) {
            if (auto d = kbGetData(m_trigger)) {
                if (m_page >= 0 && m_page < (int)d->pages.size()) {
                    d->pages[m_page].targetGroup = numFromString<int>(str).unwrapOr(0);
                    this->rebuildList();
                }
            }
        });
        editBox->addChild(m_groupInput);

        auto modeTitle = CCLabelBMFont::create("Mode:", "bigFont.fnt");

        modeTitle->setScale(0.4f);
        modeTitle->setPosition({-90.f, -45.f});
        editBox->addChild(modeTitle);

        m_modeLabel = CCLabelBMFont::create("Hold", "bigFont.fnt");
        m_modeLabel->setScale(0.4f);
        m_modeLabel->setPosition({-10.f, -45.f});
        editBox->addChild(m_modeLabel);

        auto modeBtn = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Switch", "bigFont.fnt", "GJ_button_05.png", 0.45f),
            [this](CCMenuItem*) {
                if (auto d = kbGetData(m_trigger)) {
                    if (m_page >= 0 && m_page < (int)d->pages.size()) {
                        auto& b = d->pages[m_page];
                        b.activateOnHold = !b.activateOnHold;
                        if (m_modeLabel) m_modeLabel->setString(b.activateOnHold ? "Hold" : "Press");
                        this->rebuildList();
                    }
                }
            }
        );

        modeBtn->setPosition({70.f, -45.f});
        editMenu->addChild(modeBtn);

        auto hint = CCLabelBMFont::create(
            "Keys work only AFTER this trigger fires\n"
            "(Controller & system keys blocked)\n"
            "Pages are SEQUENTIAL: only current is active\n"
            "After press → next page unlocks, past locks",
            "chatFont.fnt"
        );

        hint->setScale(0.32f);
        hint->setAlignment(kCCTextAlignmentCenter);
        hint->setPosition({0.f, -95.f});
        editBox->addChild(hint);

        this->rebuildList();
        this->loadPageToFields();

        this->schedule(schedule_selector(KeyboardTriggerPopup::updateListen), 0.f);
        return true;
    }

    std::string pagePreview(KeyboardBinding const& b) {
        std::string key = kbKeyName(b.keyCode);
        std::string mode = b.activateOnHold ? "Hold" : "Press";
        return fmt::format("{} → G{} ({})", key, b.targetGroup, mode);
    }

    void rebuildList() {
        if (!m_scroll) return;
        m_scroll->m_contentLayer->removeAllChildren();

        auto d = kbGetData(m_trigger);
        if (!d) return;
        d->ensurePages();

        constexpr float cellH = 28.f;
        float totalH = std::max(210.f, d->pages.size() * cellH + 10.f);

        m_listMenu = CCMenu::create();
        m_listMenu->setAnchorPoint({0.f, 0.f});
        m_listMenu->ignoreAnchorPointForPosition(false);
        m_listMenu->setPosition({0.f, 0.f});
        m_listMenu->setContentSize({170.f, totalH});

        for (size_t i = 0; i < d->pages.size(); ++i) {
            bool selected = (int)i == m_page;
            auto label = CCLabelBMFont::create(pagePreview(d->pages[i]).c_str(), "chatFont.fnt");
            label->setScale(0.4f);
            label->setAnchorPoint({0.f, 0.5f});
            if (selected) label->setColor({100, 255, 100});

            auto item = CCMenuItemExt::createSpriteExtra(label, [this, i](CCMenuItem*) {
                m_page = (int)i;
                this->rebuildList();
                this->loadPageToFields();
            });
            item->m_animationEnabled = false;
            item->setPosition({10.f, totalH - 10.f - i * cellH - cellH * 0.5f});
            m_listMenu->addChild(item);
        }

        m_scroll->m_contentLayer->setContentSize({170.f, totalH});
        m_scroll->m_contentLayer->addChild(m_listMenu);
        m_scroll->scrollToTop();
    }

    void loadPageToFields() {
        auto d = kbGetData(m_trigger);
        if (!d) return;
        d->ensurePages();
        if (m_page < 0 || m_page >= (int)d->pages.size()) m_page = 0;

        auto& b = d->pages[m_page];

        if (m_pageInfo)
            m_pageInfo->setString(fmt::format("Page {} / {}", m_page + 1, d->pages.size()).c_str());

        if (m_keyLabel)
            m_keyLabel->setString(kbKeyName(b.keyCode).c_str());

        if (m_groupInput)
            m_groupInput->setString(std::to_string(b.targetGroup));

        if (m_modeLabel)
            m_modeLabel->setString(b.activateOnHold ? "Hold" : "Press");

        if (m_listenBtn) {
            if (auto spr = typeinfo_cast<ButtonSprite*>(m_listenBtn->getNormalImage()))
                spr->setString("Press key...");
        }
        m_listening = false;
        if (g_kbListeningPopup == this) g_kbListeningPopup = nullptr;
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
            if (m_page >= 0 && m_page < (int)d->pages.size()) {
                d->pages[m_page].keyCode = code;
                if (m_keyLabel) m_keyLabel->setString(kbKeyName(code).c_str());
                this->rebuildList();
            }
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

        [](EffectGameObject* trigger, GJBaseGameLayer* game, int, std::vector<int> const*) {
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

    config->objectFromVector([](GameObject* obj, std::vector<std::string>& p0, std::vector<void*>&, GJBaseGameLayer*, bool) -> GameObject* {
        auto data = kbEnsureData(obj);
        if (p0.size() > 228 && !p0[228].empty()) {
            data->fromString(ZipUtils::base64URLDecode(p0[228].c_str()));
            log::info("Loaded Keyboard Trigger: {} page(s)", data->pages.size());
            for (size_t i = 0; i < data->pages.size(); ++i) {
                auto& b = data->pages[i];
                log::info("  page {}: key={} ({}) group={} mode={}",
                    i, kbKeyName(b.keyCode), b.keyCode, b.targetGroup,
                    b.activateOnHold ? "Hold" : "Press");
            }
        }
        data->enabled = false;
        return obj;
    });

    config->saveString([](std::string str, GameObject* obj, GJBaseGameLayer*) -> std::string {
        if (auto data = kbGetData(obj)) {
            str += ",228,";
            str += ZipUtils::base64URLEncode(data->toString().c_str()).c_str();
        }
        return std::string(str.c_str());
    });

    config->resetObject([](GameObject* obj) {
        if (auto e = typeinfo_cast<EffectGameObject*>(obj))
            kbUnregister(e);
    });

    config->registerMe();
    log::info("Keyboard Trigger registered (id={})", KEYBOARD_TRIGGER_ID);
}

#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        g_kbActiveTriggers.clear();
        g_kbPressedKeys.clear();
        return PlayLayer::init(level, dontCreateObjects);
    }

    void onQuit() {
        g_kbActiveTriggers.clear();
        g_kbPressedKeys.clear();
        PlayLayer::onQuit();
    }
};

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(GJBaseGameLayer) {
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
            if (!data || !data->enabled) {
                ++it;
                continue;
            }

            if (data->currentPage < 0 || data->currentPage >= (int)data->pages.size()) {
                ++it;
                continue;
            }

            auto& b = data->pages[data->currentPage];
            if (b.keyCode == 0 || b.targetGroup <= 0) {
                data->currentPage++;
                ++it;
                continue;
            }

            bool pressed = kbIsKeyDown(b.keyCode);
            bool fired = false;

            if (b.activateOnHold) {
                if (pressed) {
                    if (!b.wasPressed) {
                        log::info("HOLD start [page {}]: {} → group {}",
                            data->currentPage, kbKeyName(b.keyCode), b.targetGroup);
                        this->spawnGroup(b.targetGroup, false, 0.0, std::vector<int>(), -1, -1);
                        fired = true;
                    }
                    this->toggleGroup(b.targetGroup, true);
                }
                b.wasPressed = pressed;
            } else {
                if (pressed && !b.wasPressed) {
                    log::info("PRESS [page {}]: {} → group {}",
                        data->currentPage, kbKeyName(b.keyCode), b.targetGroup);
                    this->spawnGroup(b.targetGroup, false, 0.0, std::vector<int>(), -1, -1);
                    this->toggleGroup(b.targetGroup, true);
                    fired = true;
                }
                b.wasPressed = pressed;
            }

            if (fired) {
                int prev = data->currentPage;
                data->currentPage++;
                if (data->currentPage < (int)data->pages.size()) {
                    data->pages[data->currentPage].wasPressed = kbIsKeyDown(data->pages[data->currentPage].keyCode);
                    log::info("Advanced to page {} / {}", data->currentPage, (int)data->pages.size() - 1);
                } else {
                    log::info("Sequence finished (was page {})", prev);
                }
            }
            ++it;
        }
    }
};

$execute {
    registerKeyboardTrigger();
}