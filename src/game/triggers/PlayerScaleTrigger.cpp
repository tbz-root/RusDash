using namespace geode::prelude;

#include <Geode/Geode.hpp>
#include "../../../include/main.hpp"
#include "../../../include/impl.hpp"
#include "../../nodes/TriggerTemplatePopup.hpp"
#include "../../nodes/TargetGroupInput.hpp"
constexpr int PROP_DATA = 228;

static std::string psEncode(float sx, float sy, bool p1, bool p2, bool rel) {
    return fmt::format("{};{};{};{};{}", sx, sy, p1 ? 1 : 0, p2 ? 1 : 0, rel ? 1 : 0);
}

static void psDecode(std::string const& raw, float& sx, float& sy, bool& p1, bool& p2, bool& rel) {
    sx = 1.f; sy = 1.f; p1 = true; p2 = false; rel = false;
    if (raw.empty()) return;

    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= raw.size()) {
        auto pos = raw.find(';', start);
        if (pos == std::string::npos) {
            parts.push_back(raw.substr(start));
            break;
        }
        parts.push_back(raw.substr(start, pos - start));
        start = pos + 1;
    }
    if (parts.size() > 0) sx  = numFromString<float>(parts[0]).unwrapOr(1.f);
    if (parts.size() > 1) sy  = numFromString<float>(parts[1]).unwrapOr(1.f);
    if (parts.size() > 2) p1  = numFromString<int>(parts[2]).unwrapOr(1) != 0;
    if (parts.size() > 3) p2  = numFromString<int>(parts[3]).unwrapOr(0) != 0;
    if (parts.size() > 4) rel = numFromString<int>(parts[4]).unwrapOr(0) != 0;
}

static CCNode* psEnsureData(GameObject* obj) {
    if (!obj) return nullptr;
    auto data = typeinfo_cast<CCNode*>(obj->getUserObject("data"_spr));
    if (!data) {
        data = CCNode::create();
        data->setID(psEncode(1.f, 1.f, true, false, false).c_str());
        obj->setUserObject("data"_spr, data);
    }
    return data;
}

static void psWrite(EffectGameObject* t, float sx, float sy, bool p1, bool p2, bool rel) {
    if (!t) return;
    if (auto data = psEnsureData(t)) {
        data->setID(psEncode(sx, sy, p1, p2, rel).c_str());
    }
}

static void psRead(EffectGameObject* t, float& sx, float& sy, bool& p1, bool& p2, bool& rel) {
    sx = 1.f; sy = 1.f; p1 = true; p2 = false; rel = false;
    if (!t) return;
    if (auto data = typeinfo_cast<CCNode*>(t->getUserObject("data"_spr))) {
        psDecode(data->getID(), sx, sy, p1, p2, rel);
    }
}

class PlayerScalePopup : public TriggerTemplatePopup {
protected:
    TargetGroupInput* m_scaleXInput = nullptr;
    TargetGroupInput* m_scaleYInput = nullptr;
    TargetGroupInput* m_durationInput = nullptr;
    CCMenuItemToggler* m_p1Toggle = nullptr;
    CCMenuItemToggler* m_p2Toggle = nullptr;
    CCMenuItemToggler* m_relativeToggle = nullptr;

