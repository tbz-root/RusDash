#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/EffectGameObject.hpp>
#include "../../../nodes/FileSelectNode.hpp"
#include <fryy_55.amber/include/amber.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include "Events.hpp"
#include "PythonInterpreter.hpp"

using namespace geode::prelude;

class ExecuteScriptTrigger : public EffectGameObject {
public:
    static constexpr int SCRIPT_KEY = 140;
    static constexpr int FILENAME_KEY = 141;
    static constexpr int IGNORE_TIMEOUT_KEY = 144;

    std::string m_b64code;
    std::string m_filename;
    bool m_ignoreTimeout = false;
    bool m_active = false;

    ListenerHandle m_resetListener;

    ExecuteScriptTrigger();
    ~ExecuteScriptTrigger() override;

    static ExecuteScriptTrigger* create();

    void customSetup() override;
    void triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) override;

    void checkMod();

    void stopScript();
    void pauseScript();
    void resumeScript();

    static std::string highlightSyntax(const std::string& code);

    void setupResetListener();
    void resetScriptState();

    static void setupEditPopup(EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects);
};
