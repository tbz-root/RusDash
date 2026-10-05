#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

class ThemePopup : public geode::Popup {
protected:
    cocos2d::CCMenu* m_buttonsMenu = nullptr;
    geode::LoadingSpinner* m_spinner = nullptr;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_fetchTask;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_selectTask;

    bool init() override;
    void fetchAvailableThemes();
    void onSelectTheme(cocos2d::CCObject* sender);
    void onClose(cocos2d::CCObject* sender) override;

public:
    static ThemePopup* create();
};