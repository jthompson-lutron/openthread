/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements of MLE types and constants.
 */

#include "mle_types.hpp"

#include "common/array.hpp"
#include "common/bit_utils.hpp"
#include "common/code_utils.hpp"
#include "common/enum_to_string.hpp"
#include "common/message.hpp"
#include "common/random.hpp"
#include "utils/static_counter.hpp"

namespace ot {
namespace Mle {

//---------------------------------------------------------------------------------------------------------------------
// RouterUpgradeReasonFlags

#if OPENTHREAD_FTD
otError RouterUpgradeReasonFlags::AsStatusTlvReason(StatusTlvEnum &aStatusTlvReason) const
{
    otError error = kErrorNone;

    // Select a single reason to report in the Status TLV prioritized in reverse bit order
    if ((mRouterUpgradeReasonFlags & kUpgradeReasonManagedRouterFlag) != 0)
    {
        // Managed routers will always use the kReasonParentPartitionChangeOrManagedRouter
        // reason in the Address Solicit status TLV to prevent older leaders from rejecting
        // the address solicit because of a mismatch in role upgrade thresholds.
        aStatusTlvReason = RouterUpgradeReasonFlags::kReasonParentPartitionChangeOrManagedRouter;
    }
    else if ((mRouterUpgradeReasonFlags & kUpgradeReasonBorderRouterRequestFlag) != 0)
    {
        aStatusTlvReason = RouterUpgradeReasonFlags::kReasonBorderRouterRequest;
    }
    else if ((mRouterUpgradeReasonFlags & kUpgradeReasonParentPartitionChangeFlag) != 0)
    {
        aStatusTlvReason = RouterUpgradeReasonFlags::kReasonParentPartitionChangeOrManagedRouter;
    }
    else if ((mRouterUpgradeReasonFlags & kUpgradeReasonHaveChildIdRequestFlag) != 0)
    {
        aStatusTlvReason = RouterUpgradeReasonFlags::kReasonHaveChildIdRequest;
    }
    else if ((mRouterUpgradeReasonFlags & kUpgradeReasonTooFewRoutersFlag) != 0)
    {
        aStatusTlvReason = RouterUpgradeReasonFlags::kReasonTooFewRouters;
    }
    else
    {
        ExitNow(error = kErrorInvalidArgs);
    }

exit:
    return error;
}
static_assert(sizeof(RouterUpgradeReasonFlags) == 1, "sizeof(RouterUpgradeReasonFlags) must be 1");
#endif

//---------------------------------------------------------------------------------------------------------------------
// DeviceMode

void DeviceMode::Get(ModeConfig &aModeConfig) const
{
    aModeConfig.mRxOnWhenIdle = IsRxOnWhenIdle();
    aModeConfig.mDeviceType   = IsFullThreadDevice();
    aModeConfig.mNetworkData  = (GetNetworkDataType() == NetworkData::kFullSet);
}

void DeviceMode::Set(const ModeConfig &aModeConfig)
{
    mMode = kModeReserved;
    mMode |= aModeConfig.mRxOnWhenIdle ? kModeRxOnWhenIdle : 0;
    mMode |= aModeConfig.mDeviceType ? kModeFullThreadDevice : 0;
    mMode |= aModeConfig.mNetworkData ? kModeFullNetworkData : 0;
}

DeviceMode::InfoString DeviceMode::ToString(void) const
{
    InfoString string;

    string.Append("rx-on:%s ftd:%s full-net:%s", ToYesNo(IsRxOnWhenIdle()), ToYesNo(IsFullThreadDevice()),
                  ToYesNo(GetNetworkDataType() == NetworkData::kFullSet));

    return string;
}

//---------------------------------------------------------------------------------------------------------------------
// DeviceProperties

#if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE

DeviceProperties::DeviceProperties(void)
{
    Clear();

    mPowerSupply            = OPENTHREAD_CONFIG_DEVICE_POWER_SUPPLY;
    mLeaderWeightAdjustment = kDefaultAdjustment;
#if OPENTHREAD_CONFIG_BORDER_ROUTING_ENABLE
    mIsBorderRouter = true;
#endif
}

void DeviceProperties::ClampWeightAdjustment(void)
{
    mLeaderWeightAdjustment = Clamp(mLeaderWeightAdjustment, kMinAdjustment, kMaxAdjustment);
}

uint8_t DeviceProperties::CalculateLeaderWeight(void) const
{
    static const int8_t kPowerSupplyIncs[] = {
        kPowerBatteryInc,          // (0) kPowerSupplyBattery
        kPowerExternalInc,         // (1) kPowerSupplyExternal
        kPowerExternalStableInc,   // (2) kPowerSupplyExternalStable
        kPowerExternalUnstableInc, // (3) kPowerSupplyExternalUnstable
    };

    struct EnumCheck
    {
        InitEnumValidatorCounter();
        ValidateNextEnum(kPowerSupplyBattery);
        ValidateNextEnum(kPowerSupplyExternal);
        ValidateNextEnum(kPowerSupplyExternalStable);
        ValidateNextEnum(kPowerSupplyExternalUnstable);
    };

    uint8_t     weight      = kBaseWeight;
    PowerSupply powerSupply = MapEnum(mPowerSupply);

    if (mIsBorderRouter)
    {
        weight += (mSupportsCcm ? kCcmBorderRouterInc : kBorderRouterInc);
    }

    if (powerSupply < GetArrayLength(kPowerSupplyIncs))
    {
        weight += kPowerSupplyIncs[powerSupply];
    }

    if (mIsUnstable)
    {
        switch (powerSupply)
        {
        case kPowerSupplyBattery:
        case kPowerSupplyExternalUnstable:
            break;

        default:
            weight += kIsUnstableInc;
        }
    }

    weight += mLeaderWeightAdjustment;

    return weight;
}

#endif // #if OPENTHREAD_FTD && OPENTHREAD_CONFIG_MLE_DEVICE_PROPERTY_LEADER_WEIGHT_ENABLE

//---------------------------------------------------------------------------------------------------------------------
// RouterIdMask

uint8_t RouterIdMask::DetermineAllocatedCount(void) const
{
    uint8_t count = 0;

    for (uint8_t byte : mMask)
    {
        count += CountBitsInMask(byte);
    }

    return count;
}

Error RouterIdMask::AppendMaskTo(Message &aMessage) const { return aMessage.AppendBytes(mMask, kMaskSize); }

Error RouterIdMask::ReadMaskFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    return aMessage.Read(aOffsetRange, mMask, kMaskSize);
}

//---------------------------------------------------------------------------------------------------------------------
// TxChallenge

void TxChallenge::GenerateRandom(void) { IgnoreError(Random::Crypto::Fill(*this)); }

//---------------------------------------------------------------------------------------------------------------------
// RxChallenge

Error RxChallenge::ReadFrom(const Message &aMessage, const OffsetRange &aOffsetRange)
{
    Error       error       = kErrorNone;
    OffsetRange offsetRange = aOffsetRange;

    Clear();

    offsetRange.ShrinkLength(kMaxSize);

    VerifyOrExit(offsetRange.Contains(kMinSize), error = kErrorParse);

    SuccessOrExit(error = aMessage.Read(offsetRange, mArray.GetArrayBuffer(), offsetRange.GetLength()));
    mArray.SetLength(static_cast<uint8_t>(offsetRange.GetLength()));

exit:
    return error;
}

bool RxChallenge::operator==(const TxChallenge &aTxChallenge) const
{
    return (mArray.GetLength() == kMaxSize) && (memcmp(mArray.GetArrayBuffer(), aTxChallenge.m8, kMaxSize) == 0);
}

//---------------------------------------------------------------------------------------------------------------------
// Connectivity

void Connectivity::IncrementNumForLinkQuality(uint8_t aLinkQuality)
{
    switch (aLinkQuality)
    {
    case kLinkQuality1:
        mLinkQuality1++;
        break;
    case kLinkQuality2:
        mLinkQuality2++;
        break;
    case kLinkQuality3:
        mLinkQuality3++;
        break;
    default:
        break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// RouterAdministration

#if OPENTHREAD_FTD

RouterAdministrationConfiguration::RouterAdministrationConfiguration()
    : otRouterAdministrationConfiguration{
          OT_ROUTER_ADMINISTRATION_OPTIONS_DEFAULT,
          kParentPriorityHighThresholdDefault,
          kParentPriorityMediumThresholdDefault,
          {kRouterUpgradeThresholdDefault, kRouterTransitionMinimumDefault, kRouterTransitionJitterDefault},
          {kRouterDowngradeThresholdDefault, kRouterTransitionMinimumDefault, kRouterTransitionJitterDefault}}
{
}

Error RouterAdministrationConfiguration::ApplyRouterAdministration(
    const otRouterAdministrationConfiguration &aNewConfiguration)
{
    Error error = kErrorNone;

    VerifyOrExit(IsCapacityCodeOrValue(aNewConfiguration.mParentPriorityHighThreshold), error = kErrorInvalidArgs);
    VerifyOrExit(IsCapacityCodeOrValue(aNewConfiguration.mParentPriorityMediumThreshold), error = kErrorInvalidArgs);

    // mRouterUpgradeParameters
    VerifyOrExit(IsRoleThresholdCodeOrValue(aNewConfiguration.mRouterUpgradeParameters.mThreshold),
                 error = kErrorInvalidArgs);
    VerifyOrExit(IsDelayMinimumCodeOrValue(aNewConfiguration.mRouterUpgradeParameters.mDelayMinimum),
                 error = kErrorInvalidArgs);
    VerifyOrExit(IsDelayJitterCodeOrValue(aNewConfiguration.mRouterUpgradeParameters.mDelayJitter),
                 error = kErrorInvalidArgs);

    // mRouterDowngradeParameters
    VerifyOrExit(IsRoleThresholdCodeOrValue(aNewConfiguration.mRouterDowngradeParameters.mThreshold),
                 error = kErrorInvalidArgs);
    VerifyOrExit(IsDelayMinimumCodeOrValue(aNewConfiguration.mRouterDowngradeParameters.mDelayMinimum),
                 error = kErrorInvalidArgs);
    VerifyOrExit(IsDelayJitterCodeOrValue(aNewConfiguration.mRouterDowngradeParameters.mDelayJitter),
                 error = kErrorInvalidArgs);

    if (mRouterAdministrationOptions != OT_ROUTER_ADMINISTRATION_OPTIONS_UNCHANGED_CODE)
    {
        // Options reserved bits may be set or unset and are copied as provided,
        // when all other fields validate and all bits do not match the "unchanged" code.
        mRouterAdministrationOptions = aNewConfiguration.mRouterAdministrationOptions;
    }
    ApplyCapacityCodeOrValue(mParentPriorityHighThreshold, aNewConfiguration.mParentPriorityHighThreshold,
                             kParentPriorityHighThresholdDefault);
    ApplyCapacityCodeOrValue(mParentPriorityMediumThreshold, aNewConfiguration.mParentPriorityMediumThreshold,
                             kParentPriorityMediumThresholdDefault);

    ApplyRoleThresholdCodeOrValue(mRouterUpgradeParameters.mThreshold,
                                  aNewConfiguration.mRouterUpgradeParameters.mThreshold,
                                  kRouterUpgradeThresholdDefault);
    ApplyDelayMinimumCodeOrValue(mRouterUpgradeParameters.mDelayMinimum,
                                 aNewConfiguration.mRouterUpgradeParameters.mDelayMinimum);
    ApplyDelayJitterCodeOrValue(mRouterUpgradeParameters.mDelayJitter,
                                aNewConfiguration.mRouterUpgradeParameters.mDelayJitter);

    ApplyRoleThresholdCodeOrValue(mRouterDowngradeParameters.mThreshold,
                                  aNewConfiguration.mRouterDowngradeParameters.mThreshold,
                                  kRouterDowngradeThresholdDefault);
    ApplyDelayMinimumCodeOrValue(mRouterDowngradeParameters.mDelayMinimum,
                                 aNewConfiguration.mRouterDowngradeParameters.mDelayMinimum);
    ApplyDelayJitterCodeOrValue(mRouterDowngradeParameters.mDelayJitter,
                                aNewConfiguration.mRouterDowngradeParameters.mDelayJitter);

exit:
    return error;
}

int8_t RouterAdministrationConfiguration::GetParentPriority(uint16_t aMaxChildrenAllowed,
                                                            uint16_t aCurrentValidChildren) const
{
    int8_t  parentPriority;
    int16_t highThreshold   = GetThresholdOfMaximum(mParentPriorityHighThreshold, aMaxChildrenAllowed);
    int16_t mediumThreshold = GetThresholdOfMaximum(mParentPriorityMediumThreshold, aMaxChildrenAllowed);
    if (highThreshold >= 0 && aCurrentValidChildren <= highThreshold)
    {
        // The valid child count is within the high priority threshold, which is also not disabled.
        parentPriority = kParentPriorityHigh;
    }
    else if (mediumThreshold >= 0 && aCurrentValidChildren <= mediumThreshold)
    {
        // The valid child count is within the medium priority threshold, which is also not disabled.
        parentPriority = kParentPriorityMedium;
    }
    else
    {
        parentPriority = kParentPriorityLow;
    }
    return parentPriority;
}

void RouterAdministrationConfiguration::SetRouterEligibleStatus(bool newStatus)
{
    if (newStatus)
    {
        mRouterAdministrationOptions &= ~OT_ROUTER_ADMINISTRATION_OPTIONS_INELIGIBLE_MASK;
    }
    else
    {
        mRouterAdministrationOptions |= OT_ROUTER_ADMINISTRATION_OPTIONS_INELIGIBLE_MASK;
    }
}

template <typename T, T aUseDefaultCode>
void RouterAdministrationConfiguration::ApplyCodeOrValue(T &aParameterReference,
                                                         T  aValueToApply,
                                                         T  aMaxValidValue,
                                                         T  aDefaultValue)
{
    if (aValueToApply <= aMaxValidValue)
    {
        aParameterReference = aValueToApply;
    }
    else if (aValueToApply == aUseDefaultCode)
    {
        aParameterReference = aDefaultValue;
    }
    // Unchanged codes or invalid values will leave the reference unchanged
}
void RouterAdministrationConfiguration::ApplyCapacityCodeOrValue(otCapacityThresholdEnum &aParameterReference,
                                                                 otCapacityThresholdEnum  aValueToApply,
                                                                 otCapacityThresholdEnum  aDefaultValue)
{
    ApplyCodeOrValue<otCapacityThresholdEnum, OT_CAPACITY_USED_DEFAULT_CODE>(
        aParameterReference, aValueToApply, otCapacityThresholdEnum::OT_CAPACITY_FULL, aDefaultValue);
}
void RouterAdministrationConfiguration::ApplyRoleThresholdCodeOrValue(uint8_t &aParameterReference,
                                                                      uint8_t  aValueToApply,
                                                                      uint8_t  aDefaultValue)
{
    ApplyCodeOrValue<uint8_t, OT_ACTIVE_ROUTER_THRESHOLD_USE_DEFAULT_CODE>(
        aParameterReference, aValueToApply, OT_ACTIVE_ROUTER_THRESHOLD_MAXIMUM, aDefaultValue);
}
void RouterAdministrationConfiguration::ApplyDelayMinimumCodeOrValue(uint16_t &aParameterReference,
                                                                     uint16_t  aValueToApply)
{
    ApplyCodeOrValue<uint16_t, OT_ROLE_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aParameterReference, aValueToApply, OT_ROLE_TRANSITION_DELAY_MINIMUM_LIMIT, kRouterTransitionMinimumDefault);
}
void RouterAdministrationConfiguration::ApplyDelayJitterCodeOrValue(uint16_t &aParameterReference,
                                                                    uint16_t  aValueToApply)
{
    ApplyCodeOrValue<uint16_t, OT_ROLE_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aParameterReference, aValueToApply, OT_ROLE_TRANSITION_DELAY_JITTER_LIMIT, kRouterTransitionJitterDefault);
}

template <typename T, T aUnchangedCode, T aUseDefaultCode>
bool RouterAdministrationConfiguration::IsCodeOrValue(T aValueToApply, T aMaxValidValue)
{
    return (aValueToApply <= aMaxValidValue || aValueToApply == aUseDefaultCode || aValueToApply == aUnchangedCode);
}
bool RouterAdministrationConfiguration::IsCapacityCodeOrValue(otCapacityThresholdEnum aValueToApply)
{
    return IsCodeOrValue<otCapacityThresholdEnum, OT_CAPACITY_USED_UNCHANGED_CODE, OT_CAPACITY_USED_DEFAULT_CODE>(
        aValueToApply, OT_CAPACITY_FULL);
}
bool RouterAdministrationConfiguration::IsRoleThresholdCodeOrValue(uint8_t aValueToApply)
{
    return (
        IsCodeOrValue<uint8_t, OT_ACTIVE_ROUTER_THRESHOLD_UNCHANGED_CODE, OT_ACTIVE_ROUTER_THRESHOLD_USE_DEFAULT_CODE>(
            aValueToApply, OT_ACTIVE_ROUTER_THRESHOLD_MAXIMUM));
}
bool RouterAdministrationConfiguration::IsDelayMinimumCodeOrValue(uint16_t aValueToApply)
{
    return IsCodeOrValue<uint16_t, OT_ROLE_TRANSITION_DELAY_UNCHANGED_CODE, OT_ROLE_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aValueToApply, OT_ROLE_TRANSITION_DELAY_MINIMUM_LIMIT);
}
bool RouterAdministrationConfiguration::IsDelayJitterCodeOrValue(uint16_t aValueToApply)
{
    return IsCodeOrValue<uint16_t, OT_ROLE_TRANSITION_DELAY_UNCHANGED_CODE, OT_ROLE_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aValueToApply, OT_ROLE_TRANSITION_DELAY_JITTER_LIMIT);
}

