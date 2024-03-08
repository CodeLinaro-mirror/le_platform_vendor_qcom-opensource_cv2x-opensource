/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       Utils.cpp
 *
 * @brief      This class performs error code to string conversion.
 */

#include <iostream>
#include <string>
#include <grp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <iomanip>
#include <sstream>

#include "Utils.hpp"

#define INVALID_GID -1

void Utils::validateNumericString(std::string &input) {
   char delimiter = '\n';
   bool invalidChar = false;
   do {
      for(size_t index = 0; index < input.size(); index++) {
         if(!isdigit(input[index]) && input[index] != ',' && input[index] != ' ') {
            invalidChar = true;
            break;
         }
      }
      if(invalidChar) {
         std::cout << "Enter valid input: " << std::endl;
         std::cin.clear();
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
         std::getline(std::cin, input, delimiter);
         invalidChar = false;
      }
   } while(invalidChar);
}

bool Utils::validateDigitString(std::string &input) {
   bool validChar = true;
   for(size_t index = 0; index < input.size(); index++) {
      if(!isdigit(input[index])) {
         validChar = false;
         break;
      }
    }
    return validChar;
}

int Utils::getValidSlotId() {

   int slotId = DEFAULT_SLOT_ID;
   bool valid = true;
   std::cout << "Enter Slot Id (1-Primary, 2-Secondary): ";
   std::cin >> slotId;
   do {
      Utils::validateInput(slotId);
      if (slotId != SLOT_ID_1 && slotId != SLOT_ID_2) {
         // If an error occurs then an error flag is set and future attempts to get
         // input will fail. Cear the error flag on cin.
         std::cin.clear();
         // Extracts characters from the previous input sequence and discards them,
         // until entire stream have been extracted, or one compares equal to newline.
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
         std::cout << "ERROR: Invalid Slot Id, re-enter Slot Id (1-Primary, 2-Secondary): ";
         std::cin >> slotId;
         valid = false;
      } else {
         valid = true;
      }
   } while(!valid);
   return slotId;
}

