#pragma once
#include <Geode/Geode.hpp>

class ThemePopup : public FLAlertLayer {
protected:
    cocos2d::CCMenu* m_buttonsMenu = nullptr;
    LoadingCircle* m_circle = nullptr;

    bool init() override;
    void registerWithTouchDispatcher() override;
    void fetchAvailableThemes();
    void onSelectTheme(cocos2d::CCObject* sender);
    void onClose(cocos2d::CCObject* sender);
    void keyBackClicked() override;

public:
    static ThemePopup* create();
};