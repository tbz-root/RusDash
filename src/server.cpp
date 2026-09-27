#include "server.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/CCHttpClient.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

std::string m_www_url = "www.rustps.online/database/";
std::string m_url = "rustps.online/database/////";

class $modify(MyHttpClient, cocos2d::extension::CCHttpClient) {
    void send(cocos2d::extension::CCHttpRequest* req) {
        if (!req) {
            cocos2d::extension::CCHttpClient::send(req);
            return;
        }

        std::string url = req->getUrl();

        size_t pos1 = url.find("www.boomlings.com/database/");
        if (pos1 != std::string::npos) {
            if(Mod::get()->getSettingValue<bool>("enable-mirror")) {
                url.replace(pos1, 27, m_url);
            } else {
                url.replace(pos1, 27, m_www_url);
            }
        }
        req->setUrl(url.c_str());

        gd::vector<gd::string> headers = req->getHeaders();
        for (auto it = headers.begin(); it != headers.end();) {
            std::string headerStr = std::string(*it);
            if (headerStr.find("User-Agent:") == 0 || headerStr.find("user-agent:") == 0 || headerStr.find("User-agent:") == 0) {
                it = headers.erase(it);
            } else {
                ++it;
            }
        }

        headers.push_back("User-Agent: RusDash-Global-Agent/1.0");
        req->setHeaders(headers);

        cocos2d::extension::CCHttpClient::send(req);
    }
};

class $modify(MyApplication, cocos2d::CCApplication) {
    void openURL(const char* psz) {
        if (!psz) {
            cocos2d::CCApplication::openURL(psz);
            return;
        }

        std::string url = psz;

        size_t pos1 = url.find("www.boomlings.com/database/");
        if (pos1 != std::string::npos) {
            if(Mod::get()->getSettingValue<bool>("enable-mirror")) {
                url.replace(pos1, 27, m_url);
            } else {
                url.replace(pos1, 27, m_www_url);
            }
        }

        cocos2d::CCApplication::openURL(url.c_str());
    }
};
