#include <string_view>
#include "DevRateStarsPopup.hpp"
namespace {
    std::string_view getLevelDifficulty(int stars) {
        constexpr std::string_view difficulties[] = {
            "na", "auto", "easy", "normal", "hard", "hard",
            "harder", "harder", "insane", "insane", "demon"
        };
        
        return (stars >= 0 && stars <= 10) ? difficulties[stars] : "na";
    }
}

bool DevRateStarsPopup::init(int levelId) {
    if (!Popup::init(330.f, 170.f))
        return false;

    m_levelId = levelId;

    this->setTitle("Dev: Rate Stars", "bigFont.fnt", 1.0f);
    m_title->setPositionY(m_title->getPositionY() - 5.f);

    this->setID("dev-rate-stars-popup"_spr);

    /*
        FEATURE BUTTON LOGIC
    */
    m_featureButton = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_featuredCoin_001.png"),
        this,
        menu_selector(DevRateStarsPopup::onFeature)
    );
    m_featureButton->setOpacity(DISABLED_OPACITY);
    m_featureButton->setID("feature-button"_spr);
    m_buttonMenu->addChildAtPosition(m_featureButton, Anchor::TopRight, { -10.f, -10.f });

    /*
        COINS BUTTON LOGIC
    */
    auto *coinsSprite = CCSprite::createWithSpriteFrameName("secretCoinUI2_001.png");
    coinsSprite->setScale(44.f / 53.75f);

    m_coinsButton = CCMenuItemSpriteExtra::create(
        coinsSprite,
        this,
        menu_selector(DevRateStarsPopup::onCoins)
    );
    m_coinsButton->setOpacity(DISABLED_OPACITY);
    m_coinsButton->setID("coins-button"_spr);
    m_buttonMenu->addChildAtPosition(m_coinsButton, Anchor::BottomRight, { -10.f, 10.f });

    /*
        LOWER STARS BUTTON LOGIC
    */
    auto *decreaseStarsButton = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png"),
        this,
        menu_selector(DevRateStarsPopup::onDecrease)
    );
    decreaseStarsButton->setRotation(-90.f);
    decreaseStarsButton->setID("decrease-stars-button"_spr);
    m_buttonMenu->addChildAtPosition(decreaseStarsButton, Anchor::Left, { 85.f, 0.f });

    /*
        STARS LABEL LOGIC
    */
    m_starsLabel = CCLabelBMFont::create("0", "bigFont.fnt");
    m_starsLabel->setID("stars-label"_spr);
    m_buttonMenu->addChildAtPosition(m_starsLabel, Anchor::Left, { 130.f, 0.f });

    /*
        INCREASE STARS BUTTON LOGIC
    */
    auto *increaseStarsButton = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png"),
        this,
        menu_selector(DevRateStarsPopup::onIncrease)
    );
    increaseStarsButton->setRotation(90.f);
    increaseStarsButton->setID("increase-stars-button"_spr);
    m_buttonMenu->addChildAtPosition(increaseStarsButton, Anchor::Left, { 175.f, 0.f });

    /*
        DIFFICULTY FACE LOGIC
    */
    m_difficultyFace = CCSprite::createWithSpriteFrameName("difficulty_00_btn_001.png");
    m_difficultyFace->setScale(1.25f);
    m_difficultyFace->setID("difficulty-face"_spr);
    m_buttonMenu->addChildAtPosition(m_difficultyFace, Anchor::Right, { -85.f, 0.f });

    /*
        DAILY BUTTON LOGIC
    */
    if (!Mod::get()->getSettingValue<bool>("disable-daily-button")) {
        auto *dailySprite = ButtonSprite::create(
            "Set\nDaily", 50, true, "goldFont.fnt", "GJ_button_01.png", 40.f, 1.0f
        );
        dailySprite->setScale(0.6f);

        auto *dailyButton = CCMenuItemSpriteExtra::create(
            dailySprite,
            this,
            menu_selector(DevRateStarsPopup::onDaily)
        );
        dailyButton->setID("daily-button"_spr);
        m_buttonMenu->addChildAtPosition(dailyButton, Anchor::Left, { 30.f, 15.f });
    }

    /*
        WEEKLY BUTTON LOGIC
    */
    if (!Mod::get()->getSettingValue<bool>("disable-weekly-button")) {
        auto *weeklySprite = ButtonSprite::create(
            "Set\nWeek", 50, true, "goldFont.fnt", "GJ_button_01.png", 40.f, 1.0f
        );
        weeklySprite->setScale(0.6f);

        auto *weeklyButton = CCMenuItemSpriteExtra::create(
            weeklySprite,
            this,
            menu_selector(DevRateStarsPopup::onWeekly)
        );
        weeklyButton->setID("weekly-button"_spr);
        m_buttonMenu->addChildAtPosition(weeklyButton, Anchor::Left, { 30.f, -15.f });
    }

    /*
        SEND ONLY BUTTON LOGIC
    */
    if (!Mod::get()->getSettingValue<bool>("disable-send-only-button")) {
        auto *sendOnlySprite = ButtonSprite::create(
            "Send\nOnly", 50, true, "goldFont.fnt", "GJ_button_01.png", 40.f, 1.0f
        );
        sendOnlySprite->setScale(0.6f);

        auto *sendOnlyButton = CCMenuItemSpriteExtra::create(
            sendOnlySprite,
            this,
            menu_selector(DevRateStarsPopup::onSendOnly)
        );
        sendOnlyButton->setID("send-only-button"_spr);
        m_buttonMenu->addChildAtPosition(sendOnlyButton, Anchor::Right, { -30.f, -30.f });
    }

    /*
        CANCEL BUTTON LOGIC
    */
    auto *cancelButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Cancel", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(DevRateStarsPopup::onCancel)
    );
    cancelButton->setID("cancel-button"_spr);
    m_buttonMenu->addChildAtPosition(cancelButton, Anchor::Bottom, { -60.f, 25.f });
    
    /*
        SUBMIT BUTTON LOGIC
    */
    auto *submitButton = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Submit", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(DevRateStarsPopup::onSubmit)
    );
    submitButton->setID("submit-button"_spr);
    m_buttonMenu->addChildAtPosition(submitButton, Anchor::Bottom, { 60.f, 25.f });

    return true;
};

