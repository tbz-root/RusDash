using namespace geode::prelude;

#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include "server.hpp"
#include <dasshu.badgified/include/Badgified.hpp>
#include "ds.cpp"
#include "own.h"

using namespace dasshu::badgified;

static std::unordered_map<int, std::vector<std::string>> s_userBadgesCache;
static std::unordered_set<int> s_pendingRequests;
static std::unordered_map<int, TaskHolder<web::WebResponse>> s_globalTasks;

void fetchBadgesForUser(int accountID, std::function<void()> onComplete)
{
    if (s_userBadgesCache.find(accountID) != s_userBadgesCache.end())
    {
        onComplete();
        return;
    }

    if (s_pendingRequests.count(accountID))
    {
        return;
    }

    s_pendingRequests.insert(accountID);

    auto json = matjson::makeObject({{"accountID", accountID}});

    auto req = web::WebRequest();
    req.header("Content-Type", "application/json");
    req.bodyJSON(json);
    req.timeout(std::chrono::seconds(15));

    std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://rustps.online/database" : "https://rustps.online/database";
    std::string url = baseUrl + "/main.php";

    s_globalTasks[accountID].spawn(
        req.post(url),
        [accountID, onComplete](web::WebResponse res)
        {
            s_pendingRequests.erase(accountID);
            s_globalTasks.erase(accountID);

            if (!res.ok())
            {
                s_userBadgesCache[accountID] = {};
                onComplete();
                return;
            }

            auto jsonResult = res.json();
            if (!jsonResult)
            {
                s_userBadgesCache[accountID] = {};
                onComplete();
                return;
            }

            auto json = jsonResult.unwrap();
            std::vector<std::string> userBadges;

            if (json.isObject() && json.contains("badges") && json["badges"].isArray())
            {
                for (auto const &bVal : json["badges"])
                {
                    if (auto idResult = bVal.asString())
                    {
                        userBadges.push_back(idResult.unwrap());
                    }
                }
            }

            s_userBadgesCache[accountID] = userBadges;
            onComplete();
        });
}

void handleBadgeCheck(const Badge &badge, const std::string &badgeID, const std::string &spriteName)
{
    if (!badge.user)
        return;
    int accountID = badge.user->m_accountID;

    if (s_userBadgesCache.find(accountID) != s_userBadgesCache.end())
    {
        auto &badges = s_userBadgesCache[accountID];
        if (std::find(badges.begin(), badges.end(), badgeID) != badges.end())
        {
            showBadge(badge, CCSprite::create(spriteName.c_str()));
        }
        return;
    }

    fetchBadgesForUser(accountID, [badge, badgeID, spriteName]()
                       {
        CCNode* targetNode = badge.target.data();
        if (!targetNode) return;

        auto& badges = s_userBadgesCache[badge.user->m_accountID];
        if (std::find(badges.begin(), badges.end(), badgeID) != badges.end()) {
            showBadge(badge, CCSprite::create(spriteName.c_str()));
        } });
}

class $modify(MyProfilePage, ProfilePage)
{
    bool init(int accountID, bool something)
    {
        if (!ProfilePage::init(accountID, something))
        {
            return false;
        }


        return true;
    }

    void onUpdate(cocos2d::CCObject *sender)
    {
        if (m_score)
        {
            s_userBadgesCache.erase(m_score->m_accountID);
            s_pendingRequests.erase(m_score->m_accountID);
        }
        ProfilePage::onUpdate(sender);

        if (m_score)
        {
            int accId = m_score->m_accountID;
            fetchBadgesForUser(accId, [this]() {});
        }
    }
};

class $modify(CustomProfilesPage,ProfilePage) {

    struct Fields
    {
        gd::string discordUsername;
    };
    static void onModify(auto& self) {
        if (!self.setHookPriorityAfterPost(
            "ProfilePage::setupCommentsBrowser",
            "alphalaneous.happy_textures"
        )) {
            log::warn("Failed to set hook priority AfterPost");
        }
    
    }

    void onDiscordProfile(CCObject* sender) {
        DiscordPopup::create(m_fields->discordUsername.c_str())->show();
    }

