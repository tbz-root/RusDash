using namespace geode::prelude;

int m_menu_value = 1;

struct Badge {
    std::string id;
    std::string name;
    std::string description;
    const char* sprite;
    int priority;
};

static std::vector<Badge> badges = {
    {
        "example_badge",
        "Example Badge",
        "Sex",
        "example_badge.png"_spr,
        1
    },
    {
        "a",
        "A",
        "Sex",
        "example_badge.png"_spr,
        2
    },
    {
        "test_badge",
        "Test",
        "Sucks tests",
        "example_badge.png"_spr,
        3
    }
};

#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include "server.hpp"
static async::TaskHolder<web::WebResponse> m_badgeRequest;

static void loadUserBadges(int accountID, std::function<void(std::vector<std::string>)> callback) {
    auto json = matjson::makeObject({{"accountID", accountID}});

    auto req = web::WebRequest()
        .header("Content-Type", "application/json")
        .bodyJSON(json);

    std::string url = m_value + "main.php";

    m_badgeRequest.spawn(
        req.post(url),
        [callback](web::WebResponse res) {
            if (!res.ok()) {
                callback({});
                return;
            }

            auto jsonResult = res.json();

            if (!jsonResult) {
                callback({});
                return;
            }

            auto json = jsonResult.unwrap();

            if (!json.isObject()) {
                callback({});
                return;
            }

            auto badgesValue = json["badges"];

            if (!badgesValue.isArray()) {
                callback({});
                return;
            }

            std::vector<std::string> result;

            for (auto const& badgeValue : badgesValue) {
                auto id = badgeValue.asString();

                if (id) {
                    result.push_back(id.unwrap());
                }
            }

            callback(result);
        }
    );
}

#include <Geode/modify/ProfilePage.hpp>
class $modify(MyProfilePage, ProfilePage) {
    struct Fields {
        CCMenuItemSpriteExtra* arrayBtn = nullptr;
        CCLayer* m_secondaryLayer = nullptr;

        CCPoint mainLayerPos;
        CCPoint secondaryLayerPos;

        async::TaskHolder<web::WebResponse> badgeRequest;
    };

    void loadBadgesFromServer() {
        auto json = matjson::makeObject({{"accountID", m_accountID}});

        auto req = web::WebRequest()
            .header("Content-Type", "application/json")
            .bodyJSON(json);

        std::string url = m_value + "main.php";

        m_fields->badgeRequest.spawn(
            req.post(url),
            [this](web::WebResponse res) {
                if (!res.ok()) {
                    log::error(
                        "RusDash: Badge request failed: HTTP {} :(", res.code()
                    );

                    return;
                }

                auto jsonResult = res.json();

                if (!jsonResult) {
                    log::error(
                        "RusDash: Failed to parse badge response: {} :(", jsonResult.unwrapErr()
                    );

                    return;
                }

                auto json = jsonResult.unwrap();

                if (!json.isObject()) {
                    log::error("RusDash: Server response is not an object");

                    return;
                }

                auto badgesValue = json["badges"];

                if (!badgesValue.isArray()) {
                    log::error("RusDash: 'badges' is not an array :(");
                    return;
                }

                std::vector<std::string> serverBadges;

                for (auto const& badgeValue : badgesValue) {
                    auto idResult = badgeValue.asString();

                    if (!idResult) {
                        continue;
                    }

                    serverBadges.push_back(idResult.unwrap());
                }

                auto badgesContent = static_cast<CCMenu*>(m_fields->m_secondaryLayer->getChildByID("badges-background")->getChildByID("content"));

                if (!badgesContent)
                    return;

                for (auto const& badge : badges) {
                    bool hasBadge = false;

                    for (auto const& serverBadge : serverBadges) {
                        if (serverBadge == badge.id) {
                            hasBadge = true;
                            break;
                        }
                    }

                    if (!hasBadge)
                        continue;

                    auto button = CCMenuItemSpriteExtra::create(
                        CCSprite::create(badge.sprite),
                        this,
                        menu_selector(MyProfilePage::onBadge)
                    );

                    button->setScale(1.5f);
                    button->m_baseScale = 1.5f;

                    button->setUserObject(CCString::create(badge.id));

                    badgesContent->addChild(button);
                }

                badgesContent->updateLayout();
            }
        );
    }

