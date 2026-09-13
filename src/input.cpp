using namespace geode::prelude;

static bool containsCyrillic(char const* text, int len) {
    if (!text || len <= 0) return false;

    for (int i = 0; i < len; ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == 0xD0 || c == 0xD1) return true;

        if (c >= 0xC0) i += (c >= 0xE0) ? 3 : 2;
        else ++i;
    }
    
    return false;
}

#include <Geode/modify/CCTextInputNode.hpp>
class $modify(CCTextInputNode) {
    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int nLen, enumKeyCodes keyCode) {
        if (!containsCyrillic(text, nLen)) {
            return CCTextInputNode::onTextFieldInsertText(sender, text, nLen, keyCode);
        }

        std::string current = this->getString();

        int maxLen = m_maxLabelLength;

        std::string toInsert(text, text + nLen);

        if (maxLen > 0 && static_cast<int>(current.size() + toInsert.size()) > maxLen * 2) {}

        current += toInsert;

        this->setString(current.c_str());
        this->refreshLabel();

        if (m_delegate) {
            m_delegate->textChanged(this);
        }

        return true;
    }
};

#include <Geode/modify/MultilineBitmapFont.hpp>
static bool utf8Next(char const*& it, char const* end, char32_t& out) {
    if (it >= end) return false;
    unsigned char c = static_cast<unsigned char>(*it);
    if (c < 0x80) {
        out = c;
        ++it;

        return true;
    }
    if ((c & 0xE0) == 0xC0 && it + 1 < end) {
        out = ((c & 0x1F) << 6) | (static_cast<unsigned char>(it[1]) & 0x3F);
        it += 2;

        return true;
    }
    if ((c & 0xF0) == 0xE0 && it + 2 < end) {
        out = ((c & 0x0F) << 12)
            | ((static_cast<unsigned char>(it[1]) & 0x3F) << 6)
            | (static_cast<unsigned char>(it[2]) & 0x3F);
        it += 3;

        return true;
    }
    if ((c & 0xF8) == 0xF0 && it + 3 < end) {
        out = ((c & 0x07) << 18)
            | ((static_cast<unsigned char>(it[1]) & 0x3F) << 12)
            | ((static_cast<unsigned char>(it[2]) & 0x3F) << 6)
            | (static_cast<unsigned char>(it[3]) & 0x3F);
        it += 4;

        return true;
    }
    ++it;

    return false;
}

static void utf8Append(std::string& s, char32_t cp) {
    if (cp < 0x80) {
        s += static_cast<char>(cp);
    } else if (cp < 0x800) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

static std::vector<std::string> splitByWidth(std::string const& text, float maxWidth, char const* font) {
    std::vector<std::string> lines;
    if (text.empty()) {
        lines.push_back("");
        return lines;
    }

    auto* lbl = CCLabelBMFont::create("", font);
    if (!lbl) {
        lines.push_back(text);

        return lines;
    }
    lbl->retain();

    std::vector<std::string> paragraphs;
    {
        size_t start = 0;
        while (start <= text.size()) {
            size_t pos = text.find('\n', start);
            if (pos == std::string::npos) {
                paragraphs.push_back(text.substr(start));
                break;
            }
            paragraphs.push_back(text.substr(start, pos - start));
            start = pos + 1;
        }
    }

    for (auto const& para : paragraphs) {
        if (para.empty()) {
            lines.push_back("");
            continue;
        }

        char const* it = para.data();
        char const* end = para.data() + para.size();
        std::string current;
        std::string lastGood;

        while (it < end) {
            char32_t cp;
            char const* before = it;
            if (!utf8Next(it, end, cp)) break;

            utf8Append(current, cp);
            lbl->setString(current.c_str());

            if (lbl->getContentSize().width > maxWidth) {
                if (!lastGood.empty()) {
                    auto sp = lastGood.rfind(' ');
                    if (sp != std::string::npos && sp + 1 < lastGood.size()) {
                        lines.push_back(lastGood.substr(0, sp));
                        current = lastGood.substr(sp + 1);
                        utf8Append(current, cp);
                    } else {
                        lines.push_back(lastGood);
                        current.clear();
                        utf8Append(current, cp);
                    }
                    lastGood = current;
                    lbl->setString(current.c_str());
                    if (lbl->getContentSize().width > maxWidth && current.size() > 0) {}
                } else {
                    lines.push_back(current);
                    current.clear();
                    lastGood.clear();
                }
            } else {
                lastGood = current;
            }
        }

        if (!current.empty()) {
            lines.push_back(current);
        }
    }

    lbl->release();
    if (lines.empty()) lines.push_back("");

    return lines;
}

#include <Geode/modify/TextArea.hpp>
class $modify(TextArea) {
    void setString(gd::string str) {
        std::string text = str;

        char const* font = "chatFont.fnt";
        if (!m_fontFile.empty()) {
            font = m_fontFile.c_str();
        }

        float width = m_width;
        if (width <= 0.f) {
            return TextArea::setString(str);
        }

        auto lines = splitByWidth(text, width, font);
        if (lines.empty()) {
            return TextArea::setString(str);
        }

        std::string dummy(lines.size() > 0 ? lines.size() - 1 : 0, '\n');
        TextArea::setString(gd::string(dummy));

        auto* children = m_label ? m_label->getChildren() : nullptr;
        if (!children) return;

        int i = 0;
        for (int j = 0; j < children->count(); ++j) {
            auto* node = static_cast<CCNode*>(children->objectAtIndex(j));
            auto* lbl = typeinfo_cast<CCLabelBMFont*>(node);
            if (!lbl) continue;

            if (i < static_cast<int>(lines.size())) {
                lbl->setString(lines[i].c_str());
                lbl->setAnchorPoint({ m_anchorPoint.x, lbl->getAnchorPoint().y });
            }
            ++i;
        }
    }
};