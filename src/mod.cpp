#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

// Force RusDash onto page 1 ABOVE Geode.
// Uses pin weight (+4) only for sorting, button is hidden & disabled.
// Visual order is forced to index 0 so we appear above Geode (+5).

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
    // Pin is used ONLY so Geode's sorter puts us on page 1 (weight +4).
    // The pin button itself is hidden — user cannot toggle it from the list.
    if (auto* self = Mod::get()) {
        self->setPinned(true);
    }

    ModItemUIEvent().listen([](CCNode* item, std::string_view modID, std::optional<Mod*>) {
        if (!item) return ListenerResult::Propagate;

        // Only our mod
        if (modID != Mod::get()->getID()) {
            return ListenerResult::Propagate;
        }

        // Keep pin forced on (in case something unpinned us)
        if (auto* self = Mod::get()) {
            if (!self->isPinned()) {
                self->setPinned(true);
            }
        }

        hideListToggles(item);

        // After the list finishes layout this frame → put us at index 0 (above Geode)
        queueInMainThread([item = Ref(item)] {
            forceToTop(item.data());
        });

        return ListenerResult::Propagate;
    }).leak();
}