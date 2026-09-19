using namespace geode::prelude;

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "../../../include/main.hpp"
#include "../../../include/impl.hpp"
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/binding/SetupCollisionStateTriggerPopup.hpp>
class $modify(EffectGameObject) {
	class ButtonMenuItem : public CCMenuItemSpriteExtra {
	public:
		CREATE_FUNC(ButtonMenuItem);

		virtual bool init() override {
			CCMenuItemSpriteExtra::init(CCNode::create(), CCNode::create(), nullptr, nullptr);
			this->setAnchorPoint({0.5f, 0.5f});
			this->setEnabled(true);
			m_animationEnabled = false;
			m_colorEnabled = false;
			m_activateSound = "no sound";
			m_selectSound = "no sound";
			
			return true;
		}

		std::function<void()> m_onActivate = []() {};
		std::function<void()> m_onSelected = []() {};
		std::function<void()> m_onUnselected = []() {};

		virtual void activate() override {
			log::info("activate() called");
			if (m_onActivate) m_onActivate();
		}
		virtual void selected() override {
			log::info("selected() called");
			if (m_onSelected) m_onSelected();
		}
		virtual void unselected() override {
			log::info("unselected() called");
			if (m_onUnselected) m_onUnselected();
		}

		ButtonMenuItem* onActivate(std::function<void()> cb) {
			m_onActivate = std::move(cb);
			return this;
		}
		ButtonMenuItem* onSelected(std::function<void()> cb) {
			m_onSelected = std::move(cb);
			return this;
		}
		ButtonMenuItem* onUnselected(std::function<void()> cb) {
			m_onUnselected = std::move(cb);
			return this;
		}
	};

	class ButtonDataStore : public CCObject {
	public:
		matjson::Value json = matjson::Value::object();

		static ButtonDataStore* create() {
			auto* ret = new ButtonDataStore();
			ret->autorelease();
			return ret;
		}

		int getInt(std::string_view key, int def = 0) const {
			return json[key].asInt().unwrapOr(def);
		}

		void setInt(std::string_view key, int value) {
			json[key] = value;
		}

		std::string dump() const {
			return json.dump(matjson::NO_INDENTATION);
		}

		void load(std::string_view str) {
			json = matjson::parse(str).unwrapOr(matjson::Value::object());
		}
	};

	static ButtonDataStore* getStore(GameObject* obj) {
		auto* store = typeinfo_cast<ButtonDataStore*>(obj->getUserObject("button-data"_spr));
		if (!store) {
			store = ButtonDataStore::create();
			obj->setUserObject("button-data"_spr, store);
		}
		return store;
	}

	inline static GameObjectsFactory::GameObjectConfig* conf = nullptr;

