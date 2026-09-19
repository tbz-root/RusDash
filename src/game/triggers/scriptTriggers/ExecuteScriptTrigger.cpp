#include "ExecuteScriptTrigger.hpp"
#include <unordered_set>
#include <cctype>
#include "DocsPopup.hpp"
#include "../../../nodes/FileSelectNode.hpp"
#include <Geode/binding/LevelTools.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>

using namespace geode::prelude;

ExecuteScriptTrigger::ExecuteScriptTrigger() = default;
ExecuteScriptTrigger::~ExecuteScriptTrigger() {
    if (auto* pl = PlayLayer::get()) {
        PythonInterpreter::cleanupLayer(pl);
    } else if (auto* g = GJBaseGameLayer::get()) {
        PythonInterpreter::cleanupLayer(g);
    }
}

ExecuteScriptTrigger* ExecuteScriptTrigger::create() {
    auto ret = new ExecuteScriptTrigger();
    if (ret && ret->init("edit_eStartPosBtn_001.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void ExecuteScriptTrigger::customSetup() {
    this->checkMod();
    this->setupResetListener();
}

void ExecuteScriptTrigger::triggerObject(GJBaseGameLayer* layer, int uniqueID, gd::vector<int> const* remapKeys) {
    if (!layer) return;
    if (!Mod::get()->getSettingValue<bool>("script-triggers-enabled")) return;

    auto interp = PythonInterpreter::forLayer(layer);
    if (!interp) return;

    if (!m_b64code.empty()) {
        std::string rawCode = LevelTools::base64DecodeString(m_b64code);
        interp->runString(rawCode, m_ignoreTimeout);
    }
}

void ExecuteScriptTrigger::checkMod() {
    m_active = !m_b64code.empty();
}

void ExecuteScriptTrigger::stopScript() {
    this->stopAllActions();
    if (auto pl = PlayLayer::get()) {
        if (auto interp = PythonInterpreter::forLayer(pl)) interp->stop();
    }
}

void ExecuteScriptTrigger::pauseScript() {
    this->pauseSchedulerAndActions();
    if (auto pl = PlayLayer::get()) {
        if (auto interp = PythonInterpreter::forLayer(pl)) interp->pause();
    }
}

void ExecuteScriptTrigger::resumeScript() {
    this->resumeSchedulerAndActions();
    if (auto pl = PlayLayer::get()) {
        if (auto interp = PythonInterpreter::forLayer(pl)) interp->resume();
    }
}

void ExecuteScriptTrigger::setupResetListener() {
    m_resetListener = LevelResetEvent().listen([this]() {
        this->resetScriptState();
        return true;
    });
}

void ExecuteScriptTrigger::resetScriptState() {
    log::info("Level reset event received! Resetting trigger state...");
    this->stopAllActions();
    if (auto pl = PlayLayer::get()) {
        if (auto interp = PythonInterpreter::forLayer(pl)) interp->resetState();
    }
}


namespace {
bool isKeyword(const std::string& word) {
    static const std::unordered_set<std::string> keywords = {
        "and", "as", "assert", "async", "await", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "False", "finally", "for",
        "from", "global", "if", "import", "in", "is", "lambda", "None",
        "nonlocal", "not", "or", "pass", "raise", "return", "True", "try",
        "while", "with", "yield"
    };
    return keywords.find(word) != keywords.end();
}

bool isFunction(const std::string& word, const std::string& code, size_t pos) {
    while (pos < code.size() && std::isspace(static_cast<unsigned char>(code[pos]))) pos++;
    return pos < code.size() && code[pos] == '(';
}

bool isClass(const std::string& word) {
    return !word.empty() && std::isupper(static_cast<unsigned char>(word[0]));
}
} // namespace

std::string ExecuteScriptTrigger::highlightSyntax(const std::string& code) {
    std::string result;
    size_t i = 0;

    while (i < code.size()) {
        if (code[i] == '#') {
            result += "<c-808080>";
            while (i < code.size() && code[i] != '\n' && code[i] != '\r') {
                result += code[i];
                i++;
            }
            result += "</c>";
            continue;
        }

        if (i + 1 < code.size() && code[i] == '-' && code[i + 1] == '-') {
            result += "<c-808080>";
            while (i < code.size() && code[i] != '\n' && code[i] != '\r') {
                result += code[i];
                i++;
            }
            result += "</c>";
            continue;
        }

        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            result += fmt::format("<c-FFFACD>{}", quote);
            i++;

            while (i < code.size()) {
                if (code[i] == '\\' && i + 1 < code.size()) {
                    result += code[i];
                    result += code[i + 1];
                    i += 2;
                    continue;
                }
                if (code[i] == quote) {
                    result += code[i];
                    i++;
                    break;
                }
                if (code[i] == '\n' || code[i] == '\r') break;
                result += code[i];
                i++;
            }
            result += "</c>";
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(code[i])) || code[i] == '_') {
            std::string word;
            while (i < code.size() && (std::isalnum(static_cast<unsigned char>(code[i])) || code[i] == '_')) {
                word += code[i];
                i++;
            }

            if (isKeyword(word)) {
                result += fmt::format("<c-ADD8E6>{}</c>", word);
            } else if (isFunction(word, code, i)) {
                result += fmt::format("<c-90EE90>{}</c>", word);
            } else if (isClass(word)) {
                result += fmt::format("<c-FFB6C1>{}</c>", word);
            } else {
                result += word;
            }
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(code[i]))) {
            std::string num;
            while (i < code.size() && (std::isdigit(static_cast<unsigned char>(code[i])) || code[i] == '.')) {
                num += code[i];
                i++;
            }
            result += fmt::format("<c-FFDAB9>{}</c>", num);
            continue;
        }

        result += code[i];
        i++;
    }

    return result;
}

