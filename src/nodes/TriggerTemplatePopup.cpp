#include "TriggerTemplatePopup.hpp"

TriggerTemplatePopup* TriggerTemplatePopup::create(EffectGameObject* trigger, CCArray* triggers) {
    auto ret = new TriggerTemplatePopup();
    if (ret && ret->init(trigger, triggers)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool TriggerTemplatePopup::init(EffectGameObject* trigger, CCArray* triggers) {
    m_trigger = trigger;
    m_triggers = triggers;

    if (!SetupTriggerPopup::init(trigger, triggers, m_popupWidth, m_popupHeight, 0)) {
        return false;
    }

    auto bg = m_mainLayer->getChildByType<CCScale9Sprite>(0);

    bg->setContentSize({ m_popupWidth, m_popupHeight });
    bg->setTexture("GJ_square01.png");

    m_title = CCLabelBMFont::create(m_titleText.c_str(), "goldFont.fnt");
    m_title->setID("title");
    m_title->setScale(0.7f);
    m_title->setPosition({ m_popupWidth / 2.f, m_popupHeight - 25.f });
    m_mainLayer->addChild(m_title);

    auto okSpr = ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", 0.8f);

    m_okBtn = CCMenuItemSpriteExtra::create(
        okSpr,
        this,
        menu_selector(TriggerTemplatePopup::onOk)
    );

    m_okBtn->setID("ok-btn");
    m_okBtn->setPosition({ 0.f, 0.f });

    m_buttonMenu->addChild(m_okBtn);

    const float toggleX = m_popupWidth / 2.f - 55.f;
    const float baseY = -m_popupHeight / 2.f + 70.f;
    const float spacing = 38.f;

    auto makeToggle = [&](const char* text, float y, SEL_MenuHandler selector, const char* nodeID, const char* labelID)-> std::pair<CCMenuItemToggler*, CCLabelBMFont*> {
        auto off = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        auto on  = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        auto tog = CCMenuItemToggler::create(off, on, this, selector);

        tog->setID(nodeID);
        tog->setPosition({ toggleX, y });
        tog->setScale(0.8f);

        m_buttonMenu->addChild(tog);

        auto lbl = CCLabelBMFont::create(text, "bigFont.fnt");

        lbl->setID(labelID);
        lbl->setScale(0.35f);
        lbl->setAnchorPoint({ 1.f, 0.5f });
        lbl->setPosition({ toggleX - 18.f, y });

        m_buttonMenu->addChild(lbl);

        return {tog, lbl};
    };

    auto [touchTog, touchLbl] = makeToggle(
        "Touch\nTrigger",
        baseY + spacing,
        menu_selector(TriggerTemplatePopup::onTouchToggle),
        "touch-trigger-toggle",
        "touch-trigger-label"
    );

    m_touchToggle = touchTog;
    m_touchLabel  = touchLbl;

    auto [spawnTog, spawnLbl] = makeToggle(
        "Spawn\nTrigger",
        baseY,
        menu_selector(TriggerTemplatePopup::onSpawnToggle),
        "spawn-trigger-toggle",
        "spawn-trigger-label"
    );

    m_spawnToggle = spawnTog;
    m_spawnLabel  = spawnLbl;

    auto [multiTog, multiLbl] = makeToggle(
        "Multi\nTrigger",
        baseY - spacing,
        menu_selector(TriggerTemplatePopup::onMultiToggle),
        "multi-trigger-toggle",
        "multi-trigger-label"
    );

    m_multiToggle = multiTog;
    m_multiLabel  = multiLbl;

    m_multiToggle->setVisible(false);
    m_multiLabel->setVisible(false);

    this->loadTriggerValues();
    this->updateMultiVisibility();

    if (!this->setup()) {
        return false;
    }

    return true;
}

void TriggerTemplatePopup::setTitle(std::string const& text) {
    m_titleText = text;
    if (m_title) {
        m_title->setString(text.c_str());
    }
}

void TriggerTemplatePopup::loadTriggerValues() {
    if (!m_trigger) return;

    bool touch = m_trigger->m_isTouchTriggered;
    bool spawn = m_trigger->m_isSpawnTriggered;
    bool multi = false;

    if (m_trigger->m_isMultiTriggered) {
        multi = m_trigger->m_isMultiTriggered;
    }

    if (m_touchToggle) m_touchToggle->toggle(touch);
    if (m_spawnToggle) m_spawnToggle->toggle(spawn);
    if (m_multiToggle) m_multiToggle->toggle(multi);
}

void TriggerTemplatePopup::applyTriggerValues() {
    auto applyTo = [&](EffectGameObject* obj) {
        if (!obj) return;
        obj->m_isTouchTriggered = m_touchToggle && m_touchToggle->isToggled();
        obj->m_isSpawnTriggered = m_spawnToggle && m_spawnToggle->isToggled();
        if (obj->m_isMultiTriggered || true) {
            obj->m_isMultiTriggered = m_multiToggle && m_multiToggle->isToggled()
                                      && (obj->m_isTouchTriggered || obj->m_isSpawnTriggered);
        }
    };

    applyTo(m_trigger);

    if (m_triggers) {
        for (auto obj : CCArrayExt<EffectGameObject*>(m_triggers)) {
            applyTo(obj);
        }
    }
}

void TriggerTemplatePopup::updateMultiVisibility() {
    bool show = false;
    if (m_touchToggle && m_touchToggle->isToggled()) show = true;
    if (m_spawnToggle && m_spawnToggle->isToggled()) show = true;

    if (m_multiToggle) m_multiToggle->setVisible(show);
    if (m_multiLabel)  m_multiLabel->setVisible(show);

    if (!show && m_multiToggle && m_multiToggle->isToggled()) {
        m_multiToggle->toggle(false);
    }
}

void TriggerTemplatePopup::onTouchToggle(CCObject*) {
    if (m_touchToggle && m_touchToggle->isToggled()) {
        if (m_spawnToggle && m_spawnToggle->isToggled()) {
            m_spawnToggle->toggle(false);
        }
    }
    this->updateMultiVisibility();
}

void TriggerTemplatePopup::onSpawnToggle(CCObject*) {
    if (m_spawnToggle && m_spawnToggle->isToggled()) {
        if (m_touchToggle && m_touchToggle->isToggled()) {
            m_touchToggle->toggle(false);
        }
    }
    this->updateMultiVisibility();
}

void TriggerTemplatePopup::onMultiToggle(CCObject*) {}

void TriggerTemplatePopup::onOk(CCObject* sender) {
    this->applyTriggerValues();
    this->onClose(sender);
}

void TriggerTemplatePopup::determineStartValues() {
    SetupTriggerPopup::determineStartValues();
    this->loadTriggerValues();
    this->updateMultiVisibility();
}

void TriggerTemplatePopup::onClose(CCObject* sender) {
    this->applyTriggerValues();
    SetupTriggerPopup::onClose(sender);
}