using namespace geode::prelude;

#include <Geode/Geode.hpp>
std::string keyForType(IconType type = IconType::Cube) {
    switch (static_cast<int>(type)) {
        case 1: return "ship";
        case 2: return "player_ball";
        case 3: return "bird";
        case 4: return "dart";
        case 5: return "robot";
        case 6: return "spider";
        case 7: return "swing";
        case 8: return "jetpack";
        default: return "player";
    }
}

IconType typeForKey(std::string const& key) {
    if (key == "ship") return IconType::Ship;
    if (key == "player_ball") return IconType::Ball;
    if (key == "bird") return IconType::Ufo;
    if (key == "dart") return IconType::Wave;
    if (key == "robot") return IconType::Robot;
    if (key == "spider") return IconType::Spider;
    if (key == "swing") return IconType::Swing;
    if (key == "jetpack") return IconType::Jetpack;
    return IconType::Cube;
}

std::array<std::string, 5> frameNamesInVec(int index, IconType type) {
    auto base = keyForType(type);
    std::array<std::string, 5> names = {
        fmt::format("{}_{:02d}_001.png", base, index),
        fmt::format("{}_{:02d}_2_001.png", base, index),
        fmt::format("{}_{:02d}_3_001.png", base, index),
        fmt::format("{}_{:02d}_glow_001.png", base, index),
        fmt::format("{}_{:02d}_extra_001.png", base, index)
    };

    static constexpr const char* placeholder = "emptyGlow.png";
    auto* cache = CCSpriteFrameCache::sharedSpriteFrameCache();

    for (auto& name : names) {
        if (!cache->spriteFrameByName(name.c_str())) {
            name = placeholder;
        }
    }

    return names;
}

#include <Geode/modify/GameManager.hpp>
class $modify(MyGameManager, GameManager) {
    inline static Ref<CCNode> return_original_count_for_type = CCNode::create();

    $override bool init() {
        return_original_count_for_type->setID("return_original_count_for_type");
        return_original_count_for_type->setVisible(false);
        return GameManager::init();
    }

    $override int countForType(IconType p0) {
        auto rtn = GameManager::countForType(p0);

        if (return_original_count_for_type->isVisible()) return rtn;
        if (p0 == IconType::DeathEffect) return rtn;
        if (p0 == IconType::Special) return rtn;
        if (p0 == IconType::Item) return rtn;
        if (p0 == IconType::ShipFire) return rtn;

        for (int id = rtn + 1; id <= rtn + 255; ++id) {
            auto simple_frame_names = frameNamesInVec(id, p0);
            auto animated_part1_name = fmt::format("{}_{:02d}_01_001.png", keyForType(p0), id);

            bool incr = false;
            if (simple_frame_names[0] != "emptyGlow.png") incr = true;
            if (CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(animated_part1_name.c_str())) {
                incr = true;
            }

            if (incr) {
                rtn += 1;
            } else {
                break;
            }
        }

        return rtn;
    }
};

#include <Geode/modify/PlayerObject.hpp>
class $modify(PlayerObject) {
    static CCSpriteFrame* frame(const char* name) {
        return CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(name);
    }

    int customFramesUpdateFor(int index, IconType type, bool forVehicle = false) {
        MyGameManager::return_original_count_for_type->setVisible(true);
        auto original_count = GameManager::get()->countForType(type);
        MyGameManager::return_original_count_for_type->setVisible(false);

        if (index == original_count || index == (original_count + 1)) {
            index = GameManager::get()->activeIconForType(type);
        }

        auto names = frameNamesInVec(index, type);

        if (!forVehicle) {
            if (m_iconSprite) m_iconSprite->setDisplayFrame(frame(names[0].c_str()));
            if (m_iconSpriteSecondary) m_iconSpriteSecondary->setDisplayFrame(frame(names[1].c_str()));
            if (m_iconSpriteWhitener) m_iconSpriteWhitener->setDisplayFrame(frame(names[4].c_str()));
            if (m_iconGlow) m_iconGlow->setDisplayFrame(frame(names[3].c_str()));

            if (m_iconSprite) {
                auto* df = m_iconSprite->displayFrame();
                if (df) {
                    m_iconSprite->setPosition({
                        df->getOriginalSize().width / m_iconSprite->getContentSize().width,
                        df->getOriginalSize().height / m_iconSprite->getContentSize().height
                    });
                }
            }
            if (m_iconSpriteSecondary && m_iconSprite) {
                m_iconSpriteSecondary->setPosition(m_iconSprite->getContentSize() / 2);
            }
            if (m_iconSpriteWhitener && m_iconSprite) {
                m_iconSpriteWhitener->setPosition(m_iconSprite->getContentSize() / 2);
            }
            if (m_iconGlow) {
                auto* df = m_iconGlow->displayFrame();
                if (df) {
                    m_iconGlow->setPosition({
                        df->getOriginalSize().width / m_iconGlow->getContentSize().width,
                        df->getOriginalSize().height / m_iconGlow->getContentSize().height
                    });
                }
            }
        } else {
            if (m_vehicleSprite) m_vehicleSprite->setDisplayFrame(frame(names[0].c_str()));
            if (m_vehicleSpriteSecondary) m_vehicleSpriteSecondary->setDisplayFrame(frame(names[1].c_str()));
            if (m_vehicleSpriteWhitener) m_vehicleSpriteWhitener->setDisplayFrame(frame(names[4].c_str()));
            if (m_vehicleGlow) m_vehicleGlow->setDisplayFrame(frame(names[3].c_str()));
            if (m_birdVehicle) m_birdVehicle->setDisplayFrame(frame(names[2].c_str()));

            if (m_vehicleSprite) {
                auto* df = m_vehicleSprite->displayFrame();
                if (df) {
                    m_vehicleSprite->setPosition({
                        df->getOriginalSize().width / m_vehicleSprite->getContentSize().width,
                        df->getOriginalSize().height / m_vehicleSprite->getContentSize().height
                    });
                }
            }
            if (m_vehicleSpriteSecondary && m_vehicleSprite) {
                m_vehicleSpriteSecondary->setPosition(m_vehicleSprite->getContentSize() / 2);
            }
            if (m_vehicleSpriteWhitener && m_vehicleSprite) {
                m_vehicleSpriteWhitener->setPosition(m_vehicleSprite->getContentSize() / 2);
            }
            if (m_birdVehicle && m_vehicleSprite) {
                m_birdVehicle->setPosition(m_vehicleSprite->getContentSize() / 2);
            }
            if (m_vehicleGlow) {
                auto* df = m_vehicleGlow->displayFrame();
                if (df) {
                    m_vehicleGlow->setPosition({
                        df->getOriginalSize().width / m_vehicleGlow->getContentSize().width,
                        df->getOriginalSize().height / m_vehicleGlow->getContentSize().height
                    });
                }
            }

            if (m_vehicleSprite && type == IconType::Ufo) {
                m_vehicleSprite->setPositionY(-8.f);
                if (m_vehicleGlow) m_vehicleGlow->setPositionY(-8.f);
            }
            if (m_vehicleSprite && type == IconType::Ship) {
                m_vehicleSprite->setPositionY(-6.f);
                if (m_vehicleGlow) m_vehicleGlow->setPositionY(-6.f);
            }
        }

        return index;
    }

