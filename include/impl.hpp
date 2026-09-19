#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
using namespace geode::prelude;

#include <regex>
#include <functional>

static inline bool ALREADY_HAVE_GOF_IMPL = false;

#define THIS_CONF auto config = typeinfo_cast<GameObjectsFactory::GameObjectConfig*>(this ? getUserObject("config"_spr) : nullptr)
#define TRY_GET_CONF(p) auto config = typeinfo_cast<GameObjectsFactory::GameObjectConfig*>(p ? p->getUserObject("config"_spr) : nullptr)

#include <Geode/modify/MenuLayer.hpp>
class $modify(MenuLayerObjectFactoryExt, MenuLayer) {
    bool init() {
        if (ALREADY_HAVE_GOF_IMPL) return MenuLayer::init();

        for (auto searchPath : CCFileUtils::sharedFileUtils()->getSearchPaths()) {
            auto file = std::string(searchPath) + "register-game-objects.json";
            if (!CCFileUtils::sharedFileUtils()->isFileExist(file.c_str())) continue;
            auto parseRes = file::readJson(file);
            if (!parseRes) continue;
            for (auto v : parseRes.unwrap()) { // so silent
                auto c = GameObjectsFactory::GameObjectConfig::create();

                c->m_objectID = UNIQ_ID(v.dump(0).c_str());

                if (v.contains("objID")) c->objID(v["objID"].template as<int>().unwrapOr(c->m_objectID));
                if (v.contains("refID")) c->refID(v["refID"].template as<int>().unwrapOr(c->m_refObjectID));
                if (v.contains("frame")) c->frame(v["frame"].template as<std::string>().unwrapOr(c->m_spriteFrame));

                if (v.contains("preset")) {
                    auto preset = v["preset"].template as<std::string>().unwrapOr("");
                    if (preset == "deco") {
                        auto detail = v.contains("detail") ? v["detail"].template as<std::string>().unwrapOr("") : std::string{};
                        c = detail.size() ? GameObjectsFactory::createDecorationObjectConfig(
                            c->m_objectID, c->m_spriteFrame, detail
                        ) : GameObjectsFactory::createDecorationObjectConfig(
                            c->m_objectID, c->m_spriteFrame
                        );
                    }
                    if (preset == "rotated") c = GameObjectsFactory::createRotatedConfig(
                        c->m_objectID, c->m_spriteFrame);
                    if (preset == "pulse") c = GameObjectsFactory::createPulseConfig(
                        c->m_objectID, c->m_spriteFrame);
                }

                if (v.contains("tab")) c->tab(v["tab"].template as<int>().unwrapOr(c->m_createTabBar));
                if (v.contains("insertIndex")) c->insertIndex(v["insertIndex"].template as<int>().unwrapOr(c->m_tabBarInsertIndex));
                if (v.contains("btnBG")) c->btnBG(v["btnBG"].template as<int>().unwrapOr(c->m_createBtnBg));
                c->registerMe();
                //meant to be like "unity/GJ_GameSheet.plist" for custom frames via "Sprite Frames Unity" mod
                if (v.contains("spritesheet")) CCSpriteFrameCache::get()->addSpriteFramesWithFile(
                    v["spritesheet"].template as<std::string>().unwrapOr("").c_str()
                );
            }
        }

        return MenuLayer::init();
    };
};
$on_mod(Loaded) {
    auto factory = GameObjectsFactory::getFactoryManager();
    factory->m_implID = factory->m_implID.empty() ? std::string(Mod::get()->getID()) : factory->m_implID;
    ALREADY_HAVE_GOF_IMPL = factory->m_implID != std::string(Mod::get()->getID()); 
}

