using namespace geode::prelude;

bool isReload = false;

void onRestart(CCObject* sender) {
    if (CCDirector::get()->getRunningScene()->getChildByID("restart-popup") || CCDirector::get()->getRunningScene()->getChildByID("reload-popup")) return;
    createQuickPopup(
        "Restart Game",
        "Are you sure you want to <cg>restart</c>?",
        "Cancel", "Yes",
        [](auto, bool btn2) {
            if (btn2) {
                game::restart(true);
            }
        }
    )->setID("restart-popup");
}

void onReload(CCObject* sender) {
    if (CCDirector::get()->getRunningScene()->getChildByID("restart-popup")
    || CCDirector::get()->getRunningScene()->getChildByID("reload-popup")) return;
    isReload = true;
    createQuickPopup(
        "Reload Textures",
        "Are you sure you want to <cj>reload textures</c>?",
        "Cancel", "Yes",
        [](auto, bool btn2) {
            if (btn2) {
                GameManager::get()->reloadAll(false, false, true);
            }
        }
    )->setID("reload-popup");
}

#include <Geode/utils/Keyboard.hpp>
#include <Geode/modify/MenuLayer.hpp>
class $modify(RestartMenuLayer, MenuLayer) {
	static CCScene* scene(bool isVideoOptionsOpen) {
		if (!isReload) return MenuLayer::scene(isVideoOptionsOpen);
		else {
			isReload = false;
			return MenuLayer::scene(false);
		}
	}

	bool init() {
		if (!MenuLayer::init()) return false;

		auto winSize = CCDirector::get()->getWinSize();

		if (!this->getChildByID("close-menu")) {
			auto menu = CCMenu::create();
			menu->setID("close-menu");
			menu->setContentSize(CCSize(200, 33.25));
			menu->ignoreAnchorPointForPosition(false);
			this->addChild(menu);
		}
			
		auto menu = this->getChildByID("close-menu");
		menu->setAnchorPoint(CCPoint(0, 1));
		menu->setPosition(winSize.width/300.f, winSize.height - winSize.width/300.f);
		auto layout = ColumnLayout::create()
					->setAxisReverse(true)
					->setAxisAlignment(AxisAlignment::End)
					->setGap(4.f)
					->setCrossAxisOverflow(false);

		menu->setLayout(layout);
		menu->setContentSize({menu->getContentSize().height, menu->getContentSize().width});

        if (!menu->getChildByID("close-button")) {
            auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
            closeSprite->setScale(0.7);

            auto closeButton = CCMenuItemSpriteExtra::create(
                closeSprite, 
                this, 
                menu_selector(MenuLayer::onQuit)
            );

            closeButton->setID("close-button");
            menu->addChild(closeButton);
        }

		if (!menu->getChildByID("restart-button")) {
			auto restartSprite = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
			restartSprite->setScale(0.7);

			auto restartButton = CCMenuItemSpriteExtra::create(
				restartSprite, 
				this, 
				menu_selector(RestartMenuLayer::onRestart)
            );

			restartButton->setID("restart-button");
			menu->addChild(restartButton);
		}

		if (!menu->getChildByID("reload-button")) {
			auto reloadSprite = CCSprite::createWithSpriteFrameName("GJ_replayBtn_001.png");
			reloadSprite->setScale(0.525);

			auto reloadButton = CCMenuItemSpriteExtra::create(
				reloadSprite, 
				this, 
				menu_selector(RestartMenuLayer::onReload)
            );

			reloadButton->setID("reload-button");
			menu->addChild(reloadButton);
		}

		menu->updateLayout();

		return true;
	}

    void onRestart(CCObject* sender) {
        ::onRestart(sender);
    }

    void onReload(CCObject* sender) {
        ::onReload(sender);
    }
};

$execute {
	listenForKeybindSettingPresses("restart-keybind", [](Keybind const& keybind, bool down, bool repeat, double dt) {
		if (Mod::get()->getSettingValue<bool>("keybind_anywhere") ? LevelEditorLayer::get() == nullptr : CCDirector::get()->getRunningScene()->getChildByID("MenuLayer") != nullptr) {
			if (down && !repeat) {
				onRestart(nullptr);
			}
		}
	});
	listenForKeybindSettingPresses("reload-keybind", [](Keybind const& keybind, bool down, bool repeat, double dt) {
		if (Mod::get()->getSettingValue<bool>("keybind_anywhere") ? LevelEditorLayer::get() == nullptr : CCDirector::get()->getRunningScene()->getChildByID("MenuLayer") != nullptr) {
			if (down && !repeat) {
				onReload(nullptr);
			}
		}
	});
}