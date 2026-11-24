using namespace geode::prelude;

/* #include <Geode/utils/random.hpp>
#include <Geode/modify/LevelAreaInnerLayer.hpp>
class $modify(LevelAreaInnerLayer) {
    struct Fields {
        int clickCount = 0;
    };

    void onNextFloor(CCObject* sender) {
        m_fields.self()->clickCount++;

        if (m_fields.self()->clickCount >= 10) {
            auto button = typeinfo_cast<CCMenuItem*>(this->getChildByID("main-node")->getChildByID("main-menu")->getChildByID("second-floor-button"));

            button->setEnabled(false);

            FMODAudioEngine::get()->playEffect("unlockPath.ogg");

            auto originalPos = button->getPosition();
            auto actions = CCArray::create();

        for (int i = 0; i < 24; i++) {
            float x = random::generate<float>(-4.f, 4.f);
            float y = random::generate<float>(-4.f, 4.f);

            actions->addObject(CCPlace::create({originalPos.x + x, originalPos.y + y}));
            actions->addObject(CCDelayTime::create(0.05f));
            actions->addObject(CCPlace::create(originalPos));
            actions->addObject(CCDelayTime::create(0.05f));
        }

            button->runAction(CCSequence::create(CCSequence::create(actions),
                CallFuncExt::create([button, originalPos]() {
                    button->setPosition(originalPos);
                    button->setEnabled(true);
                }), nullptr
            ));
        } else {
            LevelAreaInnerLayer::onNextFloor(sender);
        }
    }
}; */