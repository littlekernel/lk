/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except compliance with the License.
 * You may obtaa copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHWARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __RUNTIME_SERVICE_H_
#define __RUNTIME_SERVICE_H_

#include "boot_service.h"
#include "types.h"
#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define EFI_2_80_SYSTEM_TABLE_REVISION ((2 << 16) | (80))
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2 << 16) | (70))
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2 << 16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2 << 16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2 << 16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2 << 16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2 << 16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2 << 16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2 << 16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2 << 16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1 << 16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1 << 16) | (02))
#define EFI_SYSTEM_TABLE_REVISION EFI_2_70_SYSTEM_TABLE_REVISION
#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION

#define EFI_RUNTIME_SERVICES_REVISION EFI_SPECIFICATION_VERSION

/**
   Returns the current time and date information, and the time-keeping
 capabilities of the hardware platform.

   @param[out]  Time             A pointer to storage to receive a snapshot of
 the current time.
   @param[out]  Capabilities     An optional pointer to a buffer to receive the
 real time clock device's capabilities.

   @retval EFI_SUCCESS           The operation completed successfully.
   @retval EFI_INVALID_PARAMETER Time is NULL.
   @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware
 error.
   @retval EFI_UNSUPPORTED       This call is not supported by this platform at
 the time the call is made. The platform should describe this runtime service as
 unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

 **/
typedef EFI_STATUS (*EFI_GET_TIME)(EfiTime *Time,
                                   EFI_TIME_CAPABILITIES *Capabilities);

/**
  Sets the current local time and date information.

  @param[in]  Time              A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware
error.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_SET_TIME)(EfiTime *Time);

/**
  Returns the current wakeup alarm clock setting.

  @param[out]  Enabled          Indicates if the alarm is currently enabled or
disabled.
  @param[out]  Pending          Indicates if the alarm signal is pending and
requires acknowledgement.
  @param[out]  Time             The current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Enabled is NULL.
  @retval EFI_INVALID_PARAMETER Pending is NULL.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a
hardware error.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_GET_WAKEUP_TIME)(bool *Enabled, bool *Pending,
                                          EfiTime *Time);

/**
  Sets the system wakeup alarm clock time.

  @param[in]  Enable            Enable or disable the wakeup alarm.
  @param[in]  Time              If Enable is TRUE, the time to set the wakeup
alarm for. If Enable is FALSE, then this parameter is optional, and may be NULL.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was
enabled. If Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a
hardware error.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_SET_WAKEUP_TIME)(bool Enable, EfiTime *Time);

/**
  Changes the runtime addressing mode of EFI firmware from physical to virtual.

  @param[in]  MemoryMapSize     The size in bytes of VirtualMap.
  @param[in]  DescriptorSize    The size in bytes of an entry in the VirtualMap.
  @param[in]  DescriptorVersion The version of the structure entries in
VirtualMap.
  @param[in]  VirtualMap        An array of memory descriptors which contain new
virtual address mapping information for all runtime ranges.

  @retval EFI_SUCCESS           The virtual address map has been applied.
  @retval EFI_UNSUPPORTED       EFI firmware is not at runtime, or the EFI
firmware is already in virtual address mapped mode.
  @retval EFI_INVALID_PARAMETER DescriptorSize or DescriptorVersion is invalid.
  @retval EFI_NO_MAPPING        A virtual address was not supplied for a range
in the memory map that requires a mapping.
  @retval EFI_NOT_FOUND         A virtual address was supplied for an address
that is not found in the memory map.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_SET_VIRTUAL_ADDRESS_MAP)(
    size_t MemoryMapSize, size_t DescriptorSize, uint32_t DescriptorVersion,
    EfiMemoryDescriptor *VirtualMap);

/**
  Determines the new virtual address that is to be used on subsequent memory
accesses.

  @param[in]       DebugDisposition  Supplies type information for the pointer
being converted.
  @param[in, out]  Address           A pointer to a pointer that is to be fixed
to be the value needed for the new virtual address mappings being applied.

  @retval EFI_SUCCESS           The pointer pointed to by Address was modified.
  @retval EFI_NOT_FOUND         The pointer pointed to by Address was not found
to be part of the current memory map. This is normally fatal.
  @retval EFI_INVALID_PARAMETER Address is NULL.
  @retval EFI_INVALID_PARAMETER *Address is NULL and DebugDisposition does
                                not have the EFI_OPTIONAL_PTR bit set.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_CONVERT_POINTER)(size_t DebugDisposition,
                                          void **Address);

/**
   Returns the value of a variable.

   @param[in]       VariableName  A Null-terminated string that is the name of
 the vendor's variable.
   @param[in]       VendorGuid    A unique identifier for the vendor.
   @param[out]      Attributes    If not NULL, a pointer to the memory location
 to return the attributes bitmask for the variable.
   @param[in, out]  DataSize      On input, the size in bytes of the return Data
 buffer. On output the size of data returned in Data.
   @param[out]      Data          The buffer to return the contents of the
 variable. May be NULL with a zero DataSize in order to determine the size
 buffer needed.

   @retval EFI_SUCCESS            The function completed successfully.
   @retval EFI_NOT_FOUND          The variable was not found.
   @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
   @retval EFI_INVALID_PARAMETER  VariableName is NULL.
   @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
   @retval EFI_INVALID_PARAMETER  DataSize is NULL.
   @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is
 NULL.
   @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a
 hardware error.
   @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an
 authentication failure.
   @retval EFI_UNSUPPORTED        After ExitBootServices() has been called, this
 return code may be returned if no variable storage is supported. The platform
 should describe this runtime service as unsupported at runtime via an
 EFI_RT_PROPERTIES_TABLE configuration table.

 **/
