using namespace geode::prelude;

#include "SettingsLayer.hpp"
#include <Geode/modify/OptionsLayer.hpp>
class $modify(OptionsLayer) {
	void onOptions(CCObject* sender) {
		SettingsLayer::create()->show();
	}
};