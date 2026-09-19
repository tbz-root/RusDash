#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include <Geode/binding/LevelTools.hpp>
#include "Events.hpp"
#include "PythonInterpreter.hpp"

using namespace geode::prelude;

struct ConditionScriptData : public CCObject {
    std::string b64code;
    int trueGroup = 0;
    int falseGroup = 0;
    bool ignoreTimeout = false;
    bool active = false;

    static ConditionScriptData* create() {
        auto ret = new ConditionScriptData();
        ret->autorelease();
        return ret;
    }
};

class ConditionScriptTrigger {
public:
    static ConditionScriptData* getData(GameObject* obj, bool createIfMissing = true) {
        if (!obj) return nullptr;
        if (auto* existing = typeinfo_cast<ConditionScriptData*>(
                obj->getUserObject("condition-script-data"_spr))) {
            return existing;
        }
        if (!createIfMissing) return nullptr;
        auto* data = ConditionScriptData::create();
        obj->setUserObject("condition-script-data"_spr, data);
        return data;
    }

    static void ensureData(GameObject* obj) {
        getData(obj, true);
    }

    static void onCustomSetup(GameObject* obj) {
        ensureData(obj);
    }

    static void onTriggerObject(
        EffectGameObject* obj,
        GJBaseGameLayer* layer,
        int uniqueID,
        gd::vector<int> const* remapKeys
    ) {
        if (!obj || !layer) return;
        if (!Mod::get()->getSettingValue<bool>("script-triggers-enabled")) return;

        auto* data = getData(obj, false);
        if (!data) return;

        int targetGroup = data->falseGroup;

        if (!data->b64code.empty()) {
            auto interp = PythonInterpreter::forLayer(layer);
            if (!interp) return;

            auto decoded = LevelTools::base64DecodeString(data->b64code);
            std::string rawExpr(decoded.c_str(), decoded.size());
            std::string wrappedExpr = "not not (" + rawExpr + ")";

            auto result = interp->evaluateExpression<bool>(wrappedExpr, data->ignoreTimeout);
            if (result.has_value() && result.value()) {
                targetGroup = data->trueGroup;
            }
        }

        if (targetGroup > 0) {
            static const gd::vector<int> emptyRemap{};
            const gd::vector<int>& remap = remapKeys ? *remapKeys : emptyRemap;
            layer->spawnGroup(targetGroup, true, 0.0, remap, uniqueID, 0);
        }
    }

    static gd::string onSaveString(gd::string str, GameObject* object, GJBaseGameLayer* level) {
        return str;
    }

    static GameObject* onObjectFromVector(
        GameObject* object,
        gd::vector<gd::string>& p0,
        gd::vector<void*>& p1,
        GJBaseGameLayer* level,
        bool p4
    ) {
        ensureData(object);
        return object;
    }

    static void setupEditPopup(EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects);
};
