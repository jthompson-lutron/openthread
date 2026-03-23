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
// RouterIdSet

uint8_t RouterIdSet::GetNumberOfAllocatedIds(void) const
{
    uint8_t count = 0;

    for (uint8_t byte : mRouterIdSet)
    {
        count += CountBitsInMask(byte);
    }

    return count;
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
// CapacityThreshold

#if OPENTHREAD_FTD
CapacityThreshold::CapacityThreshold(otCapacityThreshold aCapacityThreshold)
    : mCapacityThreshold(aCapacityThreshold)
{
}

bool CapacityThreshold::IsValidConfiguration(otCapacityThreshold aCapacityThreshold)
{
    return aCapacityThreshold <= OT_CAPACITY_FULL || aCapacityThreshold == OT_CAPACITY_USED_UNCHANGED ||
           aCapacityThreshold == OT_CAPACITY_USED_DEFAULT;
}

otCapacityThreshold CapacityThreshold::GetDefaultCodeOrValue(otCapacityThreshold aDefaultValue) const
{
    return (mCapacityThreshold == aDefaultValue) ? OT_CAPACITY_USED_DEFAULT : mCapacityThreshold;
}

uint16_t CapacityThreshold::GetThresholdOfMaximum(uint16_t aFullMaxCount) const
{
    uint16_t threshold = 0;
    switch (mCapacityThreshold)
    {
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
        break;
    }
    return threshold;
}

void CapacityThreshold::ApplyCapacityThreshold(otCapacityThreshold aNewCapacity, otCapacityThreshold aDefault)
{
    if (IsValidConfiguration(aNewCapacity) && aNewCapacity != OT_CAPACITY_USED_UNCHANGED)
    {
        mCapacityThreshold = ((aNewCapacity == OT_CAPACITY_USED_DEFAULT) ? aDefault : aNewCapacity);
    }
}

#endif

//---------------------------------------------------------------------------------------------------------------------
// RouterAdministration

#if OPENTHREAD_FTD

namespace RouterAdministration {

bool IsDefault(const otRouterAdministrationConfiguration &aRouterAdministration)
{
    // Verify values match use-default codes or default values
    // The mRouterAdministrationOptions verifies that reserved bits are also in their default state
    return (aRouterAdministration.mRouterAdministrationOptions == 0 &&
            (aRouterAdministration.mRouterUpgradeThreshold == OT_ROUTER_THRESHOLD_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterUpgradeThreshold == kRouterUpgradeThresholdDefault) &&
            (aRouterAdministration.mRouterDowngradeThreshold == OT_ROUTER_THRESHOLD_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterDowngradeThreshold == kRouterDowngradeThresholdDefault) &&
            (aRouterAdministration.mParentPriorityThreshold == OT_CAPACITY_USED_DEFAULT ||
             aRouterAdministration.mParentPriorityThreshold == CapacityThreshold::kParentPriorityThresholdDefault) &&
            (aRouterAdministration.mParentDeprioritizationThreshold == OT_CAPACITY_USED_DEFAULT ||
             aRouterAdministration.mParentPriorityThreshold ==
                 CapacityThreshold::kParentDeprioritizationThresholdDefault) &&
            (aRouterAdministration.mRouterUpgradeDelayMinimum == OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterUpgradeDelayMinimum == kRouterTransitionMinimumDefault) &&
            (aRouterAdministration.mRouterUpgradeDelayJitter == OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterUpgradeDelayJitter == kRouterTransitionJitterDefault) &&
            (aRouterAdministration.mRouterDowngradeDelayMinimum == OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterDowngradeDelayMinimum == kRouterTransitionMinimumDefault) &&
            (aRouterAdministration.mRouterDowngradeDelayJitter == OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE ||
             aRouterAdministration.mRouterDowngradeDelayJitter == kRouterTransitionJitterDefault));
}

bool ConfigurationsDiffer(const otRouterAdministrationConfiguration &aRouterAdministration,
                          const otRouterAdministrationConfiguration &aOther)
{
    // mRouterAdministrationOptions ignores differences in reserved bits,
    // so that existing settings will not be overwritten if all other parameters match.
    return (((aRouterAdministration.mRouterAdministrationOptions & kRouterRoleConfigMask) !=
             (aOther.mRouterAdministrationOptions & kRouterRoleConfigMask)) ||
            (aRouterAdministration.mRouterUpgradeThreshold != aOther.mRouterUpgradeThreshold) ||
            (aRouterAdministration.mRouterDowngradeThreshold != aOther.mRouterDowngradeThreshold) ||
            (aRouterAdministration.mParentPriorityThreshold != aOther.mParentPriorityThreshold) ||
            (aRouterAdministration.mParentDeprioritizationThreshold != aOther.mParentDeprioritizationThreshold) ||
            (aRouterAdministration.mRouterUpgradeDelayMinimum != aOther.mRouterUpgradeDelayMinimum) ||
            (aRouterAdministration.mRouterUpgradeDelayJitter != aOther.mRouterUpgradeDelayJitter) ||
            (aRouterAdministration.mRouterDowngradeDelayMinimum != aOther.mRouterDowngradeDelayMinimum) ||
            (aRouterAdministration.mRouterDowngradeDelayJitter != aOther.mRouterDowngradeDelayJitter));
}

template <typename T, T aUnchangedCode, T aDefaultCode>
static inline void ApplyCodeOrValue(T &aParameterReference, T aValueToApply, T aDefaultValue)
{
    if (aValueToApply != aUnchangedCode)
    {
        aParameterReference = (aValueToApply == aDefaultCode) ? aDefaultValue : aValueToApply;
    }
}
template <typename T, T aUnchangedCode, T aDefaultCode> static inline bool IsValid(T aValue, T aMaxValue)
{
    return (aValue <= aMaxValue || aValue == aUnchangedCode || aValue == aDefaultCode);
}

bool IsValidThreshold(uint8_t aRouterThreshold)
{
    return IsValid<uint8_t, OT_ROUTER_THRESHOLD_UNCHANGED_CODE, OT_ROUTER_THRESHOLD_USE_DEFAULT_CODE>(
        aRouterThreshold, OT_ROUTER_THRESHOLD_MAXIMUM);
}
bool IsValidTransitionDelay(uint16_t aTransitionDelay, uint16_t aMaxValue)
{
    return IsValid<uint16_t, OT_ROUTER_TRANSITION_DELAY_UNCHANGED_CODE, OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aTransitionDelay, aMaxValue);
}

void ApplyRouterThreshold(uint8_t &aRouterThresholdReference, uint8_t aNewThreshold, uint8_t aDefaultThreshold)
{
    ApplyCodeOrValue<uint8_t, OT_ROUTER_THRESHOLD_UNCHANGED_CODE, OT_ROUTER_THRESHOLD_USE_DEFAULT_CODE>(
        aRouterThresholdReference, aNewThreshold, aDefaultThreshold);
}
void ApplyTransitionDelayValue(uint16_t &aTransitionDelayReference, uint16_t aNewDelay, uint16_t aDefaultDelay)
{
    ApplyCodeOrValue<uint16_t, OT_ROUTER_TRANSITION_DELAY_UNCHANGED_CODE, OT_ROUTER_TRANSITION_DELAY_USE_DEFAULT_CODE>(
        aTransitionDelayReference, aNewDelay, aDefaultDelay);
}

} // namespace RouterAdministration

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