    void loadPageFromUserInfo(GJUserScore* a2) {
        ProfilePage::loadPageFromUserInfo(a2);

        m_fields->mainLayerPos = m_mainLayer->getPosition();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_mainLayer->setID("main-layer");
        m_fields->m_secondaryLayer = CCMenu::create();
        m_fields->secondaryLayerPos = CCPoint(0.f, winSize.height);
        m_fields->m_secondaryLayer->setPosition(m_fields->secondaryLayerPos);
        m_fields->m_secondaryLayer->setID("secondary-layer");

        this->addChild(m_fields->m_secondaryLayer);

        auto m_secondaryLayer = m_fields->m_secondaryLayer;

        auto background = CCScale9Sprite::create(CCRect(10, 10, 10, 10), "GJ_square01.png");

        background->setContentSize({440.f, 290.f});
        background->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        background->setID("background");

        m_secondaryLayer->addChild(background);

        auto floorLine = CCSprite::createWithSpriteFrameName("floorLine_001.png");

        floorLine->setPosition({ winSize.width / 2.f, winSize.height - 56.f});
        floorLine->setID("floor-line");
        floorLine->setOpacity(100);
        floorLine->setScaleX(0.8f);

        m_secondaryLayer->addChild(floorLine);

        auto badgesBackground = CCScale9Sprite::create(CCRect(10, 10, 10, 10), "GJ_square07.png");

        badgesBackground->setContentSize({340.f, 45.f});
        badgesBackground->setPosition({winSize.width / 2.f, winSize.height / 2.f + 27.5f});
        badgesBackground->setID("badges-background");

        m_secondaryLayer->addChild(badgesBackground);

        auto badgesContent = CCMenu::create();

        badgesContent->setContentSize({340.f, 45.f});
        badgesContent->setPosition({170.f, 22.5f});
        badgesContent->setID("content");
        badgesContent->setLayout(
            RowLayout::create()
                ->setGap(5.f)
                ->setAutoScale(false)
        );

        badgesBackground->addChild(badgesContent);

        this->loadBadgesFromServer();

        auto arrayMenu = CCMenu::create();

        arrayMenu->setPosition(0.f, 0.f);

        arrayMenu->setID("array-menu");

        this->addChild(arrayMenu);

        m_fields->arrayBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png"),
            this,
            menu_selector(MyProfilePage::onArrayBtn)
        );

        m_fields->arrayBtn->setPosition({winSize.width / 2.f, winSize.height - 20.f});
        m_fields->arrayBtn->setRotation(90.f);
        m_fields->arrayBtn->setID("array-button");

        arrayMenu->addChild(m_fields->arrayBtn);
    }

    void onBadge(CCObject* sender) {
        auto button = static_cast<CCMenuItemSpriteExtra*>(sender);

        auto idObject = button->getUserObject();

        if (!idObject)
            return;

        auto id = static_cast<CCString*>(idObject)->getCString();

        for (auto const& badge : badges) {
            if (badge.id == id) {
                FLAlertLayer::create(
                    badge.name.c_str(),
                    gd::string(badge.description),
                    "OK"
                )->show();

                return;
            }
        }
    }

    void onArrayBtn(CCObject* sender) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (m_menu_value == 1) {
            m_menu_value = 2;

            m_fields->arrayBtn->runAction(CCEaseInOut::create(CCRotateTo::create(0.2f, -90.f), 2.0f));
            m_mainLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, winSize.height)), 2.0f));
            m_fields->m_secondaryLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, -winSize.height)), 2.0f));
        } else {
            m_menu_value = 1;

            m_fields->arrayBtn->runAction(CCEaseInOut::create(CCRotateTo::create(0.2f, 90.f), 2.0f));
            m_mainLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, -winSize.height)), 2.0f));
            m_fields->m_secondaryLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, winSize.height)), 2.0f));
        }
    }
};

