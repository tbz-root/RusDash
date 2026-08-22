#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <km7dev.server_api/include/ServerAPIEvents.hpp>

using namespace geode::prelude;

const std::string PRIMARY_URL = "https://rustps.online/database/";
const std::string FALLBACK_URL = "https://www.rustps.online/database/";

void applyServerEndpoint() {
    bool useMirror = Mod::get()->getSettingValue<bool>("use-mirror");
    std::string targetUrl = useMirror ? FALLBACK_URL : PRIMARY_URL;

    log::info("RusDash: Applying server -> {}", targetUrl);
    ServerAPIEvents::registerServer(targetUrl, -10);
    ServerAPIEvents::updateServer(0, targetUrl);
}

$on_mod(Loaded) {
    applyServerEndpoint();

    listenForSettingChanges<bool>("use-mirror", [](bool useMirror) {
        applyServerEndpoint();

        std::string serverName = useMirror ? "www.rustps.online (Mirror)" : "rustps.online (Primary)";
        FLAlertLayer::create(
            "RusDash",
            fmt::format("Current Server: <cg>{}</c>", serverName),
            "OK"
        )->show();
    });
}

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
