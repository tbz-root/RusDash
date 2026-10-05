using namespace geode::prelude;

#include <Geode/modify/GJGameLoadingLayer.hpp>
class $modify(GJGameLoadingLayer) {
	class hop : public CCLayer {
	public:
		CREATE_FUNC(hop);
		void sch(float) {
			Ref layer = getParent()->getChildByType<EditLevelLayer*>(0);
			layer->getSkewX() ? layer->onEdit(this) : layer->onPlay(this);
		}

		void onEnterTransitionDidFinish() override {
			CCLayer::onEnterTransitionDidFinish();
			scheduleOnce(schedule_selector(hop::sch), 0.1f);
		};
	};
    
	static GJGameLoadingLayer* transitionToLoadingLayer(GJGameLevel * level, bool editor) {
		Ref layer = EditLevelLayer::create(level);

		switchToScene(layer);

		layer->setScale(0.f);
		layer->setSkewX(editor);
		layer->setKeypadEnabled(false);
		layer->getParent()->addChild(hop::create());

		return nullptr;
	}
};