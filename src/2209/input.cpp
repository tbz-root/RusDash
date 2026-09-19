using namespace geode::prelude;

#include <optional>
#include <algorithm>
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
    static void onModify(auto& self) {
        if (auto h = self.getHook("CCTextInputNode::onTextFieldInsertText")) {
            (void)h.unwrap()->setPriority(1000);
        }
    }

    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int nLen, enumKeyCodes keyCode) {
        if (!containsCyrillic(text, nLen)) {
            return CCTextInputNode::onTextFieldInsertText(sender, text, nLen, keyCode);
        }

        std::string current = this->getString();
        std::string toInsert(text, text + nLen);

        if (m_maxLabelLength > 0 &&
            static_cast<int>(current.size() + toInsert.size()) > m_maxLabelLength * 2) {
            return true;
        }

        current += toInsert;

        if (m_textField) {
            m_textField->setString(current.c_str());
        }
        this->setString(current);
        this->refreshLabel();

        if (m_delegate) {
            m_delegate->textChanged(this);
        }

        return true;
    }
};

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

static std::optional<ccColor3B> colorFromTag(char tag) {
    switch (tag) {
        case 'b': return ccColor3B{0x4A, 0x52, 0xE1};
        case 'g': return ccColor3B{0x40, 0xE3, 0x48};
        case 'l': return ccColor3B{0x60, 0xAB, 0xEF};
        case 'j': return ccColor3B{0x32, 0xC8, 0xFF};
        case 'y': return ccColor3B{0xFF, 0xFF, 0x00};
        case 'o': return ccColor3B{0xFF, 0xA5, 0x4B};
        case 'r': return ccColor3B{0xFF, 0x5A, 0x5A};
        case 'p': return ccColor3B{0xFF, 0x00, 0xFF};
        case 'a': return ccColor3B{0x96, 0x32, 0xFF};
        case 'd': return ccColor3B{0xFF, 0x96, 0xFF};
        case 'c': return ccColor3B{0xFF, 0xFF, 0x96};
        case 'f': return ccColor3B{0x96, 0xFF, 0xFF};
        case 's': return ccColor3B{0xFF, 0xDC, 0x41};
        default:  return std::nullopt;
    }
}

static size_t trySkipTag(std::string const& s, size_t pos, ccColor3B& current) {
    if (pos >= s.size() || s[pos] != '<') return 0;

    if (pos + 3 < s.size() && s[pos + 1] == '/' && s[pos + 2] == 'c' && s[pos + 3] == '>') {
        current = {255, 255, 255};
        return 4;
    }
    if (pos + 3 < s.size() && s[pos + 1] == 'c' && s[pos + 3] == '>') {
        if (auto col = colorFromTag(s[pos + 2])) {
            current = *col;
        }
        return 4;
    }
    if (pos + 5 < s.size() && s[pos + 1] == 'i' && s[pos + 5] == '>') return 6;
    if (pos + 3 < s.size() && s[pos + 1] == '/' && s[pos + 2] == 'i' && s[pos + 3] == '>') return 4;
    if (pos + 5 < s.size() && s[pos + 1] == 'd' && s[pos + 5] == '>') return 6;
    if (pos + 5 < s.size() && s[pos + 1] == 's' && s[pos + 5] == '>') return 6;
    if (pos + 3 < s.size() && s[pos + 1] == '/' && s[pos + 2] == 's' && s[pos + 3] == '>') return 4;

    return 0;
}

static std::string removeTags(std::string const& str) {
    std::string out;
    out.reserve(str.size());
    ccColor3B dummy = {255, 255, 255};
    size_t i = 0;
    while (i < str.size()) {
        if (size_t n = trySkipTag(str, i, dummy)) {
            i += n;
            continue;
        }
        out += str[i];
        ++i;
    }

    return out;
}

static void colorizeLine(
    std::string const& original,
    size_t& pos,
    std::string const& line,
    CCLabelBMFont* lbl,
    ccColor3B& current,
    bool spacesHaveSprites
) {
    if (!lbl) return;
    auto* letters = lbl->getChildren();
    if (!letters) return;

    int spriteIdx = 0;
    int const nSprites = letters->count();

    char const* lit = line.data();
    char const* lend = line.data() + line.size();

    while (lit < lend) {
        char32_t want;
        if (!utf8Next(lit, lend, want)) break;

        while (pos < original.size()) {
            if (size_t n = trySkipTag(original, pos, current)) {
                pos += n;
                continue;
            }

            char const* p = original.data() + pos;
            char const* end = original.data() + original.size();
            char32_t cp;
            if (!utf8Next(p, end, cp)) {
                pos = original.size();
                break;
            }
            size_t nextPos = static_cast<size_t>(p - original.data());

            if (cp == '\n' || cp == '\r') {
                pos = nextPos;
                continue;
            }

            if (cp == want) {
                pos = nextPos;
                break;
            }

            if ((cp == ' ' || cp == '\t') && want != ' ' && want != '\t') {
                pos = nextPos;
                continue;
            }

            pos = nextPos;
            break;
        }

        if (!spacesHaveSprites && (want == ' ' || want == '\t')) {
            continue;
        }

        if (spriteIdx >= nSprites) break;

        auto* node = static_cast<CCNode*>(letters->objectAtIndex(spriteIdx));
        if (auto* spr = typeinfo_cast<CCSprite*>(node)) {
            spr->setColor(current);
        }
        ++spriteIdx;
    }
}

#include <Geode/modify/MultilineBitmapFont.hpp>
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
        std::string original = str;
        std::string plain = removeTags(original);

        char const* font = "chatFont.fnt";
        if (!m_fontFile.empty()) {
            font = m_fontFile.c_str();
        }

        float width = m_width;
        if (width <= 0.f) {
            return TextArea::setString(str);
        }

        auto lines = splitByWidth(plain, width, font);
        if (lines.empty()) {
            return TextArea::setString(str);
        }

        std::string dummy(lines.size() > 1 ? lines.size() - 1 : 0, '\n');
        TextArea::setString(gd::string(dummy));

        auto* children = m_label ? m_label->getChildren() : nullptr;
        if (!children) return;

        ccColor3B current = {255, 255, 255};
        size_t srcPos = 0;

        int lineIdx = 0;
        for (int j = 0; j < children->count(); ++j) {
            auto* node = static_cast<CCNode*>(children->objectAtIndex(j));
            auto* lbl = typeinfo_cast<CCLabelBMFont*>(node);
            if (!lbl) continue;

            if (lineIdx >= static_cast<int>(lines.size())) break;

            auto const& line = lines[lineIdx];
            lbl->setString(line.c_str());
            lbl->setAnchorPoint({ m_anchorPoint.x, lbl->getAnchorPoint().y });

            int nSprites = lbl->getChildren() ? lbl->getChildren()->count() : 0;
            int nChars = 0;
            {
                char const* it = line.data();
                char const* end = line.data() + line.size();
                char32_t cp;
                while (utf8Next(it, end, cp)) ++nChars;
            }

            colorizeLine(original, srcPos, line, lbl, current, nSprites == nChars);

            ++lineIdx;
        }
    }
};