    void setupCommentsBrowser(CCArray* array) {
    
        ProfilePage::setupCommentsBrowser(array);

        ownUtils::FixLayer(m_mainLayer, 340, 100);
    }


    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);


        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (auto socialsMenu = m_mainLayer->getChildByID("socials-menu")) {

            if (!Mod::get()->getSettingValue<bool>("Disable-discord-button")) {

                if (score->m_discordUsername != "")
                {
                    if (!socialsMenu->getChildByID("discord-button"))
                    {
                        this->m_fields->discordUsername = fmt::format("@{}", score->m_discordUsername);
                        
                        auto discordSpr = CCSprite::createWithSpriteFrameName("gj_discordIcon_001.png");

                        auto discordButton = CCMenuItemSpriteExtra::create(
                            discordSpr,
                            discordSpr,
                            this,
                            menu_selector(CustomProfilesPage::onDiscordProfile)
                        );

                        float referenceScale = 1.f;

                        int buttonCount = 0;

                        for (auto node : CCArrayExt<CCNode*>(socialsMenu->getChildren())) {
                            if (auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(node)) {
                                referenceScale = btn->getScale();

                                buttonCount++;
                            }
                        }

                        if (buttonCount > 0) {
                            discordButton->setScale(referenceScale);

                        }

                        if (buttonCount == 4) {
                            discordSpr->setScale(0.9f);
                            discordButton->setAnchorPoint({ 0.5,0.45 });
                            discordButton->setContentHeight(discordButton->getContentHeight() - 2);
                        }
                        else if (buttonCount == 5) {
                            discordSpr->setScale(0.825f);
                            discordButton->setAnchorPoint({ 0.5,0.4 });
                            discordButton->setContentHeight(discordButton->getContentHeight() - 2.5);
                        }

                        if (buttonCount == 1) {
                            auto loader = geode::Loader::get();
               
                                if (auto statsMenu = m_mainLayer->getChildByID("stats-menu")) {
                                    if (!loader->isModLoaded("itzkiba.better_progression")) {
                                        statsMenu->setScale(0.825f);
                                    }
                                    if (auto myStuffHint = typeinfo_cast<CCSprite*>(m_mainLayer->getChildByID("my-stuff-hint"))) {
                                        auto* frame = cocos2d::CCSpriteFrameCache::get()->spriteFrameByName("GJ_stuffTxt_001.png");
                                        myStuffHint->setDisplayFrame(frame);
                                    }
                                }
                        }

                        discordButton->setTag(buttonCount + 1);
                        discordButton->setID("discord-button");
                        socialsMenu->addChild(discordButton);

                        if (!m_mainLayer->getChildByID("my-stuff-hint")) {

                            auto myStuffHint = CCSprite::createWithSpriteFrameName("GJ_stuffTxt_001.png");
                            myStuffHint->setPosition({ (winSize.width / 2) + 139.f, (winSize.height / 2) + 124.1f });

                            myStuffHint->setID("my-stuff-hint");
                            m_mainLayer->addChild(myStuffHint);
                            m_buttons->addObject(myStuffHint);
                        }

                        if (m_buttons) {
                            m_buttons->addObject(discordButton);
                        }

                    }
                    socialsMenu->updateLayout();
                }
            }
        }

        if (m_ownProfile) {
            auto bottomMenu = (CCMenu*)m_mainLayer->getChildByID("bottom-menu");
            auto settingsButton = (CCMenuItemSpriteExtra*)bottomMenu->getChildByID("settings-button");
            settingsButton->setTarget(this, menu_selector(ProfilePage::onSettings));
        }

        if (Mod::get()->getSettingValue<bool>("Disable-info-button")) {
            auto menu = m_mainLayer->getChildByID("main-menu");
            auto infobutton = menu->getChildByID("info-button");
            if (infobutton) infobutton->setPositionY(100000);
        }

        if (m_mainLayer->getChildByTag(3)) return;

        auto Layer = m_mainLayer;
        auto brownBG = (CCSprite*)Layer->getChildByID("background");
        brownBG->setVisible(false);
        ownUtils::FixLayerv2(Layer, 340, 45);

        int color1 = score->m_color1;
        int color2 = score->m_color2;
        auto col1 = ownUtils::ToColor4B(GameManager::sharedState()->colorForIdx(color1));
        auto col2 = ownUtils::ToColor4B(GameManager::sharedState()->colorForIdx(color2));

        auto normalGradient = CCLayerGradient::create(col1, col2);
        auto invertGradient = CCLayerGradient::create(col2, col1);

        color1 = score->m_color1;
        color2 = score->m_color2;
        col1 = ownUtils::ToColor4B(GameManager::sharedState()->colorForIdx(color1));
        col2 = ownUtils::ToColor4B(GameManager::sharedState()->colorForIdx(color2));
        normalGradient = CCLayerGradient::create(col1, col2);
        invertGradient = CCLayerGradient::create(col2, col1);
        invertGradient->runAction(CCRepeatForever::create(CCSequence::create(CCFadeOut::create(1.8f), CCFadeIn::create(1.8f), nullptr)));
        Layer->addChild(normalGradient);
        Layer->addChild(invertGradient);

        normalGradient->setID("normal-gradient"_spr);
        invertGradient->setID("invert-gradient"_spr);
        normalGradient->setTag(3);
        normalGradient->setZOrder(-5);
        normalGradient->setContentSize({ 435, 288 });
        normalGradient->setPosition(winSize.width / 2 - 217, winSize.height / 2 - 145);

        invertGradient->setZOrder(-3);
        invertGradient->setContentSize(normalGradient->getContentSize());
        invertGradient->setPosition(normalGradient->getPosition());

        // Corners
        auto roundCorner = cocos2d::extension::CCScale9Sprite::create("GJ_square07.png");
        roundCorner->setContentSize({ 438, 293 });
        roundCorner->setPosition(winSize / 2);
        roundCorner->setPositionY(roundCorner->getPositionY() - 1);
        roundCorner->setScale(1.006);
        roundCorner->setZOrder(7);

        auto whiteCorner = CCLayerGradient::create({ 255,255,255,255 }, { 255,255,255,255 });
        whiteCorner->setContentSize({ 435, 290 });
        whiteCorner->setPosition(normalGradient->getPosition());
        whiteCorner->setPositionY(whiteCorner->getPositionY() - 1);
        whiteCorner->setScale(1.02);
        whiteCorner->setZOrder(-7);

        auto blackCorner = CCLayerGradient::create({ 0,0,0,255 }, { 0,0,0,255 });
        blackCorner->setContentSize(whiteCorner->getContentSize());
        blackCorner->setPosition(whiteCorner->getPosition());
        blackCorner->setScale(1.01);
        blackCorner->setZOrder(-6);

        Layer->addChild(roundCorner); 

        auto blackBG = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
        blackBG->setPosition(winSize.width / 2, winSize.height / 2 - 1);
        blackBG->setColor(ccBLACK);
        blackBG->setOpacity(50);
        blackBG->setZOrder(-1);
        blackBG->setContentSize({ 850, 559 });
        blackBG->setScale(0.5f);
        Layer->addChild(blackBG);

        auto iconsContainer = cocos2d::extension::CCScale9Sprite::create("square02c_001.png");
        iconsContainer->setPosition(winSize.width / 2, winSize.height / 2 + 40);
        iconsContainer->setContentSize({ 356, 56 });
        iconsContainer->setColor({ 154,154,154 });
        iconsContainer->setOpacity(77);
        iconsContainer->setID("ccscale-icons"_spr);
        iconsContainer->setZOrder(-2);
        Layer->addChild(iconsContainer);

        auto commentsContainer = cocos2d::extension::CCScale9Sprite::create("square02c_001.png");
        commentsContainer->setPosition(winSize.width / 2, winSize.height / 2 - 53);
        commentsContainer->setContentSize({ 356, 106 });
        commentsContainer->setColor({ 154,154,154 });
        commentsContainer->setOpacity(77);
        commentsContainer->setID("ccscale-comments"_spr);
        commentsContainer->setZOrder(-2);
        Layer->addChild(commentsContainer);
  
    }

    void onSettings(CCObject* sender) { 
        geode::openSettingsPopup(Mod::get()); 
    }

   
};