void DevRateStarsPopup::onFeature(CCObject *) {
    auto *sprite = static_cast<CCSprite *>(m_featureButton->getNormalImage());
    auto *spriteCache = CCSpriteFrameCache::sharedSpriteFrameCache();

    m_featureState = static_cast<FeatureState>((static_cast<int>(m_featureState) + 1) % 5);

    char const *frame = "GJ_featuredCoin_001.png";
    switch (m_featureState) {
        case FeatureState::Epic:
            frame = "GJ_epicCoin_001.png";
            break;
        case FeatureState::Legendary:
            frame = "GJ_epicCoin2_001.png";
            break;
        case FeatureState::Mythic:
            frame = "GJ_epicCoin3_001.png";
            break;
        default:
            break;
    }
    
    sprite->setDisplayFrame(spriteCache->spriteFrameByName(frame));
    m_featureButton->setOpacity(m_featureState == FeatureState::None ? DISABLED_OPACITY : ENABLED_OPACITY);
}

void DevRateStarsPopup::onCoins(CCObject *) {
    m_coinsEnabled = !m_coinsEnabled;
    m_coinsButton->setOpacity(m_coinsEnabled ? ENABLED_OPACITY : DISABLED_OPACITY);
}

void DevRateStarsPopup::onDecrease(CCObject *) {
    if (m_selectedStars <= 0)
        return;

    m_selectedStars--;
    updateDifficultyVisuals(m_selectedStars);
}

void DevRateStarsPopup::onIncrease(CCObject *) {
    if (m_selectedStars >= 10)
        return;

    m_selectedStars++;
    updateDifficultyVisuals(m_selectedStars);
}

void DevRateStarsPopup::onDaily(CCObject *) {
    GameLevelManager::sharedState()->uploadComment(
        "!daily", CommentType::Level, m_levelId, 0
    );

    this->onClose(nullptr);
}

void DevRateStarsPopup::onWeekly(CCObject *) {
    GameLevelManager::sharedState()->uploadComment(
        "!weekly", CommentType::Level, m_levelId, 0
    );

    this->onClose(nullptr);
}

void DevRateStarsPopup::onSendOnly(CCObject *) {
    if (m_selectedStars > 0) {
        GameLevelManager::sharedState()->uploadComment(
            fmt::format(
                "!send {} {} {}",
                getLevelDifficulty(m_selectedStars),
                m_selectedStars,
                static_cast<int>(m_featureState)
            ),
            CommentType::Level, m_levelId, 0
        );
    } else {
        GameLevelManager::sharedState()->uploadComment(
            "!unsend", CommentType::Level, m_levelId, 0
        );
    }

    this->onClose(nullptr);
}

void DevRateStarsPopup::onCancel(CCObject *) {
    this->onClose(nullptr);
}

void DevRateStarsPopup::onSubmit(CCObject *) {
    if (m_selectedStars > 0) {
        GameLevelManager::sharedState()->uploadComment(
            fmt::format(
                "!rate {} {} {} {}",
                getLevelDifficulty(m_selectedStars),
                m_selectedStars,
                m_coinsEnabled ? 1 : 0,
                static_cast<int>(m_featureState)
            ),
            CommentType::Level, m_levelId, 0
        );
    } else {
        GameLevelManager::sharedState()->uploadComment(
            "!unrate", CommentType::Level, m_levelId, 0
        );
    }

    this->onClose(nullptr);
}

void DevRateStarsPopup::updateDifficultyVisuals(int stars) {
    auto *spriteCache = CCSpriteFrameCache::sharedSpriteFrameCache();

    static constexpr std::array difficultyFrames {
        "difficulty_00_btn_001.png", // N/A
        "difficulty_auto_btn_001.png", // Auto
        "difficulty_01_btn_001.png", // Easy
        "difficulty_02_btn_001.png", // Normal
        "difficulty_03_btn_001.png", // Hard
        "difficulty_03_btn_001.png", // Hard
        "difficulty_04_btn_001.png", // Harder
        "difficulty_04_btn_001.png", // Harder
        "difficulty_05_btn_001.png", // Insane
        "difficulty_05_btn_001.png", // Insane
        "difficulty_06_btn_001.png" // Demon
    };

    auto frame = (stars >= 0 && stars < difficultyFrames.size())
        ? difficultyFrames[stars] : "difficulty_00_btn_001.png";

    m_difficultyFace->setDisplayFrame(spriteCache->spriteFrameByName(frame));
    m_starsLabel->setString(std::to_string(stars).c_str());
}

DevRateStarsPopup *DevRateStarsPopup::create(int levelId) {
    auto ret = new DevRateStarsPopup();
    if (ret->init(levelId)) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}