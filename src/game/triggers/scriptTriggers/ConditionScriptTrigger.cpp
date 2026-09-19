#include "ConditionScriptTrigger.hpp"
#include "DocsPopup.hpp"
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/binding/LevelTools.hpp>

using namespace geode::prelude;

void ConditionScriptTrigger::setupEditPopup(
    EditTriggersPopup* popup,
    EffectGameObject* trigger,
    CCArray* objects
) {

    if (!popup) {
        return;
    }
    if (!trigger) {
        return;
    }

    auto* data = getData(trigger, true);
    if (!data) {
        return;
    }

    auto* root = popup->m_mainLayer;
    if (!root) {
        root = popup;
    }

    auto winSize = root->getContentSize();
    if (winSize.width < 50.f || winSize.height < 50.f) {
        winSize = CCSize{ 420.f, 300.f };
    }

    auto panel = NineSlice::create("GJ_square01.png");
    if (!panel) {
        auto fallback = CCLabelBMFont::create(
            "Conditional Script Trigger\n(UI assets missing)",
            "bigFont.fnt"
        );

        fallback->setScale(0.45f);
        fallback->setPosition(winSize / 2);

        root->addChild(fallback);

        return;
    }

    panel->setContentSize({ 360.f, 220.f });
    panel->setOpacity(250);
    panel->setPosition({ winSize.width / 2.f, winSize.height / 2.f });
    root->addChild(panel);

    float cx = 180.f;
    float top = 200.f;

    auto title = CCLabelBMFont::create("Conditional Script Trigger", "bigFont.fnt");

    title->setScale(0.42f);
    title->setPosition({ cx, top });
    panel->addChild(title);

    auto docsMenu = CCMenu::create();
    docsMenu->setPosition({ 0.f, 0.f });
    panel->addChild(docsMenu);

    auto docsBtn = CCMenuItemExt::createSpriteExtra(
        ButtonSprite::create("Docs", 55, true, "goldFont.fnt", "GJ_button_01.png", 22.f, 0.4f),
        [](CCObject*) {
            if (auto p = DocsPopup::create()) {
                p->show();
            }
        }
    );
    docsBtn->setPosition({ 320.f, top });
    docsMenu->addChild(docsBtn);

    auto help = CCLabelBMFont::create(
        "Python expression -> True / False group",
        "chatFont.fnt"
    );

    help->setScale(0.55f);
    help->setPosition({ cx, top - 24.f });

    panel->addChild(help);

    auto trueLabel = CCLabelBMFont::create("True Group", "bigFont.fnt");

    trueLabel->setScale(0.3f);
    trueLabel->setColor({ 80, 255, 120 });
    trueLabel->setPosition({ cx - 90.f, top - 52.f });
    panel->addChild(trueLabel);

    auto trueInput = TextInput::create(100.f, "0", "bigFont.fnt");

    trueInput->setCommonFilter(CommonFilter::Uint);
    trueInput->setMaxCharCount(6);
    trueInput->setString(std::to_string(data->trueGroup));
    trueInput->setPosition({ cx - 90.f, top - 80.f });
    trueInput->setCallback([data](std::string const& str) {
        try {
            data->trueGroup = str.empty() ? 0 : std::stoi(str);
        } catch (...) {
            data->trueGroup = 0;
        }
    });

    panel->addChild(trueInput);

    auto falseLabel = CCLabelBMFont::create("False Group", "bigFont.fnt");

    falseLabel->setScale(0.3f);
    falseLabel->setColor({ 255, 90, 90 });
    falseLabel->setPosition({ cx + 90.f, top - 52.f });

    panel->addChild(falseLabel);

    auto falseInput = TextInput::create(100.f, "0", "bigFont.fnt");

    falseInput->setCommonFilter(CommonFilter::Uint);
    falseInput->setMaxCharCount(6);
    falseInput->setString(std::to_string(data->falseGroup));
    falseInput->setPosition({ cx + 90.f, top - 80.f });
    falseInput->setCallback([data](std::string const& str) {
        try {
            data->falseGroup = str.empty() ? 0 : std::stoi(str);
        } catch (...) {
            data->falseGroup = 0;
        }
    });

    panel->addChild(falseInput);

    auto exprLabel = CCLabelBMFont::create("Condition Expression", "bigFont.fnt");
    exprLabel->setScale(0.3f);
    exprLabel->setPosition({ cx, top - 112.f });
    panel->addChild(exprLabel);

    std::string currentExpr;
    if (!data->b64code.empty()) {
        auto decoded = LevelTools::base64DecodeString(data->b64code);
        currentExpr.assign(decoded.c_str(), decoded.size());
    }

    auto exprInput = TextInput::create(300.f, "e.g. state.score > 100", "chatFont.fnt");
    exprInput->setString(currentExpr);
    exprInput->setPosition({ cx, top - 140.f });
    exprInput->setCallback([data](std::string const& str) {
        if (str.empty()) {
            data->b64code.clear();
            data->active = false;
            return;
        }
        gd::string encoded = LevelTools::base64EncodeString(
            gd::string(str.c_str(), str.size())
        );
        data->b64code.assign(encoded.c_str(), encoded.size());
        data->active = true;
    });
    panel->addChild(exprInput);

    auto ignoreLabel = CCLabelBMFont::create("Ignore Timeout", "bigFont.fnt");

    ignoreLabel->setScale(0.28f);
    ignoreLabel->setPosition({ cx - 30.f, 28.f });

    panel->addChild(ignoreLabel);

    auto toggleMenu = CCMenu::create();

    toggleMenu->setPosition({ 0.f, 0.f });

    panel->addChild(toggleMenu);

    class IgnoreTimeoutProxy : public CCNode {
    public:
        ConditionScriptData* data = nullptr;
        void onToggle(CCObject* sender) {
            auto* toggler = static_cast<CCMenuItemToggler*>(sender);
            if (data) {
                data->ignoreTimeout = !toggler->isToggled();
            }
        }
        static IgnoreTimeoutProxy* create() {
            auto ret = new IgnoreTimeoutProxy();
            if (ret && ret->init()) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }
    };

    auto proxy = IgnoreTimeoutProxy::create();
    if (proxy) {
        proxy->data = data;
        panel->addChild(proxy);
    }

    auto toggle = CCMenuItemToggler::createWithStandardSprites(
        proxy,
        menu_selector(IgnoreTimeoutProxy::onToggle),
        0.55f
    );
    toggle->setPosition({ cx + 55.f, 28.f });
    if (data->ignoreTimeout) {
        toggle->toggle(true);
    }
    toggleMenu->addChild(toggle);
}

#include <main.hpp>
$execute {
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createTriggerConfig(
            UNIQ_ID("condition-script-trigger"),
            "condition.png"_spr
        )
        ->triggerObject([](EffectGameObject* obj, GJBaseGameLayer* layer, int p1, gd::vector<int> const* p2) {
            ConditionScriptTrigger::onTriggerObject(obj, layer, p1, p2);
        })
        ->customSetup([](GameObject* obj) {
            ConditionScriptTrigger::onCustomSetup(obj);
        })
        ->editPopupSetup([](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects) {
            ConditionScriptTrigger::setupEditPopup(popup, trigger, objects);
        })
        ->saveString([](gd::string str, GameObject* object, GJBaseGameLayer* level) {
            return ConditionScriptTrigger::onSaveString(str, object, level);
        })
        ->objectFromVector([](GameObject* object, gd::vector<gd::string>& p0, gd::vector<void*>& p1, GJBaseGameLayer* level, bool p4) {
            return ConditionScriptTrigger::onObjectFromVector(object, p0, p1, level, p4);
        })
    );
}
