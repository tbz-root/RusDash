#include "hpp/selectMenu.hpp"
#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/Geode.hpp>
#include <chrono>
#include <fstream>

using namespace geode::prelude;

static TaskHolder<web::WebResponse> s_updateCheckTask;
static TaskHolder<web::WebResponse> s_downloadTask;

class ProgressPopupLayer : public CCLayerColor {
public:
    static ProgressPopupLayer* create() {
        auto ret = new ProgressPopupLayer();
        if (ret && ret->initWithColor(ccc4(0, 0, 0, 150))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    virtual bool init() {
        if (!CCLayerColor::init()) return false;
        return true;
    }

    virtual void onEnter() {
        CCLayerColor::onEnter();
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
    }

    virtual void onExit() {
        CCDirector::sharedDirector()->getTouchDispatcher()->removeDelegate(this);
        CCLayerColor::onExit();
    }

    virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        return true;
    }
};

static ProgressPopupLayer* s_progressPopup = nullptr;
static CCLabelBMFont* s_progressLabel = nullptr;
static CCLabelBMFont* s_titleLabel = nullptr;

class ExitTrigger : public CCObject {
public:
    void onExit(CCObject*) {
        geode::utils::game::restart(true);
    }
};
static ExitTrigger s_exitTrigger;

void showRestartPopup() {
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto runningScene = CCDirector::sharedDirector()->getRunningScene();
    if (!runningScene) return;

    s_progressPopup = ProgressPopupLayer::create();
    s_progressPopup->setID("rusdash-restart-layer"_spr);

    auto bg = CCScale9Sprite::create("GJ_square01.png");
    bg->setContentSize({ 280.0f, 140.0f });
    bg->setPosition(winSize / 2);
    s_progressPopup->addChild(bg);

    auto titleLabel = CCLabelBMFont::create("Update Complete", "goldFont.fnt");
    titleLabel->setScale(0.75f);
    titleLabel->setPosition(winSize.width / 2, winSize.height / 2 + 35);
    s_progressPopup->addChild(titleLabel);

    auto descLabel = CCLabelBMFont::create("RusDash has been successfully updated!", "chatFont.fnt");
    descLabel->setScale(0.75f);
    descLabel->setPosition(winSize.width / 2, winSize.height / 2 + 10);
    s_progressPopup->addChild(descLabel);

    auto btnSprite = ButtonSprite::create("Restart", "goldFont.fnt", "GJ_button_01.png");
    auto btnItem = CCMenuItemSpriteExtra::create(
        btnSprite,
        s_progressPopup,
        menu_selector(ExitTrigger::onExit)
    );
    btnItem->setTarget(&s_exitTrigger, menu_selector(ExitTrigger::onExit));

    auto menu = CCMenu::create();
    menu->addChild(btnItem);
    menu->setPosition(winSize.width / 2, winSize.height / 2 - 30);
    s_progressPopup->addChild(menu);

    menu->setTouchPriority(-501);

    runningScene->addChild(s_progressPopup, 1001);
}

void createProgressPopup() {
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto runningScene = CCDirector::sharedDirector()->getRunningScene();
    if (!runningScene) return;

    s_progressPopup = ProgressPopupLayer::create();
    s_progressPopup->setID("rusdash-progress-layer"_spr);

    auto bg = CCScale9Sprite::create("GJ_square01.png");
    bg->setContentSize({ 280.0f, 140.0f });
    bg->setPosition(winSize / 2);
    s_progressPopup->addChild(bg);

    s_titleLabel = CCLabelBMFont::create("RusDash Update Found", "goldFont.fnt");
    s_titleLabel->setScale(0.75f);
    s_titleLabel->setPosition(winSize.width / 2, winSize.height / 2 + 35);
    s_progressPopup->addChild(s_titleLabel);

    s_progressLabel = CCLabelBMFont::create("0%", "bigFont.fnt");
    s_progressLabel->setScale(0.8f);
    s_progressLabel->setPosition(winSize.width / 2, winSize.height / 2 - 15);
    s_progressPopup->addChild(s_progressLabel);

    runningScene->addChild(s_progressPopup, 1000);
}

void downloadLatestVersion(std::string const& downloadUrl) {
    auto modsDir = geode::dirs::getModsDir();
    auto newModPath = modsDir / (Mod::get()->getID() + ".geode.new");

    Loader::get()->queueInMainThread([]() {
        createProgressPopup();
    });

    auto req = web::WebRequest();
    req.header("User-Agent", "RusDash-Updater");

    req.onProgress([](web::WebProgress const& progress) {
        if (!s_progressLabel) return;

        double downloaded = static_cast<double>(progress.downloaded());

        Loader::get()->queueInMainThread([downloaded]() {
            if (!s_progressLabel) return;
            
            double downloadedMB = downloaded / (1024.0 * 1024.0);
            s_progressLabel->setString(fmt::format("{:.1f} MB downloaded", downloadedMB).c_str());
        });
    });

    s_downloadTask.spawn(
        req.get(downloadUrl),
        [newModPath](web::WebResponse res) {
            if (!res.ok() || res.code() != 200) {
                log::error("Dropbox download failed. HTTP {}", res.code());
                Loader::get()->queueInMainThread([]() {
                    s_progressLabel = nullptr;
                    s_titleLabel = nullptr;
                    if (s_progressPopup) {
                        s_progressPopup->removeFromParentAndCleanup(true);
                        s_progressPopup = nullptr;
                    }
                    Notification::create("Download failed!", NotificationIcon::Error, 3.0f)->show();
                });
                return;
            }

            auto bytes = res.data();
            std::ofstream file(newModPath, std::ios::binary);

            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                file.close();
                log::info("New version staged as .geode.new!");
                auto unzippedDir = geode::dirs::getGeodeDir() / "unzipped" / Mod::get()->getID();
                auto cachedManifest = unzippedDir / "mod.json";
                if (std::filesystem::exists(cachedManifest)) {
                    try {
                        std::filesystem::remove(cachedManifest);
                    } catch(...) {}
                }

                Loader::get()->queueInMainThread([]() {
                    s_progressLabel = nullptr;
                    s_titleLabel = nullptr;
                    if (s_progressPopup) {
                        s_progressPopup->removeFromParentAndCleanup(true);
                        s_progressPopup = nullptr;
                    }
                    showRestartPopup();
                });
            } else {
                Loader::get()->queueInMainThread([]() {
                    s_progressLabel = nullptr;
                    s_titleLabel = nullptr;
                    if (s_progressPopup) {
                        s_progressPopup->removeFromParentAndCleanup(true);
                        s_progressPopup = nullptr;
                    }
                    Notification::create("Installation failed!", NotificationIcon::Error, 3.0f)->show();
                });
            }
        }
    );
}

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        std::string modVersion = Mod::get()->getVersion().toVString();
        matjson::Value json = matjson::makeObject({{"modVersion", modVersion}});

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        req.bodyJSON(json);
        req.timeout(std::chrono::seconds(15));

        std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getUpdates.php" : "https://www.rustps.online/database/getUpdates.php";

        s_updateCheckTask.spawn(
            req.post(url),
            [](web::WebResponse res) {
                if (!res.ok()) return;

                auto body = res.string();
                if (!body) return;

                std::string responseStr = body.unwrap();

                while (!responseStr.empty() && std::isspace(static_cast<unsigned char>(responseStr.back()))) responseStr.pop_back();
                while (!responseStr.empty() && std::isspace(static_cast<unsigned char>(responseStr.front()))) responseStr.erase(responseStr.begin());

                if (responseStr == "true" || responseStr.empty()) return;

                std::string downloadUrl = responseStr;

                Loader::get()->queueInMainThread([downloadUrl]() {
                    downloadLatestVersion(downloadUrl);
                });
            }
        );

        if (!MenuLayer::init()) return false;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
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

        auto versionLabel = CCLabelBMFont::create(
            modVersion.c_str(),
            "bigFont.fnt"
        );

        versionLabel->setScale(0.7f);
        versionLabel->setID("version"_spr);
        versionLabel->setPosition({winSize.width / 2.f, winSize.height - versionLabel->getContentSize().height / 2.f - 2.f});
        versionLabel->setOpacity(100);
        
        this->addChild(versionLabel);

        return true;
    }

    static auto onModify(auto) {
        CCTexturePack rd;
        rd.m_id = std::string(Mod::get()->getID());
        rd.m_paths.push_back(string::pathToString(Mod::get()->getResourcesDir() / "resources"));
        CCFileUtils::get()->addTexturePack(rd);
    }

    void onOpenSettings(CCObject *) {
        int myAccountID = GJAccountManager::sharedState()->m_accountID;
        if (myAccountID > 0) {
        if (auto popup = ThemePopup::create()) {
            popup->show();
        }
    } else {
        openSettingsPopup(Mod::get());
    }
}
};

$on_mod(Loaded) {
    auto modsDir = geode::dirs::getModsDir();
    auto currentModPath = modsDir / (Mod::get()->getID() + ".geode");
    auto stagedModPath = modsDir / (Mod::get()->getID() + ".geode.new");

    if (std::filesystem::exists(stagedModPath)) {
        try {
            if (std::filesystem::exists(currentModPath)) {
                std::filesystem::remove(currentModPath);
            }
            std::filesystem::rename(stagedModPath, currentModPath);
            log::info("RusDash successfully auto-replaced on startup!");
            geode::utils::game::restart(true);
        } catch (std::exception const& e) {
            log::error("Failed to apply update on startup: {}", e.what());
        }
    }
}

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
            if (auto ui = this->getChildByID("UILayer")) {
                if (auto pauseMenu = ui->getChildByID("pause-button-menu")) {
                    pauseMenu->setVisible(false);
                }
            }
        #endif

        return true;
    }
};