    $override bool init(int p0, int p1, GJBaseGameLayer* p2, cocos2d::CCLayer* p3, bool p4) {
        if (!PlayerObject::init(p0, p1, p2, p3, p4)) return false;
        PlayerObject::updatePlayerFrame(m_maybeSavedPlayerFrame);
        return true;
    }

    $override void updatePlayerFrame(int p0) {
        PlayerObject::updatePlayerFrame(p0);
        this->m_maybeSavedPlayerFrame = customFramesUpdateFor(p0, IconType::Cube);
    }

    $override void updatePlayerShipFrame(int p0) {
        PlayerObject::updatePlayerShipFrame(p0);
        this->m_maybeSavedPlayerFrame = customFramesUpdateFor(m_maybeSavedPlayerFrame, IconType::Cube);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Ship, true);
    }

    $override void updatePlayerRollFrame(int p0) {
        PlayerObject::updatePlayerRollFrame(p0);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Ball);
    }

    $override void updatePlayerBirdFrame(int p0) {
        PlayerObject::updatePlayerBirdFrame(p0);
        this->m_maybeSavedPlayerFrame = customFramesUpdateFor(m_maybeSavedPlayerFrame, IconType::Cube);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Ufo, true);
    }

    $override void updatePlayerDartFrame(int p0) {
        PlayerObject::updatePlayerDartFrame(p0);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Wave);
    }

    $override void updatePlayerSwingFrame(int p0) {
        PlayerObject::updatePlayerSwingFrame(p0);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Swing);
    }

    $override void updatePlayerJetpackFrame(int p0) {
        PlayerObject::updatePlayerJetpackFrame(p0);
        this->m_maybeSavedPlayerFrame = customFramesUpdateFor(m_maybeSavedPlayerFrame, IconType::Cube);
        this->m_iconRequestID = customFramesUpdateFor(p0, IconType::Jetpack, true);
    }

    $override void createRobot(int frame) {
        PlayerObject::createRobot(frame);
        this->m_iconRequestID = frame;
    }

    $override void createSpider(int frame) {
        PlayerObject::createSpider(frame);
        this->m_iconRequestID = frame;
    }
};

#include <Geode/modify/SimplePlayer.hpp>
class $modify(SimplePlayer) {
    $override void updatePlayerFrame(int p0, IconType p1) {
        SimplePlayer::updatePlayerFrame(p0, p1);

        if (p1 == IconType::Robot || p1 == IconType::Spider) {
            return;
        }

        auto names = frameNamesInVec(p0, p1);
        setFrames(
            names[0].c_str(),
            names[1].c_str(),
            names[2].c_str(),
            names[3].c_str(),
            names[4].c_str()
        );
    }
};

#include <Geode/modify/GJGarageLayer.hpp>
class $modify(GJGarageLayer) {
    $override void setupPage(int p0, IconType p1) {
        if (GameManager::sharedState()->countForType(p1) <= 36) {
            p0 = 0;
        }
        
        GJGarageLayer::setupPage(p0, p1);

        if (m_playerObject) {
            m_playerObject->updatePlayerFrame(
                GameManager::get()->activeIconForType(GameManager::get()->m_playerIconType),
                GameManager::get()->m_playerIconType
            );
        }
    }
};