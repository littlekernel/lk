/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <lib/unittest.h>
#include <stdint.h>
#include <uefi/protocols/hii_protocol.h>

#include "../uefi_platform.h"

namespace {

constexpr uint32_t package_fields(uint8_t type, uint32_t length) {
  return (static_cast<uint32_t>(type) << 24) | length;
}

struct EndOnlyPackageList {
  EfiHiiPackageListHeader header;
  EfiHiiPackageHeader end;
} __PACKED;

EndOnlyPackageList make_package_list(uint32_t guid_data1) {
  return {
      .header =
          {
              .package_list_guid =
                  {
                      .data1 = guid_data1,
                      .data2 = 0x1234,
                      .data3 = 0x5678,
                      .data4 = {0x90, 0xab, 0xcd, 0xef, 1, 2, 3, 4},
                  },
              .package_length = sizeof(EndOnlyPackageList),
          },
      .end = {.fields = package_fields(EFI_HII_PACKAGE_END,
                                       sizeof(EfiHiiPackageHeader))},
  };
}

bool hii_package_list_lifecycle_test() {
  BEGIN_TEST;

  EfiHiiDatabaseProtocol *protocol = open_hii_database_protocol();
  ASSERT_NONNULL(protocol);

  EndOnlyPackageList package_list = make_package_list(1);
  int first_driver = 0;
  int second_driver = 0;
  EfiHiiHandle first_handle = nullptr;
  EfiHiiHandle second_handle = nullptr;

  EXPECT_EQ(EFI_STATUS_SUCCESS,
            protocol->new_package_list(protocol, &package_list.header,
                                       &first_driver, &first_handle));
  ASSERT_NONNULL(first_handle);

  EfiHandle driver_handle = nullptr;
  EXPECT_EQ(EFI_STATUS_SUCCESS, protocol->get_package_list_handle(
                                    protocol, first_handle, &driver_handle));
  EXPECT_EQ(static_cast<EfiHandle>(&first_driver), driver_handle);

  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header,
                                       &first_driver, &second_handle));
  EXPECT_NULL(second_handle);

  EXPECT_EQ(EFI_STATUS_SUCCESS,
            protocol->new_package_list(protocol, &package_list.header,
                                       &second_driver, &second_handle));
  ASSERT_NONNULL(second_handle);

  EXPECT_EQ(EFI_STATUS_SUCCESS,
            protocol->remove_package_list(protocol, first_handle));
  EXPECT_EQ(EFI_STATUS_NOT_FOUND,
            protocol->remove_package_list(protocol, first_handle));
  EXPECT_EQ(EFI_STATUS_SUCCESS,
            protocol->remove_package_list(protocol, second_handle));

  END_TEST;
}

bool hii_package_list_validation_test() {
  BEGIN_TEST;

  EfiHiiDatabaseProtocol *protocol = open_hii_database_protocol();
  ASSERT_NONNULL(protocol);

  EndOnlyPackageList package_list = make_package_list(2);
  EfiHiiHandle handle = nullptr;

  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(nullptr, &package_list.header, nullptr,
                                       &handle));
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, nullptr, nullptr, &handle));
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header, nullptr,
                                       nullptr));

  package_list.header.package_length = sizeof(EfiHiiPackageListHeader);
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header, nullptr,
                                       &handle));

  package_list = make_package_list(2);
  package_list.end.fields = package_fields(EFI_HII_PACKAGE_FORMS, 0);
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header, nullptr,
                                       &handle));

  package_list = make_package_list(2);
  package_list.end.fields =
      package_fields(EFI_HII_PACKAGE_FORMS, sizeof(EfiHiiPackageHeader));
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header, nullptr,
                                       &handle));

  package_list = make_package_list(2);
  package_list.end.fields =
      package_fields(EFI_HII_PACKAGE_END, sizeof(EfiHiiPackageHeader) + 1);
  EXPECT_EQ(EFI_STATUS_INVALID_PARAMETER,
            protocol->new_package_list(protocol, &package_list.header, nullptr,
                                       &handle));

  END_TEST;
}

BEGIN_TEST_CASE(hii_protocol_tests);
RUN_TEST(hii_package_list_lifecycle_test);
RUN_TEST(hii_package_list_validation_test);
END_TEST_CASE(hii_protocol_tests);

}  // namespace
