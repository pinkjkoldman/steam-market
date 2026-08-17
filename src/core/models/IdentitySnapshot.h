#pragma once

#include <QMetaType>
#include <QString>

enum class IdentityState { ChoiceRequired, Guest, PublicInventory, Authenticating, Authenticated, Expired };

struct IdentitySnapshot {
    IdentityState state = IdentityState::ChoiceRequired;
    QString steamId64;
    QString displayName;
    QString statusMessage;
    bool busy = false;
};

inline bool operator==(const IdentitySnapshot &a, const IdentitySnapshot &b) {
    return a.state == b.state && a.steamId64 == b.steamId64 && a.displayName == b.displayName
           && a.statusMessage == b.statusMessage && a.busy == b.busy;
}
inline bool operator!=(const IdentitySnapshot &a, const IdentitySnapshot &b) { return !(a == b); }

Q_DECLARE_METATYPE(IdentityState)
Q_DECLARE_METATYPE(IdentitySnapshot)
