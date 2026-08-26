using namespace geode::prelude;

#include "server.hpp"
#include <Geode/Geode.hpp>
std::string m_primary_url = "rustps.online/database/////";
std::string m_fallback_url = "www.rustps.online/database/";
std::string m_value;

void applyServerEndpoint() {
    bool useMirror = Mod::get()->getSettingValue<bool>("use-mirror");

    m_value = useMirror ? m_fallback_url : m_primary_url;

    log::info("RusDash: Applying server -> {}", m_value);
}

#include <Geode/modify/CCHttpClient.hpp>
class $modify(CCHttpClient) {
    void send(CCHttpRequest* req) {
        std::string url = req->getUrl();

        url = string::replace(url, "www.boomlings.com/database/", m_value);
        url = string::replace(url,"boomlings.com/database/", m_value);

        req->setUrl(url.c_str());

        return CCHttpClient::send(req);
    }
};

#include <Geode/modify/CCApplication.hpp>
class $modify(CCApplication) {
    void openURL(const char* psz) {
        std::string url = psz;

        url = string::replace(url, "www.boomlings.com/database/", m_value);
        url = string::replace(url, "boomlings.com/database/", m_value);

        return CCApplication::openURL(url.c_str());
    }
};

$on_mod(Loaded) {
    applyServerEndpoint();

    listenForSettingChanges<bool>("use-mirror", [](bool useMirror) {
        applyServerEndpoint();

        std::string serverName = useMirror ? "www.rustps.online (Mirror)" : "rustps.online (Primary)";

        FLAlertLayer::create("RusDash", fmt::format("Current Server: <cg>{}</c>", serverName), "OK")->show();
    });
}