#include <asp/fs.hpp>
#include "../hpp/saves.hpp"
void RusDashGManager::setup() {
    if (m_fields->originalFileName.empty()) {
        m_fields->originalFileName = m_fileName;
    }

    auto saveDir = geode::dirs::getSaveDir() / "gdpses" / "rusdash";

    if (!saveDir.exists()) {
        auto result = asp::fs::createDirAll(saveDir);
        if (result.isErr()) {
            log::error(
                "Failed to create RusDash save directory: {}",
                result.unwrapErr()
            );
            
            return GManager::setup();
        }
    }

    m_fileName = fmt::format(
        "gdpses/rusdash/{}",
        m_fields->originalFileName
    );

    GManager::setup();
}