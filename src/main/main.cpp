using namespace geode::prelude;

#include "../hpp/selectMenu.hpp"
#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/Geode.hpp>
#include <chrono>
#include <fstream>
#include <asp/fs.hpp>
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
        game::restart(true, false);
    }
};
static ExitTrigger s_exitTrigger;

void showRestartPopup() {
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto runningScene = CCDirector::sharedDirector()->getRunningScene();
    if (!runningScene) return;

    s_progressPopup = ProgressPopupLayer::create();
    s_progressPopup->setID("rusdash-restart-layer"_spr);

    auto bg = NineSlice::create("GJ_square01.png");
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

    auto bg = NineSlice::create("GJ_square01.png");
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
    auto modsDir = dirs::getModsDir();
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
            std::ofstream file(newModPath.string(), std::ios::binary);

            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                file.close();
                log::info("New version staged as .geode.new!");
                auto unzippedDir = dirs::getGeodeDir() / "unzipped" / Mod::get()->getID();
                auto cachedManifest = unzippedDir / "mod.json";
                if (cachedManifest.exists()) {
                    (void)asp::fs::remove(cachedManifest);
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
        #ifdef BRELEASE
        std::string body = "modVersion=" + modVersion;

        auto req = web::WebRequest();
        req.bodyString(body);
        req.timeout(std::chrono::seconds(15));

        std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getGJVersion22.php" : "https://www.rustps.online/database/getGJVersion22.php";

        s_updateCheckTask.spawn(
            req.post(url),
            [](web::WebResponse res) {
                if (!res.ok()) return;

                auto body = res.string();
                if (!body) return;

                std::string responseStr = body.unwrap();

                while (!responseStr.empty() && std::isspace(static_cast<unsigned char>(responseStr.back()))) responseStr.pop_back();
                while (!responseStr.empty() && std::isspace(static_cast<unsigned char>(responseStr.front()))) responseStr.erase(responseStr.begin());

                if (responseStr == "1" || responseStr.empty()) return;

                std::string downloadUrl = responseStr;

                Loader::get()->queueInMainThread([downloadUrl]() {
                    downloadLatestVersion(downloadUrl);
                });
            }
        );
        #endif

        if (!MenuLayer::init()) return false;
        
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto themesButton = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::createWithSpriteFrameName("tabsek.png"_spr, 0.85f, CircleBaseColor::Blue, CircleBaseSize::MediumAlt),
            this,
            menu_selector(MyMenuLayer::onOpenSettings)
        );

        themesButton->setID("themes-button"_spr);

        this->getChildByID("bottom-menu")->addChild(themesButton);
        this->getChildByID("bottom-menu")->updateLayout();

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
        rd.m_paths.push_back((Mod::get()->getResourcesDir() / "resources").string());
        CCFileUtils::get()->addTexturePack(rd);
    }

    void onOpenSettings(CCObject *) {
        int myAccountID = GJAccountManager::sharedState()->m_accountID;
        if (myAccountID > 0) {
            ThemePopup::create()->show();
    } else {
        openSettingsPopup(Mod::get());
    }
}
};

$on_mod(Loaded) {
    auto modsDir = dirs::getModsDir();
    auto currentModPath = modsDir / (Mod::get()->getID() + ".geode");
    auto stagedModPath = modsDir / (Mod::get()->getID() + ".geode.new");

    if (stagedModPath.exists()) {
        if (currentModPath.exists()) {
            auto rm = asp::fs::remove(currentModPath);
            if (rm.isErr()) log::error("Failed to remove old mod: {}", rm.unwrapErr());
        }
        auto ren = asp::fs::rename(stagedModPath, currentModPath);
        if (ren.isOk()) {
            log::info("RusDash successfully auto-replaced on startup!");
            game::restart(true, false);
        } else {
            log::error("Failed to apply update on startup: {}", ren.unwrapErr());
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
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects))
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

#include <Geode/modify/LoadingLayer.hpp>
class $modify(LoadingLayer) {
    bool init(bool refresh) {
        if (!LoadingLayer::init(refresh)) return false;

        this->getChildByID("gd-logo")->setScale(1.25f);

        return true;
    }
};
#ifdef BDEBUG
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/utils/Keyboard.hpp>
class $modify(LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI))
            return false;

        this->addEventListener(
            "",
            KeyboardInputEvent(KEY_K),
            [this](KeyboardInputData& data) {
                if (data.action == KeyboardInputData::Action::Press) {
                    clipboard::write(getLevelString());
                }

                return ListenerResult::Propagate;
            }
        );

        return true;
    }
};
#endif