using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>
class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCMenuItemToggler* m_hidePauseBtn = nullptr;
        bool m_hidden = false;
    };

    void customSetup() {
        PauseLayer::customSetup();

        auto offSpr = CCSprite::createWithSpriteFrameName("hidePauseBtn_001.png"_spr);

        offSpr->setScale(0.55f);

        auto onSpr = CCSprite::createWithSpriteFrameName("hidePauseBtn_002.png"_spr);

        onSpr->setScale(0.55f);

        m_fields->m_hidePauseBtn = CCMenuItemToggler::create(
            offSpr,
            onSpr,
            this,
            menu_selector(MyPauseLayer::onHidePauseBtn)
        );

        m_fields->m_hidePauseBtn->setID("hide-pause-button");

        this->getChildByID("right-button-menu")->addChild(m_fields->m_hidePauseBtn);
        this->getChildByID("right-button-menu")->updateLayout();

        auto questsSpr = CCSprite::createWithSpriteFrameName("quickQuestsBtn_001.png"_spr);

        questsSpr->setScale(0.65f);

        auto questsBtn = CCMenuItemSpriteExtra::create(
            questsSpr,
            this,
            menu_selector(MyPauseLayer::onQuestsBtn)
        );

        questsBtn->setID("quests-button");

        this->getChildByID("left-button-menu")->addChild(questsBtn);

        auto level = PlayLayer::get()->m_level;
        if (level && level->m_levelType != GJLevelType::Main) {
            auto chatSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");

            chatSpr->setScale(0.65f);

            auto chatBtn = CCMenuItemSpriteExtra::create(
                chatSpr,
                this,
                menu_selector(MyPauseLayer::onCommentsBtn)
            );

            chatBtn->setID("comments-button");
            
            this->getChildByID("left-button-menu")->addChild(chatBtn);
        }

        this->getChildByID("left-button-menu")->updateLayout();
    }

    void onHidePauseBtn(CCObject*) {
        auto rightButtonMenu = this->getChildByID("right-button-menu");

        m_fields->m_hidden = !m_fields->m_hidePauseBtn->isToggled();

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

        this->setOpacity(m_fields->m_hidden ? 0 : 75);
    }

    void onQuestsBtn(CCObject*) {
        ChallengesPage::create()->show();
    }

    void onCommentsBtn(CCObject*) {
        InfoLayer::create(PlayLayer::get()->m_level, nullptr, nullptr)->show();
    }
};