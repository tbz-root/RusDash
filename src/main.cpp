#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <chrono>

using namespace geode::prelude;

static TaskHolder<web::WebResponse> s_themeTask;

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer)
{
    bool init()
    {

        std::string myVersion = "v1.0.2";

        matjson::Value json = matjson::makeObject({
            {"modVersion", myVersion},
        });

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