typedef EFI_STATUS (*EFI_GET_VARIABLE)(char16_t *VariableName,
                                       EfiGuid *VendorGuid,
                                       uint32_t *Attributes, size_t *DataSize,
                                       void *Data);

/**
  Enumerates the current variable names.

  @param[in, out]  VariableNameSize The size of the VariableName buffer. The
size must be large enough to fit input string supplied in VariableName buffer.
  @param[in, out]  VariableName     On input, supplies the last VariableName
that was returned by GetNextVariableName(). On output, returns the
Nullterminated string of the current variable.
  @param[in, out]  VendorGuid       On input, supplies the last VendorGuid that
was returned by GetNextVariableName(). On output, returns the VendorGuid of the
current variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the
result. VariableNameSize has been updated with the size needed to complete the
request.
  @retval EFI_INVALID_PARAMETER VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER VariableName is NULL.
  @retval EFI_INVALID_PARAMETER VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER The input values of VariableName and VendorGuid
are not a name and GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER Null-terminator is not found in the first
VariableNameSize bytes of the input VariableName buffer.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved due to a
hardware error.
  @retval EFI_UNSUPPORTED       After ExitBootServices() has been called, this
return code may be returned if no variable storage is supported. The platform
should describe this runtime service as unsupported at runtime via an
EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_GET_NEXT_VARIABLE_NAME)(size_t *VariableNameSize,
                                                 char16_t *VariableName,
                                                 EfiGuid *VendorGuid);

/**
  Sets the value of a variable.

  @param[in]  VariableName       A Null-terminated string that is the name of
the vendor's variable. Each VariableName is unique for each VendorGuid.
VariableName must contain 1 or more characters. If VariableName is an empty
string, then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless
the EFI_VARIABLE_APPEND_WRITE or
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
attribute is set, a size of zero causes the variable to be deleted. When the
EFI_VARIABLE_APPEND_WRITE attribute is set, then a SetVariable() call with a
DataSize of zero will not cause any change to the variable value (the timestamp
associated with the variable may be updated however even if no new data value is
provided,see the description of the EFI_VARIABLE_AUTHENTICATION_2 descriptor
below. In this case the DataSize will not be zero since the
EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data               The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the
variable and its data as defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name,
and GUID was supplied, or the DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a
hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to
EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set, but the AuthInfo
does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted
was not found.
  @retval EFI_UNSUPPORTED        This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_SET_VARIABLE)(char16_t *VariableName,
                                       EfiGuid *VendorGuid, uint32_t Attributes,
                                       size_t DataSize, void *Data);

/**
Returns the next high 32 bits of the platform's monotonic counter.

@param[out]  HighCount        The pointer to returned value.

@retval EFI_SUCCESS           The next high monotonic count was returned.
@retval EFI_INVALID_PARAMETER HighCount is NULL.
@retval EFI_DEVICE_ERROR      The device is not functioning properly.
@retval EFI_UNSUPPORTED       This call is not supported by this platform at the
time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_GET_NEXT_HIGH_MONO_COUNT)(uint32_t *HighCount);

///
/// Enumeration of reset types.
///
typedef enum {
  ///
  /// Used to induce a system-wide reset. This sets all circuitry within the
  /// system to its initial state.  This type of reset is asynchronous to system
  /// operation and operates withgout regard to cycle boundaries.  EfiColdReset
  /// is tantamount to a system power cycle.
  ///
  EfiResetCold,
  ///
  /// Used to induce a system-wide initialization. The processors are set to
  /// their
  /// initial state, and pending cycles are not corrupted.  If the system does
  /// not support this reset type, then an EfiResetCold must be performed.
  ///
  EfiResetWarm,
  ///
  /// Used to induce an entry into a power state equivalent to the ACPI G2/S5 or
  /// G3
  /// state.  If the system does not support this reset type, then when the
  /// system
  /// is rebooted, it should exhibit the EfiResetCold attributes.
  ///
  EfiResetShutdown,
  ///
  /// Used to induce a system-wide reset. The exact type of the reset is defined
  /// by
  /// the EFI_GUID that follows the Null-terminated Unicode string passed into
  /// ResetData. If the platform does not recognize the EFI_GUID in ResetData
  /// the
  /// platform must pick a supported reset type to perform. The platform may
  /// optionally log the parameters from any non-normal reset that occurs.
  ///
  EfiResetPlatformSpecific
} EFI_RESET_TYPE;

/**
  Resets the entire platform.

  @param[in]  ResetType         The type of reset to perform.
  @param[in]  ResetStatus       The status code for the reset.
  @param[in]  DataSize          The size, in bytes, of ResetData.
  @param[in]  ResetData         For a ResetType of EfiResetCold, EfiResetWarm,
or EfiResetShutdown the data buffer starts with a Null-terminated string,
optionally followed by additional binary data. The string is a description that
the caller may use to further indicate the reason for the system reset. For a
ResetType of EfiResetPlatformSpecific the data buffer also starts with a
Null-terminated string that is followed by an EfiGuid that describes the
specific type of reset to perform.
**/
typedef void (*EFI_RESET_SYSTEM)(EFI_RESET_TYPE ResetType,
                                 EFI_STATUS ResetStatus, size_t DataSize,
                                 void *ResetData);

