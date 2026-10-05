#pragma once

using namespace geode::prelude;

class DevRateStarsPopup : public geode::Popup {
protected:
    enum class FeatureState {
        None = 0,
        Featured = 1,
        Epic = 2, 
        Legendary = 3,
        Mythic = 4
    };
    
    static constexpr GLubyte DISABLED_OPACITY = 127; // 50%
    static constexpr GLubyte ENABLED_OPACITY = 255; // 100%

    int m_levelId = 0;
    CCMenuItemSpriteExtra *m_featureButton = nullptr;
    FeatureState m_featureState = FeatureState::None;
    CCMenuItemSpriteExtra *m_coinsButton = nullptr;
    bool m_coinsEnabled = false;
    CCSprite *m_difficultyFace = nullptr;
    CCLabelBMFont *m_starsLabel = nullptr;
    int m_selectedStars = 0;

    bool init(int levelId);

    void onFeature(CCObject *);
    void onCoins(CCObject *);
    void onDecrease(CCObject *);
    void onIncrease(CCObject *);
    void onDaily(CCObject *);
    void onWeekly(CCObject *);
    void onSendOnly(CCObject *);
    void onCancel(CCObject *);
    void onSubmit(CCObject *);

    void updateDifficultyVisuals(int stars);

public:
    static DevRateStarsPopup *create(int levelId);
};