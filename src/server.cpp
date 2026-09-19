using namespace geode::prelude;

#include "server.hpp"
#include <Geode/modify/CCHttpClient.hpp>

std::string m_url;
class $modify(CCHttpClient) {
    void send(CCHttpRequest* req) {
        std::string url = req->getUrl();

        url = string::replace(url, "www.boomlings.com/database/", m_url);
        url = string::replace(url,"boomlings.com/database/", m_url);

        req->setUrl(url.c_str());

        return CCHttpClient::send(req);
    }
};

#include <Geode/modify/CCApplication.hpp>
class $modify(CCApplication) {
    void openURL(const char* psz) {
        std::string url = psz;

        url = string::replace(url, "www.boomlings.com/database/", m_url);
        url = string::replace(url, "boomlings.com/database/", m_url);

        return CCApplication::openURL(url.c_str());
    }
};