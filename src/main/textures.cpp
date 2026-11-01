using namespace geode::prelude;

#include <asp/fs.hpp>
void resourceSetup() {
    auto resourcesDir = getMod()->getResourcesDir() / "resources";

    CCTexturePack rd;
    rd.m_id = Mod::get()->getID();

    rd.m_paths.push_back(resourcesDir.string());
    rd.m_paths.push_back(dirs::getGameDir().string());

    CCFileUtils::get()->addTexturePack(rd);

    auto iterRes = asp::fs::iterDir(resourcesDir);
    if (iterRes.isErr()) {
        log::warn("resourceSetup: failed to iterate {}: {}", resourcesDir.string(), iterRes.unwrapErr());
        return;
    }

    for (auto entryRes : iterRes.unwrap()) {
        if (entryRes.isErr()) continue;
        auto& entry = entryRes.unwrap();
        if (!entry.isFile()) continue;

        auto extOpt = entry.path.extension();
        if (!extOpt) continue;

        std::string extLower(extOpt->begin(), extOpt->end());
        std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

        if (extLower == "png" || extLower == "plist" || extLower == "fnt") {
            auto nameOpt = entry.path.filename();
            if (!nameOpt) continue;
            std::string path = nameOpt->string();

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