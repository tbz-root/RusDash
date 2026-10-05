#ifdef BDEBUG
using namespace geode::prelude;

#include <Geode/modify/LevelInfoLayer.hpp>
#include "DevRateStarsPopup.hpp"
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel *level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge))
            return false;

        if (auto *leftMenu = static_cast<CCMenu *>(this->getChildByID("left-side-menu"))) {
            auto *devRateButton = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_starBtnMod_001.png"),
                this,
                menu_selector(MyLevelInfoLayer::onDevRateButton)
            );
            devRateButton->setColor({ 74, 242, 250 });
            devRateButton->setID("dev-rate-button"_spr);

            leftMenu->addChild(devRateButton);
            leftMenu->updateLayout();
        }
        
        return true;
    }

    void onDevRateButton(CCObject *) {
        DevRateStarsPopup::create(m_level->m_levelID)->show();
    }
};
#endif