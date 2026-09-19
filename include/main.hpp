#pragma once

#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/EditTriggersPopup.hpp>
#include <Geode/binding/GJBaseGameLayer.hpp>
#include <Geode/binding/ObjectToolbox.hpp>
#include <Geode/cocos/particle_nodes/CCParticleSystemQuad.h>

inline constexpr unsigned int __gmobfctr____fnv1a_hash(const char* str) {
    unsigned int hash = 2166136261u;
    while (*str) {
        hash ^= static_cast<unsigned char>(*str++);
        hash *= 16777619u;
    }
    return hash;
}
#define UNIQ_ID(string) (10000 + (__gmobfctr____fnv1a_hash(#string) % 90000))

/*
Example usage:

#include <user95401.game-objects-factory/include/main.hpp>

#include <Geode/Geode.hpp>
using namespace geode::prelude;

void registerCustomObjects();
$execute{ registerCustomObjects(); }
inline void registerCustomObjects() {

    // Single line deco objects register
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a1"), "d_sign_img_01_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a2"), "d_sign_img_02_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a3"), "d_sign_img_03_001.png")->registerMe();

    // Trigger object register
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createTriggerConfig(
        UNIQ_ID("test-trigger"),
        "framename.png"_spr,

        [](EffectGameObject* trigger, GJBaseGameLayer* game, int p1, std::vector<int> const* p2)
        {
            log::info("trigger callback!");
        },

        [](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects)
        {
            popup->m_buttonMenu->addChild(SimpleTextArea::create(
                "edit triggers popup init callback to setup!"
            ));
        }
    ));

    //  Trigger object with additional callbacks
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createTriggerConfig(
            UNIQ_ID("test-trigger2"),
            "frame_name.png"_spr
        )->triggerObject(
            [](EffectGameObject* trigger, GJBaseGameLayer* game, int p1, std::vector<int> const* p2) {
                log::info("trigger2 callback!");
            }
        )->customSetup(
            [](GameObject* object) {}
        )->saveString(
            [](std::string str, GameObject* object, GJBaseGameLayer* level) {
                // do something with str
                return str;
            }
        )->objectFromVector(
            [](GameObject* object, std::vector<std::string>& p0, std::vector<void*>&, void*, bool) {
                // do something with object by p0
                return object;
            }
        )
    );

    // Add simple deco object
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createDecorationObjectConfig(
        UNIQ_ID("pxa1"), "pxa1.png"_spr
    ));
    // Add deco object with detail sprite (color 2)
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createDecorationObjectConfig(
        UNIQ_ID("pxb1"), "pxb1.png"_spr, "pxb1_2.png"_spr
    ));
    // Add deco object with pulse animation
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createPulseConfig(
        UNIQ_ID("d_sign_img_03__shine"), "d_sign_img_03_001.png"
    ));
    // Add deco object with rotate animation (and vanilla rotate setup ability)
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createRotatedConfig(
        UNIQ_ID("shine_d_sign_i"), "d_sign_img_01_001.png",
        [](void*) { log::info("setup"); }
    ));
    // Add ring (orb)
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createRingConfig(
            UNIQ_ID("test-ring"),
            "d_ball_06_001.png",
            [](EnhancedGameObject* object, PlayerObject* plr) {
                plr->spiderTestJump(true); log::info("activated by player, {}, {}", object, plr);
            }
        )->onEditObjectSpecial(
            [](EditorUI* ui, GameObject* game) {
                queueInMainThread([]() // run this code after FLAlertLayer is shown to get it
                    {
                        auto alert = CCScene::get()->getChildByType<FLAlertLayer>(-1);
                        if (!alert) return;
                        auto title = alert->m_mainLayer->getChildByType<CCLabelBMFont>(0);
                        if (!title) return;
                        title->setString("My Custom Orb...");
                    }
                );
                return false; // call default onEditObjectSpecial, if you set true it will be skipped
            }
        )
    );
}

*/

/**
 * Game Objects Factory API
 *
 * This header provides a simple interface for creating custom game objects
 * that integrate seamlessly with Geometry Dash's editor and gameplay.
 *
 * All data is stored in GameManager child nodes, ensuring persistence
 * and avoiding export-based dependencies.
 */
namespace GameObjectsFactory {

    // Forward declarations
    class GameObjectConfig;
    class ConfigRegistry;
    class Manager;

    /**
     * Configuration class for custom game objects
     * All members are public for easy access and modification
     */
    class GameObjectConfig : public cocos2d::CCNode {
    public:

