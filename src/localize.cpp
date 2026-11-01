using namespace geode::prelude;

#include <Geode/Geode.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/CCLabelTTF.hpp>
#include <Geode/modify/CCNode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <matjson.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
#include <vector>
#if !defined(_MSC_VER)
#include <cxxabi.h>
#include <cstdlib>
#endif
struct LocKeyEntry {
    std::string id;
    std::string originalText;
    std::optional<std::string> nodeId;
    std::optional<std::string> parentId;
    std::optional<std::string> layer;
};

struct LocTranslation {
    std::string text;
    float offsetX = 0.f;
    float offsetY = 0.f;
    float scaleDelta = 0.f;
    bool hasOffset = false;
    bool hasScale = false;
};

static bool g_locEnabled = false;
static std::vector<LocKeyEntry> g_locKeys;
static std::unordered_map<std::string, LocTranslation> g_locTranslations;
static std::unordered_map<std::string, std::string> g_locTextToKey;
static std::unordered_set<void*> g_locProcessed;

static bool locIsRussian() {
    auto lang = Mod::get()->getSettingValue<std::string>("language");
    return lang.find("Русский") != std::string::npos || lang == "ru_RU";
}

static std::optional<matjson::Value> locLoadJson(std::string const& fileName) {
    std::vector<asp::fs::Path> candidates = {
        asp::fs::Path(R"(F:\dev\IUseRusDashBtw\resources\tabz.rusdash\lang)") / fileName,
        Mod::get()->getResourcesDir() / "lang" / fileName,
    };

    for (auto const& path : candidates) {
        log::info("Loading JSON: {}", path.string());
        if (!path.exists()) {
            log::warn("JSON not found: {}", path.string());
            continue;
        }
        auto res = file::readJson(path);
        if (!res) {
            log::warn("Failed to parse {}: {}", path.string(), res.unwrapErr());
            continue;
        }
        log::info("Loaded {}", path.string());
        return *res;
    }
    
    return std::nullopt;
}

static std::string locJsonString(matjson::Value const& v, char const* key) {
    if (!v.contains(key)) return "";
    auto r = v[key].asString();
    return r ? *r : "";
}

static std::optional<double> locJsonNumber(matjson::Value const& v, char const* key) {
    if (!v.contains(key)) return std::nullopt;
    auto r = v[key].asDouble();
    if (r) return *r;
    auto ri = v[key].asInt();
    if (ri) return static_cast<double>(*ri);
    return std::nullopt;
}

static void locParseKeys(matjson::Value const& root) {
    g_locKeys.clear();
    g_locTextToKey.clear();

    if (!root.isObject()) {
        log::error("keys.json must be an object");
        return;
    }

    for (auto const& [keyId, val] : root) {
        LocKeyEntry e;
        e.id = keyId;

        if (val.isString()) {
            e.originalText = val.asString().unwrapOr("");
        } else if (val.isObject()) {
            e.originalText = locJsonString(val, "text");
            if (auto s = locJsonString(val, "node"); !s.empty()) e.nodeId = s;
            if (auto s = locJsonString(val, "nodeId"); !s.empty()) e.nodeId = s;
            if (auto s = locJsonString(val, "parent"); !s.empty()) e.parentId = s;
            if (auto s = locJsonString(val, "parentId"); !s.empty()) e.parentId = s;
            if (auto s = locJsonString(val, "layer"); !s.empty()) e.layer = s;
            if (auto s = locJsonString(val, "layerName"); !s.empty()) e.layer = s;
        } else {
            continue;
        }

        if (!e.originalText.empty()) {
            g_locTextToKey[e.originalText] = e.id;
        }
        g_locKeys.push_back(std::move(e));
    }
}