void ExecuteScriptTrigger::setupEditPopup(EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects) {
    auto* self = typeinfo_cast<ExecuteScriptTrigger*>(trigger);
    auto mainLayer = popup->m_mainLayer;
    if (!mainLayer) return;

    auto winSize = mainLayer->getContentSize();

    popup->getChildByType<CCLabelBMFont*>(0)->setString("Execute Script Trigger");
    popup->getChildByType<CCLabelBMFont*>(0)->setFntFile("bigFont.fnt");

    auto docsMenu = CCMenu::create();
    docsMenu->setPosition({ 0.f, 0.f });
    mainLayer->addChild(docsMenu);

    auto docsBtn = CCMenuItemExt::createSpriteExtra(
        ButtonSprite::create("Open Docs", 90, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.45f),
        [](CCObject*) {
            if (auto p = DocsPopup::create()) {
                p->show();
            }
        }
    );
    docsBtn->setPosition({ winSize.width - 60.f, winSize.height - 22.f });
    docsMenu->addChild(docsBtn);

    std::string initialCode = "print(\"Upload a .py file to see the preview here...\")";
    std::string initialFilename;

    if (self) {
        if (!self->m_b64code.empty()) {
            initialCode = LevelTools::base64DecodeString(self->m_b64code);
        }

        initialFilename = self->m_filename;
    }

    auto previewRef = std::make_shared<amber::ScrollTextArea*>(nullptr);

    std::string highlighted = ExecuteScriptTrigger::highlightSyntax(initialCode);

    auto textPreview = amber::ScrollTextArea::create(
        highlighted,
        { 340.f, 120.f },
        1.f,
        "jetbrains.fnt"_spr
    );

    textPreview->setAnchorPoint({ 0.5f, 0.5f });
    textPreview->setPosition({ winSize.width / 2, winSize.height / 2 + 20 });

    mainLayer->addChild(textPreview);
    *previewRef = textPreview;

    auto select = FileSelectNode::create(300.f);
    select->setAnchorPoint({ 0.5f, 0.5f });
    select->setPosition({ winSize.width / 2, 90.f });

    if (!initialFilename.empty()) {
        select->preloadFilename(initialFilename);
    }

    select->setOnFileSelected([self, previewRef, trigger](std::filesystem::path const& path) {
        if (path.empty()) {
            FLAlertLayer::create("Error", "No file path has been set", "OK")->show();
            return;
        }

        auto result = utils::file::readString(path);
        if (result.isErr()) {
            FLAlertLayer::create(
                "Error",
                fmt::format("Failed to read {}: {}",
                    utils::string::pathToString(path),
                    result.unwrapErr()),
                "OK"
            )->show();
            return;
        }

        std::string code = result.unwrap();
        gd::string encoded = LevelTools::base64EncodeString(gd::string(code.c_str(), code.size()));
        std::string filename = utils::string::pathToString(path.filename());

        if (self) {
            self->m_b64code  = std::string(encoded.c_str(), encoded.size());
            self->m_filename = filename;
            self->checkMod();
        } else if (trigger) {
            auto data = CCDictionary::create();
            data->setObject(CCString::create(std::string(encoded.c_str(), encoded.size())), "b64code");
            data->setObject(CCString::create(filename), "filename");
            trigger->setUserObject("script-data"_spr, data);
        }

        if (*previewRef) {
            (*previewRef)->setText(ExecuteScriptTrigger::highlightSyntax(code));
        }
    });
    mainLayer->addChild(select);

    auto ignoreLabel = CCLabelBMFont::create("Ignore Timeout", "bigFont.fnt");

    ignoreLabel->setScale(0.3f);
    ignoreLabel->setPosition({ winSize.width / 2 - 20.f, 45.f });

    mainLayer->addChild(ignoreLabel);

    bool currentIgnore = self ? self->m_ignoreTimeout : false;

    class IgnoreTimeoutProxy : public CCNode {
    public:
        ExecuteScriptTrigger* trigger = nullptr;
        void onToggle(CCObject* sender) {
            auto toggler = static_cast<CCMenuItemToggler*>(sender);
            if (trigger) {
                trigger->m_ignoreTimeout = !toggler->isToggled();
            }
        }
        static IgnoreTimeoutProxy* create() {
            auto ret = new IgnoreTimeoutProxy();
            if (ret && ret->init()) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }
    };

    auto proxy = IgnoreTimeoutProxy::create();

    if (proxy) {
        proxy->trigger = self;
        mainLayer->addChild(proxy);
    }

    auto toggle = CCMenuItemToggler::createWithStandardSprites(
        proxy,
        menu_selector(IgnoreTimeoutProxy::onToggle),
        0.6f
    );

    if (currentIgnore) toggle->toggle(true);

    auto toggleMenu = CCMenu::create();

    toggleMenu->setPosition({ winSize.width / 2 + 70.f, 45.f });
    toggleMenu->addChild(toggle);
    mainLayer->addChild(toggleMenu);
}

#include <main.hpp>
#include <impl.hpp>
$execute {
    GameObjectsFactory::registerGameObject(
        GameObjectsFactory::createTriggerConfig(
            UNIQ_ID("script-trigger"),
            "execute.png"_spr
        )
        ->triggerObject([](EffectGameObject* obj, GJBaseGameLayer* layer, int p1, gd::vector<int> const* p2) {
            if (auto* trig = typeinfo_cast<ExecuteScriptTrigger*>(obj)) {
                trig->triggerObject(layer, p1, p2);
            }
        })
        ->customSetup([](GameObject* obj) {
            if (auto* trig = typeinfo_cast<ExecuteScriptTrigger*>(obj)) {
                trig->customSetup();
            }
        })
        ->editPopupSetup([](EditTriggersPopup* popup, EffectGameObject* trigger, CCArray* objects) {
            ExecuteScriptTrigger::setupEditPopup(popup, trigger, objects);
        })
    );
}
