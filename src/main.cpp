#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/Geode.hpp>
#include <chrono>

using namespace geode::prelude;

static TaskHolder<web::WebResponse> s_themeTask;

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        std::string modVersion = "v1.0.2";

        matjson::Value json = matjson::makeObject({{"modVersion", modVersion}});

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        req.bodyJSON(json);
        req.timeout(std::chrono::seconds(15));

        std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://www.rustps.online/database" : "https://rustps.online/database";
        std::string url = baseUrl + "/getUpdates.php";

        s_themeTask.spawn(
            req.post(url),
            [](web::WebResponse res) {
                bool isSuccess = false;

                if (res.ok()) {
                    std::string responseStr = res.string().unwrapOr("false");
                    
                    while (!responseStr.empty() && (responseStr.back() == '\n' || responseStr.back() == '\r' || responseStr.back() == ' ')) {
                        responseStr.pop_back();
                    }
                    
                    if (responseStr == "true") {
                        isSuccess = true;
                    }
                }

                if (!isSuccess) {
                    Loader::get()->queueInMainThread([]() {
                        FLAlertLayer::create(
                            "Update Required",                       
                            "Please, update RusDash Geode Mod or delete it. The game will close now!", 
                            "OK"                                    
                        )->show();
                    });

                    std::terminate();
                }
            }
        );

        if (!MenuLayer::init())
            return false;

        auto holyShit = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::createWithSpriteFrameName("tabsek.png"_spr, 0.85f, CircleBaseColor::Blue, CircleBaseSize::MediumAlt),
            this,
            menu_selector(MyMenuLayer::onOpenSettings)
        );

        auto menu = this->getChildByID("bottom-menu");
        if (menu) {
            menu->addChild(holyShit);
            holyShit->setID("rusdash-holy-shit-btn"_spr);
            menu->updateLayout();
        }

        return true;
    }

	static auto onModify(auto) {
		CCTexturePack rd;

		rd.m_id = std::string(Mod::get()->getID());
		rd.m_paths.push_back(string::pathToString(Mod::get()->getResourcesDir() / "resources"));

        // F:\dev\IuseRusDashBtw\resources\tabz.rusdash\resources
        rd.m_paths.push_back(R"(F:\dev\IuseRusDashBtw\resources\tabz.rusdash\resources)");

		CCFileUtils::get()->addTexturePack(rd);
    }

    void onOpenSettings(CCObject *) {
        openSettingsPopup(Mod::get());
    }
};

#include <Geode/modify/GameManager.hpp>
class $modify(GameManager) {
	bool getGameVariable(char const* tag) {
		if (std::string(tag) == "0024") return "Show Cursor In-Game";
		if (std::string(tag) == "0128") return not "Lock Cursor In-Game";

		return GameManager::getGameVariable(tag);
	};
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        #ifdef GEODE_IS_DESKTOP
            this->getChildByID("UILayer")->getChildByID("pause-button-menu")->setVisible(false);
        #endif

        return true;
    }
};