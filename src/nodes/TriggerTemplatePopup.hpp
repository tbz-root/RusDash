#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SetupTriggerPopup.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/ButtonSprite.hpp>

using namespace geode::prelude;

class TriggerTemplatePopup : public SetupTriggerPopup {
protected:
    CCLabelBMFont* m_title = nullptr;
    CCMenuItemSpriteExtra* m_okBtn = nullptr;
    CCScale9Sprite* m_background = nullptr;
    EffectGameObject* m_trigger = nullptr;
    CCArray* m_triggers = nullptr;
    std::string m_titleText = "Trigger";

    float m_popupWidth  = 440.f;
    float m_popupHeight = 310.f;

    bool init(EffectGameObject* trigger, CCArray* triggers);
    virtual bool setup() { return true; }

    void onOk(CCObject* sender);
    void onTouchToggle(CCObject* sender);
    void onSpawnToggle(CCObject* sender);

    void loadTriggerValues();
    void applyTriggerValues();
    void updateMultiVisibility();

public:
    static TriggerTemplatePopup* create(EffectGameObject* trigger, CCArray* triggers = nullptr);

    void setTitle(std::string const& text);

    CCLabelBMFont* getTitleLabel() const { return m_title; }
    EffectGameObject* getTrigger() const { return m_trigger; }
    CCArray* getTriggers() const { return m_triggers; }

    virtual void determineStartValues() override;
    virtual void onClose(CCObject* sender) override;
};