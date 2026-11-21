using namespace geode::prelude;

#include <Geode/utils/crypto.hpp>
#include <unordered_map>
#include <fstream>
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

    std::ifstream hashFile(path.string(), std::ios::binary);
    if (hashFile.is_open()) {
        Sha256Hasher configHasher;
        std::array<char, 8192> buffer;
        while (hashFile.read(buffer.data(), buffer.size()) || hashFile.gcount() > 0) {
            configHasher.update(buffer.data(), static_cast<size_t>(hashFile.gcount()));
        }
        std::string currentHash = configHasher.finish().toString();

        std::string expectedHash = "63d7af438fd105950a2a09bebe2b77e8a7e8e2451a638e3657de7b9f93303223";

        if (currentHash != expectedHash) {
            return levels;
        }
    }

    std::ifstream file(path.string());
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
    {1, "BAAC8024B41F90E5D08E519F4B51B423E2038853AECB3CE59C6E428D1B49AF12"},
    {2, "3BF1E1BAA73216280E96C93330A85654B9C4F0B6C2EFEF7B2F477F18A48F6FCB"},
    {3, "D0883B50D27ABF700D3B6EB152F4F2153F4F80E4245323C235558A47262EE5A8"},
    {4, "9139D1D3C505141E0CAAED204CC2BA92FADB1C8CBD9F2D411432FD0F52C32C0F"},
    {5, "2A2143235EC46696C9077C14A3721DDC88547F81356BC53B9093EB66D48B5E43"},
    {6, "F89F71FC36E256D7E96D925E04822583F10871CC21059F339D1CCB68076DA044"},
    {7, "99749F16E7B8B3211DE387FF104BC801591357D620B6E04F3F3F91DD089C263A"},
    {8, "C45C30E0BA45D127E7C0C087C6AF94F510C6A92F3FCFA6D17BF023BD3ED1CB5D"},
    {9, "61F3EC6ACDD8B98061EF381CFBF6B80FD45F10FC61CA20C181DA468D5ACE34C3"},
    {10, "18041CCD52F2D5B15AE0858A850D2D5DBDC04800F8268A17B945674EECFD9827"},
    {11, "05FDCAEBF2B7480E9142D509164A874C6ECABC33FA4F490EE6325A0725032A21"},
    {12, "33BDC4163E047B1CEA6FFD1441E63019508AC80CFE826CD63065D130F9A56C03"},
    {13, "EBC332DC7B0D9119F1D28BBD014F718CEC60042CA5D3E7B63CDEB26E7C550399"},
    {14, "D50F1CD9AD2228759F64DDB871985CAB94FD8FC4FD2CBF072B2D316CE2CAC8B3"},
    {15, "F4569B07E9F8F68498A5380343D957C0F9D8739135E392CDC634A07E44A7F5D6"},
    {16, "EA8DBB33A95307F74E0F2B4E3DEB710401D3F9C0B7FA88EA6E73E9DB6D2EDD55"},
    {17, "308BF3E58723D7601044AD2B30401B1AD4291C85A36BB9F7B4D4123CFEB896B2"},
    {18, "F96306B478C6C2856FD2F186053EA04B461BEA01F390C67D9DAA8938C2EF4105"},
    {19, "F0614C3D10C0E779EA2C09AFCB9877D8486841AF5B59A19327A0FDF99C6AB268"},
    {20, "4F2A1AAC70ABCD6547158B0FC7333BCC0BC97BABBE38407804214661B70FC07B"},
    {21, "74A865F457B6E275803EC81D8F8FB8456B32F017EF731A0E3DD27D5DC278EFB6"},
    {22, "B2942B6D99E3BE6011E8D3855FA7BCEC8C045E816EA4CA28DA95510752AA265A"},
    {17500, "500130f48b757305e1b167a08d8323818246c9a27631c126286b212cfe75a242"},
    {17501, "efb4b01535effc45a34c85be060df02f645bf3fec4799da06bda15dd82bd73e9"},
    {3001, "0229C5A3792A9E28032E60F9971D8808244E61320D3A44AA65D1FB58B22C5D7E"},
    {5001, "B25A37281E632BF3A369F05298D8E2BC49C3D5B445A53F16C01795E59FA3D4B6"},
    {5002, "C9DB9E016DE3E95BFA37B7272ED9EFBBDEE6F00E1657ED3992EF810AEA8501A2"},
    {5003, "2025625A20170E025CD0FC34C63CC054BD92DA5CB0A6C09B711EA706BD69B77F"},
    {5004, "4F7BE8D5E3AAA41ED010872D847E37A04ACB423587C5D054A4B5A6EB97B67C6F"},
};

static bool verifyCustomLevel(int levelID) {
    auto it = g_levelHashes.find(levelID);

    if (it == g_levelHashes.end()) {
        log::error("No hash registered for level {}", levelID);
        return false;
    }

    auto file = Mod::get()->getResourcesDir() / "levels" / fmt::format("{}.txt", levelID);

    std::ifstream stream(file.string(), std::ios::binary);

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
        log::error("Level {} integrity check failed", levelID);
        log::debug("Level {}: expected={}, calculated={}", levelID, expectedHash, calculatedHash);

        return false;
    }

    return true;
}

