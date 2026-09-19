#include <Geode/Geode.hpp>
#include "../../../../include/main.hpp"
#include "../../../../include/impl.hpp"
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include "keyboardTriggerShared.hpp"

using namespace geode::prelude;
using namespace GameObjectsFactory;

class StopKeyboardTriggerPopup : public Popup {
protected:
    EffectGameObject* m_trigger = nullptr;
    Ref<TextInput> m_groupInput;

    bool init(EffectGameObject* trigger) {
        if (!Popup::init(280.f, 160.f, "GJ_square01.png")) return false;
        
        m_trigger = trigger;

        this->setTitle("Stop Keyboard Trigger");

        auto data = stopKbEnsureData(trigger);
        auto menu = CCMenu::create();

        menu->setPosition(m_mainLayer->getContentSize() / 2.f);
        m_mainLayer->addChild(menu);

        auto lab = CCLabelBMFont::create("Target Group:", "bigFont.fnt");

        lab->setScale(0.4f);
        lab->setPosition({-70.f, 25.f});
        menu->addChild(lab);

        m_groupInput = TextInput::create(100.f, "0", "bigFont.fnt");

        m_groupInput->setCommonFilter(CommonFilter::Uint);
        m_groupInput->setString(std::to_string(data->targetGroup));
        m_groupInput->setPosition({50.f, 25.f});
        m_groupInput->setScale(0.8f);
        m_groupInput->setCallback([this](std::string const& str) {
            if (auto d = stopKbGetData(m_trigger))
                d->targetGroup = numFromString<int>(str).unwrapOr(0);
        });
        
        menu->addChild(m_groupInput);

        auto hint = CCLabelBMFont::create(
            "0 = stop ALL keyboard triggers\n"
            "N = stop keyboard triggers in group N\n"
            "(put Keyboard Triggers into that group)",
            "chatFont.fnt"
        );

        hint->setScale(0.4f);
        hint->setAlignment(kCCTextAlignmentCenter);
        hint->setPosition({0.f, -35.f});

        menu->addChild(hint);

        return true;
    }

public:
    static StopKeyboardTriggerPopup* create(EffectGameObject* trigger) {
        auto ret = new StopKeyboardTriggerPopup();
        if (ret->init(trigger)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

static void registerStopKeyboardTrigger() {
    auto config = createTriggerConfig(
        STOP_KEYBOARD_TRIGGER_ID,
        "stopKeyboardTrigger.png",

        [](EffectGameObject* trigger, GJBaseGameLayer* game, int, gd::vector<int> const*) {
            if (!trigger || !game) return;
            auto data = stopKbEnsureData(trigger);
            log::info("Stop Keyboard Trigger fired (target group={})", data->targetGroup);
            kbStopByGroup(game, data->targetGroup);
        },

        [](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray*) {
            if (!popup || !trigger) return;
            if (auto title = popup->getChildByType<CCLabelBMFont*>(0))
                title->setString("Stop Keyboard Trigger");
            if (auto inf = popup->m_buttonMenu->getChildByType<InfoAlertButton*>(0))
                inf->setVisible(false);

            auto btn = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Edit", "bigFont.fnt", "GJ_button_01.png", 0.6f),
                [trigger = Ref(trigger)](CCMenuItem*) {
                    if (auto p = StopKeyboardTriggerPopup::create(trigger)) p->show();
                }
            );
            btn->setPosition({0.f, 40.f});
            popup->m_buttonMenu->addChild(btn);
        }
    );

    config->customSetup([](GameObject* obj) {
        stopKbEnsureData(obj);
    });

    config->objectFromVector([](GameObject* obj, gd::vector<gd::string>& p0, gd::vector<void*>&, GJBaseGameLayer*, bool) -> GameObject* {
        auto data = stopKbEnsureData(obj);
        if (p0.size() > 228 && !p0[228].empty()) {
            data->fromString(ZipUtils::base64URLDecode(p0[228].c_str()));
            log::info("Loaded Stop Keyboard Trigger: group={}", data->targetGroup);
        }
        return obj;
    });

    config->saveString([](std::string str, GameObject* obj, GJBaseGameLayer*) -> gd::string {
        if (auto data = stopKbGetData(obj)) {
            str += ",228,";
            str += ZipUtils::base64URLEncode(data->toString().c_str()).c_str();
        }
        return gd::string(str.c_str());
    });

    config->registerMe();
    log::info("Stop Keyboard Trigger registered (id={})", STOP_KEYBOARD_TRIGGER_ID);
}

$execute {
    registerStopKeyboardTrigger();
}
