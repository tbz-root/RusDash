#pragma once

#include <Geode/ui/NineSlice.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class TargetGroupInput : public CCNode {
protected:
    CCTextInputNode* m_input = nullptr;
    CCMenuItemSpriteExtra* m_leftBtn = nullptr;
    CCMenuItemSpriteExtra* m_rightBtn = nullptr;
    CCLabelBMFont* m_label = nullptr;
    NineSlice* m_bg = nullptr;

    float m_step = 1.f;
    float m_min = 0.f;
    float m_max = 9999.f;
    bool m_isFloat = false;
    int m_decimals = 2;

    std::function<void(float)> m_callback = nullptr;

    bool init(std::string const& labelText, bool isFloat, float step, float minVal, float maxVal);
    void onLeft(CCObject*);
    void onRight(CCObject*);
    void updateValue(float delta);

public:
    static TargetGroupInput* create(std::string const& labelText = "Target Group:");
    static TargetGroupInput* createFloat(std::string const& labelText, float step = 0.1f, float minVal = 0.05f, float maxVal = 10.f, int decimals = 2);

    void setString(std::string const& str);
    std::string getString() const;

    int getValue() const;
    void setValue(int value);

    float getFloatValue() const;
    void setFloatValue(float value);

    void setCallback(std::function<void(float)> cb) { m_callback = std::move(cb); }

    CCTextInputNode* getInput() const { return m_input; }
};