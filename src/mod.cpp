#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

static void forceToTop(CCNode* item) {
    if (!item) return;

    auto* parent = item->getParent();
    if (!parent) return;

    auto* children = parent->getChildren();
    if (!children || children->count() == 0) return;

    // Already first
    if (children->objectAtIndex(0) == item) {
        if (parent->getLayout()) parent->updateLayout();
        return;
    }

    item->retain();
    children->removeObject(item, false);
    children->insertObject(item, 0);
    item->release();

    parent->reorderChild(item, -9999);

    if (parent->getLayout()) {
        parent->updateLayout();
    }
}

static void hideListToggles(CCNode* item) {
    if (auto* pin = typeinfo_cast<CCMenuItem*>(item->querySelector("pin-toggler"))) {
        pin->setVisible(false);
        pin->setScale(0.f);
        pin->setEnabled(false);
    }
    if (auto* enable = typeinfo_cast<CCMenuItem*>(item->querySelector("enable-toggler"))) {
        enable->setVisible(false);
        enable->setScale(0.f);
        enable->setEnabled(false);
    }
}

$on_mod(Loaded) {
    if (auto* self = Mod::get()) {
        self->setPinned(true);
    }

    ModItemUIEvent().listen([](CCNode* item, std::string_view modID, std::optional<Mod*>) {
        if (!item) return ListenerResult::Propagate;

        if (modID != Mod::get()->getID()) {
            return ListenerResult::Propagate;
        }

        if (auto* self = Mod::get()) {
            if (!self->isPinned()) {
                self->setPinned(true);
            }
        }

        hideListToggles(item);

        queueInMainThread([item = Ref(item)] {
            forceToTop(item.data());
        });

        return ListenerResult::Propagate;
    }).leak();
}