///
/// EFI Capsule Header.
///
typedef struct {
  ///
  /// A GUID that defines the contents of a capsule.
  ///
  EfiGuid CapsuleGuid;
  ///
  /// The size of the capsule header. This may be larger than the size of
  /// the EFI_CAPSULE_HEADER since CapsuleGuid may imply
  /// extended header entries
  ///
  uint32_t HeaderSize;
  ///
  /// Bit-mapped list describing the capsule attributes. The Flag values
  /// of 0x0000 - 0xFFFF are defined by CapsuleGuid. Flag values
  /// of 0x10000 - 0xFFFFFFFF are defined by this specification
  ///
  uint32_t Flags;
  ///
  /// Size in bytes of the capsule (including capsule header).
  ///
  uint32_t CapsuleImageSize;
} EFI_CAPSULE_HEADER;

/**
  Passes capsules to the firmware with both virtual and physical mapping.
Depending on the intended consumption, the firmware may process the capsule
immediately. If the payload should persist across a system reset, the reset
value returned from EFI_QueryCapsuleCapabilities must be passed into
ResetSystem() and will cause the capsule to be processed by the firmware as part
of the reset process.

  @param[in]  CapsuleHeaderArray Virtual pointer to an array of virtual pointers
to the capsules being passed into update capsule.
  @param[in]  CapsuleCount       Number of pointers to EFI_CAPSULE_HEADER in
                                 CaspuleHeaderArray.
  @param[in]  ScatterGatherList  Physical pointer to a set of
                                 EFI_CAPSULE_BLOCK_DESCRIPTOR that describes the
                                 location in physical memory of a set of
capsules.

  @retval EFI_SUCCESS           Valid capsule was passed. If
                                CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set,
the capsule has been successfully processed by the firmware.
  @retval EFI_INVALID_PARAMETER CapsuleSize is NULL, or an incompatible set of
flags were set in the capsule header.
  @retval EFI_INVALID_PARAMETER CapsuleCount is 0.
  @retval EFI_DEVICE_ERROR      The capsule update was started, but failed due
to a device error.
  @retval EFI_UNSUPPORTED       The capsule type is not supported on this
platform.
  @retval EFI_OUT_OF_RESOURCES  When ExitBootServices() has been previously
called this error indicates the capsule is compatible with this platform but is
not capable of being submitted or processed in runtime. The caller may resubmit
the capsule prior to ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES  When ExitBootServices() has not been previously
called then this error indicates the capsule is compatible with this platform
but there are insufficient resources to process.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_UPDATE_CAPSULE)(
    EFI_CAPSULE_HEADER **CapsuleHeaderArray, size_t CapsuleCount,
    uint64_t ScatterGatherList);

/**
  Returns if the capsule can be supported via UpdateCapsule().

  @param[in]   CapsuleHeaderArray  Virtual pointer to an array of virtual
pointers to the capsules being passed into update capsule.
  @param[in]   CapsuleCount        Number of pointers to EFI_CAPSULE_HEADER in
                                   CaspuleHeaderArray.
  @param[out]  MaxiumCapsuleSize   On output the maximum size that
UpdateCapsule() can support as an argument to UpdateCapsule() via
                                   CapsuleHeaderArray and ScatterGatherList.
  @param[out]  ResetType           Returns the type of reset required for the
capsule update.

  @retval EFI_SUCCESS           Valid answer returned.
  @retval EFI_UNSUPPORTED       The capsule type is not supported on this
platform, and MaximumCapsuleSize and ResetType are undefined.
  @retval EFI_INVALID_PARAMETER MaximumCapsuleSize is NULL.
  @retval EFI_OUT_OF_RESOURCES  When ExitBootServices() has been previously
called this error indicates the capsule is compatible with this platform but is
not capable of being submitted or processed in runtime. The caller may resubmit
the capsule prior to ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES  When ExitBootServices() has not been previously
called then this error indicates the capsule is compatible with this platform
but there are insufficient resources to process.
  @retval EFI_UNSUPPORTED       This call is not supported by this platform at
the time the call is made. The platform should describe this runtime service as
unsupported at runtime via an EFI_RT_PROPERTIES_TABLE configuration table.

**/
typedef EFI_STATUS (*EFI_QUERY_CAPSULE_CAPABILITIES)(
    EFI_CAPSULE_HEADER **CapsuleHeaderArray, size_t CapsuleCount,
    uint64_t *MaximumCapsuleSize, EFI_RESET_TYPE *ResetType);

