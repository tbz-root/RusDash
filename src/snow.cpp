using namespace geode::prelude;

#include <ctime>
#include <Geode/modify/MenuLayer.hpp>
class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        std::time_t now = std::time(nullptr);
        std::tm local{};
        
        if (Mod::get()->getSettingValue<bool>("winter-snow") && localtime_s(&local, &now) == 0 && (local.tm_mon == 11 || local.tm_mon == 0 || local.tm_mon == 1)) {
            auto* snow = CCParticleSnow::createWithTotalParticles(700);
            snow->setID("snow"_spr);

            this->addChild(snow, 999);
        }

        return true;
    }
};