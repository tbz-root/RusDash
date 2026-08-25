using namespace geode::prelude;

#include <matjson.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include "server.hpp"
#include <dasshu.badgified/include/badgified.hpp>

using namespace dasshu::badgified;

static std::unordered_map<int, std::vector<std::string>> s_userBadgesCache;
static std::unordered_set<int> s_pendingRequests;
static std::unordered_map<int, TaskHolder<web::WebResponse>> s_globalTasks;

void fetchBadgesForUser(int accountID, std::function<void()> onComplete) {
    if (s_pendingRequests.count(accountID)) {
        onComplete();
        return;
    }

    if (s_userBadgesCache.find(accountID) != s_userBadgesCache.end()) {
        onComplete();
        return;
    }

    s_pendingRequests.insert(accountID);

    auto json = matjson::makeObject({
        {"accountID", accountID}
    });

    auto req = web::WebRequest();
    req.header("Content-Type", "application/json");
    req.bodyJSON(json);
    req.timeout(std::chrono::seconds(15));

    std::string baseUrl = Mod::get()->getSettingValue<bool>("use-mirror") ? "https://www.rustps.online/database" : "https://rustps.online/database";
    std::string url = baseUrl + "/main.php";

    s_globalTasks[accountID].spawn(
        req.post(url),
        [accountID, onComplete](web::WebResponse res) {
            s_pendingRequests.erase(accountID);
            s_globalTasks.erase(accountID);

            if (!res.ok()) {
                onComplete();
                return;
            }

            auto jsonResult = res.json();
            if (!jsonResult) {
                onComplete();
                return;
            }

            auto json = jsonResult.unwrap();
            if (!json.isObject() || !json.contains("badges")) {
                onComplete();
                return;
            }

            auto badgesValue = json["badges"];
            if (!badgesValue.isArray()) {
                onComplete();
                return;
            }

            std::vector<std::string> userBadges;
            for (auto const& bVal : badgesValue) {
                if (auto idResult = bVal.asString()) {
                    userBadges.push_back(idResult.unwrap());
                }
            }

            s_userBadgesCache[accountID] = userBadges;
            onComplete();
        }
    );
}

void handleBadgeCheck(const Badge& badge, const std::string& badgeID, const std::string& spriteName) {
    if (!badge.user) return;
    int accountID = badge.user->m_accountID;

    if (s_userBadgesCache.find(accountID) != s_userBadgesCache.end()) {
        auto& badges = s_userBadgesCache[accountID];
        if (std::find(badges.begin(), badges.end(), badgeID) != badges.end()) {
            showBadge(badge, CCSprite::create(spriteName.c_str()));
        }
        return;
    }

    fetchBadgesForUser(accountID, [badge, badgeID, spriteName]() {
        if (s_userBadgesCache.find(badge.user->m_accountID) != s_userBadgesCache.end()) {
            auto& badges = s_userBadgesCache[badge.user->m_accountID];
            if (std::find(badges.begin(), badges.end(), badgeID) != badges.end()) {
                showBadge(badge, CCSprite::create(spriteName.c_str()));
            }
        }
    });
}

class $modify(MyProfilePage, ProfilePage) {
    bool init(int accountID, bool something) {
        if (!ProfilePage::init(accountID, something)) {
            return false;
        }

        s_userBadgesCache.erase(accountID);

        return true;
    }
};

$execute {
    registerBadge(
        "example_badge"_spr, "Example Badge", "Description here", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "example_badge", "example_badge.png"_spr); }
    );

    registerBadge(
        "a"_spr, "A Badge", "UHD version of the badge for testing of the resize function!", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "a", "a_badge.png"_spr); }
    );

    registerBadge(
        "coowner"_spr, "Co-Owner Badge", "The badge that dedicated for the RusDash Co-Owner(s)", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "coowner", "coowner_badge.png"_spr); }
    );

    registerBadge(
        "owner"_spr, "Owner Badge", "The badge that dedicated for the RusDash Owner", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "owner", "owner_badge.png"_spr); }
    );

    registerBadge(
        "creator"_spr, "Creator Badge", "The badge for very active creators. You can get it if your Creator Points Score is > 50!", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "creator", "creator01_badge.png"_spr); }
    );

    registerBadge(
        "creativness"_spr, "Creativness Badge", "The badge for the best Creators! You can get it if your Creator Points Score is > 100!", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "creativness", "creator02_badge.png"_spr); }
    );

    registerBadge(
        "inviter"_spr, "Inviter Badge", "The badge for the inviters. Inviters is peoples that invites other active players to the RusDash! You can get it if you publish a popular video about RusDash or invite > 30 players to any RusDash social!", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "inviter", "inviter_badge.png"_spr); }
    );

    registerBadge(
        "neko"_spr, "Neko Badge", "That badge is from secrets one! Find way to get it by yourself. 0-0", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "neko", "neko_badge.png"_spr); }
    );

    registerBadge(
        "telegram"_spr, "Telegram Badge", "That badge is from secrets one! You can not get it by yourself.", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "telegram", "telegram01_badge.png"_spr); }
    );

    registerBadge(
        "trash"_spr, "Trash Badge", "That badge Tabz gives without any public reasons. Goog luck!", 
        [] (const Badge& badge) { handleBadgeCheck(badge, "trash", "trash_badge.png"_spr); }
    );
};
