using namespace geode::prelude;

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer)
{
    bool init()
    {
        if (!MenuLayer::init())
        {
            return false;
        }

        auto holyShit = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::createWithSpriteFrameName("tabsek.png"_spr, 0.85f, CircleBaseColor::Blue, CircleBaseSize::MediumAlt),
            this,
            menu_selector(MyMenuLayer::onOpenSettings)
        );

        auto menu = this->getChildByID("bottom-menu");
        if (menu)
        {
            menu->addChild(holyShit);
            holyShit->setID("rusdash-holy-shit-btn"_spr);
            menu->updateLayout();
        }

        return true;
    }

    void onOpenSettings(CCObject *)
    {
        openSettingsPopup(Mod::get());
    }
};
