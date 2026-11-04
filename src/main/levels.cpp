using namespace geode::prelude;

#include <fstream>
#include <Geode/utils/crypto.hpp>
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
    {2, "F63F13D67EAEE6A48D5091BC9EB3452012DA0FFCD5935C0091A8B09EACE76C36"},
    {3, "9A62980B5BDAB51BE598A167194E1590AA98D579C95726815A880B8456E72514"},
    {4, "3E7B86657AFEA48402A95E92DB874D57E3FD5D117FC4E351A4EFCFBB66D79578"},
    {5, "587A6F5B7343280DDB0F6A9D499B227E9515019A78FDD309D3FC1B4714CA81E0"},
    {6, "454024D034A256DA8053A680D31B3EB276C4C0ADE7D41D7DCC859CA68BA5F353"},
    {7, "2D563E48BEA58C119677170869E0C3E1993869119860286D4AFFE5F361881E3E"},
    {8, "40692F43B3CA70F762486E25FEECC4ACD9A77798B9FF69F958D40BF1B81E6860"},
    {9, "18F9AEACFA4FF2CD93BC476AC66B5EEFF8A9E837BD2F3D7F50F8DCCE66DE5BD0"},
    {10, "B3ECD3453FD29FBED349C9FC24569CE8D121BE5FD04496EFFB0AD67167738F75"},
    {11, "29823EB793DBA21D04438A3B86DAE8E48A87175567300EB4AFF5781FF3EC6DBA"},
    {12, "967AABD790174A69E08D2D20B36238D7D8E7A28E843B4FA7196DFE393DCC2BFA"},
    {13, "911C5F96E2932594E9731B08E35D8BE281FB8A64E222B25741D3CACCF49A28F4"},
    {14, "07E7533705F2301DCBFED988FAC597F3891CC1C2CB1CB9E6AF3A8AC534D5F4E2"},
    {15, "BA0EF34B0A16D48C624B96A4C4E15858EE9B42EE700E8C541813DBD6E6E9463B"},
    {16, "C36BB3A029BE1CCE8524DF3800E6846D9F275C5A92953F48483587A6DF10B9F6"},
    {17, "F4AFAEB1CBDBE70F0D1D1A66E705A5F15784BB51835BC3EF59A476F06A953565"},
    {18, "28CE1F70966D5CCA3044F5C711D3A30404F0E97D040ED9D22353492198B645B3"},
    {19, "3B1554AA43245F82505E3EC06024BFE8A58859B01266A5FB7F432C78E6D89C56"},
    {20, "3084F80E992A7420B2292E0E7B8E1BB294D60993D98D3EF7D39088ED87F43679"},
    {21, "6277E571240D628EF87624104F2B3664C0DFA98CBF8865DD4EBC1ABEE65B9707"},
    {22, "B2942B6D99E3BE6011E8D3855FA7BCEC8C045E816EA4CA28DA95510752AA265A"},
    {17500, "500130f48b757305e1b167a08d8323818246c9a27631c126286b212cfe75a242"},
    {17501, "efb4b01535effc45a34c85be060df02f645bf3fec4799da06bda15dd82bd73e9"},
    {3001, "CB33607DBBA041BE4D840AA90B5041E7CCAD457D78FEDB0EDEF516CF6653A8FD"},
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

    auto file =
        Mod::get()->getResourcesDir() /
        "levels" /
        fmt::format("{}.txt", levelID);

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