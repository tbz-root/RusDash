using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>
class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCMenuItemSpriteExtra* m_hidePauseBtn = nullptr;
        bool m_hidden = false;
    };

    void customSetup() {
        PauseLayer::customSetup();

        auto hidePauseSpr = CCSprite::createWithSpriteFrameName("hidePauseBtn_001.png"_spr);

        hidePauseSpr->setScale(0.55f);

        m_fields->m_hidePauseBtn = CCMenuItemSpriteExtra::create(
            hidePauseSpr,
            this,
            menu_selector(MyPauseLayer::onHidePauseBtn)
        );

        m_fields->m_hidePauseBtn->setID("hide-pause-button");

        this->getChildByID("right-button-menu")->addChild(m_fields->m_hidePauseBtn);
        this->getChildByID("right-button-menu")->updateLayout();

        auto questsSpr = CCSprite::createWithSpriteFrameName("quickQuestsBtn_001.png"_spr);

        questsSpr->setScale(0.55f);

        auto questsBtn = CCMenuItemSpriteExtra::create(
            questsSpr,
            this,
            menu_selector(MyPauseLayer::onQuestsBtn)
        );

        questsBtn->setID("quests-button");

        this->getChildByID("left-button-menu")->addChild(questsBtn);
        this->getChildByID("left-button-menu")->updateLayout();
    }

    void onHidePauseBtn(CCObject*) {
        auto rightButtonMenu = this->getChildByID("right-button-menu");

        m_fields->m_hidden = !m_fields->m_hidden;

        auto children = this->getChildren();

        for (unsigned int i = 0; i < children->count(); i++) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));

            if (child != rightButtonMenu)
                child->setVisible(!m_fields->m_hidden);
        }

        auto menuChildren = rightButtonMenu->getChildren();

        for (unsigned int i = 0; i < menuChildren->count(); i++) {
            auto child = static_cast<CCNode*>(menuChildren->objectAtIndex(i));

            if (child != m_fields->m_hidePauseBtn)
                child->setVisible(!m_fields->m_hidden);
        }

        rightButtonMenu->setVisible(true);
        m_fields->m_hidePauseBtn->setVisible(true);
    }

    void onQuestsBtn(CCObject*) {
        ChallengesPage::create()->show();
    }
};