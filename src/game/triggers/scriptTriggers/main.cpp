#include "PythonInterpreter.hpp"
#include <Geode/Geode.hpp>
#include <miskaa.notif/src/includes/notif_api.hpp>
#include <filesystem>
#include <cstdlib>
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

std::unordered_map<GJBaseGameLayer*, std::shared_ptr<PythonInterpreter>> PythonInterpreter::s_registry;
bool PythonInterpreter::s_pythonReady = false;

std::shared_ptr<PythonInterpreter> PythonInterpreter::forLayer(GJBaseGameLayer* layer) {
    if (!layer) return nullptr;

    auto it = s_registry.find(layer);
    if (it != s_registry.end()) {
        return it->second;
    }

    auto interp = std::make_shared<PythonInterpreter>();
    interp->init(layer);
    s_registry[layer] = interp;
    return interp;
}

void PythonInterpreter::cleanupLayer(GJBaseGameLayer* layer) {
    s_registry.erase(layer);
}

PythonInterpreter::PythonInterpreter() = default;

PythonInterpreter::~PythonInterpreter() {
    try {
        if (s_pythonReady) {
            py::gil_scoped_acquire gil;
            m_globals = py::dict();
            m_state = py::dict();
        }
    } catch (...) {
    }
}

void PythonInterpreter::ensurePython() {
    if (s_pythonReady) return;

    try {
#if defined(SCRIPT_TRIGGER_BUNDLE_PYTHON) && defined(_WIN32)
        auto resDir = Mod::get()->getResourcesDir() / "python-windows";
        auto saveDir = Mod::get()->getSaveDir() / "python-windows";
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);

        auto tryCopyTree = [&](std::filesystem::path const& from) {
            if (!std::filesystem::exists(from)) return false;
            std::filesystem::copy(
                from, saveDir,
                std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::skip_existing,
                ec
            );
            return true;
        };
        tryCopyTree(resDir);

        auto dllPath = saveDir / "python312.dll";
        if (!std::filesystem::exists(dllPath)) {
            dllPath = resDir / "python312.dll";
        }

        if (std::filesystem::exists(dllPath)) {
            SetDllDirectoryW(saveDir.wstring().c_str());
            auto home = saveDir.wstring();
            SetEnvironmentVariableW(L"PYTHONHOME", home.c_str());
            SetEnvironmentVariableW(L"PYTHONPATH", home.c_str());
            SetEnvironmentVariableW(L"PYTHONDONTWRITEBYTECODE", L"1");

            HMODULE py = LoadLibraryW(dllPath.wstring().c_str());
            if (!py) {
                log::error("LoadLibrary python312.dll failed (err={})", GetLastError());
            } else {
                log::info("Loaded bundled python312.dll from {}", dllPath.string());
            }
        } else {
            log::warn(
                "Bundled python312.dll not found in resources/python-windows — "
                "falling back to system Python if available"
            );
        }
#endif

#ifdef SCRIPT_TRIGGER_ANDROID_PYTHON
        auto saveDir = Mod::get()->getSaveDir() / "python";
        auto stdlibDst = saveDir / "lib";
        std::error_code ec;
        std::filesystem::create_directories(stdlibDst, ec);

        auto tryCopyStdlib = [&](std::filesystem::path const& srcZipOrDir) {
            if (!std::filesystem::exists(srcZipOrDir)) return false;

            if (std::filesystem::is_directory(srcZipOrDir)) {
                std::filesystem::copy(
                    srcZipOrDir,
                    stdlibDst,
                    std::filesystem::copy_options::recursive |
                        std::filesystem::copy_options::skip_existing,
                    ec
                );

                return !ec;
            }

            return false;
        };

        auto resDir = Mod::get()->getResourcesDir();

        tryCopyStdlib(resDir / "python" / "stdlib");
        tryCopyStdlib(saveDir / "stdlib-src");

        auto home = saveDir.string();
        
        setenv("PYTHONHOME", home.c_str(), 1);
        setenv("PYTHONPATH", stdlibDst.string().c_str(), 1);
        setenv("PYTHONDONTWRITEBYTECODE", "1", 1);

        log::info("Android PYTHONHOME={}", home);
#endif

        py::initialize_interpreter();
        s_pythonReady = true;
        log::info("Embedded Python interpreter initialized");
    } catch (std::exception& e) {
        log::error("Failed to init Python: {}", e.what());
        s_pythonReady = false;
    }
}

