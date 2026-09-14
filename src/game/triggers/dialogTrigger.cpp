#include <roadhogstudios.game-objects-factory/include/main.hpp>
#include <roadhogstudios.game-objects-factory/include/impl.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/modify/CCActionInterval.hpp>

static std::string encodeBase64(std::vector<uint8_t> const& data) {
	static constexpr char kTbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя0123456789+/";
	std::string out;
	out.reserve(((data.size() + 2) / 3) * 4);
	unsigned int val = 0;
	int valb = -6;
	for (uint8_t c : data) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(kTbl[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(kTbl[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

static CCSprite* makeSpr(char const* name, float target = 40.f) {
	CCSprite* spr = CCSprite::create(name);
	if (!spr) spr = CCSprite::createWithSpriteFrameName(name);
	if (spr && spr->getContentSize().width > 0.f) {
		spr->setScale(target / spr->getContentSize().width);
	}

	return spr;
}

static std::string dialogIconName(int frame) {
	frame = std::clamp(frame, 1, 56);
	
	return fmt::format("dialogIcon_{:03}.png", frame);
}

#include <Geode/modify/TextArea.hpp>
class $modify(MyTextArea, TextArea) {
	static inline auto ForceWidth = 0.f;
	static TextArea* create(
		gd::string str, char const* font, float scale,
		float width, cocos2d::CCPoint anchor, float lineHeight, bool disableColor
	) {
		return TextArea::create(str, font, scale, ForceWidth ? ForceWidth : width, anchor, lineHeight, disableColor);
	};
};

class DialogBgPickerPopup : public geode::Popup {
protected:
	std::function<void(int)> m_onPick;
	int m_selected = 1;

	bool init(int selected, std::function<void(int)> onPick) {
		if (!Popup::init(280.f, 160.f, "GJ_square01.png"))
			return false;
		m_selected = selected;
		m_onPick = std::move(onPick);
		this->setTitle("Dialog Background");

		auto menu = CCMenu::create();
		menu->setID("bg-options-menu"_spr);
		menu->setPosition(m_mainLayer->getContentSize() / 2.f);
		m_mainLayer->addChild(menu);

		for (int i = 1; i <= 6; i++) {
			auto name = fmt::format("GJ_square0{}.png", i);

			auto base = CCSprite::create(name.c_str());
			if (!base) base = CCSprite::createWithSpriteFrameName(name.c_str());
			if (!base) continue;
			base->setScale(40.f / std::max(base->getContentSize().width, 1.f));

			CCNode* node = base;
			if (i == m_selected) {
				auto holder = CCNode::create();
				base->setPosition({0.f, 0.f});
				holder->addChild(base);
				auto outline = CCSprite::create("GJ_square07.png");
				if (!outline) outline = CCSprite::createWithSpriteFrameName("GJ_square07.png");
				if (outline) {
					outline->setScale(46.f / std::max(outline->getContentSize().width, 1.f));
					outline->setPosition({0.f, 0.f});
					holder->addChild(outline);
				}
				node = holder;
			}

			auto item = CCMenuItemExt::createSpriteExtra(node, [this, i](CCMenuItem*) {
				if (m_onPick) m_onPick(i);
				this->onClose(nullptr);
			});
			item->m_animationEnabled = false;
			item->m_colorEnabled = true;
			item->setID(fmt::format("bg-pick-{}", i).c_str());

			int col = (i - 1) % 3;
			int row = (i - 1) / 3;
			item->setPosition({ (col - 1) * 70.f, (0.5f - row) * 55.f - 5.f });
			menu->addChild(item);
		}
		return true;
	}

public:
	static DialogBgPickerPopup* create(int selected, std::function<void(int)> onPick) {
		auto ret = new DialogBgPickerPopup();
		if (ret->init(selected, std::move(onPick))) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};

class DialogFramePickerPopup : public geode::Popup {
protected:
	std::function<void(int)> m_onPick;
	int m_selected = 1;

	bool init(int selected, std::function<void(int)> onPick) {
		if (!Popup::init(360.f, 260.f, "GJ_square01.png"))
			return false;
		m_selected = std::clamp(selected, 1, 56);
		m_onPick = std::move(onPick);
		this->setTitle("Character Frame");

		CCSize scrollSize = {330.f, 200.f};
		auto win = m_mainLayer->getContentSize();
		float scrollX = win.width / 2.f - scrollSize.width / 2.f;
		float scrollY = win.height / 2.f - scrollSize.height / 2.f - 8.f;

		auto scroll = geode::ScrollLayer::create(scrollSize, true, true);
		scroll->setID("frame-scroll"_spr);
		scroll->setPosition({scrollX, scrollY});
		m_mainLayer->addChild(scroll);

		constexpr int cols = 7;
		constexpr float cell = 44.f;
		constexpr float pad = 6.f;
		int rows = (56 + cols - 1) / cols;
		float totalH = pad * 2.f + rows * cell;

		auto menu = CCMenu::create();

		menu->setAnchorPoint({0.f, 0.f});
		menu->ignoreAnchorPointForPosition(false);
		menu->setPosition({0.f, 0.f});
		menu->setContentSize({scrollSize.width, totalH});

		float gridW = cols * cell;
		float startX = (scrollSize.width - gridW) * 0.5f + cell * 0.5f;

		for (int i = 1; i <= 56; i++) {
			auto name = dialogIconName(i);
			auto base = makeSpr(name.c_str(), 36.f);
			if (!base) base = CCSprite::create("GJ_button_04.png");
			CCNode* node = base;
			if (i == m_selected) {
				auto holder = CCNode::create();
				if (base) {
					base->setPosition({0.f, 0.f});
					holder->addChild(base);
				}

				if (auto outline = makeSpr("dialogIconInterface_selected.png"_spr, 42.f)) {
					outline->setPosition({0.f, 0.f});
					holder->addChild(outline);
				}

				node = holder;
			}
			auto item = CCMenuItemExt::createSpriteExtra(node ? node : CCNode::create(), [this, i](CCMenuItem*) {
				if (m_onPick) m_onPick(i);
				this->onClose(nullptr);
			});

			item->m_animationEnabled = false;
			item->m_colorEnabled = true;
			item->setID(fmt::format("frame-pick-{}", i).c_str());
			int col = (i - 1) % cols;
			int row = (i - 1) / cols;
			item->setPosition({startX + col * cell, totalH - pad - row * cell - cell * 0.5f});

			menu->addChild(item);
		}
		scroll->m_contentLayer->setContentSize({scrollSize.width, totalH});
		scroll->m_contentLayer->addChild(menu);
		scroll->scrollToTop();

		auto bar = geode::Scrollbar::create(scroll);
		bar->setID("frame-scrollbar"_spr);
		bar->setPosition({scrollX + scrollSize.width + 6.f, scrollY + scrollSize.height / 2.f});

		m_mainLayer->addChild(bar);

		return true;
	}

public:
	static DialogFramePickerPopup* create(int selected, std::function<void(int)> onPick) {
		auto ret = new DialogFramePickerPopup();
		if (ret->init(selected, std::move(onPick))) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};

class DialogIconModePopup : public geode::Popup {
protected:
	std::function<void(int)> m_onPick;

	bool init(std::function<void(int)> onPick) {
		if (!Popup::init(280.f, 140.f, "GJ_square01.png"))
			return false;
		m_onPick = std::move(onPick);
		this->setTitle("Dialog Icon");

		auto menu = CCMenu::create();
		menu->setID("icon-mode-menu"_spr);
		menu->setPosition(m_mainLayer->getContentSize() / 2.f);
		m_mainLayer->addChild(menu);

		struct Opt { int mode; char const* spr; char const* label; };
		Opt opts[] = {
			{ 0, "dialogIconInterface_none.png"_spr, "None" },
			{ 2, "dialogIconInterface_custom.png"_spr, "Custom" },
			{ 1, "dialogIcon_016.png", "Frame" },
		};
		for (int i = 0; i < 3; i++) {
			auto spr = makeSpr(opts[i].spr, 48.f);
			if (!spr) spr = CCSprite::create("GJ_button_04.png");
			auto item = CCMenuItemExt::createSpriteExtra(spr ? (CCNode*)spr : CCNode::create(), [this, mode = opts[i].mode](CCMenuItem*) {
				if (m_onPick) m_onPick(mode);
				this->onClose(nullptr);
			});
			item->m_animationEnabled = false;
			item->m_colorEnabled = true;
			item->setID(fmt::format("icon-mode-{}", opts[i].mode).c_str());
			item->setPosition({ (i - 1) * 80.f, 5.f });
			menu->addChild(item);

			auto lab = CCLabelBMFont::create(opts[i].label, "chatFont.fnt");
			lab->setScale(0.45f);
			lab->setPosition({ (i - 1) * 80.f, -40.f });
			menu->addChild(lab);
		}
		return true;
	}

public:
	static DialogIconModePopup* create(std::function<void(int)> onPick) {
		auto ret = new DialogIconModePopup();
		if (ret->init(std::move(onPick))) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};

static const std::vector<std::string> kPageTypes = {
	"text", "popup", "ntfy", "activate", "toggle", "exit", "close", "tap"
};
static const std::vector<std::string> kPageTypeLabels = {
	"Dialog", "Popup", "Notify", "Activate", "Toggle", "Exit", "Close", "Tap"
};

static matjson::Value makeEmptyPage(std::string type = "text") {
	matjson::Value line = matjson::Value::object();
	line["type"] = type;
	if (type == "text") {
		line["text"] = "";
		line["character"] = "";
		line["bg"] = 1;
	} else if (type == "popup") {
		line["title"] = "";
		line["body"] = "";
		line["btn1"] = "OK";
		line["group1"] = 0;
		line["btn2"] = "";
		line["group2"] = 0;
	} else if (type == "ntfy") {
		line["text"] = "";
		line["icon"] = "";
		line["time"] = 1.0;
	} else if (type == "activate" || type == "toggle") {
		line["id"] = 0;
	}
	return line;
}

static std::string pagePreview(matjson::Value const& line) {
	std::string type = line.isObject() && line.contains("type")
		? line["type"].asString().unwrapOr("text") : "text";
	std::string label = type;
	for (size_t i = 0; i < kPageTypes.size(); i++) {
		if (kPageTypes[i] == type) { label = kPageTypeLabels[i]; break; }
	}
	std::string extra;
	if (type == "text") extra = line.contains("text") ? line["text"].asString().unwrapOr("") : "";
	else if (type == "popup") extra = line.contains("title") ? line["title"].asString().unwrapOr("") : "";
	else if (type == "ntfy") extra = line.contains("text") ? line["text"].asString().unwrapOr("") : "";
	else if (type == "activate" || type == "toggle")
		extra = line.contains("id") ? std::to_string(line["id"].asDouble().unwrapOr(0)) : "";
	if (extra.size() > 28) extra = extra.substr(0, 28) + "...";
	return label + (extra.empty() ? "" : (" — " + extra));
}

class DialogPagesPopup : public geode::Popup {
protected:
	Ref<CCNode> m_dataNode;
	matjson::Value m_root;
	int m_page = 0;
	int m_typeIndex = 0;
	ScrollLayer* m_scroll = nullptr;
	CCMenu* m_listMenu = nullptr;
	Ref<CCLabelBMFont> m_pageInfo;
	Ref<CCLabelBMFont> m_typeLabel;
	Ref<TextInput> m_f1;
	Ref<TextInput> m_f2;
	Ref<TextInput> m_f3;
	Ref<TextInput> m_f4;
	Ref<CCLabelBMFont> m_h1;
	Ref<CCLabelBMFont> m_h2;
	Ref<CCLabelBMFont> m_h3;
	Ref<CCLabelBMFont> m_h4;
	CCNode* m_editBox = nullptr;
	int m_bg = 1;
	Ref<CCMenuItemSpriteExtra> m_bgBtn;
	int m_iconMode = 0;
	int m_frame = 1;
	std::string m_rawIcon;
	Ref<CCMenuItemSpriteExtra> m_iconBtn;

	bool init(CCNode* dataNode) {
		if (!Popup::init(420.f, 280.f, "GJ_square01.png"))
			return false;

		m_dataNode = dataNode;

		this->setTitle("Dialog Pages");

		{
			auto raw = dataNode->getID();
			auto parse = matjson::parse(raw);
			if (!parse.err() && parse.unwrap().isObject() && parse.unwrap().contains("lines")) {
				m_root = parse.unwrap();
			} else {
				m_root = matjson::Value::object();
				m_root["lines"] = matjson::Value::array();
				if (!raw.empty()) {
					matjson::Value line = matjson::Value::object();
					line["type"] = "text";
					line["text"] = raw;
					m_root["lines"].asArray().unwrap().push_back(line);
				}
			}
			if (!m_root.contains("lines") || !m_root["lines"].isArray())
				m_root["lines"] = matjson::Value::array();
			if (m_root["lines"].asArray().unwrap().empty())
				m_root["lines"].asArray().unwrap().push_back(makeEmptyPage());
		}

		auto win = this->m_mainLayer->getContentSize();

		CCSize scrollSize = {170.f, 210.f};
		CCPoint scrollPos = {15.f, win.height / 2.f - 115.f};

		auto listBg = CCScale9Sprite::create("square02b_001.png");
		listBg->setID("pages-list-bg"_spr);
		listBg->setContentSize(scrollSize);
		listBg->setColor({0, 0, 0});
		listBg->setOpacity(80);
		listBg->setAnchorPoint({0.f, 0.f});
		listBg->setPosition(scrollPos);
		this->m_mainLayer->addChild(listBg);

		m_scroll = ScrollLayer::create(scrollSize);
		m_scroll->setID("pages-scroll"_spr);
		m_scroll->setPosition(scrollPos);
		this->m_mainLayer->addChild(m_scroll);

		auto listCtrl = CCMenu::create();
		listCtrl->setID("pages-list-controls"_spr);
		float listCenterX = scrollPos.x + scrollSize.width * 0.5f;
		listCtrl->setPosition({listCenterX, 55.f});
		this->m_mainLayer->addChild(listCtrl);

		auto plusSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
		plusSpr->setScale(0.55f);
		auto addBtn = CCMenuItemExt::createSpriteExtra(plusSpr, [this](CCMenuItem*) {
			this->autoSave();
			m_root["lines"].asArray().unwrap().push_back(makeEmptyPage("text"));
			m_page = (int)m_root["lines"].asArray().unwrap().size() - 1;
			this->rebuildList();
			this->loadPageToFields();
			this->autoSave();
		});
		addBtn->setID("page-add-btn"_spr);
		addBtn->setPosition({-48.f, 0.f});
		listCtrl->addChild(addBtn);

		auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
		trashSpr->setScale(0.7f);
		auto delBtn = CCMenuItemExt::createSpriteExtra(trashSpr, [this](CCMenuItem*) {
			auto& lines = m_root["lines"].asArray().unwrap();
			if (lines.size() <= 1) return;
			this->autoSave();
			lines.erase(lines.begin() + m_page);
			if (m_page >= (int)lines.size()) m_page = (int)lines.size() - 1;
			this->rebuildList();
			this->loadPageToFields();
			this->autoSave();
		});
		delBtn->setID("page-del-btn"_spr);
		delBtn->setPosition({-16.f, 0.f});
		listCtrl->addChild(delBtn);

		auto upSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
		upSpr->setScale(0.45f);
		upSpr->setRotation(-90.f);
		upSpr->setFlipX(true);
		auto upBtn = CCMenuItemExt::createSpriteExtra(upSpr, [this](CCMenuItem*) {
			if (m_page <= 0) return;
			this->autoSave();
			auto& lines = m_root["lines"].asArray().unwrap();
			std::swap(lines[m_page], lines[m_page - 1]);
			m_page--;
			this->rebuildList();
			this->loadPageToFields();
			this->autoSave();
		});
		upBtn->setID("page-up-btn"_spr);
		upBtn->setPosition({16.f, 0.f});
		listCtrl->addChild(upBtn);

		auto downSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
		downSpr->setScale(0.45f);
		downSpr->setRotation(-90.f);
		auto downBtn = CCMenuItemExt::createSpriteExtra(downSpr, [this](CCMenuItem*) {
			auto& lines = m_root["lines"].asArray().unwrap();
			if (m_page >= (int)lines.size() - 1) return;
			this->autoSave();
			std::swap(lines[m_page], lines[m_page + 1]);
			m_page++;
			this->rebuildList();
			this->loadPageToFields();
			this->autoSave();
		});
		downBtn->setID("page-down-btn"_spr);
		downBtn->setPosition({48.f, 0.f});
		listCtrl->addChild(downBtn);

		m_editBox = CCNode::create();
		m_editBox->setID("page-edit-box"_spr);
		m_editBox->setPosition({300.f, win.height / 2.f});
		this->m_mainLayer->addChild(m_editBox);

		m_pageInfo = CCLabelBMFont::create("Page 1", "bigFont.fnt");
		m_pageInfo->setID("page-info-label"_spr);
		m_pageInfo->setScale(0.35f);
		m_pageInfo->setPosition({0.f, 110.f});
		m_editBox->addChild(m_pageInfo);

		auto typeMenu = CCMenu::create();
		typeMenu->setID("type-menu"_spr);
		typeMenu->setPosition({0.f, 85.f});
		m_editBox->addChild(typeMenu);

		m_typeLabel = CCLabelBMFont::create("Dialog", "bigFont.fnt");
		m_typeLabel->setID("type-label"_spr);
		m_typeLabel->setScale(0.4f);
		m_typeLabel->setPosition({0.f, 85.f});
		m_editBox->addChild(m_typeLabel);

		auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
		if (!prevSpr) prevSpr = CCSprite::create("GJ_arrow_03_001.png");
		if (prevSpr) prevSpr->setScale(0.7f);
		auto tPrev = CCMenuItemExt::createSpriteExtra(prevSpr ? prevSpr : CCNode::create(), [this](CCMenuItem*) {
			this->autoSave();
			m_typeIndex = (m_typeIndex - 1 + (int)kPageTypes.size()) % (int)kPageTypes.size();
			this->clearFields();
			this->updateHints();
			this->autoSave();
			this->rebuildList();
		});
		tPrev->setID("type-prev-btn"_spr);
		tPrev->setPosition({-90.f, 0.f});
		typeMenu->addChild(tPrev);

		auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
		if (!nextSpr) nextSpr = CCSprite::create("GJ_arrow_03_001.png");
		if (nextSpr) {
			nextSpr->setScale(0.7f);
			nextSpr->setFlipX(true);
		}
		auto tNext = CCMenuItemExt::createSpriteExtra(nextSpr ? nextSpr : CCNode::create(), [this](CCMenuItem*) {
			this->autoSave();
			m_typeIndex = (m_typeIndex + 1) % (int)kPageTypes.size();
			this->clearFields();
			this->updateHints();
			this->autoSave();
			this->rebuildList();
		});
		tNext->setID("type-next-btn"_spr);
		tNext->setPosition({90.f, 0.f});
		typeMenu->addChild(tNext);

		auto noNl = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
		auto withNl = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\n";

		auto mkHint = [this](float y, const char* id) {
			auto lab = CCLabelBMFont::create("", "chatFont.fnt");
			lab->setID(id);
			lab->setScale(0.4f);
			lab->setColor({160, 160, 160});
			lab->setPosition({0.f, y});
			m_editBox->addChild(lab);
			return lab;
		};

		m_h1 = mkHint(55.f, "hint-1"_spr);
		m_h2 = mkHint(15.f, "hint-2"_spr);
		m_h3 = mkHint(-25.f, "hint-3"_spr);
		m_h4 = mkHint(-65.f, "hint-4"_spr);

		auto mkIn = [this, noNl, withNl](float y, bool multi, const char* id) {
			auto inp = TextInput::create(220.f, "...", "chatFont.fnt");
			inp->setID(id);
			inp->setCommonFilter(CommonFilter::Any);
			inp->setPosition({0.f, y});
			inp->getInputNode()->setUserObject("no-parse", CCNode::create());
			inp->getInputNode()->m_allowedChars = multi ? withNl : noNl;
			if (multi) {
				inp->getBGSprite()->setContentHeight(70.f);
				inp->getInputNode()->m_textLabel->setWidth(inp->getContentWidth() - 20.f);
			}
			inp->setCallback([this](std::string const&) { this->autoSave(); this->rebuildList(); });
			m_editBox->addChild(inp);
			return inp;
		};
		
		m_f1 = mkIn(40.f, false, "field-1"_spr);
		m_f2 = mkIn(-5.f, true, "field-2"_spr);
		m_f3 = mkIn(-50.f, false, "field-3"_spr);
		m_f4 = mkIn(-90.f, false, "field-4"_spr);

		auto pickMenu = CCMenu::create();

		pickMenu->setID("icon-bg-menu"_spr);
		pickMenu->setPosition({0.f, -70.f});
		m_editBox->addChild(pickMenu);

		auto iconNode = makeSpr("dialogIconInterface_none.png"_spr, 40.f);
		
		if (!iconNode) iconNode = makeSpr("dialogIcon_001.png", 40.f);
		m_iconBtn = CCMenuItemExt::createSpriteExtra(iconNode ? (CCNode*)iconNode : CCNode::create(), [this](CCMenuItem*) {
			this->openIconModePicker();
		});
		m_iconBtn->setID("icon-btn"_spr);
		m_iconBtn->setPosition({-28.f, 0.f});
		pickMenu->addChild(m_iconBtn);

		auto bgNode = makeSpr("GJ_square01.png", 40.f);

		m_bgBtn = CCMenuItemExt::createSpriteExtra(bgNode ? (CCNode*)bgNode : CCNode::create(), [this](CCMenuItem*) {
			if (auto p = DialogBgPickerPopup::create(m_bg, [this](int picked) {
				m_bg = picked;
				this->autoSave();
				this->refreshBgButton();
			})) {
				p->show();
			}
		});
		m_bgBtn->setID("bg-btn"_spr);
		m_bgBtn->setPosition({28.f, 0.f});
		pickMenu->addChild(m_bgBtn);

		this->rebuildList();
		this->loadPageToFields();

		return true;
	}

	void openIconModePicker() {
		if (auto p = DialogIconModePopup::create([this](int mode) {
			if (mode == 0) {
				m_iconMode = 0;
				m_rawIcon.clear();
				this->autoSave();
				this->refreshIconButton();
			} else if (mode == 1) {
				if (auto fp = DialogFramePickerPopup::create(m_frame, [this](int frame) {
					m_iconMode = 1;
					m_frame = std::clamp(frame, 1, 56);
					m_rawIcon.clear();
					this->autoSave();
					this->refreshIconButton();
				})) {
					fp->show();
				}
			} else if (mode == 2) {
				file::FilePickOptions opts;
				opts.filters = { { "PNG images", { "*.png" } } };
				geode::async::spawn(
					file::pick(file::PickMode::OpenFile, opts),
					[this](Result<std::optional<std::filesystem::path>> result) {
						if (result.isErr()) {
							log::warn("file pick failed: {}", result.unwrapErr());
							return;
						}
						auto optPath = result.unwrap();
						if (!optPath.has_value()) return;
						auto path = *optPath;
						auto bin = file::readBinary(path);
						if (bin.isErr()) {
							log::warn("failed to read PNG: {}", bin.unwrapErr());
							return;
						}
						m_iconMode = 2;
						m_rawIcon = encodeBase64(bin.unwrap());
						m_frame = 1;
						this->autoSave();
						this->refreshIconButton();
						log::info("custom rawIcon set ({} chars)", m_rawIcon.size());
					}
				);
			}
		})) {
			p->show();
		}
	}

	static void fitBtnToSprite(CCMenuItemSpriteExtra* btn, CCSprite* spr) {
		if (!btn || !spr) return;
		btn->setNormalImage(spr);
		btn->setSelectedImage(spr);

		auto cs = spr->getContentSize();
		btn->setContentSize(cs);
		spr->setAnchorPoint({0.5f, 0.5f});
		spr->setPosition(cs / 2.f);
		
		if (spr->getScaleX() != 1.f || spr->getScaleY() != 1.f) {
			auto vis = CCSize{
				cs.width * spr->getScaleX(),
				cs.height * spr->getScaleY()
			};
			btn->setContentSize(vis);
			spr->setPosition(vis / 2.f);
		}
	}

	void refreshIconButton() {
		if (!m_iconBtn) return;
		constexpr float kSz = 40.f;
		CCSprite* spr = nullptr;
		if (m_iconMode == 2 && !m_rawIcon.empty()) {
			std::string b64 = m_rawIcon;
			if (auto pos = b64.find("base64,"); pos != std::string::npos)
				b64 = b64.substr(pos + 7);
			for (char& c : b64) {
				if (c == '+') c = '-';
				else if (c == '/') c = '_';
			}
			auto decoded = ZipUtils::base64URLDecode(b64);
			CCSprite* customSpr = nullptr;
			CCTexture2D* customTex = nullptr;
			if (!decoded.empty()) {
				auto image = new CCImage();
				if (image->initWithImageData(
					reinterpret_cast<unsigned char*>(const_cast<char*>(decoded.data())),
					static_cast<int>(decoded.size())
				)) {
					customTex = new CCTexture2D();
					if (customTex->initWithImage(image)) {
						image->release();
						customSpr = CCSprite::createWithTexture(customTex);
						customTex->release();
					} else {
						customTex->release();
						customTex = nullptr;
						image->release();
					}
				} else {
					image->release();
				}
			}
			if (customSpr) {
				auto osz = customSpr->getContentSize();
				if (osz.width > 0.f)
					customSpr->setScale(kSz / osz.width);
				spr = customSpr;
			} else {
				spr = makeSpr("dialogIconInterface_custom.png"_spr, kSz);
			}
		} else if (m_iconMode == 1) {
			spr = makeSpr(dialogIconName(m_frame).c_str(), kSz);
		} else {
			spr = makeSpr("dialogIconInterface_none.png"_spr, kSz);
		}
		if (!spr) spr = makeSpr("GJ_button_04.png", kSz);
		fitBtnToSprite(m_iconBtn, spr);
		m_iconBtn->setVisible(curType() == "text");
	}

	void autoSave() {
		this->fieldsToPage();
		if (m_dataNode) m_dataNode->setID(m_root.dump());
	}

	void clearFields() {
		if (m_f1) m_f1->setString("");
		if (m_f2) m_f2->setString("");
		if (m_f3) m_f3->setString("");
		if (m_f4) m_f4->setString("");
	}

	std::string curType() const {
		if (m_typeIndex < 0 || m_typeIndex >= (int)kPageTypes.size()) return "text";
		return kPageTypes[m_typeIndex];
	}

	void updateHints() {
		auto t = curType();
		auto set = [](Ref<CCLabelBMFont> l, const char* s) { if (l) l->setString(s); };
		if (t == "text") {
			set(m_h1, "Character"); set(m_h2, "Text"); set(m_h3, ""); set(m_h4, "");
		} else if (t == "popup") {
			set(m_h1, "Title"); set(m_h2, "Body"); set(m_h3, "Btn1 (->group)"); set(m_h4, "Btn2 (->group)");
		} else if (t == "ntfy") {
			set(m_h1, "Text"); set(m_h2, "Icon"); set(m_h3, "Time"); set(m_h4, "");
		} else if (t == "activate") {
			set(m_h1, "Group id"); set(m_h2, ""); set(m_h3, ""); set(m_h4, "");
		} else if (t == "toggle") {
			set(m_h1, "Group (123.0=on)"); set(m_h2, ""); set(m_h3, ""); set(m_h4, "");
		} else {
			set(m_h1, "(no fields)"); set(m_h2, ""); set(m_h3, ""); set(m_h4, "");
		}
		if (m_typeLabel) m_typeLabel->setString(kPageTypeLabels[m_typeIndex].c_str());

		auto vis = [](Ref<TextInput> i, bool v) { if (i) i->setVisible(v); };
		
		if (t == "text") { vis(m_f1,1); vis(m_f2,1); vis(m_f3,0); vis(m_f4,0); }
		else if (t == "popup") { vis(m_f1,1); vis(m_f2,1); vis(m_f3,1); vis(m_f4,1); }
		else if (t == "ntfy") { vis(m_f1,1); vis(m_f2,1); vis(m_f3,1); vis(m_f4,0); }
		else if (t == "activate" || t == "toggle") { vis(m_f1,1); vis(m_f2,0); vis(m_f3,0); vis(m_f4,0); }
		else { vis(m_f1,0); vis(m_f2,0); vis(m_f3,0); vis(m_f4,0); }

		this->refreshBgButton();
		this->refreshIconButton();
	}

	void fieldsToPage() {
		auto& lines = m_root["lines"].asArray().unwrap();
		if (m_page < 0 || m_page >= (int)lines.size()) return;
		auto& line = lines[m_page];
		if (!line.isObject()) line = matjson::Value::object();
		auto t = curType();
		line["type"] = t;
		auto g = [](Ref<TextInput> i) { return i ? std::string(i->getString()) : ""; };
		if (t == "text") {
			line["character"] = g(m_f1);
			line["text"] = g(m_f2);
			line["bg"] = m_bg;
			line.erase("icon");
			if (m_iconMode == 1) {
				line["frame"] = m_frame;
				line.erase("rawIcon");
			} else if (m_iconMode == 2 && !m_rawIcon.empty()) {
				line["rawIcon"] = m_rawIcon;
				line["frame"] = m_frame > 0 ? m_frame : 1;
			} else {
				line.erase("frame");
				line.erase("rawIcon");
			}
			line.erase("iconBg");
		} else if (t == "popup") {
			line["title"] = g(m_f1); line["body"] = g(m_f2);
			auto b1 = g(m_f3), b2 = g(m_f4);
			line["btn1"] = b1; line["btn2"] = b2; line["group1"] = 0; line["group2"] = 0;
			if (auto p = b1.find("->"); p != std::string::npos) {
				line["btn1"] = b1.substr(0, p);
				line["group1"] = utils::numFromString<int>(b1.substr(p + 2)).unwrapOr(0);
			}
			if (auto p = b2.find("->"); p != std::string::npos) {
				line["btn2"] = b2.substr(0, p);
				line["group2"] = utils::numFromString<int>(b2.substr(p + 2)).unwrapOr(0);
			}
		} else if (t == "ntfy") {
			line["text"] = g(m_f1); line["icon"] = g(m_f2);
			line["time"] = utils::numFromString<float>(g(m_f3)).unwrapOr(1.f);
		} else if (t == "activate") {
			line["id"] = utils::numFromString<int>(g(m_f1)).unwrapOr(0);
		} else if (t == "toggle") {
			line["id"] = utils::numFromString<float>(g(m_f1)).unwrapOr(0.f);
		}
	}

	void loadPageToFields() {
		auto& lines = m_root["lines"].asArray().unwrap();
		if (lines.empty()) return;
		if (m_page < 0) m_page = 0;
		if (m_page >= (int)lines.size()) m_page = (int)lines.size() - 1;
		auto& line = lines[m_page];
		std::string type = "text";
		if (line.isObject() && line.contains("type")) type = line["type"].asString().unwrapOr("text");
		m_typeIndex = 0;
		for (int i = 0; i < (int)kPageTypes.size(); i++)
			if (kPageTypes[i] == type) { m_typeIndex = i; break; }

		this->clearFields();
		auto set = [](Ref<TextInput> i, std::string s) { if (i) i->setString(s); };
		m_iconMode = 0;
		m_frame = 1;
		m_rawIcon.clear();
		if (line.isObject()) {
			if (type == "text") {
				set(m_f1, line.contains("character") ? line["character"].asString().unwrapOr("") : "");
				set(m_f2, line.contains("text") ? line["text"].asString().unwrapOr("") : "");
				m_bg = line.contains("bg") ? line["bg"].asInt().unwrapOr(1) : 1;
				if (m_bg < 1 || m_bg > 6) m_bg = 1;
				if (line.contains("rawIcon") && line["rawIcon"].isString()) {
					m_rawIcon = line["rawIcon"].asString().unwrapOr("");
					if (!m_rawIcon.empty()) m_iconMode = 2;
				}
				if (line.contains("frame")) {
					m_frame = line["frame"].asInt().unwrapOr(1);
					if (m_frame < 1 || m_frame > 56) m_frame = 1;
					if (m_iconMode != 2) m_iconMode = 1;
				}
			} else if (type == "popup") {
				set(m_f1, line.contains("title") ? line["title"].asString().unwrapOr("") : "");
				set(m_f2, line.contains("body") ? line["body"].asString().unwrapOr("") : "");
				std::string b1 = line.contains("btn1") ? line["btn1"].asString().unwrapOr("OK") : "OK";
				int g1 = line.contains("group1") ? line["group1"].asInt().unwrapOr(0) : 0;
				if (g1) b1 += "->" + std::to_string(g1);
				std::string b2 = line.contains("btn2") ? line["btn2"].asString().unwrapOr("") : "";
				int g2 = line.contains("group2") ? line["group2"].asInt().unwrapOr(0) : 0;
				if (g2) b2 += "->" + std::to_string(g2);
				set(m_f3, b1); set(m_f4, b2);
			} else if (type == "ntfy") {
				set(m_f1, line.contains("text") ? line["text"].asString().unwrapOr("") : "");
				set(m_f2, line.contains("icon") ? (line["icon"].isNumber() ? std::to_string(line["icon"].asInt().unwrapOr(0)) : line["icon"].asString().unwrapOr("")) : "");
				set(m_f3, line.contains("time") ? std::to_string(line["time"].asDouble().unwrapOr(1.0)) : "1");
			} else if (type == "activate" || type == "toggle") {
				set(m_f1, line.contains("id") ? std::to_string(line["id"].asDouble().unwrapOr(0)) : "0");
			}
		}
		if (m_pageInfo)
			m_pageInfo->setString(("Page " + std::to_string(m_page + 1) + " / " + std::to_string(lines.size())).c_str());
		this->updateHints();
	}

	void refreshBgButton() {
		if (!m_bgBtn) return;
		auto name = fmt::format("GJ_square0{}.png", m_bg);
		auto spr = makeSpr(name.c_str(), 40.f);
		fitBtnToSprite(m_bgBtn, spr);
		m_bgBtn->setVisible(curType() == "text");
	}

	void rebuildList() {
		if (!m_scroll) return;
		m_scroll->m_contentLayer->removeAllChildren();
		auto& lines = m_root["lines"].asArray().unwrap();
		float y = 0.f;
		float rowH = 28.f;
		auto menu = CCMenu::create();
		menu->setPosition({0.f, 0.f});
		for (int i = 0; i < (int)lines.size(); i++) {
			auto preview = pagePreview(lines[i]);
			bool selected = (i == m_page);
			auto bs = ButtonSprite::create(preview.c_str(), "chatFont.fnt", selected ? "GJ_button_02.png" : "GJ_button_04.png", 0.5f);
			bs->setScale(0.85f);
			auto item = CCMenuItemExt::createSpriteExtra(bs, [this, i](CCMenuItem*) {
				this->autoSave();
				m_page = i;
				this->loadPageToFields();
				this->rebuildList();
			});
			item->setID(("page-item-" + std::to_string(i)).c_str());
			item->setPosition({85.f, -y - rowH / 2.f});
			menu->addChild(item);
			y += rowH;
		}
		menu->setContentSize({170.f, std::max(y, 210.f)});
		menu->setPosition({0.f, std::max(y, 210.f)});
		m_scroll->m_contentLayer->setContentSize({170.f, std::max(y, 210.f)});
		m_scroll->m_contentLayer->addChild(menu);
		m_scroll->scrollToTop();
	}

public:
	static DialogPagesPopup* create(CCNode* dataNode) {
		auto ret = new DialogPagesPopup();
		if (ret->init(dataNode)) {
			ret->autorelease();
			return ret;
		}

		delete ret;
		return nullptr;
	}
};

#include <Geode/modify/DialogLayer.hpp>
class $modify(DialogLayer) {
	class Delegate : public DialogDelegate, public CCNode {
	public:
		inline static Delegate* s_pForNextDialogLayer;
		CREATE_FUNC(Delegate);

		Ref<DialogLayer> m_dialogLayer;
		Ref<GJBaseGameLayer> m_game;
		std::string m_replacedTextures = "";
		virtual void dialogClosed(DialogLayer* p0) {
			m_dialogLayer = nullptr;
			if (m_game) m_game->resumeSchedulerAndActions();
			if (m_game) m_game->setKeyboardEnabled(true);
			if (m_game) m_game->setTouchEnabled(true);
			if (m_game) m_game->setKeypadEnabled(true);

			if (m_game->m_uiLayer) {
			m_game->m_uiLayer->setKeyboardEnabled(false);
			m_game->m_uiLayer->setKeyboardEnabled(true);
			m_game->m_uiLayer->setKeypadEnabled(false);
			m_game->m_uiLayer->setKeypadEnabled(true);
			}

			for (auto name : string::split(m_replacedTextures, ",")) {
				CCFileUtils::get()->m_fullPathCache.erase(name.c_str());
				auto result = CCTextureCache::get()->reloadTexture(name.c_str());
			}
		};

		DialogChatPlacement placement = DialogChatPlacement::Center;
		bool hide = false;
		bool no_pause = false;
		bool unskipable = false;
		std::string character = ("");
		int characterFrame = 0;
		bool hadCharacterFrame = false;
		std::optional<GLuint> opacity = std::nullopt;
		;; std::optional<float> scale = std::nullopt;
		;;;;; std::optional<float> px = std::nullopt;
		;;;;; std::optional<float> py = std::nullopt;
		;; std::optional<int> animate = std::nullopt;

		std::map<DialogObject*, std::string> m_rawIcons;
		std::map<DialogObject*, int> m_bgs;
		int m_defaultBg = 1;
	};

	inline static GameObjectsFactory::GameObjectConfig* conf;
	static void setup() {

		GameObjectsFactory::registerGameObject(GameObjectsFactory::createTriggerConfig(
			UNIQ_ID("dialog-trigger"), "dialogTrigger.png",
			[](EffectGameObject* trigger, GJBaseGameLayer* game, int p1, gd::vector<int> const* p2)
			{
				if (!trigger) return;
				if (!game) return;

				auto sharedDialogTriggerDelegate = typeinfo_cast<Delegate*>(trigger->getUserObject("dialog-delegate"));

				if (!sharedDialogTriggerDelegate) return;

				auto DialogTriggerDataNode = typeinfo_cast<CCNode*>(trigger->getUserObject("data"_spr));

				if (!DialogTriggerDataNode) return;

				sharedDialogTriggerDelegate->m_game = game;
				sharedDialogTriggerDelegate->m_rawIcons.clear();
				sharedDialogTriggerDelegate->m_bgs.clear();
				sharedDialogTriggerDelegate->m_defaultBg = 1;

				auto raw = DialogTriggerDataNode->getID();
				matjson::Value root;
				bool isNewFormat = false;

				{
					auto parse = matjson::parse(raw);
					if (!parse.err() && parse.unwrap().isObject()) {
						root = parse.unwrap();
						isNewFormat = root.contains("lines");
					}
				}

				if (!isNewFormat) {
					auto raw_data = "[" + raw + "]";
					auto parse = matjson::parse(raw_data);
					root = parse.err()
						? matjson::parse("[ \"<cr>err: " + parse.err().value().message + "</c>\" ]").unwrapOrDefault()
						: parse.unwrapOrDefault();
				}

				auto dialogObjectsArr = CCArrayExt<DialogObject>();
				auto& placement = sharedDialogTriggerDelegate->placement;
				auto& hide = sharedDialogTriggerDelegate->hide;
				auto& no_pause = sharedDialogTriggerDelegate->no_pause;
				auto& unskipable = sharedDialogTriggerDelegate->unskipable;
				auto& character = sharedDialogTriggerDelegate->character;
				auto& characterFrame = sharedDialogTriggerDelegate->characterFrame;
				auto& hadCharacterFrame = sharedDialogTriggerDelegate->hadCharacterFrame;
				auto& opacity = sharedDialogTriggerDelegate->opacity;
				auto& scale = sharedDialogTriggerDelegate->scale;
				auto& px = sharedDialogTriggerDelegate->px;
				auto& py = sharedDialogTriggerDelegate->py;
				auto& animate = sharedDialogTriggerDelegate->animate;

				if (isNewFormat) {
					if (root.contains("placement")) {
						auto p = root["placement"].asString().unwrapOr("c");
						if (p == "t" || p == "top") placement = DialogChatPlacement::Top;
						else if (p == "b" || p == "bottom") placement = DialogChatPlacement::Bottom;
						else placement = DialogChatPlacement::Center;
					}
					if (root.contains("hide")) hide = root["hide"].asBool().unwrapOr(false);
					if (root.contains("no_pause")) no_pause = root["no_pause"].asBool().unwrapOr(false);
					if (root.contains("unskipable")) unskipable = root["unskipable"].asBool().unwrapOr(false);
					if (root.contains("opacity") && !root["opacity"].isNull())
						opacity = root["opacity"].asInt().unwrapOr(255);

					if (root.contains("scale") && !root["scale"].isNull())
						scale = root["scale"].asDouble().unwrapOr(1.0);

					if (root.contains("px") && !root["px"].isNull())
						px = root["px"].asDouble().unwrapOr(0.5);

					if (root.contains("py") && !root["py"].isNull())
						py = root["py"].asDouble().unwrapOr(0.5);

					if (root.contains("animate") && !root["animate"].isNull())
						animate = root["animate"].asInt().unwrapOr(0);

					if (root.contains("character"))
						character = root["character"].asString().unwrapOr("");

					if (root.contains("characterFrame")) {
						characterFrame = root["characterFrame"].asInt().unwrapOr(0);
						hadCharacterFrame = true;
					}

					if (root.contains("rawIcon") && root["rawIcon"].isString()) {
						auto node = CCNode::create();
						node->setID(root["rawIcon"].asString().unwrapOr(""));
						sharedDialogTriggerDelegate->setUserObject("global-raw-icon"_spr, node);
					}
				}

				auto processLegacyString = [&](std::string text) {
					if (string::contains(text, "->")) {
						auto val = string::split(text, "->");
						if (val.size() == 2) if (fileExistsInSearchPaths(val[0].c_str())) {
							CCFileUtils::get()->m_fullPathCache.erase(val[0].c_str());
							CCFileUtils::get()->m_fullPathCache.erase(val[1].c_str());
							CCFileUtils::get()->m_fullPathCache[val[0].c_str()] = CCFileUtils::get()->fullPathForFilename(val[1].c_str(), 0);
							CCTextureCache::get()->reloadTexture(val[0].c_str());

							sharedDialogTriggerDelegate->m_replacedTextures += val[0] + ",";

							return;
						}
					}

					bool idle = true;

					idle = game->m_player1->m_isOnGround ? idle : false;
					idle = fabs(game->m_player1->m_platformerXVelocity) < 0.01f ? idle : false;
					idle = fabs(game->m_player1->m_yVelocity) < 0.01f ? idle : false;

					if (string::startsWith(text, "!if_idle")) {
						if (!idle) {
							dialogObjectsArr.inner()->removeAllObjects();
						}

						return;
					}

					if (string::startsWith(text, "!no_pause")) { no_pause = true; return; }
					if (string::startsWith(text, "!hide")) { hide = true; return; }
					if (text == "!") { unskipable = !unskipable; return; }

					if (auto a = "!op:"; string::startsWith(text, a)) {
						opacity = utils::numFromString<int>(string::replace(text, a, "")).unwrapOrDefault();
						return;
					}

					if (auto a = "!s:"; string::startsWith(text, a)) {
						scale = utils::numFromString<float>(string::replace(text, a, "")).unwrapOrDefault();
						return;
					}

					if (auto a = "!px:"; string::startsWith(text, a)) {
						px = utils::numFromString<float>(string::replace(text, a, "")).unwrapOrDefault();
						return;
					}

					if (auto a = "!py:"; string::startsWith(text, a)) {
						py = utils::numFromString<float>(string::replace(text, a, "")).unwrapOrDefault();
						return;
					}

					if (auto a = "!anim:"; string::startsWith(text, a)) {
						animate = utils::numFromString<int>(string::replace(text, a, "")).unwrapOrDefault();
						return;
					}

					text = string::replace(text, "!place:", "!p:");
					if (string::startsWith(text, "!p:")) {
						auto place = string::replace(text, "!p:", "");
						if (place == "t") placement = DialogChatPlacement::Top;
						if (place == "c") placement = DialogChatPlacement::Center;
						if (place == "b") placement = DialogChatPlacement::Bottom;

						return;
					}

					text = string::replace(text, "!char:", "!c:");
					if (string::startsWith(text, "!c:")) {
						character = string::replace(text, "!c:", "");

						return;
					}

					dialogObjectsArr.push_back(DialogObject::create(character, text, characterFrame, 1.f, unskipable, ccWHITE));
				};

				matjson::Value lines = isNewFormat ? root["lines"] : root;

				for (auto& val : lines) {
					if (val.isNumber()) {
						characterFrame = val.asInt().unwrapOrDefault();
						hadCharacterFrame = true;

						continue;
					}

					if (val.isString()) {
						processLegacyString(val.asString().unwrapOrDefault());

						continue;
					}

					if (val.isObject()) {
						auto type = val.contains("type") ? val["type"].asString().unwrapOr("text") : "text";

						if (type == "text") {
							std::string txt = val.contains("text") ? val["text"].asString().unwrapOr("") : "";
							std::string chr = val.contains("character") ? val["character"].asString().unwrapOr(character) : character;
							int frame = val.contains("frame") ? val["frame"].asInt().unwrapOr(characterFrame) : characterFrame;
							bool unsk = val.contains("unskipable") ? val["unskipable"].asBool().unwrapOr(unskipable) : unskipable;

							std::string rawIcon;
							if (val.contains("rawIcon") && val["rawIcon"].isString())
								rawIcon = val["rawIcon"].asString().unwrapOr("");
							
							if (!rawIcon.empty()) {
								if (chr.empty()) chr = " ";
								if (frame <= 0) frame = 1;

								log::info("rawIcon present → characterFrame={}", frame);
							}

							int bg = 1;
							if (val.contains("bg")) {
								bg = val["bg"].asInt().unwrapOr(1);
								if (bg < 1 || bg > 6) bg = 1;
							}
							if (dialogObjectsArr.empty()) {
								sharedDialogTriggerDelegate->m_defaultBg = bg;
							}

							log::info("line bg={}", bg);

							auto obj = DialogObject::create(chr, txt, frame, 1.f, unsk, ccWHITE);
							if (!rawIcon.empty()) {
								sharedDialogTriggerDelegate->m_rawIcons[obj] = rawIcon;
								log::info("line has rawIcon ({} chars)", rawIcon.size());
							}
							sharedDialogTriggerDelegate->m_bgs[obj] = bg;
							dialogObjectsArr.push_back(obj);
						}

						else if (type == "texture") {
							auto from = val["from"].asString().unwrapOr("");
							auto to = val["to"].asString().unwrapOr("");
							if (!from.empty() && !to.empty() && fileExistsInSearchPaths(from.c_str())) {
								CCFileUtils::get()->m_fullPathCache.erase(from.c_str());
								CCFileUtils::get()->m_fullPathCache.erase(to.c_str());
								CCFileUtils::get()->m_fullPathCache[from.c_str()] = CCFileUtils::get()->fullPathForFilename(to.c_str(), 0);
								CCTextureCache::get()->reloadTexture(from.c_str());

								sharedDialogTriggerDelegate->m_replacedTextures += from + ",";
							}
						}

						else if (type == "if_idle") {
							bool idle = true;
							idle = game->m_player1->m_isOnGround ? idle : false;
							idle = fabs(game->m_player1->m_platformerXVelocity) < 0.01f ? idle : false;
							idle = fabs(game->m_player1->m_yVelocity) < 0.01f ? idle : false;
							if (!idle) {
								dialogObjectsArr.inner()->removeAllObjects();

								break;
							}
						}
						
						else if (type == "activate") {
							int id = val["id"].asInt().unwrapOr(0);
							dialogObjectsArr.push_back(DialogObject::create(
								character, "!activate:" + std::to_string(id), characterFrame, 1.f, unskipable, ccWHITE
							));
						}

						else if (type == "toggle") {
							double id = val["id"].asDouble().unwrapOr(0.0);
							dialogObjectsArr.push_back(DialogObject::create(character, "!toggle:" + std::to_string(id), characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (type == "ntfy") {
							std::string text = val.contains("text") ? val["text"].asString().unwrapOr("") : "";
							std::string icon = val.contains("icon") ? (val["icon"].isNumber() ? std::to_string(val["icon"].asInt().unwrapOr(0)) : val["icon"].asString().unwrapOr("")) : "";
							std::string time = val.contains("time") ? std::to_string(val["time"].asDouble().unwrapOr(NOTIFICATION_DEFAULT_TIME)) : "";
							std::string cmd = "!ntfy:" + text;
							if (!icon.empty()) cmd += "//" + icon;
							if (!time.empty()) cmd += "//" + time;
							dialogObjectsArr.push_back(DialogObject::create(character, cmd, characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (type == "popup") {
							std::string title = val.contains("title") ? val["title"].asString().unwrapOr("") : "";
							std::string body = val.contains("body") ? val["body"].asString().unwrapOr("") : "";
							std::string btn1 = val.contains("btn1") ? val["btn1"].asString().unwrapOr("OK") : "OK";
							int group1 = val.contains("group1") ? val["group1"].asInt().unwrapOr(0) : 0;
							std::string btn2 = val.contains("btn2") ? val["btn2"].asString().unwrapOr("") : "";
							int group2 = val.contains("group2") ? val["group2"].asInt().unwrapOr(0) : 0;

							std::string cmd = "!popup:" + title + "//" + body + "//" + btn1;
							if (group1) cmd += "->" + std::to_string(group1);
							if (!btn2.empty()) {
								cmd += "//" + btn2;
								if (group2) cmd += "->" + std::to_string(group2);
							}

							dialogObjectsArr.push_back(DialogObject::create(character, cmd, characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (type == "exit") {
							dialogObjectsArr.push_back(DialogObject::create(character, "!exit", characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (type == "close") {
							dialogObjectsArr.push_back(DialogObject::create(character, "!close", characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (type == "tap") {
							dialogObjectsArr.push_back(DialogObject::create(character, "!tap", characterFrame, 1.f, unskipable, ccWHITE));
						}

						else if (val.contains("text")) {
							dialogObjectsArr.push_back(DialogObject::create(character, val["text"].asString().unwrapOr(""), characterFrame, 1.f, unskipable, ccWHITE));
						}
					}
				}

				if (false) log::debug("placement {}", static_cast<int>(placement));

				auto& dialog = sharedDialogTriggerDelegate->m_dialogLayer;
				if (dialogObjectsArr.size()) {
					if (dialog) dialog->removeFromParent();
					Delegate::s_pForNextDialogLayer = sharedDialogTriggerDelegate;
					dialog = DialogLayer::createDialogLayer(
						dialogObjectsArr[0], dialogObjectsArr.inner(),
						sharedDialogTriggerDelegate->m_defaultBg
					);
					dialog->updateChatPlacement(placement);
					if (animate.has_value()) dialog->animateIn(
						(DialogAnimationType)animate.value()
					); else dialog->animateInRandomSide();

					if (game and game->isRunning()) {
						auto scene = CCDirector::get()->m_pRunningScene;
						CCDirector::get()->m_pRunningScene = (CCScene*)game->m_uiLayer;
						CCDirector::get()->m_pRunningScene->setVisible(1);
						dialog->addToMainScene();
						CCDirector::get()->m_pRunningScene = scene;
					}

					dialog->runAction(CCRepeatForever::create(CCSequence::create(CallFuncExt::create(
						[dialog = Ref(dialog), l = Ref(dialog->m_mainLayer), opacity, scale, px, py]() {
							auto someSprite = l->getChildByType<CCSprite*>(0);
							auto hasIcon = !someSprite ? false : someSprite->getZOrder() == 2;

							if (Ref a = l->getChildByType<TextArea*>(0)) {
								a->setPositionX(hasIcon ? -92.000f : -174.000f);
							}

							if (Ref a = l->getChildByType<CCLabelBMFont*>(0)) {
								a->setPositionX(hasIcon ? -93.000f : -176.000f);
							}

							if (Ref a = l->getChildByType<CCScale9Sprite*>(1)) a->setOpacity(0);

							if (opacity.has_value()) dialog->setOpacity(opacity.value());
							if (scale.has_value()) dialog->m_mainLayer->setScale(scale.value());

							auto ptmp = dialog->m_mainLayer->getAnchorPoint();
							if (px.has_value()) ptmp.x = px.value();
							if (py.has_value()) ptmp.y = py.value();

							dialog->m_mainLayer->setAnchorPoint(ptmp);
						}
					), nullptr)));

					if (game and not no_pause) {
						if (auto playLayer = typeinfo_cast<PlayLayer*>(game)) {
        					if (playLayer->m_player1) {
            					playLayer->m_player1->m_platformerXVelocity = 0.f;
        					}
        					if (playLayer->m_player2) {
            					playLayer->m_player2->m_platformerXVelocity = 0.f;
        					}
    				}
						game->setKeyboardEnabled(false);
						game->setTouchEnabled(false);
						game->pauseSchedulerAndActions();
					}
				}

				if(dialog and hide) dialog->removeFromParent();

			},
			[](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects)
			{
				if (!popup) return;
				if (!trigger) return;
				if (!objects) return;
				if (auto data = typeinfo_cast<CCNode*>(trigger->getUserObject("data"_spr))) {
					if (auto title = popup->getChildByType<CCLabelBMFont*>(0)) {
						title->setString("Dialog Trigger");
						title->setAnchorPoint(CCPointMake(0.5f, 0.3f));
					}
					if (auto inf = popup->m_buttonMenu->getChildByType<InfoAlertButton*>(0)) {
						inf->setVisible(false);
					}

					auto pagesBtn = CCMenuItemExt::createSpriteExtra(
						ButtonSprite::create("Edit Pages", "bigFont.fnt", "GJ_button_01.png", 0.6f),
						[data = Ref(data)](CCMenuItem*) {
							if (auto p = DialogPagesPopup::create(data)) {
								p->show();
							}
						}
					);
					pagesBtn->setID("edit-pages-btn"_spr);
					pagesBtn->setPosition(CCPointMake(0.f, 40.f));
					popup->m_buttonMenu->addChild(pagesBtn);

					auto run = CCMenuItemExt::createSpriteExtra(
						ButtonSprite::create("Run"),
						[trigger = Ref(trigger)](CCMenuItem*) {
							trigger->triggerObject(GameManager::get()->m_gameLayer, 0, nullptr);
						}
					);

					run->setID("run-btn"_spr);
					run->setPosition(CCPointMake(66.f, 0.f));

					popup->m_buttonMenu->addChild(run);
				}
			}
		)->customSetup(
			[](GameObject* object)
			{
				if (!object) return object;
				object->m_addToNodeContainer = true;
				object->setUserObject("dialog-delegate", Delegate::create());
				auto data = CCNode::create();
				object->setUserObject("data"_spr, data);
				object->m_objectType = GameObjectType::CustomRing;
				object->m_hasNoEffects = true;
				((RingObject*)object)->RingObject::m_claimTouch = true;
				
				return object;
			}
		)->saveString(
			[](std::string str, GameObject* object, GJBaseGameLayer* level)
			{
				if (!object) return gd::string(str.c_str());
				if (!level) return gd::string(str.c_str());
				if (auto data = typeinfo_cast<CCNode*>(object->getUserObject("data"_spr))) {
					str += ",228,";
					str += ZipUtils::base64URLEncode(data->getID().c_str()).c_str();
				}

				return gd::string(str.c_str());
			}
		)->objectFromVector(
			[](GameObject* object, gd::vector<gd::string>& p0, gd::vector<void*>&, void*, bool)
			{
				if (!object) return object;

				auto data = typeinfo_cast<CCNode*>(object->getUserObject("data"_spr));
				if (data) {
					data->setID(ZipUtils::base64URLDecode(p0[228].c_str()).c_str());
				};

				return object;
			}
		)->activatedByPlayer(
			[](EnhancedGameObject* asd, PlayerObject* lsd) { asd->triggerObject(lsd->m_gameLayer, 0, nullptr); }
		));

	};

	static void onModify(auto&) { setup(); }

	void skip(bool close = false) {
		close ? queueInMainThread([xd = Ref(this)] { if (xd) xd->onClose(); }) : queueInMainThread([xd = Ref(this)] { if (xd) xd->handleDialogTap(); });
	};

	bool processDialogObject(DialogObject * object) {
		Ref del = typeinfo_cast<Delegate*>(m_delegate);

		if (!del) return false;
		if (!del->m_game) return false;

		auto& placement = del->placement;
		auto& hide = del->hide;
		auto& no_pause = del->no_pause;
		auto& unskipable = del->unskipable;
		auto& character = del->character;
		auto& characterFrame = del->characterFrame;
		auto& hadCharacterFrame = del->hadCharacterFrame;
		auto& opacity = del->opacity;
		auto& scale = del->scale;
		auto& px = del->px;
		auto& py = del->py;
		auto& animate = del->animate;

		std::string text = object->m_text.c_str();

		if (string::contains(text, "->")) {
			auto val = string::split(text, "->");
			if (val.size() == 2) if (fileExistsInSearchPaths(val[0].c_str())) {
				CCFileUtils::get()->m_fullPathCache.erase(val[0].c_str());
				CCFileUtils::get()->m_fullPathCache.erase(val[1].c_str());
				CCFileUtils::get()->m_fullPathCache[val[0].c_str()] = CCFileUtils::get()->fullPathForFilename(val[1].c_str(), 0);
				CCTextureCache::get()->reloadTexture(val[0].c_str());
				del->m_replacedTextures += val[0] + ",";

				return true;
			}
		}

		if (string::startsWith(text, "!exit")) {
			Ref playlayer = typeinfo_cast<PlayLayer*>(del->m_game.data());
			if (playlayer and playlayer->isRunning()) {
				playlayer->pauseGame(0);
				CCScene::get()->getChildByType<PauseLayer>(0)->onQuit(0);
			}
			return true;
		}

		if (auto a = "!activate:"; string::startsWith(text, a)) {
			auto id = utils::numFromString<int>(string::replace(text, a, "")).unwrapOrDefault();
			if (del->m_game) del->m_game->spawnGroup(id, false, 0, gd::vector<int>(), -1, -1);

			return true;
		}
		if (auto a = "!toggle:"; string::startsWith(text, a)) {
			auto id = utils::numFromString<float>(string::replace(text, a, "")).unwrapOrDefault();
			if (del->m_game) del->m_game->toggleGroup(id, id > (int)id);

			return true;
		}

		if (auto a = "!ntfy:"; string::startsWith(text, a)) {
			auto args = string::split(string::replace(text, a, ""), "//");
			auto str = args[0];
			auto icon = NotificationIcon::None;
			auto sprite = (CCSprite*)nullptr;
			auto time = NOTIFICATION_DEFAULT_TIME;

			if (args.size() > 1) {
				auto id = utils::numFromString<int>(args[1]);
				if (id.isOk()) icon = (NotificationIcon)id.unwrapOrDefault();
				else {
					sprite = CCSprite::createWithSpriteFrameName(args[1].c_str());
					if (auto a = CCSprite::create(args[1].c_str())) sprite = a;
				}
			}

			if (args.size() > 2) time = utils::numFromString<float>(
				args[2]
			).unwrapOrDefault();

			if (sprite) Notification::create(str, sprite, time)->show();
			else Notification::create(str, icon, time)->show();

			return true;
		}

		if (auto a = "!popup:"; string::startsWith(text, a)) {
			auto args = string::split(string::replace(text, a, ""), "//");

			auto title = args[0];
			auto cap = std::string("");
			auto btn1 = std::string("OK");
			auto btn2 = std::string();
			auto group1 = 0;
			auto group2 = 0;

			if (args.size() > 1) cap = args[1];
			if (args.size() > 2) {
				auto spl = string::split(args[2], "->");
				btn1 = spl[0];
				if (spl.size() > 1) group1 = utils::numFromString<int>(spl[1]).unwrapOrDefault();
			}
			if (args.size() > 3) {
				auto spl = string::split(args[3], "->");
				btn2 = spl[0];
				if (spl.size() > 1) group2 = utils::numFromString<int>(spl[1]).unwrapOrDefault();
			}

			if (del->m_game and del->m_game->isRunning()) {
				auto scene = CCDirector::get()->m_pRunningScene;
				CCDirector::get()->m_pRunningScene = (CCScene*)del->m_game->m_uiLayer;
				CCDirector::get()->m_pRunningScene->setVisible(1);
				if (not no_pause) {
					del->m_game->setKeyboardEnabled(false);
					del->m_game->setTouchEnabled(false);
					del->m_game->pauseSchedulerAndActions();
				}
				auto popup = createQuickPopup(
					title.c_str(), cap.c_str(),
					btn1.empty() ? nullptr : btn1.c_str(),
					btn2.empty() ? nullptr : btn2.c_str(),
					[_ = Ref(this), object = Ref(object),
					game = Ref(del->m_game), del = Ref(del),
					group1, group2](CCNode* a, bool btn2) {
						del->dialogClosed(nullptr);
						if (game) game->spawnGroup(
							btn2 ? group2 : group1, false, 0, gd::vector<int>(), -1, -1
						);
						if (_) {
							a->getParent()->addChild(_);
							_->setUserObject("call-org-display", object);
							_->skip();
						}
					}
				);

				CCDirector::get()->m_pRunningScene = scene;

				if (placement != DialogChatPlacement::Center) {
					popup->m_mainLayer->ignoreAnchorPointForPosition(false);
					popup->m_mainLayer->setAnchorPoint(
						{ 0.f, [](DialogChatPlacement placement) -> float {
							if (placement == DialogChatPlacement::Top) return -0.250f;
							if (placement == DialogChatPlacement::Bottom) return 0.250f;
							return 0.f;
						}(placement) }
					);
				}
				if (opacity.has_value()) popup->setOpacity(opacity.value());
				if (scale.has_value()) popup->setScale(scale.value());

				auto ptmp = popup->m_mainLayer->getAnchorPoint();

				if (px.has_value()) ptmp.x = px.value();
				if (py.has_value()) ptmp.y = py.value();

				popup->m_mainLayer->setAnchorPoint(ptmp);

			}

			setUserObject("dont-skip", object);
			return true;
		}

		if (string::startsWith(text, "!close")) {
			queueInMainThread([xd = Ref(this)] { xd->onClose(); });
		}
		if (string::startsWith(text, "!tap")) {
			queueInMainThread([xd = Ref(this)] { xd->handleDialogTap(); });
		}

		return false;
	}

	void displayDialogObject(DialogObject * object) {
		if (typeinfo_cast<Delegate*>(m_delegate)) MyTextArea::ForceWidth = 340.f;
		if (getUserObject("call-org-display") == object) {
			DialogLayer::displayDialogObject(object);
			applyCustomIcon(object);
			applyDialogBg(object);
			this->runAction(CCSequence::createWithTwoActions(
				CCDelayTime::create(0.0f),
				CallFuncExt::create([this, object = Ref(object)] {
					if (object) {
						applyCustomIcon(object);
						applyDialogBg(object);
					}
				})
			));

			MyTextArea::ForceWidth = false;

			return;
		}
		if (processDialogObject(object)) {
			if (getUserObject("dont-skip") == object) queueInMainThread(
				[xd = Ref(this)] { xd->removeFromParentAndCleanup(false); }
			); else skip();
		}
		else {
			DialogLayer::displayDialogObject(object);
			applyCustomIcon(object);
			applyDialogBg(object);
			this->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(0.0f), CallFuncExt::create([this, object = Ref(object)] {
					if (object) {
						applyCustomIcon(object);
						applyDialogBg(object);
					}
				})
			));
		}

		MyTextArea::ForceWidth = false;
	};

	void applyDialogBg(DialogObject* object) {
		if (!object || !m_mainLayer) return;
		int bg = 1;
		if (auto del = typeinfo_cast<Delegate*>(m_delegate)) {
			if (auto it = del->m_bgs.find(object); it != del->m_bgs.end())
				bg = it->second;
			else
				bg = del->m_defaultBg;
		}

		if (bg < 1 || bg > 6) bg = 1;

		auto name = fmt::format("GJ_square0{}.png", bg);

		log::info("applying dialog bg {}", name);

		CCScale9Sprite* target = nullptr;
		float bestArea = 0.f;
		for (auto* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
			if (auto s9 = typeinfo_cast<CCScale9Sprite*>(child)) {
				if (s9->getOpacity() == 0) continue;
				auto sz = s9->getContentSize();
				float area = sz.width * sz.height;

				if (area > bestArea) {
					bestArea = area;
					target = s9;
				}
			}
		}
		if (!target) {
			log::warn("no Scale9Sprite found for dialog bg");

			return;
		}

		auto size = target->getContentSize();
		auto caps = target->getCapInsets();
		if (!target->initWithFile(name.c_str())) {
			log::warn("initWithFile failed for {}", name);
			return;
		}
		if (caps.size.width > 0 || caps.size.height > 0)
			target->setCapInsets(caps);
		target->setPreferredSize(size);
		target->setContentSize(size);
		target->setVisible(true);
		target->setOpacity(255);
		log::info("bg re-inited on Scale9Sprite ({}x{})", size.width, size.height);
	}

	CCSprite* ensurePortraitSprite() {
		if (m_characterSprite) {
			m_characterSprite->setVisible(true);
			log::info("using DialogLayer m_characterSprite");
			return m_characterSprite;
		}

		if (m_mainLayer) {
			for (auto* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
				if (auto spr = typeinfo_cast<CCSprite*>(child)) {
					if (spr->getZOrder() == 2) {
						log::info("found portrait sprite by zOrder 2");
						m_characterSprite = spr;
						spr->setVisible(true);
						return spr;
					}
				}
			}
		}

		log::info("creating custom portrait sprite");

		auto spr = CCSprite::create();

		spr->setID("custom-portrait"_spr);
		spr->setZOrder(2);

		if (m_mainLayer) {
			spr->setPosition({-130.f, 0.f});
			m_mainLayer->addChild(spr);
		}

		m_characterSprite = spr;

		return spr;
	}

	static constexpr float kPortraitW = 70.f;
	static constexpr float kPortraitH = 75.f;

	void normalizePortraitSize() {
		if (!m_characterSprite) return;

		auto sz = m_characterSprite->getContentSize();

		if (sz.width <= 0.f || sz.height <= 0.f) {
			m_characterSprite->setContentSize({kPortraitW, kPortraitH});
			m_characterSprite->setScale(1.f);
		} else {
			m_characterSprite->setScaleX(kPortraitW / sz.width);
			m_characterSprite->setScaleY(kPortraitH / sz.height);
		}

		log::info("portrait size normalized to {}x{} (tex {}x{}, scale {}x{})", kPortraitW, kPortraitH, sz.width, sz.height, m_characterSprite->getScaleX(), m_characterSprite->getScaleY());
	}

	void applyCustomIcon(DialogObject* object) {
		if (!object) {
			log::info("applyCustomIcon: object is null");
			
			return;
		}

		std::string b64;
		auto del = typeinfo_cast<Delegate*>(m_delegate);
		if (del) {
			if (auto it = del->m_rawIcons.find(object); it != del->m_rawIcons.end())
				b64 = it->second;
			if (b64.empty()) {
				if (auto node = typeinfo_cast<CCNode*>(del->getUserObject("global-raw-icon"_spr)))
					b64 = node->getID();
			}
		}

		if (b64.empty()) return;

		auto portrait = ensurePortraitSprite();
		if (!portrait) {
			log::warn("applyCustomIcon: could not get/create portrait sprite");

			return;
		}

		log::info("loading rawIcon (base64, {} chars)", b64.size());
		if (auto pos = b64.find("base64,"); pos != std::string::npos)
			b64 = b64.substr(pos + 7);
		for (char& c : b64) {
			if (c == '+') c = '-';
			else if (c == '/') c = '_';
		}
		
		auto decoded = ZipUtils::base64URLDecode(b64);
		if (decoded.empty()) {
			log::warn("rawIcon base64 decode failed");

			return;
		}
		
		auto image = new CCImage();
		bool ok = image->initWithImageData(
			reinterpret_cast<unsigned char*>(const_cast<char*>(decoded.data())),
			static_cast<int>(decoded.size())
		);
		if (!ok) {
			log::warn("rawIcon CCImage init failed");

			image->release();
			
			return;
		}
		auto tex = new CCTexture2D();
		if (!tex->initWithImage(image)) {
			tex->release();
			image->release();

			log::warn("rawIcon texture init failed");

			return;
		}
		image->release();
		m_characterSprite->setVisible(true);
		m_characterSprite->setTexture(tex);

		auto size = tex->getContentSize();

		m_characterSprite->setTextureRect(CCRectMake(0, 0, size.width, size.height));

		tex->release();

		normalizePortraitSize();

		log::info("rawIcon applied ({}x{})", size.width, size.height);
	};

	bool init(DialogObject * object, cocos2d::CCArray * objects, int background) {
		m_delegate = m_delegate ? m_delegate : Delegate::s_pForNextDialogLayer;
		if (!DialogLayer::init(object, objects, background)) return false;
		this->runAction(CCSequence::createWithTwoActions(
			CCDelayTime::create(0.1f), CallFuncExt::create(
				[_this = Ref(this)] { _this->m_handleTap = (1); }
			)));
		return true;
	};

	void displayNextObject_() {};
};