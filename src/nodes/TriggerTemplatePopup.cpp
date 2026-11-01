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

    if (!SetupTriggerPopup::init(trigger, triggers, m_popupWidth, m_popupHeight, 1, 0.f)) {
        return false;
    }

    m_mainLayer->setID("main-layer");

    m_background = static_cast<CCScale9Sprite*>(m_mainLayer->getChildren()->objectAtIndex(0));
    m_background->setID("background");

    m_title = CCLabelBMFont::create(m_titleText.c_str(), "goldFont.fnt");

    m_title->setID("title");
    m_title->setScale(0.7f);
    m_title->setAnchorPoint({0.f, 0.f});
    m_title->setPosition({m_popupWidth / 2.f, m_popupHeight - 22.f});

    m_mainLayer->addChild(m_title);

    this->createMultiTriggerItems({m_background->getContentSize().width / 2.f, 0.f}, {m_background->getContentSize().width / 2.f + 10.f, 0.f}, {m_background->getContentSize().width, 0.f});

    if (m_touchToggle) {
        m_touchToggle->setTarget(this, menu_selector(TriggerTemplatePopup::onTouchToggle));
    }

    if (m_spawnToggle) {
        m_spawnToggle->setTarget(this, menu_selector(TriggerTemplatePopup::onSpawnToggle));
    }

    auto okSpr = ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", 0.8f);

    m_okBtn = CCMenuItemSpriteExtra::create(
        okSpr,
        this,
        menu_selector(TriggerTemplatePopup::onOk)
    );
    
    m_okBtn->setID("ok-btn");
    m_okBtn->setPosition({0.f, 0.f});

    m_buttonMenu->addChild(m_okBtn);

    this->loadTriggerValues();

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

void TriggerTemplatePopup::updateMultiVisibility() {
    if (!m_multiTriggerContainer) return;

    bool show = false;
    if (m_touchToggle && m_touchToggle->isToggled()) show = true;
    if (m_spawnToggle && m_spawnToggle->isToggled()) show = true;

    for (auto child : CCArrayExt<CCNode*>(m_multiTriggerContainer)) {
        child->setVisible(show);
        if (!show) {
            if (auto tog = typeinfo_cast<CCMenuItemToggler*>(child)) {
                if (tog->isToggled()) {
                    tog->toggle(false);
                }
            }
        }
    }
}

void TriggerTemplatePopup::loadTriggerValues() {
    if (!m_trigger) return;

    if (m_touchToggle) {
        m_touchToggle->toggle(m_trigger->m_isTouchTriggered);
    }

    if (m_spawnToggle) {
        m_spawnToggle->toggle(m_trigger->m_isSpawnTriggered);
    }

    this->updateMultiVisibility();

    if (m_multiTriggerContainer && (m_trigger->m_isTouchTriggered || m_trigger->m_isSpawnTriggered)) {
        for (auto child : CCArrayExt<CCNode*>(m_multiTriggerContainer)) {
            if (auto tog = typeinfo_cast<CCMenuItemToggler*>(child)) {
                tog->toggle(m_trigger->m_isMultiTriggered);
            }
        }
    }
}

void TriggerTemplatePopup::applyTriggerValues() {
    auto applyTo = [&](EffectGameObject* obj) {
        if (!obj) return;

        if (m_touchToggle) {
            obj->m_isTouchTriggered = m_touchToggle->isToggled();
        }

        if (m_spawnToggle) {
            obj->m_isSpawnTriggered = m_spawnToggle->isToggled();
        }

        bool multi = false;
        if (m_multiTriggerContainer) {
            for (auto child : CCArrayExt<CCNode*>(m_multiTriggerContainer)) {
                if (auto tog = typeinfo_cast<CCMenuItemToggler*>(child)) {
                    multi = tog->isToggled();
                    break;
                }
            }
        }
        obj->m_isMultiTriggered = multi && (obj->m_isTouchTriggered || obj->m_isSpawnTriggered);
    };

    applyTo(m_trigger);

    if (m_triggers) {
        for (auto obj : CCArrayExt<EffectGameObject*>(m_triggers)) {
            applyTo(obj);
        }
    }
}

void TriggerTemplatePopup::onTouchToggle(CCObject* sender) {
    bool touchOn = m_touchToggle && !m_touchToggle->isToggled();
    if (touchOn && m_spawnToggle && m_spawnToggle->isToggled()) {
        m_spawnToggle->toggle(false);
    }
    
    if (m_multiTriggerContainer) {
        bool show = touchOn || (m_spawnToggle && m_spawnToggle->isToggled());
        for (auto child : CCArrayExt<CCNode*>(m_multiTriggerContainer)) {
            child->setVisible(show);
            if (!show) {
                if (auto tog = typeinfo_cast<CCMenuItemToggler*>(child)) {
                    if (tog->isToggled()) tog->toggle(false);
                }
            }
        }
    }
}

void TriggerTemplatePopup::onSpawnToggle(CCObject* sender) {
    bool spawnOn = m_spawnToggle && !m_spawnToggle->isToggled();
    if (spawnOn && m_touchToggle && m_touchToggle->isToggled()) {
        m_touchToggle->toggle(false);
    }

    if (m_multiTriggerContainer) {
        bool show = spawnOn || (m_touchToggle && m_touchToggle->isToggled());
        for (auto child : CCArrayExt<CCNode*>(m_multiTriggerContainer)) {
            child->setVisible(show);
            if (!show) {
                if (auto tog = typeinfo_cast<CCMenuItemToggler*>(child)) {
                    if (tog->isToggled()) tog->toggle(false);
                }
            }
        }
    }
}

void TriggerTemplatePopup::onOk(CCObject* sender) {
    this->applyTriggerValues();
    this->onClose(sender);
}

void TriggerTemplatePopup::determineStartValues() {
    SetupTriggerPopup::determineStartValues();
    this->loadTriggerValues();
}

void TriggerTemplatePopup::onClose(CCObject* sender) {
    this->applyTriggerValues();
    SetupTriggerPopup::onClose(sender);
}