#include <Geode/modify/CommentCell.hpp>
class $modify(MyCommentCell, CommentCell) {
    struct Fields {
        async::TaskHolder<web::WebResponse> badgeRequest;
    };

    void loadFromComment(GJComment* p0) {
        CommentCell::loadFromComment(p0);

        if(!typeinfo_cast<ProfilePage*>(CCDirector::sharedDirector()->getRunningScene()->getChildByType<ProfilePage>(0))) {
            if (!m_comment)
                return;

            auto usernameMenu = m_mainLayer->getChildByIDRecursive("username-menu");

            if (!usernameMenu)
                return;
                
            int accountID = m_comment->m_accountID;

            auto json = matjson::makeObject({{"accountID", accountID}});

            auto req = web::WebRequest()
                .header("Content-Type", "application/json")
                .bodyJSON(json);

            std::string url = m_value + "main.php";

            m_fields->badgeRequest.spawn(
                req.post(url),
                [this](web::WebResponse res) {
                    if (!res.ok())
                        return;

                    auto jsonResult = res.json();

                    if (!jsonResult)
                        return;

                    auto json = jsonResult.unwrap();

                    auto badgesValue = json["badges"];

                    if (!badgesValue.isArray())
                        return;

                    Badge const* bestBadge = nullptr;

                    for (auto const& value : badgesValue) {
                        auto idResult = value.asString();

                        if (!idResult)
                            continue;

                        auto id = idResult.unwrap();

                        for (auto const& badge : badges) {
                            if (badge.id == id) {
                                if (!bestBadge ||
                                    badge.priority > bestBadge->priority) {
                                    bestBadge = &badge;
                                }

                                break;
                            }
                        }
                    }

                    if (!bestBadge)
                        return;

                    auto usernameMenu = m_mainLayer->getChildByIDRecursive("username-menu");

                    if (!usernameMenu)
                        return;

                    auto badgeBtn = CCMenuItemSpriteExtra::create(
                        CCSprite::create(bestBadge->sprite),
                        this,
                        menu_selector(MyCommentCell::onBadge)
                    );

                    badgeBtn->setScale(0.75f);
                    badgeBtn->m_baseScale = 0.75f;
                    badgeBtn->setUserObject(CCString::create(bestBadge->id));

                    usernameMenu->addChild(badgeBtn);
                    usernameMenu->updateLayout();
                }
            );
        }
    }

    void onBadge(CCObject* sender) {
        auto button =
            static_cast<CCMenuItemSpriteExtra*>(sender);

        auto object = button->getUserObject();

        if (!object)
            return;

        auto id =
            static_cast<CCString*>(object)->getCString();

        for (auto const& badge : badges) {
            if (badge.id == id) {
                FLAlertLayer::create(
                    badge.name.c_str(),
                    gd::string(badge.description),
                    "OK"
                )->show();

                return;
            }
        }
    }
};