static void locParseTranslations(matjson::Value const& root) {
    g_locTranslations.clear();

    if (!root.isObject()) {
        log::error("ru_RU.json must be an object");
        return;
    }

    for (auto const& [keyId, val] : root) {
        LocTranslation t;
        if (val.isString()) {
            t.text = val.asString().unwrapOr("");
        } else if (val.isObject()) {
            t.text = locJsonString(val, "text");
            if (t.text.empty()) t.text = locJsonString(val, "translation");

            if (auto x = locJsonNumber(val, "x")) {
                t.offsetX = static_cast<float>(*x);
                t.hasOffset = true;
            }
            if (auto y = locJsonNumber(val, "y")) {
                t.offsetY = static_cast<float>(*y);
                t.hasOffset = true;
            }
            if (val.contains("offset") && val["offset"].isArray()) {
                auto const& arr = val["offset"];
                if (arr.size() >= 1) {
                    if (auto n = arr[0].asDouble()) t.offsetX = static_cast<float>(*n);
                    else if (auto n = arr[0].asInt()) t.offsetX = static_cast<float>(*n);
                }
                if (arr.size() >= 2) {
                    if (auto n = arr[1].asDouble()) t.offsetY = static_cast<float>(*n);
                    else if (auto n = arr[1].asInt()) t.offsetY = static_cast<float>(*n);
                }
                t.hasOffset = true;
            }
            if (auto s = locJsonNumber(val, "scale")) {
                t.scaleDelta = static_cast<float>(*s);
                t.hasScale = true;
            } else if (auto s = locJsonNumber(val, "size")) {
                t.scaleDelta = static_cast<float>(*s);
                t.hasScale = true;
            }
        } else {
            continue;
        }

        if (!t.text.empty()) {
            g_locTranslations[keyId] = std::move(t);
        }
    }
}

static void locUnload() {
    g_locKeys.clear();
    g_locTranslations.clear();
    g_locTextToKey.clear();
    g_locProcessed.clear();
    g_locEnabled = false;
}

static void locReload() {
    locUnload();

    g_locEnabled = locIsRussian();
    if (!g_locEnabled) return;

    if (auto keys = locLoadJson("keys.json")) {
        locParseKeys(*keys);
    }
    if (auto tr = locLoadJson("ru_RU.json")) {
        locParseTranslations(*tr);
    }
}

static std::string locNodeId(CCNode* node) {
    if (!node) return "";
    return std::string(node->getID());
}

static std::string locClassName(CCNode* n) {
    if (!n) return "";
    auto typeName = typeid(*n).name();
    if (!typeName) return "";

#if defined(_MSC_VER)
    std::string tn = typeName;
    if (tn.rfind("class ", 0) == 0) tn = tn.substr(6);
    else if (tn.rfind("struct ", 0) == 0) tn = tn.substr(7);
    return tn;
#else
    int status = 0;
    char* demangled = abi::__cxa_demangle(typeName, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result = demangled;
        std::free(demangled);
        
        auto pos = result.rfind("::");

        if (pos != std::string::npos) {
            result = result.substr(pos + 2);
        }

        return result;
    }
    
    return std::string(typeName);
#endif
}

static bool locIsUnder(CCNode* node, std::string const& name) {
    if (name.empty()) return false;
    for (auto n = node; n; n = n->getParent()) {
        auto tn = locClassName(n);
        if (tn.empty()) continue;
        if (tn == name) return true;
        if (tn.find(name) != std::string::npos) return true;
    }

    return false;
}

static bool locMatchEntry(LocKeyEntry const& e, CCNode* labelNode, std::string const& currentText) {
    if (!e.originalText.empty() && e.originalText != currentText) return false;
    if (e.nodeId && locNodeId(labelNode) != *e.nodeId) return false;
    if (e.parentId) {
        auto parent = labelNode->getParent();
        if (!parent || locNodeId(parent) != *e.parentId) return false;
    }

    if (e.layer) {
        if (!locIsUnder(labelNode, *e.layer)) return false;
    }

    return true;
}

static std::optional<LocTranslation const*> locFind(
    CCNode* labelNode, std::string const& currentText) {
    if (!g_locEnabled || currentText.empty()) return std::nullopt;

    if (auto it = g_locTextToKey.find(currentText); it != g_locTextToKey.end()) {
        auto const& keyId = it->second;
        for (auto const& e : g_locKeys) {
            if (e.id != keyId) continue;
            if (!locMatchEntry(e, labelNode, currentText)) continue;
            if (auto tit = g_locTranslations.find(keyId); tit != g_locTranslations.end()) {
                return &tit->second;
            }
        }
    }

    for (auto const& e : g_locKeys) {
        if (!locMatchEntry(e, labelNode, currentText)) continue;
        if (auto tit = g_locTranslations.find(e.id); tit != g_locTranslations.end()) {
            return &tit->second;
        }
    }

    return std::nullopt;
}

