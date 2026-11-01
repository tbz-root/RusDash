#include "TargetGroupInput.hpp"
bool TargetGroupInput::init(std::string const& labelText, bool isFloat, float step, float minVal, float maxVal) {
    if (!CCNode::init()) return false;

    m_isFloat = isFloat;
    m_step = step;
    m_min = minVal;
    m_max = maxVal;

    this->setContentSize({130.f, 52.f});

    m_label = CCLabelBMFont::create(labelText.c_str(), "goldFont.fnt");

    m_label->setScale(0.4f);
    m_label->setAnchorPoint({ 0.5f, 0.f });
    m_label->setPosition({ this->getContentSize().width / 2.f, 36.f });

    this->addChild(m_label);

    m_bg = NineSlice::create("square02_small.png");
    
    m_bg->setContentSize({56.f, 30.f});
    m_bg->setPosition({this->getContentSize().width / 2.f, 16.f});
    m_bg->setOpacity(100.f);

    this->addChild(m_bg);

    m_input = CCTextInputNode::create(50.f, 26.f, m_isFloat ? "1.0" : "0", "bigFont.fnt");
    m_input->setAllowedChars(m_isFloat ? "0123456789.-" : "0123456789");
    m_input->setMaxLabelScale(0.55f);
    m_input->setMaxLabelWidth(48.f);
    m_input->setLabelPlaceholderScale(0.45f);
    m_input->setLabelPlaceholderColor(ccc3(120, 120, 120));
    m_input->setPosition({ this->getContentSize().width / 2.f, 16.f });
    m_input->setString(m_isFloat ? "1.00" : "0");

    this->addChild(m_input);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    this->addChild(menu);

    m_leftBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png"), this, menu_selector(TargetGroupInput::onLeft));

    m_leftBtn->setPosition({this->getContentSize().width / 2.f - 50.f, 16.f});

    menu->addChild(m_leftBtn);

    auto rightSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    rightSpr->setFlipX(true);
    m_rightBtn = CCMenuItemSpriteExtra::create(rightSpr, this, menu_selector(TargetGroupInput::onRight));

    m_rightBtn->setPosition({this->getContentSize().width / 2.f + 50.f, 16.f});

    menu->addChild(m_rightBtn);

    return true;
}

TargetGroupInput* TargetGroupInput::create(std::string const& labelText) {
    auto ret = new TargetGroupInput();
    if (ret && ret->init(labelText, false, 1.f, 0.f, 9999.f)) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);
    return nullptr;
}

TargetGroupInput* TargetGroupInput::createFloat(std::string const& labelText, float step, float minVal, float maxVal, int decimals) {
    auto ret = new TargetGroupInput();
    if (ret && ret->init(labelText, true, step, minVal, maxVal)) {
        ret->m_decimals = decimals;
        ret->autorelease();
        return ret;
    }
    
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void TargetGroupInput::onLeft(CCObject*) {
    updateValue(-m_step);
}

void TargetGroupInput::onRight(CCObject*) {
    updateValue(+m_step);
}

void TargetGroupInput::updateValue(float delta) {
    float value = getFloatValue() + delta;
    value = std::clamp(value, m_min, m_max);
    setFloatValue(value);
    if (m_callback) m_callback(value);
}

void TargetGroupInput::setString(std::string const& str) {
    if (m_input) m_input->setString(str.c_str());
}

std::string TargetGroupInput::getString() const {
    if (m_input) return m_input->getString();
    return m_isFloat ? "1.0" : "0";
}

int TargetGroupInput::getValue() const {
    return static_cast<int>(getFloatValue());
}

void TargetGroupInput::setValue(int value) {
    setFloatValue(static_cast<float>(value));
}

float TargetGroupInput::getFloatValue() const {
    auto str = getString();
    if (str.empty()) return m_isFloat ? 1.f : 0.f;
    return numFromString<float>(str).unwrapOr(m_isFloat ? 1.f : 0.f);
}

void TargetGroupInput::setFloatValue(float value) {
    value = std::clamp(value, m_min, m_max);
    if (m_isFloat) {
        setString(fmt::format("{:.{}f}", value, m_decimals));
    } else {
        setString(std::to_string(static_cast<int>(value)));
    }
}