inline static auto exchangeCustomObjectIDs(
    GameObject* object, CCArray* objects = nullptr,
    std::map<GameObject*, GameObjectsFactory::GameObjectConfig*> replacements = {}
) {
    if (!object && !objects) return replacements;

    auto manager = GameObjectsFactory::getFactoryManager();
    auto reg = manager->m_registry;

    // Restore replaced IDs
    if (replacements.size()) {
        for (auto replacement : replacements) {
            replacement.first->m_objectID = replacement.second->m_objectID;
        }
        return replacements;
    }

    // Replace ID of objects in CCArray
    if (objects) {
        for (auto object : CCArrayExt<GameObject*>(objects)) {
            if (!object) continue;
            if (auto c = reg->getObject(object->m_objectID)) {
                object->m_objectID = c->m_refObjectID;
                replacements[object] = c;
            }
        }
    }

    // Replace ID of single object
    if (object) {
        if (auto c = reg->getObject(object->m_objectID)) {
            object->m_objectID = c->m_refObjectID;
            replacements[object] = c;
        }
    }

    return replacements;
}

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(GJBaseGameLayerFactoryExt, GJBaseGameLayer) {
    void spawnObject(GameObject * object, double delay, gd::vector<int> const& remapKeys) {
        if (ALREADY_HAVE_GOF_IMPL) return GJBaseGameLayer::spawnObject(object, delay, remapKeys);

        auto changedObjects = exchangeCustomObjectIDs(object);
        GJBaseGameLayer::spawnObject(object, delay, remapKeys);
        exchangeCustomObjectIDs(object, nullptr, changedObjects);

        // Call custom spawn if available
        if (THIS_CONF) if (auto fn = config->m_spawnObject) fn(
            this, object, delay, remapKeys
        );
    }
};

#include <Geode/modify/EnhancedGameObject.hpp>
class $modify(EnhancedGameObjectFactoryExt, EnhancedGameObject) {
    void activatedByPlayer(PlayerObject * p0) {
        if (ALREADY_HAVE_GOF_IMPL) return EnhancedGameObject::activatedByPlayer(p0);

        EnhancedGameObject::activatedByPlayer(p0);
        if (THIS_CONF) if (auto fn = config->m_activatedByPlayer) fn(this, p0);
    }
};

#include <Geode/modify/EffectGameObject.hpp>
class $modify(EffectGameObjectFactoryExt, EffectGameObject) {
    void customSetup() {
        if (ALREADY_HAVE_GOF_IMPL) return EffectGameObject::customSetup();

        auto changedObjects = exchangeCustomObjectIDs(this);
        EffectGameObject::customSetup();
        exchangeCustomObjectIDs(this, nullptr, changedObjects);

        if (THIS_CONF) if (auto fn = config->m_customSetup) fn(this);
    }
    void triggerActivated(float p0) {
        if (ALREADY_HAVE_GOF_IMPL) return EffectGameObject::triggerActivated(p0);

        auto changedObjects = exchangeCustomObjectIDs(this);
        EffectGameObject::triggerActivated(p0);
        exchangeCustomObjectIDs(this, nullptr, changedObjects);

        // Call custom activation if available
        if (THIS_CONF) if (auto fn = config->m_triggerActivated) fn(this, p0);
    }
    void triggerObject(GJBaseGameLayer * p0, int p1, gd::vector<int> const* p2) {
        if (ALREADY_HAVE_GOF_IMPL) return EffectGameObject::triggerObject(p0, p1, p2);

        EffectGameObject::triggerObject(p0, p1, p2);

        // Call custom trigger logic if available
        if (THIS_CONF) if (auto fn = config->m_triggerObject) fn(this, p0, p1, p2);
    }
};

#include <Geode/modify/GameObject.hpp>
class $modify(GameObjectFactoryExt, GameObject) {
    void customSetup() {
        if (ALREADY_HAVE_GOF_IMPL) return GameObject::customSetup();

        GameObject::customSetup();

        // Call custom setup if available
        if (THIS_CONF) if (auto fn = config->m_customSetup) fn(this);
    }
    void resetObject() {
        if (ALREADY_HAVE_GOF_IMPL) return GameObject::resetObject();

        GameObject::resetObject();
        // Call if available
        if (THIS_CONF) if (auto fn = config->m_resetObject) fn(this);
    }
    static GameObject* createWithKey(int p0) {
        if (ALREADY_HAVE_GOF_IMPL) return GameObject::createWithKey(p0);

        if (auto config = GameObjectsFactory::getGameObjectConfig(p0)) {
            // Create base object
            auto object = GameObject::createWithKey(config->m_refObjectID);
            if (!object) return nullptr;
            object->setUserObject("config"_spr, config);

            object->customSetup();

            gd::string frame = ObjectToolbox::sharedState()->intKeyToFrame(
                config->m_refObjectID
            );
            object->addColorSprite(frame);
            object->setupCustomSprites(frame);

            // Set custom sprite if specified
            if (!config->m_spriteFrame.empty()) {
                object->initWithSpriteFrameName(config->m_spriteFrame.c_str());
            }

            object->m_objectID = p0;
            return object;
        }
        return GameObject::createWithKey(p0);
    }