int16_t RouterAdministrationConfiguration::GetThresholdOfMaximum(otCapacityThresholdEnum aCapacityThreshold,
                                                                 uint16_t                aFullMaxCount)
{
    uint16_t threshold;

    // These calculations will result in the floor of the resulting ratio
    switch (aCapacityThreshold)
    {
    case OT_CAPACITY_USED_NONE:
        threshold = -1;
        break;
    case OT_CAPACITY_USED_ONE_QUARTER:
        threshold = aFullMaxCount / 4;
        break;
    case OT_CAPACITY_USED_ONE_THIRD:
        threshold = aFullMaxCount / 3;
        break;
    case OT_CAPACITY_USED_ONE_HALF:
        threshold = aFullMaxCount / 2;
        break;
    case OT_CAPACITY_USED_TWO_THIRDS:
        threshold = (aFullMaxCount * 2) / 3;
        break;
    case OT_CAPACITY_USED_THREE_QUARTERS:
        threshold = (aFullMaxCount * 3) / 4;
        break;
    case OT_CAPACITY_FULL:
        threshold = aFullMaxCount;
        break;
    default:
        // Disabled threshold if outside the valid range (resulting in low priority)
        threshold = -1;
        break;
    }
    return threshold;
}
#endif // OPENTHREAD_FTD

//---------------------------------------------------------------------------------------------------------------------

const char *RoleToString(DeviceRole aRole)
{
#define RoleMapList(_)           \
    _(kRoleDisabled, "disabled") \
    _(kRoleDetached, "detached") \
    _(kRoleChild, "child")       \
    _(kRoleRouter, "router")     \
    _(kRoleLeader, "leader")

    DefineEnumStringArray(RoleMapList);

    return (aRole < GetArrayLength(kStrings)) ? kStrings[aRole] : "invalid";
}

} // namespace Mle
} // namespace ot