static void locApply(CCNode* node, LocTranslation const& tr) {
    if (g_locProcessed.count(node)) return;

    if (auto bm = typeinfo_cast<CCLabelBMFont*>(node)) {
        g_locProcessed.insert(node);
        bm->setString(tr.text.c_str());
        if (tr.hasOffset) {
            auto pos = bm->getPosition();
            bm->setPosition({ pos.x + tr.offsetX, pos.y + tr.offsetY });
        }
        if (tr.hasScale) {
            bm->setScale(bm->getScale() + tr.scaleDelta);
        }
        return;
    }

    if (auto ttf = typeinfo_cast<CCLabelTTF*>(node)) {
        g_locProcessed.insert(node);
        ttf->setString(tr.text.c_str());
        if (tr.hasOffset) {
            auto pos = ttf->getPosition();
            ttf->setPosition({ pos.x + tr.offsetX, pos.y + tr.offsetY });
        }
        if (tr.hasScale) {
            ttf->setScale(ttf->getScale() + tr.scaleDelta);
        }
    }
}

static void locRecursive(CCNode* node) {
    if (!node || !g_locEnabled) return;

    if (auto bm = typeinfo_cast<CCLabelBMFont*>(node)) {
        char const* s = bm->getString();
        std::string text = s ? s : "";
        if (auto found = locFind(bm, text)) {
            locApply(bm, **found);
        }
    } else if (auto ttf = typeinfo_cast<CCLabelTTF*>(node)) {
        char const* s = ttf->getString();
        std::string text = s ? s : "";
        if (auto found = locFind(ttf, text)) {
            locApply(ttf, **found);
        }
    }

    auto children = node->getChildren();
    if (!children) return;
    for (auto child : CCArrayExt<CCNode*>(children)) {
        locRecursive(child);
    }
}

static void locScene() {
    if (!g_locEnabled) return;
    if (auto scene = CCDirector::get()->getRunningScene()) {
        locRecursive(scene);
    }
}

static void locOnSceneChanged() {
    if (!g_locEnabled) return;
    g_locProcessed.clear();
    locScene();
}

class $modify(CCLabelBMFont) {
    void setString(char const* newString, bool needUpdateLabel) {
        CCLabelBMFont::setString(newString, needUpdateLabel);
        if (!g_locEnabled || !newString) return;
        if (g_locProcessed.count(this)) return;

        if (auto found = locFind(this, newString)) {
            locApply(this, **found);
        }
    }
};

class $modify(CCLabelTTF) {
    void setString(char const* newString) {
        CCLabelTTF::setString(newString);
        if (!g_locEnabled || !newString) return;
        if (g_locProcessed.count(this)) return;

        if (auto found = locFind(this, newString)) {
            locApply(this, **found);
        }
    }
};

class $modify(CCDirector) {
    void replaceScene(CCScene* scene) {
        CCDirector::replaceScene(scene);
        locOnSceneChanged();
    }

    void pushScene(CCScene* scene) {
        CCDirector::pushScene(scene);
        locOnSceneChanged();
    }

    void popScene() {
        CCDirector::popScene();
        locOnSceneChanged();
    }
};

class $modify(LocNode, CCNode) {
    void addChild(CCNode* child) {
        CCNode::addChild(child);
        if (!g_locEnabled || !child) return;

        std::string id(getID());

        bool looksLikeLayer =
            id.find("Layer") != std::string::npos
            || id.find("Popup") != std::string::npos
            || typeinfo_cast<CCLayer*>(child) != nullptr
            || typeinfo_cast<FLAlertLayer*>(child) != nullptr;

        if (looksLikeLayer) {
            Ref<CCNode> safeChild = child;

            Loader::get()->queueInMainThread([safeChild]() {
                if (g_locEnabled && safeChild) {
                    locRecursive(safeChild);
                }
            });
        }
    }
};

$on_mod(Loaded) {
    listenForSettingChanges<std::string>("language", [](std::string) {
        locReload();
        locScene();
    });
}

$on_game(TexturesLoaded) {
    locReload();
}

$on_game(TexturesUnloaded) {
    locUnload();
}