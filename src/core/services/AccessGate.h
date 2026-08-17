#pragma once

#include <QMetaType>
#include <QString>
#include "core/models/IdentitySnapshot.h"

enum class Capability { PublicMarket, LocalWatchlist, EnterPublicSteamId, PublicInventory,
                        PrivateInventory, AccountAction, OpenOfficialCommunity };
enum class AccessDecision { Allow, RequireChoice, RequireLogin, RequirePublicId, Busy, Reauthenticate };

Q_DECLARE_METATYPE(Capability)
Q_DECLARE_METATYPE(AccessDecision)

struct AccessEvaluation {
    AccessDecision decision = AccessDecision::RequireChoice;
    QString reasonCode;
};

class AccessGate {
public:
    static AccessDecision evaluate(Capability capability, const IdentitySnapshot &identity);
    static AccessEvaluation evaluateDetailed(Capability capability,
                                             const IdentitySnapshot &identity);
};