void PythonInterpreter::init(GJBaseGameLayer* layer) {
    if (m_initialized) return;

    m_layer = layer;
    ensurePython();
    if (!s_pythonReady) return;

    try {
        py::gil_scoped_acquire gil;

        m_globals = py::dict();
        py::object builtins = py::module_::import("builtins");
        m_globals["__builtins__"] = builtins;

        m_state = py::dict();
        m_globals["state"] = m_state;

        m_globals["clearState"] = py::cpp_function([this]() {
            this->resetState();
        });

        m_globals["print"] = py::cpp_function([](py::args args, py::kwargs kwargs) {
            std::string text;
            bool silent = false;
            if (kwargs.contains("silent")) {
                silent = kwargs["silent"].cast<bool>();
            }
            for (size_t i = 0; i < args.size(); ++i) {
                if (i) text += " ";
                text += py::str(args[i]).cast<std::string>();
            }
            
            if (!args.empty()) {
                try {
                    if (py::isinstance<py::bool_>(args[args.size() - 1]) && args.size() > 1) {
                        silent = args[args.size() - 1].cast<bool>();
                        
                        text.clear();
                        for (size_t i = 0; i + 1 < args.size(); ++i) {
                            if (i) text += " ";
                            text += py::str(args[i]).cast<std::string>();
                        }
                    }
                } catch (...) {
                }
            }
            log::info("{}", text);
            if (!silent) notifapi::info(text);
        });

        m_globals["warn"] = py::cpp_function([](py::args args) {
            std::string text;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i) text += " ";
                text += py::str(args[i]).cast<std::string>();
            }
            log::warn("{}", text);
            notifapi::warn(text);
        });

        m_globals["error"] = py::cpp_function([](py::args args) {
            std::string text;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i) text += " ";
                text += py::str(args[i]).cast<std::string>();
            }
            log::error("{}", text);
            notifapi::error(text);
        });

        m_globals["wait"] = py::cpp_function([](double seconds) {
            log::debug("wait({}) called — cooperative wait not fully implemented in pybind build", seconds);
        });

        if (layer) {
            bindEngineAPI(layer);
        } else if (auto pl = PlayLayer::get()) {
            bindEngineAPI(pl);
        }

        m_initialized = true;
    } catch (py::error_already_set& e) {
        log::error("Python init error: {}", e.what());
    } catch (std::exception& e) {
        log::error("Python init error: {}", e.what());
    }
}

bool PythonInterpreter::runString(const std::string& code, bool ignoreTimeout) {
    if (!m_initialized || m_disabled || code.empty()) return false;

    try {
        py::gil_scoped_acquire gil;
        py::exec(code, m_globals, m_globals);
        return true;
    } catch (py::error_already_set& e) {
        log::error("Python error: {}", e.what());
        notifapi::error(std::string("Python: ") + e.what());
        return false;
    } catch (std::exception& e) {
        log::error("Python error: {}", e.what());
        return false;
    }
}

void PythonInterpreter::stop() {
    m_disabled = true;
    m_executionToken++;
}

void PythonInterpreter::pause() {}

void PythonInterpreter::resume() {
    m_disabled = false;
}

void PythonInterpreter::resetState() {
    m_disabled = false;
    m_executionToken++;
    if (!s_pythonReady) return;
    try {
        py::gil_scoped_acquire gil;
        m_state = py::dict();
        m_globals["state"] = m_state;
    } catch (...) {
    }
}


namespace {
void moveGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, CCPoint offset, float duration, int easingType = 0, float easingRate = 2.0f) {
    if (!gameLayer) return;
    auto moveTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(901));
    if (!moveTrigger) {
        log::error("Move trigger cast failed");
        return;
    }
    moveTrigger->m_targetGroupID = targetGroupID;
    moveTrigger->m_moveOffset = offset;
    moveTrigger->m_duration = duration;
    moveTrigger->m_easingType = static_cast<EasingType>(easingType);
    moveTrigger->m_easingRate = easingRate;
    moveTrigger->triggerObject(gameLayer, -1, nullptr);
}

void rotateGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, int degrees, int times360, float duration, int easingType = 0, float easingRate = 2.0f, bool lockObjRotation = false) {
    if (!gameLayer) return;
    auto rotateTrigger = static_cast<EffectGameObject*>(GameObject::createWithKey(1346));
    if (!rotateTrigger) {
        log::error("Rotate trigger creation failed");
        return;
    }
    rotateTrigger->m_targetGroupID = targetGroupID;
    rotateTrigger->m_centerGroupID = centerGroupID;
    rotateTrigger->m_rotationDegrees = degrees;
    rotateTrigger->m_times360 = times360;
    rotateTrigger->m_duration = duration;
    rotateTrigger->m_easingType = static_cast<EasingType>(easingType);
    rotateTrigger->m_easingRate = easingRate;
    rotateTrigger->m_lockObjectRotation = lockObjRotation;
    rotateTrigger->triggerObject(gameLayer, -1, nullptr);
}

void scaleGroupWithEasing(GJBaseGameLayer* gameLayer, int targetGroupID, int centerGroupID, float scaleX, float scaleY, float duration, int easingType = 0, float easingRate = 2.0f, bool divByX = false, bool divByY = false, bool onlyMove = false, bool relativeScale = false, bool relativeRotation = false) {
    if (!gameLayer) return;
    auto scaleTrigger = static_cast<TransformTriggerGameObject*>(GameObject::createWithKey(2067));
    if (!scaleTrigger) {
        log::error("Scale trigger creation failed");
        return;
    }
    scaleTrigger->m_targetGroupID = targetGroupID;
    scaleTrigger->m_centerGroupID = centerGroupID;
    scaleTrigger->m_objectScaleX = scaleX;
    scaleTrigger->m_objectScaleY = scaleY;
    scaleTrigger->m_duration = duration;
    scaleTrigger->m_easingType = static_cast<EasingType>(easingType);
    scaleTrigger->m_easingRate = easingRate;
    scaleTrigger->m_divideX = divByX;
    scaleTrigger->m_divideY = divByY;
    scaleTrigger->m_onlyMove = onlyMove;
    scaleTrigger->m_relativeScale = relativeScale;
    scaleTrigger->m_relativeRotation = relativeRotation;
    scaleTrigger->triggerObject(gameLayer, -1, nullptr);
}
} // namespace