    bool setup() override {
        float cx = m_popupWidth / 2.f;

        m_scaleXInput = TargetGroupInput::createFloat("Scale X", 0.1f, 0.05f, 10.f, 2);
        m_scaleXInput->setPosition({ cx - 100.f, m_popupHeight - 120.f });
        m_mainLayer->addChild(m_scaleXInput);

        m_scaleYInput = TargetGroupInput::createFloat("Scale Y", 0.1f, 0.05f, 10.f, 2);
        m_scaleYInput->setPosition({ cx + 100.f, m_popupHeight - 120.f });
        m_mainLayer->addChild(m_scaleYInput);

        m_durationInput = TargetGroupInput::createFloat("Duration", 0.05f, 0.f, 999.f, 2);
        m_durationInput->setPosition({ cx, m_popupHeight - 185.f });
        m_mainLayer->addChild(m_durationInput);

        float toggleY = m_popupHeight - 240.f;
        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(toggleMenu);

        auto makeToggle = [&](char const* text, float x, CCMenuItemToggler*& out) {
            out = CCMenuItemToggler::createWithStandardSprites(this, nullptr, 0.7f);
            out->setPosition({ x, toggleY });
            toggleMenu->addChild(out);
            auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");
            lbl->setScale(0.35f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({x + 16.f, toggleY});
            m_mainLayer->addChild(lbl);
        };

        makeToggle("P1", cx - 120.f, m_p1Toggle);
        makeToggle("P2", cx - 30.f, m_p2Toggle);
        makeToggle("Relative", cx + 60.f, m_relativeToggle);

        this->loadCustomValues();
        return true;
    }

    void forceCommitInputs() {
        auto commit = [](TargetGroupInput* node) {
            if (!node) return;
            if (auto input = node->getInput()) {
                if (input->m_textField)
                    input->onTextFieldDetachWithIME(input->m_textField);
            }
        };
        commit(m_scaleXInput);
        commit(m_scaleYInput);
        commit(m_durationInput);
    }

    void loadCustomValues() {
        if (!m_trigger) return;
        float sx, sy; bool p1, p2, rel;
        psRead(m_trigger, sx, sy, p1, p2, rel);
        if (m_scaleXInput) m_scaleXInput->setFloatValue(sx);
        if (m_scaleYInput) m_scaleYInput->setFloatValue(sy);
        if (m_durationInput) m_durationInput->setFloatValue(m_trigger->m_duration);
        if (m_p1Toggle) m_p1Toggle->toggle(p1);
        if (m_p2Toggle) m_p2Toggle->toggle(p2);
        if (m_relativeToggle) m_relativeToggle->toggle(rel);
    }

    void applyCustomValues() {
        this->forceCommitInputs();
        auto applyTo = [&](EffectGameObject* obj) {
            if (!obj) return;
            float sx = m_scaleXInput ? m_scaleXInput->getFloatValue() : 1.f;
            float sy = m_scaleYInput ? m_scaleYInput->getFloatValue() : 1.f;
            float dur = m_durationInput ? m_durationInput->getFloatValue() : 0.f;
            bool p1 = m_p1Toggle ? m_p1Toggle->isToggled() : true;
            bool p2 = m_p2Toggle ? m_p2Toggle->isToggled() : false;
            bool rel = m_relativeToggle ? m_relativeToggle->isToggled() : false;
            psWrite(obj, sx, sy, p1, p2, rel);
            obj->m_duration = dur;
        };
        applyTo(m_trigger);
        if (m_triggers) {
            for (auto obj : CCArrayExt<EffectGameObject*>(m_triggers))
                applyTo(obj);
        }
    }

public:
    static PlayerScalePopup* create(EffectGameObject* trigger, CCArray* triggers = nullptr) {
        auto ret = new PlayerScalePopup();
        ret->m_titleText = "Player Scale";
        ret->m_popupWidth = 440.f;
        ret->m_popupHeight = 310.f;
        if (ret && ret->init(trigger, triggers)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void onClose(CCObject* sender) override {
        this->applyCustomValues();
        TriggerTemplatePopup::onClose(sender);
    }
};

static void applyScale(PlayerObject* player, float scaleX, float scaleY, bool relative, float duration) {
    if (!player) return;

    float targetX = scaleX;
    float targetY = scaleY;
    if (relative) {
        float cur = player->m_vehicleSize > 0.f ? player->m_vehicleSize : 1.f;
        targetX *= cur;
        targetY *= cur;
    }
    targetX = std::clamp(targetX, 0.05f, 10.f);
    targetY = std::clamp(targetY, 0.05f, 10.f);

    float size = (targetX + targetY) * 0.5f;

    float modeSize = 30.f;
    if (player->m_isDart) modeSize = 10.f;
    else if (player->m_isSpider) modeSize = 27.f;

    player->m_width  = modeSize;
    player->m_height = modeSize;

    player->m_vehicleSize       = size;
    player->m_spriteWidthScale  = size;
    player->m_spriteHeightScale = size;

    player->m_isObjectRectDirty  = true;
    player->m_isOrientedBoxDirty = true;

    player->updatePlayerScale();

    (void)player->getObjectRect();
    (void)player->getObjectRect2(player->m_width, player->m_height);

    if (std::abs(targetX - targetY) > 0.001f) {
        auto node = player->m_mainLayer
            ? static_cast<CCNode*>(player->m_mainLayer)
            : static_cast<CCNode*>(player);
        if (duration <= 0.01f) {
            node->setScaleX(targetX);
            node->setScaleY(targetY);
        } else {
            node->runAction(CCScaleTo::create(duration, targetX, targetY));
        }
    } else if (duration > 0.01f) {
        player->runAction(CCScaleTo::create(duration, size));
    }
}

static void onTrigger(EffectGameObject* trigger, GJBaseGameLayer* game, int, std::vector<int> const*) {
    if (!game || !trigger) return;
    float sx, sy; bool p1, p2, rel;
    psRead(trigger, sx, sy, p1, p2, rel);
    float duration = trigger->m_duration;
    if (p1 && game->m_player1) applyScale(game->m_player1, sx, sy, rel, duration);
    if (p2 && game->m_player2) applyScale(game->m_player2, sx, sy, rel, duration);
}

static std::string onSaveString(std::string str, GameObject* object, GJBaseGameLayer* level) {
    if (!object) return std::string(str.c_str());
    if (!level)  return std::string(str.c_str());

    if (auto data = typeinfo_cast<CCNode*>(object->getUserObject("data"_spr))) {
        str += ",228,";
        str += ZipUtils::base64URLEncode(data->getID().c_str()).c_str();
    }
    return std::string(str.c_str());
}

static GameObject* onObjectFromVector(
    GameObject* object,
    std::vector<std::string>& p0,
    std::vector<void*>&,
    void*,
    bool
) {
    if (!object) return object;

    auto data = psEnsureData(object);

    if (data && p0.size() > static_cast<size_t>(PROP_DATA) && !p0[PROP_DATA].empty()) {
        auto decoded = ZipUtils::base64URLDecode(p0[PROP_DATA].c_str());
        if (!decoded.empty()) {
            data->setID(decoded.c_str());
        } else {
            data->setID(p0[PROP_DATA].c_str());
        }
    }

    return object;
}

static void onCustomSetup(GameObject* object) {
    if (!object) return;
    object->setAnchorPoint({ 0.5f, 0.5f });
    if (auto spr = object->m_colorSprite) {
        spr->setAnchorPoint({ 0.5f, 0.5f });
        spr->setPosition({ 0.f, 0.f });
    }
    
    psEnsureData(object);
}

static bool onEditObject(EditorUI* ui, GameObject* obj) {
    auto trigger = typeinfo_cast<EffectGameObject*>(obj);
    if (!trigger) return false;
    psEnsureData(trigger);
    if (auto p = PlayerScalePopup::create(trigger, ui->m_selectedObjects))
        p->show();
    return true;
}

static void registerPlayerScaleTrigger() {
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createTriggerConfig(
            UNIQ_ID("player-scale-trigger"),
            "playerScaleTrigger.png",
            onTrigger,
            nullptr
        )
        ->saveString(onSaveString)
        ->objectFromVector(onObjectFromVector)
        ->customSetup(onCustomSetup)
        ->onEditObject(onEditObject)
    );
}

$execute {
    registerPlayerScaleTrigger();
}