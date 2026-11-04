using namespace geode::prelude;

#include <fstream>
#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/MenuLayer.hpp>
class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        if (matjson::parse([] { std::ifstream file((dirs::getModRuntimeDir() / Mod::get()->getID() / "mod.json").string()); return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); }()).unwrapOr(matjson::Value())["trailer"].asBool().unwrapOr(false)) {
            if (this->getChildByID("bottom-menu")->getChildByID("absolllute.installer/mega-hack-installer")) {
                this->getChildByID("bottom-menu")->getChildByID("absolllute.installer/mega-hack-installer")->setVisible(false);
            }

            this->getChildByID("bottom-menu")->updateLayout();

            if (this->getChildByID("close-menu")->getChildByID("restart-button")) {
                this->getChildByID("close-menu")->getChildByID("restart-button")->setVisible(false);
            }

            if (this->getChildByID("close-menu")->getChildByID("reload-button")) {
                this->getChildByID("close-menu")->getChildByID("reload-button")->setVisible(false);
            }

            if (this->getChildByID("close-menu")->getChildByID("smjs.gdintercept/GDI")) {
                this->getChildByID("close-menu")->getChildByID("smjs.gdintercept/GDI")->setVisible(false);
            }

            this->getChildByID("close-menu")->updateLayout();
        }

        return true;
    }
};

#include <Geode/modify/LevelInfoLayer.hpp>
class $modify(LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge))
            return false;

        if (matjson::parse([] { std::ifstream file((dirs::getModRuntimeDir() / Mod::get()->getID() / "mod.json").string()); return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); }()).unwrapOr(matjson::Value())["trailer"].asBool().unwrapOr(false)) {
            if (this->getChildByID("left-side-menu")->getChildByID("hjfod.gdshare/export-button")) {
                this->getChildByID("left-side-menu")->getChildByID("hjfod.gdshare/export-button")->setVisible(false);
            }

            this->getChildByID("left-side-menu")->updateLayout();
        }

        return true;
    }
};