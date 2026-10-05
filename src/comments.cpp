using namespace geode::prelude;

#include <Geode/modify/CommentCell.hpp>
class $modify(CommentCell) {
    void loadFromComment(GJComment* comment) {
        CommentCell::loadFromComment(comment);

        if (!comment || !m_mainLayer) return;

        if (comment->m_accountID == 0) {
            CCScale9Sprite* originalBg = nullptr;
            for (auto* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
                if (auto s9 = typeinfo_cast<CCScale9Sprite*>(child)) {
                    if (s9->getContentSize().width > 300.f) {
                        originalBg = s9;
                        break;
                    }
                }
            }

            if (originalBg) {
                originalBg->setVisible(false);

                auto customBg = NineSlice::createWithSpriteFrameName("test.png"_spr);
                if (customBg) {
                    customBg->setContentSize(originalBg->getContentSize());
                    customBg->setPosition(originalBg->getPosition());
                    customBg->setZOrder(originalBg->getZOrder());
                    customBg->setID("custom-author-bg"_spr);
                    
                    m_mainLayer->addChild(customBg);
                }
            }

            for (auto* child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
                if (auto label = typeinfo_cast<CCLabelBMFont*>(child)) {
                    if (label->getString() == comment->m_commentString) {
                        label->setColor({ 255, 255, 100 });
                    }
                }
            }
        }
    }
};