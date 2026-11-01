#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GManager.hpp>

using namespace geode::prelude;

class $modify(RusDashGManager, GManager) {
    struct Fields {
        std::string originalFileName;
    };

    void setup() override;
};