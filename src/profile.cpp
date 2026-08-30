using namespace geode::prelude;

#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include "server.hpp"
#include <future>
#include <dasshu.badgified/include/Badgified.hpp>

using namespace dasshu::badgified;

static std::unordered_map<int, std::vector<std::string>> s_userBadgesCache;
static std::unordered_set<int> s_pendingRequests;
static std::unordered_map<int, TaskHolder<web::WebResponse>> s_globalTasks;
static TaskHolder<web::WebResponse> s_themeTask;
static std::unordered_map<int, std::string> s_userThemeCache;

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

    std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://www.rustps.online/database" : "https://rustps.online/database";
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

void verifyAndApplyTheme(int accountID, std::string themeID)
{
    if (themeID.empty() || themeID == "default")
    {
        return;
    }

    auto json = matjson::makeObject({{"accountID", accountID},
                                     {"themeID", themeID}});

    auto req = web::WebRequest();
    req.header("Content-Type", "application/json");
    req.bodyJSON(json);
    req.timeout(std::chrono::seconds(15));

    std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://www.rustps.online/database" : "https://rustps.online/database";
    std::string url = baseUrl + "/canUseTheme.php";

    s_themeTask.spawn(
        req.post(url),
        [themeID](web::WebResponse res)
        {
            if (!res.ok())
            {
                FLAlertLayer::create("Error", "Failed to connect to the server to verify theme.", "OK")->show();
                Mod::get()->setSettingValue<std::string>("profile-theme", "default");
                return;
            }

            std::string responseStr = res.string().unwrapOr("");
            while (!responseStr.empty() && (responseStr.back() == '\n' || responseStr.back() == '\r' || responseStr.back() == ' '))
            {
                responseStr.pop_back();
            }

            bool canUse = (responseStr == "true" || responseStr == "1");

            if (!canUse)
            {
                FLAlertLayer::create("Access Denied", "You cannot use this profile theme because you do not have the required badge!", "OK")->show();
                Mod::get()->setSettingValue<std::string>("profile-theme", "default");
            }
            else
            {
                FLAlertLayer::create("Success", "Profile theme applied successfully!", "OK")->show();
            }
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
    struct Fields
    {
        TaskHolder<web::WebResponse> profileThemeTask;
    };

    bool init(int accountID, bool something)
    {
        if (!ProfilePage::init(accountID, something))
        {
            return false;
        }

        fetchAndApplyProfileTheme(accountID);
        return true;
    }

    void onUpdate(cocos2d::CCObject *sender)
    {
        if (m_score)
        {
            s_userBadgesCache.erase(m_score->m_accountID);
            s_pendingRequests.erase(m_score->m_accountID);
            s_userThemeCache.erase(m_score->m_accountID);
        }

        ProfilePage::onUpdate(sender);

        if (m_score)
        {
            int accId = m_score->m_accountID;
            fetchBadgesForUser(accId, [this]() {});
            fetchAndApplyProfileTheme(accId);
        }
    }

    void fetchAndApplyProfileTheme(int accountID)
    {
        if (s_userThemeCache.find(accountID) != s_userThemeCache.end())
        {
            applyThemeToProfile(s_userThemeCache[accountID]);
            return;
        }
        std::string themeID = Mod::get()->getSettingValue<std::string>("profile-theme");

        auto json = matjson::makeObject({{"accountID", accountID}, {"themeID", themeID}});

        auto req = web::WebRequest();
        req.header("Content-Type", "application/json");
        req.bodyJSON(json);
        req.timeout(std::chrono::seconds(15));

        std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://www.rustps.online/database" : "https://rustps.online/database";
        std::string url = baseUrl + "/getProfileTheme.php";

        m_fields->profileThemeTask.spawn(
            req.post(url),
            [this, accountID](web::WebResponse res)
            {
                std::string themeID = "default";
                if (res.ok())
                {
                    std::string responseStr = res.string().unwrapOr("default");
                    while (!responseStr.empty() && (responseStr.back() == '\n' || responseStr.back() == '\r' || responseStr.back() == ' '))
                    {
                        responseStr.pop_back();
                    }
                    if (!responseStr.empty())
                    {
                        themeID = responseStr;
                    }
                }

                s_userThemeCache[accountID] = themeID;
                applyThemeToProfile(themeID);
            });
    }

    void applyThemeToProfile(std::string themeID)
    {
        auto profileOriginalCard = MyProfilePage::getChildByIndex(0);
        if (!profileOriginalCard)
            return;

        if (themeID.empty() || themeID == "default")
            return;

        if (profileOriginalCard->getChildren())
        {
            for (auto child : geode::cocos::CCArrayExt<cocos2d::CCNode *>(profileOriginalCard->getChildren()))
            {
                if (auto gradient = dynamic_cast<cocos2d::CCLayerGradient *>(child))
                {
                    gradient->setVisible(false);
                }
            }
        }

        auto customProfiles = profileOriginalCard->getChildByID("background");
        if (customProfiles)
        {
            std::string frameName = themeID + "_card.png";
            std::string geodeFrameName = geode::Mod::get()->expandSpriteName(frameName.c_str());

            auto newProfileBack = cocos2d::CCSprite::createWithSpriteFrameName(geodeFrameName.c_str());
            if (newProfileBack)
            {
                newProfileBack->setPosition(customProfiles->getPosition());
                newProfileBack->setScale(customProfiles->getScale() + 0.3f);
                newProfileBack->setZOrder(-6);
                profileOriginalCard->addChild(newProfileBack);
            }
        }

        auto applyButtonModifications = [themeID](CCNode *container, const std::string &menuName)
        {
            if (!container || !container->getChildren())
                return;
            for (auto child : geode::cocos::CCArrayExt<cocos2d::CCNode *>(container->getChildren()))
            {
                if (auto button = dynamic_cast<CCMenuItemSpriteExtra *>(child))
                {
                    if (menuName == "main" && button->getID() == "copy-username-button" || button->getID() == "follow-button")
                        continue;

                    std::string frameName = themeID + "_" + button->getID() + ".png";
                    std::string geodeFrameName = geode::Mod::get()->expandSpriteName(frameName.c_str());

                    auto replacement = cocos2d::CCSprite::createWithSpriteFrameName(geodeFrameName.c_str());

                    if (replacement)
                    {
                        if (button->getChildren())
                        {
                            for (auto buttonChild : geode::cocos::CCArrayExt<cocos2d::CCNode *>(button->getChildren()))
                            {
                                buttonChild->setVisible(false);
                            }
                        }

                        float targetSize = 42.0f;
                        button->setContentSize({targetSize, targetSize});

                        auto newSize = replacement->getContentSize();
                        float scaleX = targetSize / newSize.width;
                        float scaleY = targetSize / newSize.height;
                        float finalScale = std::min(scaleX, scaleY);

                        replacement->setScale(finalScale + 0.25f);
                        replacement->setPosition({targetSize / 2, targetSize / 2});
                        replacement->setVisible(true);

                        button->addChild(replacement);
                        button->setScale(1.0f);
                    }
                }
            }
        };

        applyButtonModifications(profileOriginalCard->getChildByID("bottom-menu"), "bottom");
        applyButtonModifications(profileOriginalCard->getChildByID("main-menu"), "main");
        applyButtonModifications(profileOriginalCard->getChildByID("left-menu"), "left");
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

    registerBadge(
        "bk"_spr, "...", "pizdec.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "bk", "bk01_badge.png"_spr); });

    registerBadge(
        "bk02"_spr, "...", "ni ebu che skazat.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "bk02", "bk02_badge.png"_spr); });

    registerBadge(
        "bk03"_spr, "...", "ebaaat...",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "bk03", "bk03_badge.png"_spr); });

    registerBadge(
        "top01"_spr, "Top Grinder Badge", "The badge for the best star grinder!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "top01", "top01_badge.png"_spr); });

    registerBadge(
        "YT01"_spr, "Youtuber Badge", "The badge for the RusDash youtuber. Must have > 500 subscribers on youtube and invite > 30 players by your videos!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "YT01", "YT01_badge.png"_spr); });

    registerBadge(
        "creator03"_spr, "Diamond Creator Badge", "The badge for the strong creator! Must have > 150 creator points.",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "creator03", "creator03_badge.png"_spr); });

    registerBadge(
        "creator04"_spr, "Ruby Creator Badge", "The badge for really strong creator! Must have > 200 creator points!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "creator04", "creator04_badge.png"_spr); });

    registerBadge(
        "tt01"_spr, "TickToker Badge", "The badge for the RusDash ticktoker. Must have > 1000 subscribers on tiktok and invite > 50 players by your videos!",
        [](const Badge &badge)
        { handleBadgeCheck(badge, "tt01", "tt01_badge.png"_spr); });

    listenForSettingChanges<std::string>("profile-theme", [](std::string value)
                                         {
        auto gm = GJAccountManager::sharedState();
        if (gm && gm->m_accountID != 0) {
            verifyAndApplyTheme(gm->m_accountID, value);
        } });
}