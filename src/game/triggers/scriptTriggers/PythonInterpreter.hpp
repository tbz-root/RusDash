#pragma once

#include <Geode/Geode.hpp>
#include <optional>
#include <string>
#include <memory>
#include <unordered_map>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace geode::prelude;

class PythonInterpreter {
public:
    PythonInterpreter();
    ~PythonInterpreter();

    static std::shared_ptr<PythonInterpreter> forLayer(GJBaseGameLayer* layer);
    static void cleanupLayer(GJBaseGameLayer* layer);

    void init(GJBaseGameLayer* layer);

    bool runString(const std::string& code, bool ignoreTimeout = false);

    template <typename T>
    std::optional<T> evaluateExpression(const std::string& expression, bool ignoreTimeout = false);

    void stop();
    void pause();
    void resume();
    void resetState();

private:
    void ensurePython();
    void bindEngineAPI(GJBaseGameLayer* layer);

    GJBaseGameLayer* m_layer = nullptr;
    bool m_initialized = false;
    bool m_disabled = false;
    uint32_t m_executionToken = 0;

    py::dict m_globals;
    py::dict m_state;

    static bool s_pythonReady;
    static std::unordered_map<GJBaseGameLayer*, std::shared_ptr<PythonInterpreter>> s_registry;
};

template <typename T>
std::optional<T> PythonInterpreter::evaluateExpression(const std::string& expression, bool ignoreTimeout) {
    if (!m_initialized || m_disabled) return std::nullopt;

    try {
        py::gil_scoped_acquire gil;
        py::object result = py::eval(expression, m_globals, m_globals);
        return result.cast<T>();
    } catch (py::error_already_set& e) {
        log::error("Python eval error: {}", e.what());
        return std::nullopt;
    } catch (std::exception& e) {
        log::error("Python eval error: {}", e.what());
        return std::nullopt;
    }
}
