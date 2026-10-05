#include "hpp/gManager.hpp"

void RusDashGManager::setup() {
    if (m_fields->originalFileName.empty()) {
        m_fields->originalFileName = m_fileName;
    }

    auto saveDir = geode::dirs::getSaveDir() / "gdpses" / "rusdash";

    std::error_code ec;

    if (!std::filesystem::exists(saveDir, ec)) {
        if (ec) {
            log::error(
                "Failed to check RusDash save directory: {}",
                ec.message()
            );
            return GManager::setup();
        }

        if (!std::filesystem::create_directories(saveDir, ec)) {
            log::error(
                "Failed to create RusDash save directory: {}",
                ec.message()
            );
            return GManager::setup();
        }
    }

    if (ec) {
        log::error(
            "Error while creating RusDash save directory: {}",
            ec.message()
        );
        return GManager::setup();
    }

    m_fileName = fmt::format(
        "gdpses/rusdash/{}",
        m_fields->originalFileName
    );

    GManager::setup();
}