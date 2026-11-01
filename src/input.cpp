using namespace geode::prelude;

static const std::string EXTRA_CHARS = "абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯáéíóúüñÁÉÍÓÚÜÑ¿¡";

#include <Geode/modify/CCTextInputNode.hpp>
class $modify(CCTextInputNode) {
    void applyExtraChars() {
        if (getID() != "search-bar" && getID() != "level-name-input") return;

        if (this->m_allowedChars.find("а") != std::string::npos ||
            this->m_allowedChars.find("á") != std::string::npos) {
            return;
        }

        this->m_allowedChars += EXTRA_CHARS;
    }

    void setID(std::string const& id) {
        CCTextInputNode::setID(id);
        this->applyExtraChars();
    }

    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int nLen, enumKeyCodes key) {
        this->applyExtraChars();
        return CCTextInputNode::onTextFieldInsertText(sender, text, nLen, key);
    }
};