        // Callback functions - set these to customize behavior
        std::function<void(GameObject*)> m_customSetup = nullptr;
        std::function<void(GameObject*)> m_resetObject = nullptr;
        std::function<void(EnhancedGameObject*, PlayerObject*)> m_activatedByPlayer = nullptr;
        std::function<void(EffectGameObject*, float)> m_triggerActivated = nullptr;
        std::function<void(EffectGameObject*, GJBaseGameLayer*, int, gd::vector<int> const*)> m_triggerObject = nullptr;
        std::function<void(EditTriggersPopup*, EffectGameObject*, cocos2d::CCArray*)> m_editPopupSetup = nullptr;
        std::function<gd::string(gd::string, GameObject*, GJBaseGameLayer*)> m_saveString = nullptr;
        std::function<GameObject*(GameObject*, gd::vector<gd::string>&, gd::vector<void*>&, GJBaseGameLayer*, bool)> m_objectFromVector = nullptr;
        std::function<void(GJBaseGameLayer*, GameObject*, double, gd::vector<int> const&)> m_spawnObject = nullptr;
        std::function<bool(EditorUI*, GameObject*)> m_onEditObject = nullptr;
        std::function<bool(EditorUI*, GameObject*)> m_onEditObject2 = nullptr;
        std::function<bool(EditorUI*, GameObject*)> m_onEditObjectSpecial = nullptr;

        // Basic object properties
        int m_objectID = 0;                 // Your custom object ID
        int m_refObjectID = -1;             // Reference object to inherit from
        std::string m_spriteFrame = "";     // Sprite frame name (empty = keep from ref)
        int m_createTabBar = 0;             // Create buttons bar
        int m_tabBarInsertIndex = -1;       // Tab bar insert place (-1 = add at end)
        int m_createBtnBg = 4;              // BG for create button of this object

        CREATE_FUNC(GameObjectConfig);

        GameObjectConfig* objID(int v)         { m_objectID          = v; return this; }
        GameObjectConfig* refID(int v)         { m_refObjectID       = v; return this; }
        GameObjectConfig* frame(std::string v) { m_spriteFrame       = std::move(v); return this; }
        GameObjectConfig* tab(int v)           { m_createTabBar      = v; return this; }
        GameObjectConfig* insertIndex(int v)   { m_tabBarInsertIndex = v; return this; }
        GameObjectConfig* btnBG(int v)         { m_createBtnBg       = v; return this; }

        GameObjectConfig* customSetup(std::function<void(GameObject*)> f)
            { m_customSetup = std::move(f); return this; }
        GameObjectConfig* resetObject(std::function<void(GameObject*)> f)
            { m_resetObject = std::move(f); return this; }
        GameObjectConfig* activatedByPlayer(std::function<void(EnhancedGameObject*, PlayerObject*)> f)
            { m_activatedByPlayer = std::move(f); return this; }
        GameObjectConfig* triggerActivated(std::function<void(EffectGameObject*, float)> f)
            { m_triggerActivated = std::move(f); return this; }
        GameObjectConfig* triggerObject(std::function<void(EffectGameObject*, GJBaseGameLayer*, int, gd::vector<int> const*)> f)
            { m_triggerObject = std::move(f); return this; }
        GameObjectConfig* editPopupSetup(std::function<void(EditTriggersPopup*, EffectGameObject*, cocos2d::CCArray*)> f)
            { m_editPopupSetup = std::move(f); return this; }
        GameObjectConfig* saveString(std::function<gd::string(gd::string, GameObject*, GJBaseGameLayer*)> f)
            { m_saveString = std::move(f); return this; }
        GameObjectConfig* objectFromVector(std::function<GameObject*(GameObject*, gd::vector<gd::string>&, gd::vector<void*>&, GJBaseGameLayer*, bool)> f)
            { m_objectFromVector = std::move(f); return this; }
        GameObjectConfig* spawnObject(std::function<void(GJBaseGameLayer*, GameObject*, double, gd::vector<int> const&)> f)
            { m_spawnObject = std::move(f); return this; }
        GameObjectConfig* onEditObject(std::function<bool(EditorUI*, GameObject*)> f)
            { m_onEditObject = std::move(f); return this; }
        GameObjectConfig* onEditObject2(std::function<bool(EditorUI*, GameObject*)> f)
            { m_onEditObject2 = std::move(f); return this; }
        GameObjectConfig* onEditObjectSpecial(std::function<bool(EditorUI*, GameObject*)> f)
            { m_onEditObjectSpecial = std::move(f); return this; }

        void registerMe();
    };

