using namespace geode::prelude;

#include "SettingsLayer.hpp"
#include <Geode/modify/OptionsLayer.hpp>
class $modify(OptionsLayer) {
	void onOptions(CCObject* sender) {
		SettingsLayer::create()->show();
	}
};

#include <Geode/modify/GJDropDownLayer.hpp>
class $modify(MyGJDropDownLayer, GJDropDownLayer){
    struct Fields {
        bool m_onScene = false;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("GJDropDownLayer::showLayer", INT_MIN);
    }

    void setOnScene(bool onScene) {
        m_fields->m_onScene = onScene;
    }

    void showLayer(bool p0) {
        
        GJDropDownLayer::showLayer(p0);
        if (m_fields->m_onScene){
            CCScene* currentScene = CCDirector::get()->getRunningScene();
            removeFromParentAndCleanup(false);
            currentScene->addChild(this);
        }
    }
};

void showOptions(){
	OptionsLayer* optionsLayer = OptionsLayer::create();

	static_cast<MyGJDropDownLayer*>(typeinfo_cast<GJDropDownLayer*>(optionsLayer))->setOnScene(true);

	int z = CCDirector::get()->getRunningScene()->getHighestChildZ();

	if (z == INT_MAX) {
		return;
	}

	optionsLayer->setZOrder(z + 1);
	optionsLayer->showLayer(false);
}

#include <Geode/modify/PauseLayer.hpp>
class $modify(MyPauseLayer, PauseLayer) {
    virtual void customSetup(){
		PauseLayer::customSetup();

		CCSprite* settings = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
		settings->setScale(0.775f);

		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(
			settings,
			this,
			menu_selector(MyPauseLayer::onOptions)
		);

		button->setID("main-options"_spr);

		if (CCNode* rightButtonMenu = getChildByID("right-button-menu")) {
			rightButtonMenu->addChild(button);
			rightButtonMenu->updateLayout();
		}

	}
	
	void onOptions(CCObject* obj){
        showOptions();
    }
};

#include <Geode/modify/LevelInfoLayer.hpp>
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
	static void onModify(auto& self) {
        (void) self.setHookPriorityBeforePost("LevelInfoLayer::init", "capeling.soggy-mod");
    }

    bool init(GJGameLevel* level, bool challenge) {
		if (!LevelInfoLayer::init(level, challenge)) {
			return false;
		}

		CCSprite* settings = CCSprite::createWithSpriteFrameName("settingsRope.png"_spr);

		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(settings,
			this,
			menu_selector(MyLevelInfoLayer::onOptions)
		);

		button->m_animationType = MenuAnimationType::Move;
		button->m_startPosition = settings->getPosition();
		button->m_offset = ccp(0, -7.f);
		button->m_duration = 0.2f;
		button->m_unselectedDuration = 0.2f;
		button->setID("main-options"_spr);
		button->setZOrder(-10);
		button->setPosition({-45, 0});

		if (CCNode* settingsMenu = getChildByID("garage-menu")) {
			settingsMenu->addChild(button);
		}

		return true;
	}

	void onOptions(CCObject* obj) {
        showOptions();
    }
};

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(MyEditorPauseLayer, EditorPauseLayer) {
	bool init(LevelEditorLayer* p0){
		if (!EditorPauseLayer::init(p0)) {
			return false;
		}

		CCSprite* settings = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
		settings->setScale(0.775f);

		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(settings, this, menu_selector(MyEditorPauseLayer::onMainOptions));
		button->setID("main-options"_spr);

		if (CCNode* guidelinesMenu = getChildByID("guidelines-menu")) {
			guidelinesMenu->addChild(button);
			guidelinesMenu->updateLayout();
		}

		return true;
	}

	void onMainOptions(CCObject* obj) {
        showOptions();
    }
};

#include <Geode/modify/EditLevelLayer.hpp>
class $modify(MyEditLevelLayer, EditLevelLayer) {
	bool init(GJGameLevel* p0) {

		if (!EditLevelLayer::init(p0)) {
			return false;
		}

		CCSprite* settings = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
		settings->setScale(1.25f);
	
		CCMenuItemSpriteExtra* button = CCMenuItemSpriteExtra::create(settings,
			this,
			menu_selector(MyEditLevelLayer::onMainOptions)
		);

		button->setID("main-options"_spr);

		if (CCNode* levelActionsMenu = getChildByID("level-actions-menu")) {
			levelActionsMenu->insertBefore(button, levelActionsMenu->getChildByID("help-button"));
			levelActionsMenu->updateLayout();
		}

		return true;
	}

	void onMainOptions(CCObject* obj){
        showOptions();
    }
};