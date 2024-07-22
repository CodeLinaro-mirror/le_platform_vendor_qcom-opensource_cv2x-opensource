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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
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
 * Utility helper class
 * @brief Utils class performs common error code conversions
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>
#include <telux/common/CommonDefines.hpp>

#define SEC_TO_NANOS 1000000000
#define SEC_TO_MICROS 1000000

class Utils {
public:
   // Validate the input and in case of invalid input request
   // for proper input from user.
   template <typename T>
   static bool isInputValid(T input, std::initializer_list<T> list) {
      for (auto elem = list.begin(); elem != list.end(); ++elem)
      {
         if (*elem == input)
         {
            return true;
         }
      }
      return false;
   }

   template <typename T>
   static void validateInput(T &input, std::initializer_list<T> list) {
      bool valid = false;
      do {
          //If user input is within the list, just exist
         if ((std::cin.good()) && (isInputValid(input, list)))
         {
            valid = true;
         }
         else {
            //User input does not match any of the possible entries
            std::cin.clear();
            // Extracts characters from the previous input sequence and discards them,
            // until entire stream have been extracted, or one compares equal to newline.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "ERROR: Invalid input, please re-enter." << std::endl;
            std::cin >> input;
         }
      } while (!valid);
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
   }

   // Validate the input and in case of invalid input request
   // for proper input from user.
   template <typename T>
   static void validateInput(T &input) {
      bool valid = false;
      do {
         if(std::cin.good()) {
            valid = true;
         } else {
            // If an error occurs then an error flag is set and future attempts to get
            // input will fail. Cear the error flag on cin.
            std::cin.clear();
            // Extracts characters from the previous input sequence and discards them,
            // until entire stream have been extracted, or one compares equal to newline.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "ERROR: Invalid input, please re-enter." << std::endl;
            std::cin >> input;
         }
      } while(!valid);
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
   }

   // Validate input string(Ex: 1, 2, 3) which should contain
   // atleast one number or numbers seperated by either comma, space or both.
   static void validateNumericString(std::string &input);
   // Validate input string(Ex: 123) which should contain digits and returns the result
   static bool validateDigitString(std::string &input);
   // Validate the slot id and in case of invalid slot id request
   // for proper input from user.
   static int getValidSlotId();

   /**
    * Get error description for given ErrorCode
    */
   static std::string getErrorCodeAsString(telux::common::ErrorCode error);

   static size_t removeDuplicateGroups(std::vector<std::string> & grps);

   static int setSupplementaryGroups(std::vector<std::string> grps);

   // Print status message that corresponds to the return value of managers api(s) of type
   // telux::common::Status.
   static void printStatus(telux::common::Status status);

   // return current UTC time in microseconds
   static uint64_t getCurrentTimestamp(void);

   // return string of current UTC time in format of hour:minute:second
   static const std::string getCurrentTimeString(void);

   // Validate input V2X SPS interval which should comply with supported values in 3GPP
   static int validateV2xSpsInterval(uint16_t interval);

   // Get the number of nanoseconds elapsed since boot
   static uint64_t getNanosecondsSinceBoot();

   /**
    * Convert the hexadecimal string to byte array/vector
    *  Eg: i/p: 0229440680E30A51439E
    *      o/p: 2,41,68,6,128,227,10,81,67,158
    */
   static std::vector<uint8_t> convertHexToBytes(std::string hexData);
};

#endif
