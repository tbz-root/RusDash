#include <Geode/Geode.hpp>
#include <fstream>
#include <unordered_map>

using namespace geode::prelude;

struct LevelData {
    std::string levelName;
    GJDifficulty difficulty = GJDifficulty::Normal;
    int stars = 0;
    int requiredCoins = 0;
    int audioTrack = 0;
};

static int g_levelsCount = 0;

static std::unordered_map<int, LevelData> loadLevels() {
    std::unordered_map<int, LevelData> levels;

    auto path = Mod::get()->getResourcesDir() / "levels.txt";

    std::ifstream file(path);
    if (!file.is_open()) {
        log::error("Failed to open {}", path);
        return levels;
    }

    std::string line;
    int currentID = -1;
    bool inConfig = false;

    LevelData current;

    auto saveCurrent = [&]() {
        if (currentID != -1) {
            levels[currentID] = current;
        }
    };

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        if (line.front() == '[' && line.back() == ']') {
            saveCurrent();

            std::string section =
                line.substr(1, line.size() - 2);

            if (section == "Config") {
                currentID = -1;
                inConfig = true;
            }
            else {
                currentID = std::stoi(section);
                inConfig = false;
                current = LevelData{};
            }

            continue;
        }

        auto pos = line.find('=');

        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        while (!key.empty() && key.front() == ' ')
            key.erase(key.begin());

        while (!key.empty() && key.back() == ' ')
            key.pop_back();

        while (!value.empty() && value.front() == ' ')
            value.erase(value.begin());

        while (!value.empty() && value.back() == ' ')
            value.pop_back();

        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {

            value = value.substr(1, value.size() - 2);
        }

        if (inConfig) {
            if (key == "levels") {
                g_levelsCount = std::stoi(value);

                log::info(
                    "Configured levels: {}",
                    g_levelsCount
                );
            }

            continue;
        }

        if (key == "m_levelName") {
            current.levelName = value;
        }

        else if (key == "m_difficulty") {
            if (value == "GJDifficulty::NA")
                current.difficulty = GJDifficulty::NA;
            else if (value == "GJDifficulty::Easy")
                current.difficulty = GJDifficulty::Easy;
            else if (value == "GJDifficulty::Normal")
                current.difficulty = GJDifficulty::Normal;
            else if (value == "GJDifficulty::Hard")
                current.difficulty = GJDifficulty::Hard;
            else if (value == "GJDifficulty::Harder")
                current.difficulty = GJDifficulty::Harder;
            else if (value == "GJDifficulty::Insane")
                current.difficulty = GJDifficulty::Insane;
            else if (value == "GJDifficulty::Demon")
                current.difficulty = GJDifficulty::Demon;
            else if (value == "GJDifficulty::DemonEasy")
                current.difficulty = GJDifficulty::DemonEasy;
            else if (value == "GJDifficulty::DemonMedium")
                current.difficulty = GJDifficulty::DemonMedium;
            else if (value == "GJDifficulty::DemonInsane")
                current.difficulty = GJDifficulty::DemonInsane;
            else if (value == "GJDifficulty::DemonExtreme")
                current.difficulty = GJDifficulty::DemonExtreme;
        }

        else if (key == "m_stars") {
            current.stars = std::stoi(value);
        }

        else if (key == "m_requiredCoins") {
            current.requiredCoins = std::stoi(value);
        }

        else if (key == "m_audioTrack") {
            current.audioTrack = std::stoi(value);
        }
    }

    saveCurrent();

    return levels;
}

#include <Geode/modify/LevelTools.hpp>
class $modify(LevelTools) {
    static bool verifyLevelIntegrity(
        gd::string verifyString,
        int levelID
    ) {
        return true;
    }

    static GJGameLevel* getLevel(int levelID, bool loaded) {
        auto level = LevelTools::getLevel(levelID, loaded);

        static auto levels = loadLevels();

        auto it = levels.find(levelID);

        if (it == levels.end()) {
            return level;
        }

        auto& data = it->second;

        level->m_levelID = levelID;
        level->m_levelType = GJLevelType::Main;
        level->m_levelName = data.levelName;
        level->m_difficulty = data.difficulty;
        level->m_stars = data.stars;
        level->m_requiredCoins = data.requiredCoins;
        level->m_audioTrack = data.audioTrack;

        return level;
    }
};

#include <Geode/modify/LevelSelectLayer.hpp>
class $modify(LevelSelectLayer) {
    bool init(int page) {
        if (!LevelSelectLayer::init(page))
            return false;

        m_scrollLayer->m_dynamicObjects->removeAllObjects();

        auto dotsArray =
            CCArrayExt<CCSprite*>(m_scrollLayer->m_dots);

        for (CCSprite* dot : dotsArray) {
            dot->removeFromParent();
        }

        m_scrollLayer->m_dots->removeAllObjects();

        for (int i = 1; i <= g_levelsCount; i++) {
            auto level =
                GameLevelManager::get()
                    ->getMainLevel(i, true);

            if (level) {
                m_scrollLayer->m_dynamicObjects
                    ->addObject(level);
            }
        }

        auto comingSoon = GJGameLevel::create();
        comingSoon->m_levelID = -1;

        auto theTower = GJGameLevel::create();
        theTower->m_levelID = -2;

        m_scrollLayer->m_dynamicObjects->addObject(comingSoon);
        m_scrollLayer->m_dynamicObjects->addObject(theTower);

        auto batchNode = CCSpriteBatchNode::create("smallDot.png", 29);

        m_scrollLayer->addChild(batchNode, 5);

        for (int i = 0; i < m_scrollLayer->m_dynamicObjects->count(); i++) {
            auto sprite = CCSprite::create("smallDot.png");

            batchNode->addChild(sprite);

            m_scrollLayer->m_dots->addObject(sprite);
        }

        m_scrollLayer->updateDots(0.f);
        m_scrollLayer->updatePages();

        this->updatePageWithObject(
            m_scrollLayer->m_pages->objectAtIndex(page % 3),

            m_scrollLayer->m_dynamicObjects->objectAtIndex(page)
        );

        m_scrollLayer->repositionPagesLooped();

        return true;
    }
};

#include <Geode/modify/LocalLevelManager.hpp>
class $modify(LocalLevelManager) {
    gd::string getMainLevelString(int id) {
        auto file = Mod::get()->getResourcesDir() / "levels" / fmt::format("{}.txt", id);

        std::ifstream stream(file);

        if (!stream.is_open()) {
            return LocalLevelManager::getMainLevelString(id);
        }

        std::stringstream buffer;
        buffer << stream.rdbuf();

        return gd::string(buffer.str());
    }
};