    /**
     * Registry for storing all registered custom objects
     * Automatically managed by the factory system
     */
    class ConfigRegistry : public cocos2d::CCNode {
    public:
        CREATE_FUNC(ConfigRegistry);

        void registerObject(int objectID, GameObjectConfig* config) {
            while (getChildByTag(objectID)) removeChildByTag(objectID);
            config->setID(config->m_spriteFrame.c_str());
            addChild(config, 0, objectID);
        }

        GameObjectConfig* getObject(int objectID) {
            return geode::cast::typeinfo_cast<GameObjectConfig*>(getChildByTag(objectID));
        }

        bool hasObject(int objectID) {
            return getObject(objectID) != nullptr;
        }
    };

    /**
     * Main factory manager
     * Automatically created and managed in GameManager
     */
    class Manager : public cocos2d::CCNode {
    public:
        ConfigRegistry* m_registry = nullptr;
        std::string m_implID = "";

        CREATE_FUNC(Manager);

        virtual bool init() override {
            if (!CCNode::init()) return false;

            m_registry = ConfigRegistry::create();
            m_registry->setID("registry");
            this->addChild(m_registry);

            return true;
        }
    };

    /**
     * Get the main factory manager instance
     * Creates it automatically if it doesn't exist
     */
    inline Manager* getFactoryManager() {
        auto gamePresistNode = ObjectToolbox::sharedState();
        auto factory = geode::cast::typeinfo_cast<Manager*>(gamePresistNode->getChildByID(
            "object-factory-manager"
        ));

        if (!factory) {
            factory = Manager::create();
            factory->setID("object-factory-manager");
            gamePresistNode->addChild(factory);
        }

        return factory;
    }

    /**
     * Get the object registry
     */
    inline ConfigRegistry* getRegistry() {
        return getFactoryManager()->m_registry;
    }

    /**
     * Register a new custom game object
     *
     * @param objectID - Unique ID for your object (should be > 100000 to avoid conflicts)
     * @param config - Configuration object with callbacks and properties
     */
    inline void registerGameObject(GameObjectConfig* config) {
        if (!config) return;
        getRegistry()->registerObject(config->m_objectID, config);
    }
    inline void GameObjectConfig::registerMe() { GameObjectsFactory::registerGameObject(this); }

    /**
     * Get configuration for a registered object
     */
    inline GameObjectConfig* getGameObjectConfig(int objectID) {
        return getRegistry()->getObject(objectID);
    }

    /**
     * Check if an object ID is a registered custom object
     */
    inline bool isCustomObject(int objectID) {
        return getRegistry()->hasObject(objectID);
    }

