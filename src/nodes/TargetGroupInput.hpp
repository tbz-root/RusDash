#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class TargetGroupInput : public CCNode {
protected:
    CCTextInputNode* m_input = nullptr;
    CCMenuItemSpriteExtra* m_leftBtn = nullptr;
    CCMenuItemSpriteExtra* m_rightBtn = nullptr;
    CCLabelBMFont* m_label = nullptr;
    NineSlice* m_bg = nullptr;

    bool init() override;

    void onLeft(CCObject*);
    void onRight(CCObject*);
    void updateValue(int delta);

public:
    static TargetGroupInput* create();

    void setString(std::string const& str);
    std::string getString() const;
    int getValue() const;
    void setValue(int value);

    CCTextInputNode* getInput() const { return m_input; }
};