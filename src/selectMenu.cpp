#include "selectMenu.hpp"
#include <Geode/utils/web.hpp>
#include <sstream>
#include <vector>
#include <string>

using namespace geode::prelude;

bool ThemePopup::init() {
    if (!FLAlertLayer::init(150)) return false;

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto bg = CCScale9Sprite::create("GJ_square01.png", { 0, 0, 80, 80 });
    bg->setContentSize({ 280.f, 220.f });
    bg->setPosition(winSize / 2);
    m_mainLayer->addChild(bg);

    auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    auto closeBtn = CCMenuItemSpriteExtra::create(
        closeSprite, this, menu_selector(ThemePopup::onClose)
    );
    
    auto closeMenu = CCMenu::create(closeBtn, nullptr);
    closeMenu->setPosition({ winSize.width / 2 - 125.f, winSize.height / 2 + 95.f });
    closeMenu->setTouchPriority(-495);
    m_mainLayer->addChild(closeMenu);

    auto titleLabel = CCLabelBMFont::create("Select Theme", "goldFont.fnt");
    titleLabel->setScale(0.8f);
    titleLabel->setPosition({ winSize.width / 2, winSize.height / 2 + 85.f });
    m_mainLayer->addChild(titleLabel);

    m_buttonsMenu = CCMenu::create();
    m_buttonsMenu->setPosition(winSize / 2);
    m_buttonsMenu->setTouchPriority(-502);
    m_mainLayer->addChild(m_buttonsMenu);

    m_circle = LoadingCircle::create();
    m_circle->setParentLayer(m_mainLayer);
    m_circle->show();

    fetchAvailableThemes();
    return true;
}

void ThemePopup::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -490, true);
}

void ThemePopup::fetchAvailableThemes() {
    int accountID = GJAccountManager::sharedState()->m_accountID;
    std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getUserThemes.php" : "https://www.rustps.online/database/getUserThemes.php";

    matjson::Value bodyData = matjson::makeObject({
        { "accountID", accountID }
    });

    web::WebRequest req;
    req.bodyJSON(bodyData);

    this->retain();

    async::spawn(req.post(url), [this](web::WebResponse response) {
        if (m_circle) {
            m_circle->fadeAndRemove();
            m_circle = nullptr;
        }

        if (response.code() != 200) {
            FLAlertLayer::create("Error", "Failed to load themes", "OK")->show();
            this->release();
            return;
        }

        auto json = response.json().unwrapOr(matjson::Value());
        std::string themesStr = "";
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
            m_buttonsMenu->addChild(label);
            this->release();
            return;
        }

        m_buttonsMenu->removeAllChildrenWithCleanup(true);

        float startY = 45.f;
        for (auto const& theme : themes) {
            auto btnSprite = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
            if (!btnSprite) continue;
            btnSprite->setContentSize({ 200.f, 28.f });
            btnSprite->setColor({ 0, 0, 0 });
            btnSprite->setOpacity(120);

            auto label = CCLabelBMFont::create(theme.c_str(), "bigFont.fnt");
            label->setScale(0.4f);
            label->setPosition(btnSprite->getContentSize() / 2);
            btnSprite->addChild(label);
            
            auto btn = CCMenuItemSpriteExtra::create(
                btnSprite, this, menu_selector(ThemePopup::onSelectTheme)
            );
            btn->setID(theme);
            btn->setPosition({0.f, startY});
            m_buttonsMenu->addChild(btn);
            
            startY -= 35.f;
        }

        this->release();
    });
}

void ThemePopup::onSelectTheme(CCObject* sender) {
    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;

    std::string themeID = btn->getID();
    int accountID = GJAccountManager::sharedState()->m_accountID;

    std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/canUseTheme.php" : "https://www.rustps.online/database/canUseTheme.php";

    matjson::Value bodyData = matjson::makeObject({
        { "accountID", accountID },
        { "themeID", themeID }
    });

    web::WebRequest req;
    req.bodyJSON(bodyData);

    this->retain();

    async::spawn(req.post(url), [this](web::WebResponse response) {
        if (response.code() == 200) {
            auto json = response.json().unwrapOr(matjson::Value());
            bool success = false;
            
            if (json.isBool()) {
                success = json.asBool().unwrapOr(false);
            } else if (json.contains("success")) {
                success = json["success"].asBool().unwrapOr(false);
            }

            if (success) {
                FLAlertLayer::create("Success", "Theme successfully applied!", "OK")->show();
                this->onClose(nullptr);
            } else {
                FLAlertLayer::create("Error", "Server rejected theme change", "OK")->show();
            }
        } else {
            FLAlertLayer::create("Error", "Failed to connect to server", "OK")->show();
        }
        this->release();
    });
}

void ThemePopup::onClose(CCObject* sender) {
    if (m_circle) {
        m_circle->fadeAndRemove();
        m_circle = nullptr;
    }
    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    this->removeFromParentAndCleanup(true);
}

void ThemePopup::keyBackClicked() {
    this->onClose(nullptr);
}

ThemePopup* ThemePopup::create() {
    auto ret = new ThemePopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}