    /**
     * Helper function to create a basic game object configuration
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called during object setup
     * @param refObjectID - Base object to inherit from
     */
    inline GameObjectConfig* createObjectConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(GameObject*)> setupCallback = nullptr,
        int refObjectID = -1
    ) {
        ObjectToolbox::sharedState()->m_allKeys[objectID] = spriteFrame.c_str();
        auto config = GameObjectConfig::create();
        config->m_objectID = objectID;
        config->m_refObjectID = refObjectID == -1 ? config->m_objectID : refObjectID;
        config->m_spriteFrame = spriteFrame;
        config->m_customSetup = setupCallback;
        config->m_createTabBar = 0;
        return config;
    }

    /**
     * Helper function to create a decoration game object configuration
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called during object setup
     */
    inline GameObjectConfig* createDecorationObjectConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(GameObject*)> setupCallback = nullptr
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            obj->setType(GameObjectType::Decoration);
            obj->m_isDecoration = true;
            obj->m_customSpriteColor = true;
            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk);
        config->refID(211);
        config->m_createTabBar = 10;
        return config;
    }

    /**
     * Helper function to create a decoration game object configuration
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param spriteFrameDetail - Detail sprite frame for the object
     * @param setupCallback - Function called during object setup
     */
    inline GameObjectConfig* createDecorationObjectConfig(
        int objectID,
        const std::string& spriteFrame,
        const std::string& spriteFrameDetail,
        std::function<void(GameObject*)> setupCallback = nullptr
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            obj->setType(GameObjectType::Decoration);
            obj->m_isDecoration = true;

            if (!spriteFrameDetail.empty())
                if (obj->m_colorSprite) obj->m_colorSprite->initWithSpriteFrameName(
                    spriteFrameDetail.c_str()
                );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk);
        config->refID(952);
        config->m_createTabBar = 0;
        config->m_tabBarInsertIndex = 4 + (45 * 12);
        return config;
    }

    /**
     * Helper function to create a configuration for basic trigger
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object (optional)
     * @param triggerCallback - Function called when trigger activates (optional)
     * @param editCallback - Function called when editing the trigger (optional)
     */
    inline GameObjectConfig* createTriggerConfig(
        int objectID,
        const std::string& spriteFrame = "edit_eEventLinkBtn_001.png",
        std::function<void(EffectGameObject*, GJBaseGameLayer*, int, gd::vector<int> const*)> triggerCallback = nullptr,
        std::function<void(EditTriggersPopup*, EffectGameObject*, cocos2d::CCArray*)> editCallback = nullptr
    ) {
        auto config = createObjectConfig(objectID, spriteFrame);
        config->m_objectID = objectID;
        config->m_refObjectID = 1613; // show player trigger
        config->m_spriteFrame = spriteFrame;
        config->m_triggerObject = triggerCallback;
        config->m_editPopupSetup = editCallback;
        config->m_createTabBar = 12;
        config->m_tabBarInsertIndex = 5 + (12 * 7);
        return config;
    }

    /**
     * Helper function to create a configuration for custom ring
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createRingConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 1594
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 16;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

    /**
     * Helper function to create a configuration for custom dash ring
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createDashRingConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 1704
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 16;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

    /**
     * Helper function to create a configuration for custom pad
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createPadConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 140
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 4;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

     /**
     * Helper function to create a configuration for custom speed portal
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createSpeedPortalConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 1334
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 41;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

    /**
     * Helper function to create a configuration for custom gamemode portal (also can be a random portal)
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createGamemodePortalConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 13
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 44;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

    /**
     * Helper function to create a configuration for custom gravity portal
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setupCallback - Function called on setup
     * @param activatedByPlayer - Function called when activates
     * @param spriteFrameDetail - Detail frame (only if refer id is 1594!)
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createGravityPortalConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(EnhancedGameObject*, PlayerObject*)> activatedByPlayer = nullptr,
        std::function<void(GameObject*)> setupCallback = nullptr,
        const std::string& spriteFrameDetail = "emptyGlow.png",
        int refObjectID = 2926
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (spriteFrameDetail.size()) if (auto a = obj->m_colorSprite) a->initWithSpriteFrameName(
                spriteFrameDetail.c_str()
            );

            if (setupCallback) setupCallback(obj);
            };
        auto config = createObjectConfig(objectID, spriteFrame, cllbk, refObjectID);
        config->m_createTabBar = 5; //portal-tab
        config->m_tabBarInsertIndex = 19;
        config->m_activatedByPlayer = activatedByPlayer;
        return config;
    }

    /**
     * Helper function to create a configuration for collectible object
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setup - Function called on setup
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createCollectibleConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(GameObject*)> setup = nullptr,
        int refObjectID = 4539
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (auto a = obj->getChildByType<cocos2d::CCSprite>(0)) a->initWithSpriteFrameName(
                spriteFrame.c_str()
            );
            if (setup) setup(obj);
            };
        auto config = createObjectConfig(objectID, "", cllbk, refObjectID);
        config->m_createTabBar = 9;
        config->m_tabBarInsertIndex = 150;
        return config;
    }

    /**
     * Helper function to create a configuration for object with rotation
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setup - Function called on setup
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createRotatedConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(GameObject*)> setup = nullptr,
        int refObjectID = 396
    ) {
        auto cllbk = [=](GameObject* obj) mutable {
            if (auto a = obj->getChildByType<cocos2d::CCSprite>(0)) a->initWithSpriteFrameName(
                spriteFrame.c_str()
            );
            if (setup) setup(obj);
            };
        auto config = createObjectConfig(objectID, "", cllbk, refObjectID);
        config->m_createTabBar = 11;
        config->m_tabBarInsertIndex = 4 + (12 * 6);
        return config;
    }

    /**
     * Helper function to create a configuration for object with pulse
     *
     * @param objectID - Your custom object ID
     * @param spriteFrame - Sprite frame for the object
     * @param setup - Function called on setup
     * @param refObjectID - Refer obj
     */
    inline GameObjectConfig* createPulseConfig(
        int objectID,
        const std::string& spriteFrame = "",
        std::function<void(GameObject*)> setup = nullptr,
        int refObjectID = 52
    ) {
        auto config = createObjectConfig(objectID, spriteFrame, setup, refObjectID);
        config->m_createTabBar = 11;
        config->m_tabBarInsertIndex = 2 + (12 * 8);
        return config;
    }
}
