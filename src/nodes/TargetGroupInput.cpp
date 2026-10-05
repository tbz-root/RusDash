#include "TargetGroupInput.hpp"
bool TargetGroupInput::init() {
    if (!CCNode::init()) return false;

    this->setContentSize({ 120.f, 50.f });

    m_label = CCLabelBMFont::create("Target Group:", "goldFont.fnt");

    m_label->setScale(0.4f);
    m_label->setAnchorPoint({ 0.5f, 0.f });
    m_label->setPosition({this->getContentSize().width / 2.f, 36.f});

    this->addChild(m_label);

    m_bg = CCScale9Sprite::create("square02_small.png");

    m_bg->setContentSize({ 48.f, 32.f });
    m_bg->setPosition({this->getContentSize().width / 2.f, 16.f});

    this->addChild(m_bg);
    
    m_input = CCTextInputNode::create(40.f, 28.f, "0", "bigFont.fnt");

    m_input->setAllowedChars("1234567890");
    m_input->setMaxLabelScale(0.6f);
    m_input->setMaxLabelWidth(40.f);
    m_input->setLabelPlaceholderScale(0.5f);
    m_input->setLabelPlaceholderColor(ccc3(120, 120, 120));
    m_input->setPosition({this->getContentSize().width / 2.f, 16.f});
    m_input->setString("0");

    this->addChild(m_input);

    auto menu = CCMenu::create();

    menu->setPosition({ 0.f, 0.f });

    this->addChild(menu);

    auto leftSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");

    leftSpr->setFlipX(true);
    leftSpr->setScale(0.7f);

    m_leftBtn = CCMenuItemSpriteExtra::create(
        leftSpr,
        this,
        menu_selector(TargetGroupInput::onLeft)
    );

    m_leftBtn->setPosition({ this->getContentSize().width / 2.f - 40.f, 16.f });

    menu->addChild(m_leftBtn);

    auto rightSpr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");

    rightSpr->setScale(0.7f);

    m_rightBtn = CCMenuItemSpriteExtra::create(
        rightSpr,
        this,
        menu_selector(TargetGroupInput::onRight)
    );

    m_rightBtn->setPosition({ this->getContentSize().width / 2.f + 40.f, 16.f });

    menu->addChild(m_rightBtn);

    return true;
}

TargetGroupInput* TargetGroupInput::create() {
    auto ret = new TargetGroupInput();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    CC_SAFE_DELETE(ret);

    return nullptr;
}

void TargetGroupInput::onLeft(CCObject*) {
    updateValue(-1);
}

void TargetGroupInput::onRight(CCObject*) {
    updateValue(+1);
}

void TargetGroupInput::updateValue(int delta) {
    int value = getValue() + delta;
    if (value < 0) value = 0;
    setValue(value);
}

void TargetGroupInput::setString(std::string const& str) {
    if (m_input) m_input->setString(str.c_str());
}

std::string TargetGroupInput::getString() const {
    if (m_input) return m_input->getString();
    return "0";
}

int TargetGroupInput::getValue() const {
    auto str = getString();
    if (str.empty()) return 0;
    return numFromString<int>(str).unwrapOr(0);
}

void TargetGroupInput::setValue(int value) {
    setString(std::to_string(value));
}