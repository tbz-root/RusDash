#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/file.hpp>
#include <fryy_55.amber/include/amber.hpp>

using namespace geode::prelude;

class DocsPopup : public Popup {
public:
    static DocsPopup* create() {
        auto ret = new DocsPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        
        delete ret;
        return nullptr;
    }

protected:
    bool init() {
        if (!Popup::init(440.f, 300.f)) {
            return false;
        }

        this->setTitle("Script Trigger - Docs");

        std::string body = "# Script Trigger\n\nCould not load resources/docs/script-trigger.md";

        auto docPath = Mod::get()->getResourcesDir() / "docs" / "script-trigger.md";

        if (auto res = utils::file::readString(docPath)) {
            body = res.unwrap();
        }

        auto scroll = amber::ScrollTextArea::create(
            body,
            { 400.f, 230.f },
            0.85f,
            "chatFont.fnt"
        );

        if (scroll) {
            scroll->setAnchorPoint({ 0.5f, 0.5f });

            auto size = this->m_mainLayer->getContentSize();

            scroll->setPosition({ size.width / 2.f, size.height / 2.f - 10.f });

            this->m_mainLayer->addChild(scroll);
        } else {
            auto label = CCLabelBMFont::create(
                body.size() > 400 ? (body.substr(0, 400) + "...").c_str() : body.c_str(),
                "chatFont.fnt"
            );

            label->setScale(0.55f);
            label->setAlignment(kCCTextAlignmentLeft);
            label->setPosition(this->m_mainLayer->getContentSize() / 2);

            this->m_mainLayer->addChild(label);
        }

        return true;
    }
};