$execute
{
    registerBadge(
        "example_badge"_spr, "Example Badge", "Description here",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "example_badge", "example_badge.png"_spr); });

    registerBadge(
        "a"_spr, "A Badge", "UHD version of the badge for testing of the resize function!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "a", "a_badge.png"_spr); });

    registerBadge(
        "coowner"_spr, "Co-Owner Badge", "The badge that dedicated for the RusDash Co-Owner(s)",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "coowner", "coowner_badge.png"_spr); });

    registerBadge(
        "owner"_spr, "Owner Badge", "The badge that dedicated for the RusDash Owner",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "owner", "owner_badge.png"_spr); });

    registerBadge(
        "creator"_spr, "Creator Badge", "The badge for very active creators. You can get it if your Creator Points Score is > 50!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "creator", "creator01_badge.png"_spr); });

    registerBadge(
        "creativness"_spr, "Creativness Badge", "The badge for the best Creators! You can get it if your Creator Points Score is > 100!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "creativness", "creator02_badge.png"_spr); });

    registerBadge(
        "inviter"_spr, "Inviter Badge", "The badge for the inviters. Inviters is peoples that invites other active players to the RusDash! You can get it if you publish a popular video about RusDash or invite > 30 players to any RusDash social!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "inviter", "inviter_badge.png"_spr); });

    registerBadge(
        "neko"_spr, "Neko Badge", "That badge is from secrets one! Find way to get it by yourself. 0-0",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "neko", "neko_badge.png"_spr); });

    registerBadge(
        "telegram"_spr, "Telegram Badge", "That badge is from secrets one! You can not get it by yourself.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "telegram", "telegram01_badge.png"_spr); });

    registerBadge(
        "trash"_spr, "Trash Badge", "That badge Tabz gives without any public reasons. Goog luck!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "trash", "trash_badge.png"_spr); });

    registerBadge(
        "grant"_spr, "Grant Moderator", "Submits rated levels to owner for fix its rate. He search levels which rate is wrong and submit it for fix. If creator of the wrong rated level dont fix level, the level got unrated, if the level fixed, the grant moderator submit the level to moderators for return its rate!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "grant", "grant01_badge.png"_spr); });

    registerBadge(
        "hunting"_spr, "Hunting Badge", "The badge is for those who are looking for something as much as possible.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "hunting", "neko05_badge.png"_spr); });

    registerBadge(
        "luck"_spr, "Luck Badge", "With this badge, you definitely have some luck, unfortunately, with receiving it, I think you out of luck.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "luck", "neko04_badge.png"_spr); });

    registerBadge(
        "aluck"_spr, "A badge of luck?", "With this badge, you definitely have some luck, unfortunately, with receiving it, I think you out of luck. You may have already read it or will read it somewhere else.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "aluck", "neko03_badge.png"_spr); });

    registerBadge(
        "tiger"_spr, "Tiger Badge", "Is it definitely a tiger? Nah. Just an badge.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "tiger", "neko02_badge.png"_spr); });
};