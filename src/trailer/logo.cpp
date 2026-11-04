using namespace geode::prelude;

#include <fstream>
#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
    bool init(GJGameLevel* level, bool dontCreateObjects) {
        if (!PlayLayer::init(level, dontCreateObjects))
            return false;

        if (matjson::parse([] { std::ifstream file((dirs::getModRuntimeDir() / Mod::get()->getID() / "mod.json").string()); return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); }()).unwrapOr(matjson::Value())["trailer"].asBool().unwrapOr(false)) {
            auto label = this->getChildByID("main-node")->getChildByID("batch-layer")->getChildByType<CCLabelBMFont>(0);
            
            label->setVisible(false);

            auto logo = CCSprite::createWithSpriteFrameName("trailerLogo.png");
            
            logo->setPosition(label->getPosition());
            logo->setScale(0.25f);
            logo->setID("trailer-logo");

            if(!this->getChildByID("main-node")->getChildByID("batch-layer")->getChildByID("trailer-logo")) {
                this->getChildByID("main-node")->getChildByID("batch-layer")->addChild(logo);
            } else {
                this->getChildByID("main-node")->getChildByID("batch-layer")->getChildByID("trailer-logo")->removeFromParentAndCleanup(true);
            }
        }

        return true;
    }
};