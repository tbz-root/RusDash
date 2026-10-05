using namespace geode::prelude;

#define saves getMod()->getSaveContainer

namespace fs {
	using namespace std::filesystem;
	using namespace geode::cocos;
	typedef CCFileUtils cocos;
};

void resourceSetup() {
    auto resourcesDir = getMod()->getResourcesDir() / "resources";

    CCTexturePack rd;
    rd.m_id = Mod::get()->getID();
	
    rd.m_paths.push_back(string::pathToString(resourcesDir));
    rd.m_paths.push_back(string::pathToString(dirs::getGameDir()));

    fs::cocos::get()->addTexturePack(rd);

    for (const auto& entry : fs::recursive_directory_iterator(resourcesDir)) {
        if (!entry.is_regular_file())
            continue;

        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (string::containsAny(ext, std::vector<std::string>{".png", ".plist", ".fnt"})) {
            auto relativePath = fs::relative(entry.path(), resourcesDir);
            auto path = string::pathToString(relativePath);

            auto sprite = CCSprite::create(path.c_str());

            if (sprite) {
                CCSpriteFrameCache::get()->addSpriteFrame(
                    sprite->displayFrame(),
                    path.c_str()
                );
            }
        }
    }
}

#include <Geode/modify/LoadingLayer.hpp>
class $modify(LoadingLayer) {
	bool init(bool p0) {
		resourceSetup();
		return LoadingLayer::init(p0);
	}
};

#include <Geode/modify/CCSprite.hpp>
class $modify(CCSprite) {
	static CCSprite* createWithTexture(CCTexture2D* pTexture) {
		if (!pTexture) pTexture = CCSprite::create()->getTexture();
		auto rtn = CCSprite::createWithTexture(pTexture);
		return rtn ? rtn : CCSprite::create();
	}
};

#include <Geode/modify/CCSpriteFrameCache.hpp>
class $modify(CCSpriteFrameCache) {
	CCSpriteFrame* spriteFrameByName(const char* pszName) {
		if (CCKeyboardDispatcher::get()->getControlKeyPressed()) {
			return CCSpriteFrameCache::spriteFrameByName(pszName);
		}

		std::string name = pszName;

		if (GameManager::get()->m_gameLayer && GameManager::get()->m_gameLayer->isRunning()) {} else if (string::contains(name, "chain_01")) {
			name = "emptyFrame.png";
		} {
			auto frameAtSprExtName = (Mod::get()->getID() + "/" + name);
			auto test = CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(frameAtSprExtName.c_str());
			name = test ? frameAtSprExtName.c_str() : name.c_str();
			if (test) {
				CCSpriteFrameCache::get()->m_pSpriteFrames->setObject(test, pszName);
			}
		}

		if (name.find("/") != std::string::npos) {
			auto test_name = Mod::get()->getID() + "/" + string::replace(name, "/", "..");
			auto test = CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(test_name);
			name = test ? test_name.data() : name.c_str();
		}

		return CCSpriteFrameCache::spriteFrameByName(name.c_str());
	}
};