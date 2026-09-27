#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCMenuItemSpriteExtra.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/ScrollLayer.hpp>

using namespace geode::prelude;

class NewsPopup : public FLAlertLayer {
protected:
    ScrollLayer* m_scrollLayer;
    LoadingCircle* m_circle;

    bool init() override {
        if (!FLAlertLayer::init(150)) return false;

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto bg = CCScale9Sprite::create("GJ_square01.png", { 0, 0, 80, 80 });
        bg->setContentSize({ 400.f, 240.f });
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(
            closeSprite, this, menu_selector(NewsPopup::onClose)
        );
        
        auto closeMenu = CCMenu::create(closeBtn, nullptr);
        closeMenu->setPosition({ winSize.width / 2 - 185.f, winSize.height / 2 + 105.f });
        closeMenu->setTouchPriority(-495); 
        m_mainLayer->addChild(closeMenu);

        auto titleLabel = CCLabelBMFont::create("Новости Сервера", "goldFont.fnt");
        titleLabel->setScale(0.8f);
        titleLabel->setPosition({ winSize.width / 2, winSize.height / 2 + 90.f });
        m_mainLayer->addChild(titleLabel);
        
        m_scrollLayer = ScrollLayer::create({ 360.f, 160.f });
        m_scrollLayer->setPosition({ winSize.width / 2 - 180.f, winSize.height / 2 - 80.f });
        m_scrollLayer->setTouchEnabled(true);
        m_scrollLayer->setID("news-scroll-layer");
        m_mainLayer->addChild(m_scrollLayer);

        m_circle = LoadingCircle::create();
        m_circle->setParentLayer(m_mainLayer);
        m_circle->show();

        fetchNewsFromServer();
        return true;
    }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -490, true);
    }

    void fetchNewsFromServer() {
        std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getnews.php" : "https://www.rustps.online/database/getnews.php";

        auto am = GJAccountManager::sharedState();
        int accountID = am->m_accountID;
        std::string username = am->m_username.c_str();

        matjson::Value bodyData = matjson::makeObject({
            { "accountID", accountID },
            { "username", username }
        });

        this->retain(); 

        web::WebRequest req;
        req.bodyJSON(bodyData);

        async::spawn(req.post(url), [this](web::WebResponse response) {
            if (m_circle) {
                m_circle->fadeAndRemove();
                m_circle = nullptr;
            }

            if (response.code() != 200) {
                this->release();
                return;
            }

            auto json = response.json().unwrapOr(matjson::Value());
            if (!json.isArray()) {
                this->release();
                return;
            }

            std::vector<CCLayer*> temporaryLayers;
            float totalHeight = 0.0f;

                        for (int i = json.size() - 1; i >= 0; --i) {
                auto item = json[i];
                std::string date = item["date"].asString().unwrapOr("");
                std::string title = item["title"].asString().unwrapOr("");
                std::string text = item["text"].asString().unwrapOr("");

                auto itemLayer = CCLayer::create();

                auto textLabel = MDTextArea::create(text, {340, 50});
                float textHeight = textLabel->getScaledContentSize().height;
                float itemHeight = textHeight + 40.f; 

                auto itemBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
                if (itemBg) {
                    itemBg->setContentSize({ 350.f, itemHeight });
                    itemBg->setColor({0, 0, 0});
                    itemBg->setOpacity(110);
                    itemBg->setPosition({180.f, itemHeight / 2.f});
                    itemLayer->addChild(itemBg, -1);
                }
                
                auto titleLabel = CCLabelBMFont::create(title.c_str(), "goldFont.fnt");
                titleLabel->setScale(0.55f);
                titleLabel->setAnchorPoint({0.5f, 1.f});
                titleLabel->setPosition({180.f, itemHeight - 5.f});
                itemLayer->addChild(titleLabel);

                auto dateLabel = CCLabelBMFont::create(date.c_str(), "chatFont.fnt");
                dateLabel->setScale(0.4f);
                dateLabel->setAnchorPoint({0.5f, 1.f});
                dateLabel->setPosition({180.f, itemHeight - 20.f});
                dateLabel->setColor({140, 140, 140});
                itemLayer->addChild(dateLabel);

                auto clipper = CCClippingNode::create();
                auto stencil = CCNode::create();
                auto maskBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
                maskBg->setContentSize({ 340.f, textHeight });
                maskBg->setPosition({180.f, textHeight / 2.f});
                stencil->addChild(maskBg);
                
                clipper->setStencil(stencil);
                clipper->setAlphaThreshold(0.05f);
                clipper->setContentSize({340.f, textHeight});
                clipper->setPosition({0, itemHeight - 32.f - textHeight});

                textLabel->setAnchorPoint({0.5f, 1.f});
                textLabel->setPosition({180.f, textHeight});
                clipper->addChild(textLabel);
                
                itemLayer->addChild(clipper);

                itemLayer->setContentSize({360.f, itemHeight + 8.f});
                
                temporaryLayers.push_back(itemLayer);
                totalHeight += itemHeight + 8.f;
            }

            float finalHeight = std::max(160.f, totalHeight);
            m_scrollLayer->m_contentLayer->setContentSize({360.f, finalHeight});
            
            float positionY = finalHeight;
            for (size_t i = 0; i < temporaryLayers.size(); ++i) {
                auto* layer = temporaryLayers[i];
                positionY -= layer->getContentSize().height;
                layer->setPosition({0, positionY});
                m_scrollLayer->m_contentLayer->addChild(layer);
            }
            
            m_scrollLayer->m_contentLayer->setPositionY(160.f - finalHeight);
            m_scrollLayer->moveToTop();
            
            this->release();
        });
    }

    void onClose(CCObject* sender) {
        if (m_circle) {
            m_circle->fadeAndRemove();
            m_circle = nullptr;
        }
        this->setKeypadEnabled(false);
        this->setTouchEnabled(false);
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }

public:
    static NewsPopup* create() {
        auto ret = new NewsPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        if (auto moreGamesMenu = this->getChildByID("more-games-menu")) {
            if (auto moreGamesBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(moreGamesMenu->getChildByID("more-games-button"))) {
                auto newSprite = CCSprite::createWithSpriteFrameName("GJ_commentBtn_001.png");
                if (newSprite && moreGamesBtn->getChildrenCount() > 0) {
                    if (auto oldSprite = typeinfo_cast<CCSprite*>(moreGamesBtn->getChildren()->objectAtIndex(0))) {
                        oldSprite->setDisplayFrame(newSprite->displayFrame());
                    }
                }
            }
        }
        return true;
    }
};

class $modify(MyMenuItemSpriteExtra, CCMenuItemSpriteExtra) {
    void activate() {
        if (this->getID() == "more-games-button") {
            auto popup = NewsPopup::create();
            if (popup) {
                popup->show();
            }
            return; 
        }
        CCMenuItemSpriteExtra::activate();
    }
};