/* using namespace geode::prelude;

int m_menu_value = 1;

#include "server.hpp"
struct Badge {
    std::string id;
    std::string name;
    std::string description;
    const char* sprite;
    int priority;
};

static std::vector<Badge> badges = {
    {
        "example_badge",
        "Example Badge",
        "Sex",
        "example_badge.png"_spr,
        1
    }
};

#include <Geode/modify/ProfilePage.hpp>
class $modify(MyProfilePage, ProfilePage) {
    struct Fields {
        CCMenuItemSpriteExtra* arrayBtn = nullptr;
        CCLayer* m_secondaryLayer = nullptr;

        CCPoint mainLayerPos;
        CCPoint secondaryLayerPos;
    };

    void loadPageFromUserInfo(GJUserScore* a2) {
        ProfilePage::loadPageFromUserInfo(a2);

        m_fields->mainLayerPos = m_mainLayer->getPosition();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_mainLayer->setID("main-layer");
        m_fields->m_secondaryLayer = CCMenu::create();
        m_fields->secondaryLayerPos = CCPoint(0.f, winSize.height * 1.f);
        m_fields->m_secondaryLayer->setPosition(m_fields->secondaryLayerPos);
        m_fields->m_secondaryLayer->setID("secondary-layer");

        this->addChild(m_fields->m_secondaryLayer);

        auto m_secondaryLayer = m_fields->m_secondaryLayer;

        auto background = CCScale9Sprite::create(CCRect(10, 10, 10, 10), "GJ_square01.png");

        background->setContentSize({440.f, 290.f});
        background->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        background->setID("background");

        m_secondaryLayer->addChild(background);

        auto floorLine = CCSprite::createWithSpriteFrameName("floorLine_001.png");

        floorLine->setPosition({winSize.width / 2.f, winSize.height - 56.f});
        floorLine->setID("floor-line");
        floorLine->setOpacity(100);
        floorLine->setScaleX(0.8f);

        m_secondaryLayer->addChild(floorLine);

        auto badgesBackground = CCScale9Sprite::create(CCRect(10, 10, 10, 10), "GJ_square07.png");

        badgesBackground->setContentSize({340.f, 45.f});
        badgesBackground->setPosition({winSize.width / 2.f, winSize.height / 2.f + 27.5f});
        badgesBackground->setID("badges-background");

        m_secondaryLayer->addChild(badgesBackground);

        auto badgesContent = CCMenu::create();

        badgesContent->setContentSize({340.f, 45.f});
        badgesContent->setPosition({170.f, 22.5f});
        badgesContent->setID("content");
        badgesContent->setLayout(
            RowLayout::create()
                ->setGap(5.f)
                ->setAutoScale(false)
        );

        badgesBackground->addChild(badgesContent);

        for (auto const& badge : badges) {
            auto button = CCMenuItemSpriteExtra::create(
                CCSprite::create(badge.sprite),
                this,
                menu_selector(MyProfilePage::onBadge)
            );

            button->setScale(1.5f);
            button->m_baseScale = 1.5f;
            button->setUserObject(CCString::create(badge.id));

            badgesContent->addChild(button);
        }

        badgesContent->updateLayout();

        auto arrayMenu = CCMenu::create();

        arrayMenu->setPosition(0.f, 0.f);
        arrayMenu->setID("array-menu");

        this->addChild(arrayMenu);

        m_fields->arrayBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png"),
            this,
            menu_selector(MyProfilePage::onArrayBtn)
        );

        m_fields->arrayBtn->setPosition({winSize.width / 2.f, winSize.height - 20.f});
        m_fields->arrayBtn->setRotation(90.f);
        m_fields->arrayBtn->setID("array-button");

        arrayMenu->addChild(m_fields->arrayBtn);
    }

    void onBadge(CCObject* sender) {
        auto button = static_cast<CCMenuItemSpriteExtra*>(sender);

        auto idObject = button->getUserObject();
        if (!idObject)
            return;

        auto id = static_cast<CCString*>(idObject)->getCString();

        for (auto const& badge : badges) {
            if (badge.id == id) {
                FLAlertLayer::create(
                    badge.name.c_str(),
                    gd::string(badge.description),
                    "OK"
                )->show();

                return;
            }
        }
    }

    void onArrayBtn(CCObject* sender) {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (m_menu_value == 1) {
            m_menu_value = 2;

            m_fields->arrayBtn->runAction(CCEaseInOut::create(CCRotateTo::create(0.2f, -90.f), 2.0f));
            m_mainLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, winSize.height)), 2.0f));
            m_fields->m_secondaryLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, -winSize.height)), 2.0f));
        }
        else {
            m_menu_value = 1;

            m_fields->arrayBtn->runAction(CCEaseInOut::create(CCRotateTo::create(0.2f, 90.f), 2.0f));
            m_mainLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, -winSize.height)), 2.0f));
            m_fields->m_secondaryLayer->runAction(CCEaseInOut::create(CCMoveBy::create(0.2f, CCPoint(0.f, winSize.height)), 2.0f));
        }
    }
}; */