std::map<telux::common::ErrorCode, std::string> errorCodeToStringMap_ = {

   {telux::common::ErrorCode::SUCCESS, "SUCCESS"},
   {telux::common::ErrorCode::RADIO_NOT_AVAILABLE, "RADIO_NOT_AVAILABLE"},
   {telux::common::ErrorCode::GENERIC_FAILURE, "GENERIC_FAILURE"},
   {telux::common::ErrorCode::PASSWORD_INCORRECT, "PASSWORD_INCORRECT"},
   {telux::common::ErrorCode::SIM_PIN2, "SIM_PIN2"},
   {telux::common::ErrorCode::SIM_PUK2, "SIM_PUK2"},
   {telux::common::ErrorCode::REQUEST_NOT_SUPPORTED, "REQUEST_NOT_SUPPORTED"},
   {telux::common::ErrorCode::CANCELLED, "CANCELLED"},
   {telux::common::ErrorCode::OP_NOT_ALLOWED_DURING_VOICE_CALL, "OP_NOT_ALLOWED_DURING_VOICE_CALL"},
   {telux::common::ErrorCode::OP_NOT_ALLOWED_BEFORE_REG_TO_NW, "OP_NOT_ALLOWED_BEFORE_REG_TO_NW"},
   {telux::common::ErrorCode::SMS_SEND_FAIL_RETRY, "SMS_SEND_FAIL_RETRY"},
   {telux::common::ErrorCode::SIM_ABSENT, "SIM_ABSENT"},
   {telux::common::ErrorCode::SUBSCRIPTION_NOT_AVAILABLE, "SUBSCRIPTION_NOT_AVAILABLE"},
   {telux::common::ErrorCode::MODE_NOT_SUPPORTED, "MODE_NOT_SUPPORTED"},
   {telux::common::ErrorCode::FDN_CHECK_FAILURE, "FDN_CHECK_FAILURE"},
   {telux::common::ErrorCode::ILLEGAL_SIM_OR_ME, "ILLEGAL_SIM_OR_ME"},
   {telux::common::ErrorCode::MISSING_RESOURCE, "MISSING_RESOURCE"},
   {telux::common::ErrorCode::NO_SUCH_ELEMENT, "NO_SUCH_ELEMENT"},
   {telux::common::ErrorCode::DIAL_MODIFIED_TO_USSD, "DIAL_MODIFIED_TO_USSD"},
   {telux::common::ErrorCode::DIAL_MODIFIED_TO_SS, "DIAL_MODIFIED_TO_SS"},
   {telux::common::ErrorCode::DIAL_MODIFIED_TO_DIAL, "DIAL_MODIFIED_TO_DIAL"},
   {telux::common::ErrorCode::USSD_MODIFIED_TO_DIAL, "USSD_MODIFIED_TO_DIAL"},
   {telux::common::ErrorCode::USSD_MODIFIED_TO_SS, "USSD_MODIFIED_TO_SS"},
   {telux::common::ErrorCode::USSD_MODIFIED_TO_USSD, "USSD_MODIFIED_TO_USSD"},
   {telux::common::ErrorCode::SS_MODIFIED_TO_DIAL, "SS_MODIFIED_TO_DIAL"},
   {telux::common::ErrorCode::SS_MODIFIED_TO_USSD, "SS_MODIFIED_TO_USSD"},
   {telux::common::ErrorCode::SUBSCRIPTION_NOT_SUPPORTED, "SUBSCRIPTION_NOT_SUPPORTED"},
   {telux::common::ErrorCode::SS_MODIFIED_TO_SS, "SS_MODIFIED_TO_SS"},
   {telux::common::ErrorCode::LCE_NOT_SUPPORTED, "LCE_NOT_SUPPORTED"},
   {telux::common::ErrorCode::NO_MEMORY, "NO_MEMORY"},
   {telux::common::ErrorCode::INTERNAL_ERR, "INTERNAL_ERR"},
   {telux::common::ErrorCode::SYSTEM_ERR, "SYSTEM_ERR"},
   {telux::common::ErrorCode::MODEM_ERR, "MODEM_ERR"},
   {telux::common::ErrorCode::INVALID_STATE, "INVALID_STATE"},
   {telux::common::ErrorCode::NO_RESOURCES, "NO_RESOURCES"},
   {telux::common::ErrorCode::SIM_ERR, "SIM_ERR"},
   {telux::common::ErrorCode::INVALID_ARGUMENTS, "INVALID_ARGUMENTS"},
   {telux::common::ErrorCode::INVALID_SIM_STATE, "INVALID_SIM_STATE"},
   {telux::common::ErrorCode::INVALID_MODEM_STATE, "INVALID_MODEM_STATE"},
   {telux::common::ErrorCode::INVALID_CALL_ID, "INVALID_CALL_ID"},
   {telux::common::ErrorCode::NO_SMS_TO_ACK, "NO_SMS_TO_ACK"},
   {telux::common::ErrorCode::NETWORK_ERR, "NETWORK_ERR"},
   {telux::common::ErrorCode::REQUEST_RATE_LIMITED, "REQUEST_RATE_LIMITED"},
   {telux::common::ErrorCode::SIM_BUSY, "SIM_BUSY"},
   {telux::common::ErrorCode::SIM_FULL, "SIM_FULL"},
   {telux::common::ErrorCode::NETWORK_REJECT, "NETWORK_REJECT"},
   {telux::common::ErrorCode::OPERATION_NOT_ALLOWED, "OPERATION_NOT_ALLOWED"},
   {telux::common::ErrorCode::EMPTY_RECORD, "EMPTY_RECORD"},
   {telux::common::ErrorCode::INVALID_SMS_FORMAT, "INVALID_SMS_FORMAT"},
   {telux::common::ErrorCode::ENCODING_ERR, "ENCODING_ERR"},
   {telux::common::ErrorCode::INVALID_SMSC_ADDRESS, "INVALID_SMSC_ADDRESS"},
   {telux::common::ErrorCode::NO_SUCH_ENTRY, "NO_SUCH_ENTRY"},
   {telux::common::ErrorCode::NETWORK_NOT_READY, "NETWORK_NOT_READY"},
   {telux::common::ErrorCode::NOT_PROVISIONED, "NOT_PROVISIONED"},
   {telux::common::ErrorCode::NO_SUBSCRIPTION, "NO_SUBSCRIPTION"},
   {telux::common::ErrorCode::NO_NETWORK_FOUND, "NO_NETWORK_FOUND"},
   {telux::common::ErrorCode::DEVICE_IN_USE, "DEVICE_IN_USE"},
   {telux::common::ErrorCode::ABORTED, "ABORTED"},
   {telux::common::ErrorCode::INCOMPATIBLE_STATE, "INCOMPATIBLE_STATE"},
   {telux::common::ErrorCode::NO_EFFECT, "NO_EFFECT"},
   {telux::common::ErrorCode::DEVICE_NOT_READY, "DEVICE_NOT_READY"},
   {telux::common::ErrorCode::MISSING_ARGUMENTS, "MISSING_ARGUMENTS"},
   {telux::common::ErrorCode::MALFORMED_MSG, "MALFORMED_MSG"},
   {telux::common::ErrorCode::INTERNAL, "INTERNAL"},
   {telux::common::ErrorCode::CLIENT_IDS_EXHAUSTED, "CLIENT_IDS_EXHAUSTED"},
   {telux::common::ErrorCode::UNABORTABLE_TRANSACTION, "UNABORTABLE_TRANSACTION"},
   {telux::common::ErrorCode::INVALID_CLIENT_ID, "INVALID_CLIENT_ID"},
   {telux::common::ErrorCode::NO_THRESHOLDS, "NO_THRESHOLDS"},
   {telux::common::ErrorCode::INVALID_HANDLE, "INVALID_HANDLE"},
   {telux::common::ErrorCode::INVALID_PROFILE, "INVALID_PROFILE"},
   {telux::common::ErrorCode::INVALID_PINID, "INVALID_PINID"},
   {telux::common::ErrorCode::INCORRECT_PIN, "INCORRECT_PIN"},
   {telux::common::ErrorCode::CALL_FAILED, "CALL_FAILED"},
   {telux::common::ErrorCode::OUT_OF_CALL, "OUT_OF_CALL"},
   {telux::common::ErrorCode::MISSING_ARG, "MISSING_ARG"},
   {telux::common::ErrorCode::ARG_TOO_LONG, "ARG_TOO_LONG"},
   {telux::common::ErrorCode::INVALID_TX_ID, "INVALID_TX_ID"},
   {telux::common::ErrorCode::OP_NETWORK_UNSUPPORTED, "OP_NETWORK_UNSUPPORTED"},
   {telux::common::ErrorCode::OP_DEVICE_UNSUPPORTED, "OP_DEVICE_UNSUPPORTED"},
   {telux::common::ErrorCode::NO_FREE_PROFILE, "NO_FREE_PROFILE"},
   {telux::common::ErrorCode::INVALID_PDP_TYPE, "INVALID_PDP_TYPE"},
   {telux::common::ErrorCode::INVALID_TECH_PREF, "INVALID_TECH_PREF"},
   {telux::common::ErrorCode::INVALID_PROFILE_TYPE, "INVALID_PROFILE_TYPE"},
   {telux::common::ErrorCode::INVALID_SERVICE_TYPE, "INVALID_SERVICE_TYPE"},
   {telux::common::ErrorCode::INVALID_REGISTER_ACTION, "INVALID_REGISTER_ACTION"},
   {telux::common::ErrorCode::INVALID_PS_ATTACH_ACTION, "INVALID_PS_ATTACH_ACTION"},
   {telux::common::ErrorCode::AUTHENTICATION_FAILED, "AUTHENTICATION_FAILED"},
   {telux::common::ErrorCode::PIN_BLOCKED, "PIN_BLOCKED"},
   {telux::common::ErrorCode::PIN_PERM_BLOCKED, "PIN_PERM_BLOCKED"},
   {telux::common::ErrorCode::SIM_NOT_INITIALIZED, "SIM_NOT_INITIALIZED"},
   {telux::common::ErrorCode::MAX_QOS_REQUESTS_IN_USE, "MAX_QOS_REQUESTS_IN_USE"},
   {telux::common::ErrorCode::INCORRECT_FLOW_FILTER, "INCORRECT_FLOW_FILTER"},
   {telux::common::ErrorCode::NETWORK_QOS_UNAWARE, "NETWORK_QOS_UNAWARE"},
   {telux::common::ErrorCode::INVALID_ID, "INVALID_ID"},
   {telux::common::ErrorCode::REQUESTED_NUM_UNSUPPORTED, "REQUESTED_NUM_UNSUPPORTED"},
   {telux::common::ErrorCode::INTERFACE_NOT_FOUND, "INTERFACE_NOT_FOUND"},
   {telux::common::ErrorCode::FLOW_SUSPENDED, "FLOW_SUSPENDED"},
   {telux::common::ErrorCode::INVALID_DATA_FORMAT, "INVALID_DATA_FORMAT"},
   {telux::common::ErrorCode::GENERAL, "GENERAL"},
   {telux::common::ErrorCode::UNKNOWN, "UNKNOWN"},
   {telux::common::ErrorCode::INVALID_ARG, "INVALID_ARG"},
   {telux::common::ErrorCode::INVALID_INDEX, "INVALID_INDEX"},
   {telux::common::ErrorCode::NO_ENTRY, "NO_ENTRY"},
   {telux::common::ErrorCode::DEVICE_STORAGE_FULL, "DEVICE_STORAGE_FULL"},
   {telux::common::ErrorCode::CAUSE_CODE, "CAUSE_CODE"},
   {telux::common::ErrorCode::MESSAGE_NOT_SENT, "MESSAGE_NOT_SENT"},
   {telux::common::ErrorCode::MESSAGE_DELIVERY_FAILURE, "MESSAGE_DELIVERY_FAILURE"},
   {telux::common::ErrorCode::INVALID_MESSAGE_ID, "INVALID_MESSAGE_ID"},
   {telux::common::ErrorCode::ENCODING, "ENCODING"},
   {telux::common::ErrorCode::AUTHENTICATION_LOCK, "AUTHENTICATION_LOCK"},
   {telux::common::ErrorCode::INVALID_TRANSITION, "INVALID_TRANSITION"},
   {telux::common::ErrorCode::NOT_A_MCAST_IFACE, "NOT_A_MCAST_IFACE"},
   {telux::common::ErrorCode::MAX_MCAST_REQUESTS_IN_USE, "MAX_MCAST_REQUESTS_IN_USE"},
   {telux::common::ErrorCode::INVALID_MCAST_HANDLE, "INVALID_MCAST_HANDLE"},
   {telux::common::ErrorCode::INVALID_IP_FAMILY_PREF, "INVALID_IP_FAMILY_PREF"},
   {telux::common::ErrorCode::SESSION_INACTIVE, "SESSION_INACTIVE"},
   {telux::common::ErrorCode::SESSION_INVALID, "SESSION_INVALID"},
   {telux::common::ErrorCode::SESSION_OWNERSHIP, "SESSION_OWNERSHIP"},
   {telux::common::ErrorCode::INSUFFICIENT_RESOURCES, "INSUFFICIENT_RESOURCES"},
   {telux::common::ErrorCode::DISABLED, "DISABLED"},
   {telux::common::ErrorCode::INVALID_OPERATION, "INVALID_OPERATION"},
   {telux::common::ErrorCode::INVALID_QMI_CMD, "INVALID_QMI_CMD"},
   {telux::common::ErrorCode::TPDU_TYPE, "TPDU_TYPE"},
   {telux::common::ErrorCode::SMSC_ADDR, "SMSC_ADDR"},
   {telux::common::ErrorCode::INFO_UNAVAILABLE, "INFO_UNAVAILABLE"},
   {telux::common::ErrorCode::SEGMENT_TOO_LONG, "SEGMENT_TOO_LONG"},
   {telux::common::ErrorCode::SEGMENT_ORDER, "SEGMENT_ORDER"},
   {telux::common::ErrorCode::BUNDLING_NOT_SUPPORTED, "BUNDLING_NOT_SUPPORTED"},
   {telux::common::ErrorCode::OP_PARTIAL_FAILURE, "OP_PARTIAL_FAILURE"},
   {telux::common::ErrorCode::POLICY_MISMATCH, "POLICY_MISMATCH"},
   {telux::common::ErrorCode::SIM_FILE_NOT_FOUND, "SIM_FILE_NOT_FOUND"},
   {telux::common::ErrorCode::EXTENDED_INTERNAL, "EXTENDED_INTERNAL"},
   {telux::common::ErrorCode::ACCESS_DENIED, "ACCESS_DENIED"},
   {telux::common::ErrorCode::HARDWARE_RESTRICTED, "HARDWARE_RESTRICTED"},
   {telux::common::ErrorCode::ACK_NOT_SENT, "ACK_NOT_SENT"},
   {telux::common::ErrorCode::INJECT_TIMEOUT, "INJECT_TIMEOUT"},
   {telux::common::ErrorCode::FDN_RESTRICT, "FDN_RESTRICT"},
   {telux::common::ErrorCode::SUPS_FAILURE_CAUSE, "SUPS_FAILURE_CAUSE"},
   {telux::common::ErrorCode::NO_RADIO, "NO_RADIO"},
   {telux::common::ErrorCode::NOT_SUPPORTED, "NOT_SUPPORTED"},
   {telux::common::ErrorCode::CARD_CALL_CONTROL_FAILED, "CARD_CALL_CONTROL_FAILED"},
   {telux::common::ErrorCode::NETWORK_ABORTED, "NETWORK_ABORTED"},
   {telux::common::ErrorCode::MSG_BLOCKED, "MSG_BLOCKED"},
   {telux::common::ErrorCode::INVALID_SESSION_TYPE, "INVALID_SESSION_TYPE"},
   {telux::common::ErrorCode::INVALID_PB_TYPE, "INVALID_PB_TYPE"},
   {telux::common::ErrorCode::NO_SIM, "NO_SIM"},
   {telux::common::ErrorCode::PB_NOT_READY, "PB_NOT_READY"},
   {telux::common::ErrorCode::PIN_RESTRICTION, "PIN_RESTRICTION"},
   {telux::common::ErrorCode::PIN2_RESTRICTION, "PIN2_RESTRICTION"},
   {telux::common::ErrorCode::PUK_RESTRICTION, "PUK_RESTRICTION"},
   {telux::common::ErrorCode::PUK2_RESTRICTION, "PUK2_RESTRICTION"},
   {telux::common::ErrorCode::PB_ACCESS_RESTRICTED, "PB_ACCESS_RESTRICTED"},
   {telux::common::ErrorCode::PB_DELETE_IN_PROG, "PB_DELETE_IN_PROG"},
   {telux::common::ErrorCode::PB_TEXT_TOO_LONG, "PB_TEXT_TOO_LONG"},
   {telux::common::ErrorCode::PB_NUMBER_TOO_LONG, "PB_NUMBER_TOO_LONG"},
   {telux::common::ErrorCode::PB_HIDDEN_KEY_RESTRICTION, "PB_HIDDEN_KEY_RESTRICTION"},
   {telux::common::ErrorCode::PB_NOT_AVAILABLE, "PB_NOT_AVAILABLE"},
   {telux::common::ErrorCode::DEVICE_MEMORY_ERROR, "DEVICE_MEMORY_ERROR"},
   {telux::common::ErrorCode::NO_PERMISSION, "NO_PERMISSION"},
   {telux::common::ErrorCode::TOO_SOON, "TOO_SOON"},
   {telux::common::ErrorCode::TIME_NOT_ACQUIRED, "TIME_NOT_ACQUIRED"},
   {telux::common::ErrorCode::OP_IN_PROGRESS, "OP_IN_PROGRESS"},
   {telux::common::ErrorCode::INTERNAL_ERROR, "INTERNAL_ERROR"},
   {telux::common::ErrorCode::SERVICE_ERROR, "SERVICE_ERROR"},
   {telux::common::ErrorCode::TIMEOUT_ERROR, "TIMEOUT_ERROR"},
   {telux::common::ErrorCode::EXTENDED_ERROR, "EXTENDED_ERROR"},
   {telux::common::ErrorCode::PORT_NOT_OPEN_ERROR, "PORT_NOT_OPEN_ERROR"},
   {telux::common::ErrorCode::MEMCOPY_ERROR, "MEMCOPY_ERROR"},
   {telux::common::ErrorCode::INVALID_TRANSACTION, "INVALID_TRANSACTION"},
   {telux::common::ErrorCode::ALLOCATION_FAILURE, "ALLOCATION_FAILURE"},
   {telux::common::ErrorCode::TRANSPORT_ERROR, "TRANSPORT_ERROR"},
   {telux::common::ErrorCode::PARAM_ERROR, "PARAM_ERROR"},
   {telux::common::ErrorCode::INVALID_CLIENT, "INVALID_CLIENT"},
   {telux::common::ErrorCode::FRAMEWORK_NOT_READY, "FRAMEWORK_NOT_READY"},
   {telux::common::ErrorCode::INVALID_SIGNAL, "INVALID_SIGNAL"},
   {telux::common::ErrorCode::TRANSPORT_BUSY_ERROR, "TRANSPORT_BUSY_ERROR"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_FAIL, "DS_PROFILE_REG_RESULT_FAIL"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_HNDL,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_HNDL"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_OP,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_OP"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_TYPE,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_TYPE"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_NUM,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_NUM"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_IDENT,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_IDENT"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL, "DS_PROFILE_REG_RESULT_ERR_INVAL"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_LIB_NOT_INITED,
    "DS_PROFILE_REG_RESULT_ERR_LIB_NOT_INITED"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_LEN_INVALID,
    "DS_PROFILE_REG_RESULT_ERR_LEN_INVALID"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_LIST_END, "DS_PROFILE_REG_RESULT_LIST_END"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_INVAL_SUBS_ID,
    "DS_PROFILE_REG_RESULT_ERR_INVAL_SUBS_ID"},
   {telux::common::ErrorCode::DS_PROFILE_REG_INVAL_PROFILE_FAMILY,
    "DS_PROFILE_REG_INVAL_PROFILE_FAMILY"},
   {telux::common::ErrorCode::DS_PROFILE_REG_PROFILE_VERSION_MISMATCH,
    "DS_PROFILE_REG_PROFILE_VERSION_MISMATCH"},
   {telux::common::ErrorCode::REG_RESULT_ERR_OUT_OF_MEMORY, "REG_RESULT_ERR_OUT_OF_MEMORY"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_FILE_ACCESS,
    "DS_PROFILE_REG_RESULT_ERR_FILE_ACCESS"},
   {telux::common::ErrorCode::DS_PROFILE_REG_RESULT_ERR_EOF, "DS_PROFILE_REG_RESULT_ERR_EOF"},
   {telux::common::ErrorCode::REG_RESULT_ERR_VALID_FLAG_NOT_SET,
    "REG_RESULT_ERR_VALID_FLAG_NOT_SET"},
   {telux::common::ErrorCode::REG_RESULT_ERR_OUT_OF_PROFILES, "REG_RESULT_ERR_OUT_OF_PROFILES"},
   {telux::common::ErrorCode::REG_RESULT_NO_EMERGENCY_PDN_SUPPORT,
    "REG_RESULT_NO_EMERGENCY_PDN_SUPPORT"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_INVAL_PROFILE_FAMILY,
    "DS_PROFILE_3GPP_INVAL_PROFILE_FAMILY"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_ACCESS_ERR, "DS_PROFILE_3GPP_ACCESS_ERR"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_CONTEXT_NOT_DEFINED,
    "DS_PROFILE_3GPP_CONTEXT_NOT_DEFINED"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_VALID_FLAG_NOT_SET,
    "DS_PROFILE_3GPP_VALID_FLAG_NOT_SET"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_READ_ONLY_FLAG_SET,
    "DS_PROFILE_3GPP_READ_ONLY_FLAG_SET"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP_ERR_OUT_OF_PROFILES,
    "DS_PROFILE_3GPP_ERR_OUT_OF_PROFILES"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP2_ERR_INVALID_IDENT_FOR_PROFILE,
    "DS_PROFILE_3GPP2_ERR_INVALID_IDENT_FOR_PROFILE"},
   {telux::common::ErrorCode::DS_PROFILE_3GPP2_ERR_OUT_OF_PROFILE,
    "DS_PROFILE_3GPP2_ERR_OUT_OF_PROFILE"}};

