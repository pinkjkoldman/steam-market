#include "core/services/AccessGate.h"

namespace {
AccessDecision denied(Capability capability, IdentityState state) {
    if (state == IdentityState::ChoiceRequired) return AccessDecision::RequireChoice;
    if (state == IdentityState::Authenticating) return AccessDecision::Busy;
    if (state == IdentityState::Expired
        && (capability == Capability::PrivateInventory || capability == Capability::AccountAction))
        return AccessDecision::Reauthenticate;
    if (capability == Capability::PublicInventory) return AccessDecision::RequirePublicId;
    return AccessDecision::RequireLogin;
}
QString reason(AccessDecision decision) {
    switch (decision) {
    case AccessDecision::Allow: return QStringLiteral("account.access.allow");
    case AccessDecision::RequireChoice: return QStringLiteral("account.access.require_choice");
    case AccessDecision::RequireLogin: return QStringLiteral("account.access.require_login");
    case AccessDecision::RequirePublicId: return QStringLiteral("account.access.require_public_id");
    case AccessDecision::Busy: return QStringLiteral("account.access.busy");
    case AccessDecision::Reauthenticate: return QStringLiteral("account.access.reauthenticate");
    }
    return QStringLiteral("account.access.require_choice");
}
}

AccessDecision AccessGate::evaluate(Capability capability, const IdentitySnapshot &identity) {
    const auto state = identity.state;
    bool allowed = false;
    switch (capability) {
    case Capability::PublicMarket:
    case Capability::LocalWatchlist:
        allowed = state != IdentityState::ChoiceRequired;
        break;
    case Capability::EnterPublicSteamId:
        allowed = state == IdentityState::Guest || state == IdentityState::PublicInventory
                  || state == IdentityState::Authenticated || state == IdentityState::Expired;
        break;
    case Capability::PublicInventory:
        allowed = state == IdentityState::PublicInventory || state == IdentityState::Authenticated;
        break;
    case Capability::PrivateInventory:
    case Capability::AccountAction:
        allowed = state == IdentityState::Authenticated;
        break;
    case Capability::OpenOfficialCommunity:
        allowed = state == IdentityState::Guest || state == IdentityState::PublicInventory
                  || state == IdentityState::Authenticated || state == IdentityState::Expired;
        break;
    }
    return allowed ? AccessDecision::Allow : denied(capability, state);
}

AccessEvaluation AccessGate::evaluateDetailed(Capability capability,
                                              const IdentitySnapshot &identity) {
    const auto decision = evaluate(capability, identity);
    return {decision, reason(decision)};
}
