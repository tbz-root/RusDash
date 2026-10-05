using namespace geode::prelude;

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Popup.hpp>

class NewsPopup : public Popup {
protected:
    ScrollLayer* m_scrollLayer = nullptr;
    LoadingCircle* m_circle = nullptr;

    bool init() {
        if (!Popup::init(420.f, 285.f, "GJ_square01.png"))
            return false;

        this->setTitle("Новости Сервера");

        m_scrollLayer = ScrollLayer::create({ 360.f, 180.f });

        m_scrollLayer->setPosition({m_size.width / 2 - 180.f, 25.f});
        m_scrollLayer->setTouchEnabled(true);
        m_scrollLayer->setID("news-scroll-layer");

        m_mainLayer->addChild(m_scrollLayer);

        m_circle = LoadingCircle::create();

        m_mainLayer->addChild(m_circle);
        m_circle->show();

        fetchNewsFromServer();

        return true;
    }

    void fetchNewsFromServer() {
        std::string url = Mod::get()->getSettingValue<bool>("enable-mirror") ? "https://rustps.online/database/getnews.php" : "https://www.rustps.online/database/getnews.php";

        auto am = GJAccountManager::sharedState();
        int accountID = am->m_accountID;
        std::string username = am->m_username.c_str();

        matjson::Value bodyData = matjson::makeObject({{"accountID", accountID}, {"username", username}});

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

            for (int i = static_cast<int>(json.size()) - 1; i >= 0; --i) {
                auto item = json[i];

                std::string date = item["date"].asString().unwrapOr("");
                std::string title = item["title"].asString().unwrapOr("");
                std::string text = item["text"].asString().unwrapOr("");

                auto itemLayer = CCLayer::create();

                auto textLabel = MDTextArea::create(text, { 340.f, 50.f });

                float textHeight = textLabel->getScaledContentSize().height;

                float itemHeight = textHeight + 40.f;

                auto itemBg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});

                if (itemBg) {
                    itemBg->setContentSize({350.f, itemHeight});
                    itemBg->setColor({ 0, 0, 0 });
                    itemBg->setOpacity(110);
                    itemBg->setPosition({180.f, itemHeight / 2.f});

                    itemLayer->addChild(itemBg, -1);
                }

                auto titleLabel = CCLabelBMFont::create(title.c_str(), "goldFont.fnt");

                titleLabel->setScale(0.55f);
                titleLabel->setAnchorPoint({ 0.5f, 1.f });
                titleLabel->setPosition({
                    180.f,
                    itemHeight - 5.f
                });

                itemLayer->addChild(titleLabel);

                auto dateLabel = CCLabelBMFont::create(date.c_str(), "chatFont.fnt");

                dateLabel->setScale(0.4f);
                dateLabel->setAnchorPoint({ 0.5f, 1.f });
                dateLabel->setPosition({180.f, itemHeight - 20.f});
                dateLabel->setColor({140, 140, 140});

                itemLayer->addChild(dateLabel);

                auto clipper = CCClippingNode::create();
                auto stencil = CCNode::create();
                auto maskBg =CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});

                maskBg->setContentSize({340.f, textHeight});
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
                itemLayer->setContentSize({360.f,itemHeight + 8.f});

                temporaryLayers.push_back(itemLayer);

                totalHeight += itemHeight + 8.f;
            }

            float finalHeight =
                std::max(180.f, totalHeight);

            m_scrollLayer->m_contentLayer->setContentSize({
                360.f,
                finalHeight
            });

            float positionY = finalHeight;

            for (auto* layer : temporaryLayers) {
                positionY -= layer->getContentSize().height;

                layer->setPosition({
                    0,
                    positionY
                });

                m_scrollLayer->m_contentLayer->addChild(layer);
            }

            m_scrollLayer->m_contentLayer->setPositionY(
                180.f - finalHeight
            );

            m_scrollLayer->moveToTop();

            this->release();
        });
    }

public:
    static NewsPopup* create() {
        auto ret = new NewsPopup();

        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

#include <Geode/modify/MenuLayer.hpp>
class $modify(NewsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("more-games-menu");
        if (!menu) return true;

        auto newsSpr = CCSprite::createWithSpriteFrameName("newsBtn_001.png"_spr);
        newsSpr->setScale(1.35f);

        auto newsBtn = CCMenuItemSpriteExtra::create(
            newsSpr,
            this,
            menu_selector(NewsMenuLayer::onNewsBtn)
        );

        if (auto moreGamesBtn = menu->getChildByID("more-games-button")) {
            moreGamesBtn->setVisible(false);
        }

        menu->addChild(newsBtn);
        menu->updateLayout();

        return true;
    }

    void onNewsBtn(CCObject*) {
        NewsPopup::create()->show();
    }
};