    static GameObject* objectFromVector(gd::vector<gd::string>& p0, gd::vector<void*>& p1, GJBaseGameLayer* p2, bool p3) {
        if (ALREADY_HAVE_GOF_IMPL) return GameObject::objectFromVector(p0, p1, p2, p3);

        auto object = GameObject::objectFromVector(p0, p1, p2, p3);
        if (!object) return object;

        if (auto config = GameObjectsFactory::getGameObjectConfig(object->m_objectID)) {
            object->setUserObject("config"_spr, config);

            if (auto effectObj = typeinfo_cast<EnhancedGameObject*>(object)) {
                auto changedObjects = exchangeCustomObjectIDs(effectObj);
                effectObj->customObjectSetup(p0, p1);
                exchangeCustomObjectIDs(effectObj, nullptr, changedObjects);
            }

            if (config->m_objectFromVector) {
                return config->m_objectFromVector(object, p0, p1, p2, p3);
            }
        }

        return object;
    }

    gd::string getSaveString(GJBaseGameLayer* p0) {
        if (ALREADY_HAVE_GOF_IMPL) return GameObject::getSaveString(p0);

        auto str = std::string(GameObject::getSaveString(p0));

        if (auto config = GameObjectsFactory::getGameObjectConfig(this->m_objectID)) {
            if (config->m_saveString) {
                return config->m_saveString(str, this, p0);
            }
        }

        return str;
    }
};

#include <Geode/modify/EditorUI.hpp>
class $modify(EditorUIFactoryExt, EditorUI) {
    void editObject(cocos2d::CCObject * p0) {
        if (ALREADY_HAVE_GOF_IMPL) return EditorUI::editObject(p0);

        auto objects = m_selectedObjects ? CCArrayExt<GameObject*>(m_selectedObjects) : CCArrayExt<GameObject*>();
        if (m_selectedObject) objects.push_back(m_selectedObject);
        for (auto object : objects) if (TRY_GET_CONF(object)) if (auto fn = config->m_onEditObject) {
            if (fn(this, object)) return;
        }

        auto changedObjects = exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects);
        EditorUI::editObject(p0);
        exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects, changedObjects);
    }
    void editObject2(cocos2d::CCObject * p0) {
        if (ALREADY_HAVE_GOF_IMPL) return EditorUI::editObject2(p0);

        auto objects = m_selectedObjects ? CCArrayExt<GameObject*>(m_selectedObjects) : CCArrayExt<GameObject*>();
        if (m_selectedObject) objects.push_back(m_selectedObject);
        for (auto object : objects) if (TRY_GET_CONF(object)) if (auto fn = config->m_onEditObject2) {
            if (fn(this, object)) return;
        }

        auto changedObjects = exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects);
        EditorUI::editObject2(p0);
        exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects, changedObjects);
    }
    void editObjectSpecial(int p0) {
        if (ALREADY_HAVE_GOF_IMPL) return EditorUI::editObjectSpecial(p0);

        auto objects = m_selectedObjects ? CCArrayExt<GameObject*>(m_selectedObjects) : CCArrayExt<GameObject*>();
        if (m_selectedObject) objects.push_back(m_selectedObject);
        for (auto object : objects) if (TRY_GET_CONF(object)) if (auto fn = config->m_onEditObjectSpecial) {
            if (fn(this, object)) return;
        }

        auto changedObjects = exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects);
        EditorUI::editObjectSpecial(p0);
        exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects, changedObjects);
    }

    void updateButtons() {
        if (ALREADY_HAVE_GOF_IMPL) return EditorUI::updateButtons();

        auto changedObjects = exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects);
        EditorUI::updateButtons();
        exchangeCustomObjectIDs(m_selectedObject, m_selectedObjects, changedObjects);
    }

    void setupCreateMenu() {
        if (ALREADY_HAVE_GOF_IMPL) return EditorUI::setupCreateMenu();

        EditorUI::setupCreateMenu();

        // Add all registered custom objects to appropriate tabs
        auto registry = GameObjectsFactory::getRegistry();
        if (registry && registry->getChildrenCount()) {
            CCArrayExt<GameObjectsFactory::GameObjectConfig*> iter = registry->getChildren();
            for (auto config : iter) if (config) {
                auto bars = CCArrayExt<EditButtonBar*>(m_createButtonBars);
                int idx = config->m_createTabBar;
                if (idx < 0 || idx >= (int)bars.size()) continue;
                if (auto tab = bars[idx]) {
                    auto newb = this->getCreateBtn(config->m_objectID, config->m_createBtnBg);
                    if (config->m_tabBarInsertIndex == -1) tab->m_buttonArray->addObject(newb);
                    else tab->m_buttonArray->insertObject(newb, config->m_tabBarInsertIndex);
                    tab->reloadItems(
                        GameManager::sharedState()->getIntGameVariable("0049"),
                        GameManager::sharedState()->getIntGameVariable("0050")
                    );
                }
            }
        }
    }
};