	static void setupButtonPopup(EditorUI*, EffectGameObject* obj, SetupCollisionStateTriggerPopup* popup) {
		if (popup->getUserObject("got-custom-setup-for-button"_spr)) return;
		popup->setUserObject("got-custom-setup-for-button"_spr, obj);

		auto* main = popup->m_mainLayer;
		auto* menu = popup->m_buttonMenu;

		if (auto* title = main->getChildByType<CCLabelBMFont>(0)) {
			title->setString("Button");
		}

		if (auto* n = main->getChildByType<CCLabelBMFont>(1)) n->setVisible(false);
		if (auto* n = main->getChildByType<CCLabelBMFont>(2)) n->setVisible(false);
		if (auto* n = main->getChildByType<CCScale9Sprite>(1)) n->setVisible(false);
		if (auto* n = main->getChildByType<CCScale9Sprite>(2)) n->setVisible(false);
		if (auto* n = main->getChildByType<CCTextInputNode>(0)) n->setVisible(false);
		if (auto* n = main->getChildByType<CCTextInputNode>(1)) n->setVisible(false);

		while (auto* n = menu->getChildByTag(51)) n->removeFromParentAndCleanup(false);
		while (auto* n = menu->getChildByTag(71)) n->removeFromParentAndCleanup(false);

		auto* data = getStore(obj);

		auto* activate = TextInput::create(52.f, "ID");

		activate->setFilter("0123456789");
		activate->getInputNode()->m_allowedChars = "0123456789";
		activate->setString(utils::numToString(data->getInt("activate")));
		activate->setPositionY(95.f);
		activate->setCallback([obj](std::string const& str) { getStore(obj)->setInt("activate", utils::numFromString<int>(str).unwrapOr(0)); });

		menu->addChild(activate);

		auto* activateLabel = CCLabelBMFont::create("Activate:\n \n \n \n ", "goldFont.fnt");

		activateLabel->setScale(0.5f);
		activate->getInputNode()->addChild(activateLabel);

		auto* selected = TextInput::create(54.f, "ID");

		selected->setFilter("0123456789");
		selected->getInputNode()->m_allowedChars = "0123456789";
		selected->setString(utils::numToString(data->getInt("selected")));
		selected->setPosition({ -95.f, 77.f });
		selected->setCallback([obj](std::string const& str) { getStore(obj)->setInt("selected", utils::numFromString<int>(str).unwrapOr(0)); });

		menu->addChild(selected);

		auto* selectedLabel = CCLabelBMFont::create("Selected:\n \n \n \n ", "goldFont.fnt");

		selectedLabel->setScale(0.5f);
		selected->getInputNode()->addChild(selectedLabel);

		auto* unselected = TextInput::create(48.f, "ID");

		unselected->setFilter("0123456789");
		unselected->getInputNode()->m_allowedChars = "0123456789";
		unselected->setString(utils::numToString(data->getInt("unselected")));
		unselected->setPosition({ 95.f, 77.f });
		unselected->setCallback([obj](std::string const& str) { getStore(obj)->setInt("unselected", utils::numFromString<int>(str).unwrapOr(0)); });

		menu->addChild(unselected);

		auto* unselectedLabel = CCLabelBMFont::create("Unselected:\n \n \n \n ", "goldFont.fnt");

		unselectedLabel->setScale(0.5f);
		unselected->getInputNode()->addChild(unselectedLabel);
	}