#include <Geode/modify/LevelTools.hpp>
class $modify(LevelTools) {
    static bool verifyLevelIntegrity(std::string verifyString, int levelID) {
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

static std::string getAudioFileName(int id) {
        if (id == 17500) {
            return (Mod::get()->getResourcesDir() / "../" / "UltimateDestruction.mp3").string();
        }

        if (id == 17501) {
            return (Mod::get()->getResourcesDir() / "../" / "Active.mp3").string();
        }

        return LevelTools::getAudioFileName(id);
    }

    static std::string getAudioTitle(int id) {
        if (id == 17500) return "Ultimate Destruction";
        if (id == 17501) return "Active";
        return LevelTools::getAudioTitle(id);
    }

    static SongInfoObject* getSongObject(int id) {
        if (id == 17500 || id == 17501) {
            auto song = SongInfoObject::create(id, getAudioTitle(id), id == 17500 ? "TMM43" : "Unknown", 0, 0.f, "", 0, "", false, 0, 0);
            return song;
        }

        return LevelTools::getSongObject(id);
    }
};

#include <vector>
#include <algorithm>
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

        static auto levels = loadLevels();

        std::vector<int> levelIDs;
        levelIDs.reserve(levels.size());

        for (auto const& [id, data] : levels) {
            levelIDs.push_back(id);
        }

        std::sort(levelIDs.begin(), levelIDs.end());

        for (int id : levelIDs) {
            auto level = GameLevelManager::get()->getMainLevel(id, true);

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
        m_scrollLayer->repositionPagesLooped();

        return true;
    }
};

#include <Geode/modify/LocalLevelManager.hpp>
class $modify(LocalLevelManager) {
    std::string getMainLevelString(int id) {
        asp::fs::Path file = Mod::get()->getResourcesDir() / "levels" / fmt::format("{}.txt", id);

        std::ifstream stream(file.string(), std::ios::binary);

        if (!stream.is_open()) {
            return LocalLevelManager::getMainLevelString(id);
        }

        std::stringstream buffer;
        buffer << stream.rdbuf();
        std::string content = buffer.str();

        content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

        return std::string(content.c_str());
    }
};

#include <Geode/modify/AchievementManager.hpp>
class $modify(AchievementManager) {
    void addManualAchievements() {
        AchievementManager::addManualAchievements();

        this->addAchievement(
            "geometry.ach.ultimate.struction",
            "Ultimate Struction",
            "Complete 'Ultimate Destruction' in Practice mode",
            "Complete 'Ultimate Destruction' in Practice mode",
            "color_88_001.png",
            0
        );

        this->addAchievement(
            "geometry.ach.ultimate.destruction",
            "Ultimate Destruction",
            "Complete 'Ultimate Destruction' in Normal mode",
            "Complete 'Ultimate Destruction' in Normal mode",
            "player_486_001.png",
            0
        );

        this->addAchievement(
            "geometry.ach.active.inactive",
            "Inactive",
            "Complete 'Active' in Practice mode",
            "Complete 'Active' in Practice mode",
            "color_55_001.png",
            0
        );

        this->addAchievement(
            "geometry.ach.active",
            "Active",
            "Complete 'Active' in Normal mode",
            "Complete 'Active' in Normal mode",
            "player_487_001.png",
            0
        );
    }
};

#include <Geode/binding/GameManager.hpp>
#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayer) {
    void levelComplete() {
        PlayLayer::levelComplete();

        if (!m_level) return;

        if (m_level->m_levelID == 17500) {
            if (m_isPracticeMode) {
                if (!AchievementManager::sharedState()->isAchievementEarned("geometry.ach.ultimate.struction")) {
                    AchievementManager::sharedState()->reportAchievementWithID("geometry.ach.ultimate.struction", 100, false);
                    GameManager::sharedState()->unlockColor(88, UnlockType::Col1);
                }
            } else {
                if (!AchievementManager::sharedState()->isAchievementEarned("geometry.ach.ultimate.destruction")) {
                    AchievementManager::sharedState()->reportAchievementWithID("geometry.ach.ultimate.destruction", 100, false);
                    GameManager::sharedState()->unlockIcon(486, IconType::Cube);
                }
            }
        }
        else if (m_level->m_levelID == 17501) {
            if (m_isPracticeMode) {
                if (!AchievementManager::sharedState()->isAchievementEarned("geometry.ach.active.inactive")) {
                    AchievementManager::sharedState()->reportAchievementWithID("geometry.ach.active.inactive", 100, false);
                    GameManager::sharedState()->unlockColor(55, UnlockType::Col1);
                }
            } else {
                if (!AchievementManager::sharedState()->isAchievementEarned("geometry.ach.active")) {
                    AchievementManager::sharedState()->reportAchievementWithID("geometry.ach.active", 100, false);
                    GameManager::sharedState()->unlockIcon(487, IconType::Cube);
                }
            }
        }
    }
};