/**
  Returns information about the EFI variables.

  @param[in]   Attributes                   Attributes bitmask to specify the
type of variables on which to return information.
  @param[out]  MaximumVariableStorageSize   On output the maximum size of the
storage space available for the EFI variables associated with the attributes
specified.
  @param[out]  RemainingVariableStorageSize Returns the remaining size of the
storage space available for the EFI variables associated with the attributes
specified.
  @param[out]  MaximumVariableSize          Returns the maximum size of the
individual EFI variables associated with the attributes specified.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits
was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this
platform, and the MaximumVariableStorageSize, RemainingVariableStorageSize,
MaximumVariableSize are undefined.

**/
typedef EFI_STATUS (*EFI_QUERY_VARIABLE_INFO)(
    uint32_t Attributes, uint64_t *MaximumVariableStorageSize,
    uint64_t *RemainingVariableStorageSize, uint64_t *MaximumVariableSize);

typedef struct {
  EfiTableHeader Hdr;

  //
  // Time Services
  //
  EFI_GET_TIME GetTime;
  EFI_SET_TIME SetTime;
  EFI_GET_WAKEUP_TIME GetWakeupTime;
  EFI_SET_WAKEUP_TIME SetWakeupTime;

  //
  // Virtual Memory Services
  //
  EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
  EFI_CONVERT_POINTER ConvertPointer;

  //
  // Variable Services
  //
  EFI_GET_VARIABLE GetVariable;
  EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
  EFI_SET_VARIABLE SetVariable;

  //
  // Miscellaneous Services
  //
  EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM ResetSystem;

  //
  // UEFI 2.0 Capsule Services
  //
  EFI_UPDATE_CAPSULE UpdateCapsule;
  EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;

  //
  // Miscellaneous UEFI 2.0 Service
  //
  EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
} EfiRuntimeService;

#endif
