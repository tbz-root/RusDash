#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/SetupTriggerPopup.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include "TargetGroupInput.hpp"

using namespace geode::prelude;

class TriggerTemplatePopup : public SetupTriggerPopup {
protected:
    CCLabelBMFont* m_title = nullptr;
    CCMenuItemSpriteExtra* m_okBtn = nullptr;

    CCMenuItemToggler* m_touchToggle = nullptr;
    CCMenuItemToggler* m_spawnToggle = nullptr;
    CCMenuItemToggler* m_multiToggle = nullptr;
    CCLabelBMFont* m_touchLabel = nullptr;
    CCLabelBMFont* m_spawnLabel = nullptr;
    CCLabelBMFont* m_multiLabel = nullptr;

    EffectGameObject* m_trigger = nullptr;
    CCArray* m_triggers = nullptr;

    std::string m_titleText = "Trigger";

    float m_popupWidth = 300.f;
    float m_popupHeight = 280.f;

    bool init(EffectGameObject* trigger, CCArray* triggers);
    virtual bool setup() { return true; }

    void onOk(CCObject* sender);
    void onTouchToggle(CCObject* sender);
    void onSpawnToggle(CCObject* sender);
    void onMultiToggle(CCObject* sender);

    void updateMultiVisibility();

    void applyTriggerValues();

    void loadTriggerValues();

public:
    static TriggerTemplatePopup* create(EffectGameObject* trigger, CCArray* triggers = nullptr);

    void setTitle(std::string const& text);

    CCLabelBMFont* getTitleLabel() const { return m_title; }
    CCMenuItemToggler* getTouchToggle() const { return m_touchToggle; }
    CCMenuItemToggler* getSpawnToggle() const { return m_spawnToggle; }
    CCMenuItemToggler* getMultiToggle() const { return m_multiToggle; }

    EffectGameObject* getTrigger() const { return m_trigger; }
    CCArray* getTriggers() const { return m_triggers; }

    virtual void determineStartValues() override;
    virtual void onClose(CCObject* sender) override;
};