#include <Geode/modify/EditTriggersPopup.hpp>
class $modify(EditTriggersPopupFactoryExt, EditTriggersPopup) {
    bool init(EffectGameObject * p0, cocos2d::CCArray * p1) {
        if (ALREADY_HAVE_GOF_IMPL) return EditTriggersPopup::init(p0, p1);

        if (!EditTriggersPopup::init(p0, p1)) return false;

        // Check if this is a custom object and call custom edit setup
        if (TRY_GET_CONF(p0)) if (auto fn = config->m_editPopupSetup) fn(this, p0, p1);

        return true;
    }
};

//example test
#if defined(GAME_OBJECTS_FACTORY_EXAMPLE_TEST_IMPL)

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

#endif

//full test
#if defined(GAME_OBJECTS_FACTORY_FULL_TEST_IMPL___)
#define logCallbacks                                                                                                             \
->customSetup([](GameObject* object)                                                                                             \
    { log::info("custom setup, {}", object); }                                                                                   \
)->resetObject([](GameObject* object)                                                                                            \
    { log::info("reset object, {}", object); }                                                                                   \
)->activatedByPlayer([](EnhancedGameObject* object, PlayerObject* plr)                                                           \
    { log::info("activated by player, {}, {}", object, plr); }                                                                   \
)->triggerActivated([](GameObject* object, float dtmayb)                                                                         \
    { log::info("trigger activated, {}, {}", object, dtmayb); }                                                                  \
)->triggerObject([](GameObject* object, GJBaseGameLayer* game, int p1, std::vector<int> const* p2)                               \
    { log::info("trigger object, {}, {}", object, game); }                                                                       \
)->editPopupSetup([](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects)                                      \
    { log::info("edit popup setup, {}, {}", popup, trigger); }                                                                   \
)->saveString([](std::string str, GameObject* object, GJBaseGameLayer* level)                                                    \
    { log::info("save string, {}", str); return str; }                                                                           \
)->objectFromVector([](GameObject* object, std::vector<std::string>& p0, std::vector<void*>& p1, GJBaseGameLayer* p2, bool p3)   \
    { log::info("object from vector"); return Ref(object); }                                                                     \
)->spawnObject([](GJBaseGameLayer*, GameObject* object, double a, std::vector<int> const& p1)                                    \
    { log::info("spawn object, {}, {}, {}", object, a, p1); }                                                                    \
)                                                                                                                                \

void registerCustomObjects();
$execute{ registerCustomObjects(); }
inline void registerCustomObjects() {

    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a1"), "d_sign_img_01_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a2"), "d_sign_img_02_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a3"), "d_sign_img_03_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a4"), "d_sign_img_01_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a5"), "d_sign_img_02_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a6"), "d_sign_img_03_001.png")->registerMe();
    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("a7"), "d_sign_img_02_001.png")->registerMe();

    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("asd1"),
        "blackCogwheel_03_001.png", "blackCogwheel_02_color_001.png")->registerMe();

    GameObjectsFactory::createDecorationObjectConfig(UNIQ_ID("asd12"),
        "block001_02_color_001.png", "block006_05_001.png")->registerMe();

    GameObjectsFactory::registerGameObject(GameObjectsFactory::createPulseConfig(
        UNIQ_ID("d_sign_img_03__shine"), "d_sign_img_03_001.png"
    ));
    GameObjectsFactory::registerGameObject(GameObjectsFactory::createRotatedConfig(
        UNIQ_ID("shine_d_sign_i"), "d_sign_img_01_001.png",
        [](void*) { log::info("setup"); }
    ));
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
                return false;
            }
        )
    );
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createTriggerConfig(
            UNIQ_ID("test-trigger"),
            "edit_eEventLinkBtn_001.png"
        )logCallbacks
    );
}

#endif

#undef THIS_CONF 
#undef TRY_GET_CONF