void PythonInterpreter::bindEngineAPI(GJBaseGameLayer* layer) {
    if (!layer || !s_pythonReady) return;

    py::gil_scoped_acquire gil;

    auto player = py::module_::import("types").attr("SimpleNamespace")();

    player.attr("Player1") = 1;
    player.attr("Player2") = 2;
    player.attr("Both") = 3;

    player.attr("kill") = py::cpp_function([layer](py::object playerType) {
        int type = 1;
        if (!playerType.is_none()) {
            try { type = playerType.cast<int>(); } catch (...) {}
        }
        auto kill = [&](PlayerObject* p) {
            if (p) layer->destroyPlayer(p, nullptr);
        };
        if (type == 1 || type == 3) kill(layer->m_player1);
        if (type == 2 || type == 3) kill(layer->m_player2);
    }, py::arg("playerType") = 1);

    player.attr("getX") = py::cpp_function([layer](py::object playerType) -> float {
        int type = 1;
        if (!playerType.is_none()) {
            try { type = playerType.cast<int>(); } catch (...) {}
        }
        auto* p = (type == 2) ? layer->m_player2 : layer->m_player1;
        return p ? p->getPositionX() : 0.f;
    }, py::arg("playerType") = 1);

    player.attr("getY") = py::cpp_function([layer](py::object playerType) -> float {
        int type = 1;
        if (!playerType.is_none()) {
            try { type = playerType.cast<int>(); } catch (...) {}
        }
        auto* p = (type == 2) ? layer->m_player2 : layer->m_player1;
        return p ? p->getPositionY() : 0.f;
    }, py::arg("playerType") = 1);

    player.attr("flipGravity") = py::cpp_function([layer](py::object playerType, py::object noEffects) {
        int type = 1;
        bool fx = true;
        if (!playerType.is_none()) {
            try { type = playerType.cast<int>(); } catch (...) {}
        }
        if (!noEffects.is_none()) {
            try { fx = !noEffects.cast<bool>(); } catch (...) {}
        }
        auto* p = (type == 2) ? layer->m_player2 : layer->m_player1;
        if (p) p->flipGravity(!p->m_isUpsideDown, fx);
    }, py::arg("playerType") = 1, py::arg("noEffects") = false);

    m_globals["Player"] = player;

    auto object = py::module_::import("types").attr("SimpleNamespace")();

    object.attr("move") = py::cpp_function(
        [layer](int group, float x, float y, py::object duration) {
            float dur = 0.f;
            if (!duration.is_none()) {
                try { dur = duration.cast<float>(); } catch (...) {}
            }
            moveGroupWithEasing(layer, group, CCPoint{ x, y }, dur);
        },
        py::arg("group"), py::arg("x"), py::arg("y"), py::arg("duration") = 0.f
    );

    object.attr("rotate") = py::cpp_function(
        [layer](int group, int center, float degrees, py::object times360, py::object duration) {
            int t360 = 0;
            float dur = 0.f;
            if (!times360.is_none()) {
                try { t360 = times360.cast<int>(); } catch (...) {}
            }
            if (!duration.is_none()) {
                try { dur = duration.cast<float>(); } catch (...) {}
            }
            rotateGroupWithEasing(layer, group, center, static_cast<int>(degrees), t360, dur);
        },
        py::arg("group"), py::arg("center"), py::arg("degrees"),
        py::arg("times360") = 0, py::arg("duration") = 0.f
    );

    object.attr("scale") = py::cpp_function(
        [layer](int group, int center, float sx, float sy, py::object duration) {
            float dur = 0.f;
            if (!duration.is_none()) {
                try { dur = duration.cast<float>(); } catch (...) {}
            }
            scaleGroupWithEasing(layer, group, center, sx, sy, dur);
        },
        py::arg("group"), py::arg("center"), py::arg("sx"), py::arg("sy"),
        py::arg("duration") = 0.f
    );

    object.attr("spawn") = py::cpp_function(
        [layer](int group, py::object delay) {
            double d = 0.0;
            if (!delay.is_none()) {
                try { d = delay.cast<double>(); } catch (...) {}
            }
            gd::vector<int> empty;
            layer->spawnGroup(group, false, d, empty, 0, 0);
        },
        py::arg("group"), py::arg("delay") = 0.0
    );

    m_globals["Object"] = object;

    auto popup = py::module_::import("types").attr("SimpleNamespace")();
    popup.attr("show") = py::cpp_function([](std::string title, std::string content) {
        FLAlertLayer::create(title.c_str(), content.c_str(), "OK")->show();
    });
    m_globals["Popup"] = popup;
}
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "ExecuteScriptTrigger.hpp"
#include "PythonInterpreter.hpp"

using namespace geode::prelude;

class $modify(GJBaseGameLayer) {
    void controlTriggersInGroup(int groupID, GJActionCommand command) {
        GJBaseGameLayer::controlTriggersInGroup(groupID, command);

        auto groupObjects = this->getGroup(groupID);
        if (!groupObjects) return;

        for (auto* obj : CCArrayExt<CCObject*>(groupObjects)) {
            if (auto* scriptTrigger = typeinfo_cast<ExecuteScriptTrigger*>(obj)) {
                log::info("Control command {} received for group {}", static_cast<int>(command), groupID);

                if (command == GJActionCommand::Stop) {
                    log::info("Stopping ScriptTrigger!");
                    scriptTrigger->stopScript();
                } else if (command == GJActionCommand::Pause) {
                    log::info("Pausing ScriptTrigger!");
                    scriptTrigger->pauseScript();
                } else if (command == GJActionCommand::Resume) {
                    log::info("Resuming ScriptTrigger!");
                    scriptTrigger->resumeScript();
                }
            }
        }
    }

    void onExit() {
        PythonInterpreter::cleanupLayer(this);
        GJBaseGameLayer::onExit();
    }
};

class $modify(PlayLayer) {
    void resetLevel() {
        LevelResetEvent().send();
        PlayLayer::resetLevel();
    }
};
