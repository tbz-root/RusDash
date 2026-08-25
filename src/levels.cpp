using namespace geode::prelude;

#include <fstream>
#include <unordered_map>
struct LevelData {
    std::string levelName;
    GJDifficulty difficulty = GJDifficulty::Normal;
    int stars = 0;
    int requiredCoins = 0;
    int audioTrack = 0;
};

static int g_levelsCount = 0;

#include <Geode/loader/Log.hpp>
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
                currentID = utils::numFromString<int>(section).unwrapOr(0);;
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
                g_levelsCount = utils::numFromString<int>(value).unwrapOr(0);
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
            current.stars = utils::numFromString<int>(value).unwrapOr(0);;
        }

        else if (key == "m_requiredCoins") {
            current.requiredCoins = utils::numFromString<int>(value).unwrapOr(0);;
        }

        else if (key == "m_audioTrack") {
            current.audioTrack = utils::numFromString<int>(value).unwrapOr(0);;
        }
    }

    saveCurrent();

    return levels;
}

static const std::unordered_map<int, std::string> g_levelHashes = {
    {1, "3F0201CCE9F6C15A22FCA80680B4B7570867613C5EF58DA1B8F9C668F87B9D17"},
    {2, "C2ED951E34B59BA31EE7542F0CBF2D1E89DFE90114BBB406690D7F4A631473BC"},
    {3, "FBB3503AF26BD9C32194E88DEC78F86027681B5F457A9D9592BE6608E60C9582"},
    {4, "BE5B7200A207A5B6D3247619CF35DD032769BE6E52E2A4CC1BAE89603B5EB20A"},
    {5, "41B06E184E5FB52B735EC18A1F2A654FAB7D30EC41EE10506ED8FB7F00C835B9"},
    {6, "DDB45D67B24DD0FF2B3D3F993787A13CA94108DB625DFC1A6C7E248AC9BA4E65"},
    {7, "B4F2D25067FA09534E993BF876621971C94C1B5AC62874EB6BD8A47F1374BB65"},
    {8, "4C5DEEF431531F782B4E8A45F98457D2D9F0AD244F81065A28AFCD9DDB503353"},
    {9, "9CCB5757FFF485BD870FC9F37E95EB025C0E10CCE0B41F366A933EE8348D2014"},
    {10, "AA7A3E36E39A97CFD5D8613C612362144F4875EC6927805DB453816750EFC260"},
    {11, "6209C8F2071C0582A934247C0F554C09E2A999FDA503634F7F2D16EDDD75C6C8"},
    {5001, "B25A37281E632BF3A369F05298D8E2BC49C3D5B445A53F16C01795E59FA3D4B6"},
    {5002, "C9DB9E016DE3E95BFA37B7272ED9EFBBDEE6F00E1657ED3992EF810AEA8501A2"},
    {5003, "2025625A20170E025CD0FC34C63CC054BD92DA5CB0A6C09B711EA706BD69B77F"},
    {5004, "4F7BE8D5E3AAA41ED010872D847E37A04ACB423587C5D054A4B5A6EB97B67C6F"},
};

#include <Geode/utils/hash.hpp>
static bool verifyCustomLevel(int levelID) {
    auto it = g_levelHashes.find(levelID);

    if (it == g_levelHashes.end()) {
        log::error("No hash registered for level {}", levelID);
        return false;
    }

    auto file =
        Mod::get()->getResourcesDir() /
        "levels" /
        fmt::format("{}.txt", levelID);

    std::ifstream stream(file, std::ios::binary);

    if (!stream.is_open()) {
        log::error("Failed to open level: {}", file);
        return false;
    }

    Sha256Hasher hasher;

    std::array<char, 8192> buffer;

    while (stream.read(buffer.data(), buffer.size()) ||
           stream.gcount() > 0) {
        hasher.update(
            buffer.data(),
            static_cast<size_t>(stream.gcount())
        );
    }

    auto hash = hasher.finish();

    std::string calculatedHash = hash.toString();

    std::string expectedHash = it->second;

    std::transform(
        expectedHash.begin(),
        expectedHash.end(),
        expectedHash.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    if (calculatedHash != expectedHash) {
        log::error(
            "Level {} integrity check failed",
            levelID
        );

        log::info(
            "Level {}: expected={}, calculated={}",
            levelID,
            expectedHash,
            calculatedHash
        );

        return false;
    }

    return true;
}

#include <Geode/modify/LevelTools.hpp>
class $modify(LevelTools) {
    static bool verifyLevelIntegrity(gd::string verifyString, int levelID) {
        return verifyCustomLevel(levelID);
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

        auto dotsArray = CCArrayExt<CCSprite*>(m_scrollLayer->m_dots);

        for (CCSprite* dot : dotsArray) {
            dot->removeFromParent();
        }

        m_scrollLayer->m_dots->removeAllObjects();

        for (int i = 1; i <= g_levelsCount; i++) {
            auto level = GameLevelManager::get()->getMainLevel(i, true);

            if (level) {
                m_scrollLayer->m_dynamicObjects->addObject(level);
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

        std::filesystem::path file = Mod::get()->getResourcesDir() / "levels" / fmt::format("{}.txt", id);

        std::ifstream stream(file, std::ios::binary);

        if (!stream.is_open()) {
            return LocalLevelManager::getMainLevelString(id);
        }

        std::stringstream buffer;
        buffer << stream.rdbuf();
        std::string content = buffer.str();

        content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

        return gd::string(content.c_str());
    }
};