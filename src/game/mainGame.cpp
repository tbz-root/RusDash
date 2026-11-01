#include <Geode/modify/GJGameLoadingLayer.hpp>
class $modify(GJGameLoadingLayer) {
    class hop : public cocos2d::CCLayer {
    public:
        CREATE_FUNC(hop);
        void sch(float) {
            geode::Ref layer = getParent()->getChildByType<EditLevelLayer*>(0);
            layer->getSkewX() ? layer->onEdit(this) : layer->onPlay(this);
        }

		void onEnterTransitionDidFinish() override {
            cocos2d::CCLayer::onEnterTransitionDidFinish();
            scheduleOnce(schedule_selector(hop::sch), 0.1f);
        };
    };
    
    static GJGameLoadingLayer* transitionToLoadingLayer(GJGameLevel* level, bool editor) {
        geode::Ref layer = EditLevelLayer::create(level);

        geode::cocos::switchToScene(layer, true);

        layer->setScale(0.f);
        layer->setSkewX(editor);
        layer->setKeypadEnabled(false);
        layer->getParent()->addChild(hop::create());

        return nullptr;
    }
};