/**
 * Error descripton
 */
std::string Utils::getErrorCodeAsString(telux::common::ErrorCode error) {

   if(errorCodeToStringMap_.find(error) != std::end(errorCodeToStringMap_)) {
      return errorCodeToStringMap_[error];
   }
   return "UNKNOWN_ERROR";
}

std::vector<gid_t> getGidByName(std::vector<std::string> names) {
    std::vector<gid_t> groupIds;
    for(auto i: names) {
        struct group* tempGrp;
        if((tempGrp = getgrnam(i.c_str())) != NULL) {
            gid_t tmpGid = tempGrp->gr_gid;
            groupIds.push_back(tmpGid);
        }
    }
    return groupIds;
}

size_t Utils::removeDuplicateGroups(std::vector<std::string> & grps) {
    int numGroups = getgroups(0, NULL);
    gid_t currentGids[numGroups]{};
    if( getgroups(numGroups, currentGids) < 0 ) {
        std::cout << "fail getgroups, abort" << std::endl;
        return grps.size();
    }
    for (int i = 0; i < numGroups; i++) {
        struct group *gr = getgrgid(currentGids[i]);
        if (gr != NULL) {
            std::string grName(gr->gr_name);
            for (auto it = grps.begin(); it != grps.end();) {
                if (*it == grName) {
                    it = grps.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    return grps.size();
}

int Utils::setSupplementaryGroups(std::vector<std::string> grps) {
    int ret = 0;
    if (removeDuplicateGroups(grps) == 0) {
        return 0;
    }
    std::vector<gid_t> groupIds = getGidByName(grps);
    int numGroups = getgroups(0, NULL);
    gid_t gid[numGroups]{};
    ret = getgroups(numGroups, gid);
    std::vector<gid_t> existingGidList(gid, gid+numGroups);
    existingGidList.insert(std::end(existingGidList), std::begin(groupIds), std::end(groupIds));
    uint32_t gidListSize = existingGidList.size();
    gid_t newGidList[gidListSize]{};
    std::copy(existingGidList.begin(), existingGidList.end(), newGidList);
    ret = setgroups(gidListSize, newGidList);
    return ret;
}

void Utils::printStatus(telux::common::Status status) {
   switch (status)
   {
      case telux::common::Status::SUCCESS:
         std::cout << "Operation processed successfully" << std::endl;
         break;
      case telux::common::Status::FAILED:
         std::cout << "Operation processing failed" << std::endl;
         break;
      case telux::common::Status::NOCONNECTION:
         std::cout << "Connection to Socket server has not been established" << std::endl;
         break;
      case telux::common::Status::NOSUBSCRIPTION:
         std::cout << "Subscription not available" << std::endl;
         break;
      case telux::common::Status::INVALIDPARAM:
         std::cout << "Input parameters are invalid" << std::endl;
         break;
      case telux::common::Status::INVALIDSTATE:
         std::cout << "Invalid State detected" << std::endl;
         break;
      case telux::common::Status::NOTREADY:
         std::cout << "Subsystem is not ready" << std::endl;
         break;
      case telux::common::Status::NOTALLOWED:
         std::cout << "Operation not allowed" << std::endl;
         break;
      case telux::common::Status::NOTIMPLEMENTED:
         std::cout << "Feature not supported" << std::endl;
         break;
      case telux::common::Status::CONNECTIONLOST:
         std::cout << "Connection to Socket server lost" << std::endl;
         break;
      case telux::common::Status::EXPIRED:
         std::cout << "Operation has expired" << std::endl;
         break;
      case telux::common::Status::ALREADY:
         std::cout << "Already registered handler" << std::endl;
         break;
      case telux::common::Status::NOSUCH:
         std::cout << "No such object" << std::endl;
         break;
      case telux::common::Status::NOTSUPPORTED:
         std::cout << "Not supported on target platform" << std::endl;
         break;
      default:
         break;
   }
}

uint64_t Utils::getCurrentTimestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

const std::string Utils::getCurrentTimeString(void) {
    std::stringstream ss;
    std::tm tmSnapshot;
    std::time_t now = std::time(nullptr);

    if (NULL == localtime_r(&now, &tmSnapshot)) {
        std::cout << "localtime_r error" << std::endl;
        return std::string();
    }
    // convert current time to format of hour:minute:second
    ss << std::put_time(&tmSnapshot, "%H:%M:%S");
    return ss.str();
}

int Utils::validateV2xSpsInterval(uint16_t interval) {
    if (0 == interval || 1000 < interval) {
        return EXIT_FAILURE;
    }

    if (20 == interval || 50 == interval || 0 == interval % 100) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

uint64_t Utils::getNanosecondsSinceBoot() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
}

std::vector<uint8_t> Utils::convertHexToBytes(std::string hexData) {
   std::vector<uint8_t> output;
   size_t i = 0, len = 0;
   uint8_t rawData1 = 0, rawData2 = 0, rawData = 0;

   len = hexData.length();
   for(i = 0; i < len; i = i + 2) {
      if(hexData[i] >= '0' && hexData[i] <= '9') {
         rawData1 = (hexData[i] - 48) * 16;
      } else if(hexData[i] >= 'A' && hexData[i] <= 'F') {
         rawData1 = (hexData[i] - 55) * 16;
      } else if(hexData[i] >= 'a' && hexData[i] <= 'f') {
         rawData1 = (hexData[i] - 87) * 16;
      }

      if(hexData[i + 1] >= '0' && hexData[i + 1] <= '9') {
         rawData2 = hexData[i + 1] - 48;
      } else if(hexData[i + 1] >= 'A' && hexData[i + 1] <= 'F') {
         rawData2 = hexData[i + 1] - 55;
      } else if(hexData[i + 1] >= 'a' && hexData[i + 1] <= 'f') {
         rawData2 = hexData[i + 1] - 87;
      }

      rawData = rawData1 + rawData2;
      output.emplace_back(rawData);
   }
   return output;
}