	static void setup() {
		conf = GameObjectsFactory::createRingConfig(
			UNIQ_ID("button"),
			"buttonTrigger.png"
		)
		->refID(3640)
		->tab(12)
		->insertIndex((12 * 6) + 3)
		->onEditObject(
			[](EditorUI* a, GameObject* aa) -> bool {
				queueInMainThread([a = Ref(a), aa = Ref(aa)] {
					if (!CCScene::get()) {
						return log::error("CCScene::get() == null");
					}
					auto* popup = CCScene::get()->getChildByType<SetupCollisionStateTriggerPopup>(0);
					if (!popup) {
						return log::error("SetupCollisionStateTriggerPopup not found");
					}
					auto* object = typeinfo_cast<EffectGameObject*>(aa.data());
					if (!object) {
						return log::error("object cast failed");
					}
					setupButtonPopup(a, object, popup);
				});
				return false;
			}
		)->saveString(
			[](std::string str, GameObject* object, GJBaseGameLayer*) {
				auto* data = getStore(object);
				str += ",228,";
				str += ZipUtils::base64URLEncode(data->dump()).c_str();
				return str;
			}
		)->objectFromVector(
			[](GameObject* object, gd::vector<gd::string>& p0, gd::vector<void*>&, GJBaseGameLayer*, bool) {
				if (p0.size() > 228 && !p0[228].empty()) {
					auto decoded = ZipUtils::base64URLDecode(p0[228].c_str());
					getStore(object)->load(decoded);
				}
				return object;
			}
		)->customSetup(
			[](GameObject* object) {
				object->m_addToNodeContainer = true;
				object->m_outerSectionIndex = -1;
				object->m_isInvisible = false;
				object->setDisplayFrame(object->m_editorEnabled ? object->displayFrame() : CCSprite::create()->displayFrame());
				getStore(object);
			}
		)->resetObject(
			[](GameObject* pObj) {
				log::info("=== Button resetObject called ===");

				auto* gameLayer = GameManager::get()->m_gameLayer;
				if (!gameLayer) {
					log::warn("m_gameLayer is null");
					return;
				}

				Ref game(gameLayer);
				Ref object(pObj);

				int uid = hash(object->getSaveString(game).c_str());
				object->setTag(uid);

				Ref menu = typeinfo_cast<CCMenu*>(game->getUserObject("objects-menu"_spr));
				if (!menu) {
					menu = CCMenu::create();
					menu->setID("objects-menu");
					menu->setPosition(CCPointZero);
					menu->setContentSize(CCSizeZero);
					menu->setAnchorPoint(CCPointZero);
					menu->setTouchEnabled(true);
					menu->setEnabled(true);

					if (game->m_uiLayer) {
						game->m_uiLayer->addChild(menu, 9999);
						log::info("Created objects-menu in m_uiLayer");
					} else if (game->m_uiTriggerUI) {
						game->m_uiTriggerUI->addChild(menu, 9999);
						log::info("Created objects-menu in m_uiTriggerUI");
					} else {
						log::error("No place to put objects-menu");
						return;
					}
					game->setUserObject("objects-menu"_spr, menu);
				}

				while (menu->getChildByTag(uid)) {
					menu->removeChildByTag(uid);
				}

				Ref item = ButtonMenuItem::create();
				if (!item) {
					log::error("Failed to create ButtonMenuItem");
					return;
				}

				log::info("Created Button item tag={}", uid);

				item->onActivate([game, object] {
					int group = getStore(object)->getInt("activate");
					log::info("ACTIVATE group={}", group);
					if (game && group > 0) {
						game->spawnGroup(group, false, 0.f, gd::vector<int>(), 0, 0);
					}
				});

				item->onSelected([game, object] {
					int group = getStore(object)->getInt("selected");
					log::info("SELECTED group={}", group);
					if (game && group > 0) {
						game->spawnGroup(group, false, 0.f, gd::vector<int>(), 0, 0);
					}
				});

				item->onUnselected([game, object] {
					int group = getStore(object)->getInt("unselected");
					log::info("UNSELECTED group={}", group);
					if (game && group > 0) {
						game->spawnGroup(group, false, 0.f, gd::vector<int>(), 0, 0);
					}
				});

				item->setUserObject("button-object"_spr, object);
				item->setTag(uid);
				menu->addChild(item);
				menu->setTouchEnabled(true);
				menu->setEnabled(true);

				if (!menu->getActionByTag(uid)) {
					auto* action = CCRepeatForever::create(CCSequence::create(CallFuncExt::create([object, item, menu, game] {
								if (!game || !object || !item || !menu) return;

								if (item->getParent() != menu) {
									item->removeFromParentAndCleanup(false);
									menu->addChild(item);
									menu->setTouchEnabled(true);
									menu->setEnabled(true);
								}

								menu->setVisible(game->m_uiLayer ? game->m_uiLayer->isVisible() : true);

								item->setContentSize({ object->m_width, object->m_height });
								item->setAnchorPoint(object->m_editorEnabled ? CCPointZero : ccp(0.5f, 0.5f));

								auto world = object->nodeToWorldTransform();
								auto inv = CCAffineTransformInvert(menu->nodeToWorldTransform());
								
								item->setAdditionalTransform(CCAffineTransformConcat(world, inv));
								item->updateTransform();
							}), nullptr
						)
					);
					
					action->setTag(uid);
					menu->runAction(action);
				}
			}
		);

		conf->registerMe();
	}

	static void onModify(auto&) {
		setup();
	}

	virtual void resetObject() override {
		EffectGameObject::resetObject();
	}
};