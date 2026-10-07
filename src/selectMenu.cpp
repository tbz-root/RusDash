using namespace geode::prelude;

#include "hpp/selectMenu.hpp"
#include <sstream>
#include <vector>
#include <string>
bool ThemePopup::init() {
    if (!Popup::init(280.f, 220.f))
        return false;

    this->setTitle("Select Theme");

    m_buttonsMenu = CCMenu::create();
    m_buttonsMenu->setContentSize(m_mainLayer->getContentSize());
    m_buttonsMenu->setPosition({0.f, 0.f});
    m_buttonsMenu->setAnchorPoint({0.f, 0.f});
    m_mainLayer->addChild(m_buttonsMenu);

    m_spinner = LoadingSpinner::create(40.f);
    m_spinner->setPosition(m_mainLayer->getContentSize() / 2);
    m_mainLayer->addChild(m_spinner);

    fetchAvailableThemes();
    return true;
}

void ThemePopup::fetchAvailableThemes() {
    int accountID = GJAccountManager::get()->m_accountID;
    std::string gjp = GJAccountManager::get()->m_GJP2;
    std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getGJUserThemes22.php" : "https://www.rustps.online/database/getGJUserThemes22.php";

    std::string body = "accountID=" + std::to_string(accountID) + "&gjp2=" + gjp;

    web::WebRequest req;
    req.bodyString(body);

    m_fetchTask.spawn(
        req.post(url),
        [this](web::WebResponse response) {
            if (m_spinner) {
                m_spinner->removeFromParent();
                m_spinner = nullptr;
            }

            if (response.code() != 200) {
                FLAlertLayer::create("Error", "Failed to load themes", "OK")->show();
                return;
            }

            auto json = response.json().unwrapOr(matjson::Value());
            std::string themesStr;
            if (json.isString()) {
                themesStr = json.asString().unwrapOr("");
            } else if (json.contains("themes")) {
                themesStr = json["themes"].asString().unwrapOr("");
            }

            std::vector<std::string> themes;
            std::stringstream ss(themesStr);
            std::string item;
            while (std::getline(ss, item, ',')) {
                if (!item.empty()) {
                    themes.push_back(item);
                }
            }

            if (themes.empty()) {
                auto label = CCLabelBMFont::create("No themes available", "bigFont.fnt");
                label->setScale(0.5f);
                label->setPosition(m_mainLayer->getContentSize() / 2);
                m_mainLayer->addChild(label);
                return;
            }

            m_buttonsMenu->removeAllChildrenWithCleanup(true);

            float startY = m_mainLayer->getContentSize().height / 2 + 30.f;
            for (auto const& theme : themes) {
                auto btnSprite = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
                if (!btnSprite) continue;
                btnSprite->setContentSize({200.f, 28.f});
                btnSprite->setColor({0, 0, 0});
                btnSprite->setOpacity(120);

                auto label = CCLabelBMFont::create(theme.c_str(), "bigFont.fnt");
                label->setScale(0.4f);
                label->setPosition(btnSprite->getContentSize() / 2);
                btnSprite->addChild(label);

                auto btn = CCMenuItemSpriteExtra::create(
                    btnSprite, this, menu_selector(ThemePopup::onSelectTheme)
                );
                btn->setID(theme);
                btn->setPosition({m_mainLayer->getContentSize().width / 2, startY});
                m_buttonsMenu->addChild(btn);

                startY -= 35.f;
            }
        }
    );
}

void ThemePopup::onSelectTheme(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;

    std::string themeID = btn->getID();
    int accountID = GJAccountManager::get()->m_accountID;
    std::string gjp = GJAccountManager::get()->m_GJP2;

    std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/updateGJUserTheme22.php" : "https://www.rustps.online/database/updateGJUserTheme22.php";

    std::string body = "accountID=" + std::to_string(accountID) + "&themeID=" + themeID + "&gjp2=" + gjp;

    web::WebRequest req;
    req.bodyString(body);

    m_selectTask.spawn(
        req.post(url),
        [this](web::WebResponse response) {
            if (response.code() == 200) {
                std::string responseStr = response.string().unwrapOr("-1");

                if (responseStr == "1") {
                    FLAlertLayer::create("Success", "Theme successfully applied!", "OK")->show();
                    this->onClose(nullptr);
                } else {
                    FLAlertLayer::create("Error", "Server rejected theme change", "OK")->show();
                }
            } else {
                FLAlertLayer::create("Error", "Failed to connect to server", "OK")->show();
            }
        }
    );
}

void ThemePopup::onClose(CCObject* sender) {
    Popup::onClose(sender);
}

ThemePopup* ThemePopup::create() {
    auto ret = new ThemePopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}