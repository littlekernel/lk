# Changelog
All notable changes to this project are documented in this file.

## [2.3.0] - 2020-08-19
### Added
- Added support for nRF52805.
- Implemented workaround for nRF52 Series anomaly 197 in the POWER HAL.
- Implemented workaround for nRF52 Series anomalies 211 and 223 in the USBD driver.
- Added support for the nRF53 Series in the QSPI HAL.
- Added function in the GPIO HAL that returns port index.
- Introduced a QSPI HAL symbol that indicates the availability of the QSPI mode 1.
- Introduced shortcut functionality in the CCM HAL.
- Added function in the TIMER HAL for setting the specified shortcuts.
- Added optional two-stage start procedure of the LFXO in the CLOCK driver.

### Changed
- Updated MDK to 8.35.0.
- Divided network and application core-specific functionalities for nRF5340 in the RESET HAL and the reset reason helper.

### Fixed
- Corrected assertions in the NVMC driver.
- Corrected return types in the CCM HAL.
- Fixed setting of program memory access mode for secure code in the NVMC driver.
- Removed usage of the NRF_UICR symbol on non-secure targets in address validity checks in the NVMC driver.
- Fixed an error message in the SPIM driver that prevented successful compilation with logging enabled.
- Fixed unused parameters in the PPI HAL.

## [2.2.0] - 2020-04-28
### Added
- Added support for nRF52820.
- Added possibility to use the nRF52833 DK in development for nRF52820. LEDs and buttons in this DK use GPIOs that are not present in the actual nRF52820 device, so accesses to these need special handling in the GPIO HAL.
- Added the nrfx_reset_reason helper layer, to facilitate developing generic code that uses the RESETREAS register.
- Implemented workarounds for nRF52 Series anomalies 170 and 196 in the I2S driver.
- Added implicit casting of uint32_t to unsigned long in log messages, to prevent compiler warnings.
- Added missing secure and non-secure peripheral symbol translations for nRF5340 and nRF9160.
- Added functions in the NVMC driver for reading and writing halfword data from the OTP region of the UICR.

### Changed
- Updated MDK to 8.32.1.
- Improved documentation regarding double-buffered reception in the UARTE driver.
- Unified policy of pin cleanup procedure during uninitialization of the drivers. Now every driver restores utilized pins to default setting.
- Removed support for revision Engineering A of nRF52840 in the USBD driver.
- Changed selected while() loops to NRFX_WAIT_FOR() macro in the SPIM, UARTE and CLOCK drivers to improve robustness.

### Fixed
- Fixed assertions that check write addresses in the NVMC driver. Now the UICR memory region is handled properly.
- Fixed a race condition in the CLOCK driver when requested clock was stopped during ramp-up from high priority interrupt.
- Fixed assertions that check pin numbers in the GPIO HAL and GPIOTE driver. Now noncontiguous groups of available pin numbers are handled properly.
- Fixed assertion that checks correctness of limit setting in the SAADC driver.
- Fixed spurious callback invocation when the SPIM driver was reinitialized to blocking mode from non-blocking mode.
- Fixed missing workaround for I2S STOP anomaly for nRF52833.
- Fixed handling of NRFX_SPIM_FLAG_HOLD_XFER setting in the blocking mode of the SPIM driver.
- Fixed void pointer cast in the CLOCK HAL, which could cause memory corruption with specific compiler settings.
- Fixed definition of the NRF_CLOCK_LFCLK_Synth value, to make it available also in builds for nRF5340.

## [2.1.0] - 2020-01-24
### Added
- Added HALs for DCNF, OSCILLATORS, USBREG, and VREQCTRL.
- Added support for 1-MHz clock frequency in TWIM.
- Introduced the NRFX_I2S_STATUS_TRANSFER_STOPPED flag in the I2S driver.
- Introduced the nrfx_power_compat layer that allows use of the nrfx_power API with new SoC.
- Added encryption support in the QSPI driver.
- Added support for USBD in nRF5340.
- Expanded HALs to cover new functions in nRF5340: GPIO, I2S, PDM, POWER, QSPI, and REGULATORS.
- Introduced new clock management system in the CLOCK driver.
- Introduced new audio clock configuration settings in the I2S and PDM drivers for nRF5340.
- Implemented workaround for nRF5340 anomaly 4 in the CLOCK driver.
- Implemented workaround for nRF5340 anomaly 10 in the CCM HAL.
- Implemented workaround for nRF9160 anomaly 21 and nRF5340 anomaly 6 in the NVMC HAL.
- Implemented workaround for nRF9160 anomaly 23 and nRF5340 anomaly 44 in the UARTE driver.
- Introduced the NRFX_TWIM_NO_SPURIOUS_STOP_CHECK flag in the TWIM driver.
- Added functions for getting shortcut configuration in the TWIM HAL.

### Changed
- Updated MDK to 8.30.2.
- Reorganized templates of nrfx_config header files for different SoCs. Now they are included through one common file according to the selected SoC.
- Improved the UARTE driver to consume less current after the driver uninitialization. Now all clocks are disabled properly after uninitialization.
- Improved the GPIOTE driver robustness by setting the LATCH functionality to be used by default.
- Changed names of the frequency divider symbols in the QSPI HAL to reflect the new frequencies in nRF5340. Old API names were preserved and are still supported.
- Improved spurious STOP condition handling in the TWIM driver.
- Improved sampling procedure in the advanced blocking mode in the SAADC driver.
- Improved calibration procedure in the SAADC driver for nRF5340 and nRF9160.

### Fixed
- Fixed address assertions in NVMC driver for the nRF5340 network core.
- Fixed an issue in the TWI driver that would make the driver stuck when a premature STOP condition was generated by a slave device. The driver now handles this situation properly and signals that a bus error occurred.
- Fixed the stopping procedure in the PWM driver. Previously in very specific circumstances the PWM output might be not stopped at all or might be immediately restarted.

## [2.0.0] - 2019-11-06
### Added
- Added support for nRF5340.
- Added HALs for: CACHE, FPU, MUTEX, and RESET.
- Added driver and HAL for IPC.
- Added possibility to configure in UART and UARTE the number of stop bits and the type of parity, when a given SoC allows it.
- Added function in the GPIO HAL for selecting the MCU to control the specified pin.
- Added support for ONESHOT register in the TIMER HAL.
- Added support for LIST feature in HALs for SPIS and TWIS.
- Added possibility to choose TIMER instance used for workarounds in the NFCT driver.

### Changed
- Updated MDK to 8.29.0.
- Enhanced PWM driver API: added the "p_context" parameter to the event handler.
- Updated address and task getters in all HALs to return values as uint32_t type.
- Updated all HAL functions to take the pointer to the structure of registers of the peripheral as their first argument.
- Changed __STATIC_INLINE symbol to NRF_STATIC_INLINE for HALs and NRFX_STATIC_INLINE for drivers.
- Refactored the SAADC driver and HAL.
- Refactored the WDT driver and HAL to support multiple instances.
- Changed nrfx_gpiote_init() function to take the interrupt priority as its parameter. Previously this priority was an nrfx_config option.
- Changed nrf_usbd_ep_all_disable() function to disable really all endpoints. Use nrf_usbd_ep_default_config() to restore the default endpoint configuration.
- Updated nrfx_gpiote_out_init() and nrfx_gpiote_in_init() return codes. Now NRFX_ERROR_INVALID_STATE is changed to NRFX_ERROR_BUSY.
- Replaced the SWI/EGU driver with one for EGU only.
- Aligned symbol names for default IRQ priority in nrfx_config. These symbols are now adhering to the following standard: NRFX_xxx_DEFAULT_CONFIG_IRQ_PRIORITY.
- Changed the way of configuring the MISO pin pull setting in SPI and SPIM drivers. Now it can be set separately for each instance.

### Removed
- Removed deprecated functions from drivers: TWI and TWIM. See migration guide for details.
- Removed deprecated functions from HALs: ECB, NVMC, and TEMP. See migration guide for details.
- Removed redundant bariers in the nrfx_usbd driver.
- Removed the default configuration values for drivers from the nrfx_config header files.

## [1.8.1] - 2019-10-21
### Added
- Added functions in the GPIOTE driver for getting task or event for the specified GPIO pin.

### Changed
- Updated MDK to version 8.27.1.
- Moved the nrfx_gppi helper from helpers/nrfx_gppi/ to helpers/.
- Changed the interrupt initialization in the GPIOTE driver, so that mapping of the GPIOTEx_IRQn enumeration values is no longer needed for nRF9160.

## [1.8.0] - 2019-08-27
### Added
- Added support for nRF52833.
- Added bus recovery feature in the TWI and TWIM drivers.
- Added the nrfx_gppi helper layer to facilitate developing generic code that can utilize PPI or DPPI, depending on which interface is available in a given SoC.

### Changed
- Updated MDK to version 8.27.0.

### Fixed
- Fixed an issue in the TWIM driver that would make the driver stuck when a premature STOP condition was generated by a slave device. The driver now handles this situation properly and signals that a bus error occurred.
- Fixed a frame timing bug in the NFCT driver. Previously, the timing of the SENS_RES response could be violated after the NFCT peripheral was put to the Sleep state with the SLP_REQ command.

## [1.7.2] - 2019-07-25
### Added
- Added functions in the DPPI, GPIOTE, PPI, RTC, and TIMER HALs for getting tasks and events specified by index.
- Added the possibility of suspending transfers in the TWI driver. This allows combining several transfers into one continuous TWI transaction.
- Added termination of transfers at deinitialization of the UARTE driver.
- Added buffer alignment checks in the QSPI driver.
- Introduced the NRFX_OFFSETOF macro that duplicates the functionality of the built-in offsetof() mechanism, but can be used without issues also with non-constant expressions.
- Added an alternative way of ending the DMA transfer loop in the USBD driver.
- Added the CTSTARTED and CTSTOPPED events to the CLOCK HAL.

### Changed
- Removed an assertion that prevented setting the data payload size of isochronous endpoints to zero, to fulfill requirements of the USB 2.0 specification, paragraph 5.6.3.
- Declared the tx_buffer_length field in the UART driver's control block as volatile to prevent issues in case of compilation with high optimization level.

### Fixed
- Fixed an incorrect conversion of frequency values in the RADIO HAL.
- Fixed an incorrectly enabled interrupt in the QSPI driver.
- Corrected the LFCLK source selection values in the template configuration file for nRF9160.
- Fixed support for external LFCLK sources for nRF52811.

## [1.7.1] - 2019-04-08
### Added
- Added functions in the NVMC driver for getting the flash page size, the count of pages and the total flash size.

### Fixed
- Fixed handling of short unaligned write requests (1 or 2 bytes in length) in the nrfx_nvmc_bytes_write() function.

## [1.7.0] - 2019-03-29
### Added
- Added drivers for NVMC and TEMP.
- Added HALs: AAR and FICR.
- Added support for the custom instruction long frame mode in the QSPI driver.

### Changed
- Reworked HAL for NVMC. Now it can be used for all SoCs supported by nrfx.
- Reworked HAL for TEMP.
- Improved documentation. Now it is more precise and can be generated without warnings with newer versions of doxygen.
- Improved the UARTE driver to consume less current after the TX operation. Now at the end of the transmission the transmitter is turned off by the STOPTX task.
- Improved C++ support in drivers. Now fields in structures are filled up in the correct order.
- Changed to size_t the type used for holding the amount of data in the TWIS driver.

### Fixed
- Fixed a race condition in the USBD driver. It could occur when an IN transfer was interrupted by an OUT transaction, which in turn was interrupted by a process with a higher priority.

## [1.6.2] - 2019-02-12
### Added
- Added the possibility to use the macro NRFX_COREDEP_DELAY_US_LOOP_CYCLES to specify the number of cycles consumed by one iteration of the internal loop in the function nrfx_coredep_delay_us().

### Changed
- Updated MDK to version 8.24.1.

## [1.6.1] - 2019-01-29
### Fixed
- Fixed an issue in the NFCT driver that caused a performance loss on nRF52832. The interrupt configuration is now properly restored after the NRFX_NFCT_EVT_FIELD_LOST event.

## [1.6.0] - 2019-01-18
### Added
- Added support for nRF52811.
- Added support for the legacy peripherals SPI, TWI, and UART in nRF52810.
- Added support for SAMPLERATE in nrf_saadc.h.
- Added clearing of the STOPPED event in the nrfx_saadc_init() function to prevent driver deadlock in some cases.
- Added HALs: BPROT, MPU, MWU.
- Added function for reading the pin input buffer configuration in the GPIO HAL.
- Implemented workaround for nRF9160 anomaly 1 in the I2S driver.

### Changed
- Improved handling of hardware anomalies in the USBD driver.
- Updated MDK to version 8.23.1.

### Fixed
- Fixed the condition in NRFX_WAIT_FOR in the nrfx_saadc_abort() function. The macro now correctly waits for a stop of the driver.
- Fixed the pending interrupt clearing in NVIC in the nrfx_usbd_stop() function. The driver now correctly handles power management.
- Fixed the case when nrfx_uarte_tx_in_progress() function would return an incorrect value. The driver now correctly updates the tx_buffer_length variable internally.

## [1.5.0] - 2018-12-12
### Added
- Added support for nRF9160.
- Added allocator for DPPI.
- Added HALs: DPPI, KMU, REGULATORS, SPU, VMC.
- Added support for DPPI subscription and publishing in HALs related to nRF9160.
- Added support for instances 2 and 3 in SPIS, TWIM, TWIS, and UARTE drivers.

### Changed
- Updated MDK to version 8.21.1.

### Fixed
- Corrected NRFX_I2S_CONFIG_RATIO value in nrfx_config.h. It now correctly uses supported value.

## [1.4.0] - 2018-11-30
### Added
- Added the nrfx_is_word_aligned() function for checking whether an address is word-aligned.
- Added HAL for ACL.
- Added functions for disabling and re-enabling interrupts in the SWI driver.
- Added possibility to completely remove interrupt handling from the WDT driver.

### Changed
- Updated the documentation for the nrfx_uarte_rx() function. It now correctly reflects the actual behavior of the function.

### Fixed
- Corrected the type of the nrfx_uarte_xfer_evt_t structure field that holds the amount of transferred bytes.
- Corrected the way of disabling interrupts in the NFCT driver when moving the peripheral to the disabled state.
- Fixed a typo in the name of the bmRequest field in the nrfx_usbd_setup_t structure. The new correct name is bRequest.
- Fixed the nrfx_ppi_channel_fork_assign() function. It now accepts also pre-programmed channels.
- Fixed handling of long custom instruction responses in the QSPI driver.
- Fixed a bug affecting the conversion of time to ticks in the TIMER HAL.

## [1.3.1] - 2018-09-28
### Fixed
- Corrected the type of nrfx_usbd_ep_status_get() return value.
- Corrected calls to undefined macros in NFCT and USBD drivers.

## [1.3.0] - 2018-09-21
### Added
- Added HAL and driver for NFCT.
- Added driver for USBD.
- Added function for setting the burst mode in the SAADC HAL.
- Added the NRFX_ARRAY_SIZE macro.

### Changed
- Moved the implementation of nrfx_power_clock_irq_handler() to nrfx_power.c, removed nrfx_power_clock.c.

### Fixed
- Replaced ARRAY_SIZE macro calls with NRFX_ARRAY_SIZE ones.

## [1.2.0] - 2018-09-06
### Added
- Added function for checking if a specific channel is enabled in the GPIOTE HAL.
- Added support for using the QDEC driver without LED.
- Added functions for modifying only the event endpoint or only the task endpoint in the PPI HAL.
- Added function for reading the pin pull configuration in the GPIO HAL.

### Changed
- Corrected ISOSPLIT enumerator names in the USBD HAL.

### Fixed
- Fixed a double buffering bug that occurred in the UARTE driver after the RX abort.
- Fixed the TXRX transfers in the TWIM driver. They can now be started after transfers that are not ended with the stop condition.

## [1.1.0] - 2018-06-15
### Added
- Implemented workaround for nRF52832 and nRF52840 anomaly 194 in the I2S driver.
- Implemented workaround for nRF52840 anomaly 195 in the SPIM driver.
- Added HALs for CCM, ECB, and RADIO.
- Extended HALs for GPIO, PPI, SAADC, and USBD.
- Added support for external LFCLK sources.

### Changed
- Corrected handling of transfer lengths in the TWI driver.
- Updated MDK to version 8.17.0.

### Fixed
- Fixed logging in the PPI driver.
- Fixed SPIM interrupt definition for nRF52810.
- Fixed Slave Select configuration in the SPIM driver.
- Corrected default settings for NRF_SPIM3.
- Fixed a typo in the UARTE TXDRDY event definition.
- Corrected the TIMEOUT event clearing in the WDT interrupt handler.

## [1.0.0] - 2018-03-21
### Added
- Added the NRFX_WAIT_FOR macro to improve the time-out functionality in QSPI and SAADC drivers.
- Added glue layer macros for checking and modifying the pending status of interrupts.
- Added new enumeration values for interrupts and events in the UARTE HAL.
- Implemented workarounds for nRF52 anomalies 192 and 201 in the CLOCK driver.
- Implemented workaround for nRF52840 anomaly 122 in the QSPI driver.
- Implemented workaround for nRF52840 anomaly 198 in the SPIM driver.

### Changed
- Updated MDK to 8.16.0.
- Extended input pin configuration in the GPIOTE driver.
- Unified the way of checking if a required event handler was provided. Now, all drivers do it with assertions.
- Changed the RNG bias correction configuration option to be enabled by default.
- Refactored the ADC driver and HAL.
- Corrected assertions in the TIMER driver to make it usable in debug version with PPI.
- Improved buffer handling in the I2S driver. The API of the driver has been slightly modified.
- Enhanced SPIS driver API: added the "p_context" parameter, allowed NULL pointers for zero-length buffers.

### Fixed
- Fixed result value casting in the TEMP HAL.
- Fixed types of conversion result and buffer size in the ADC HAL and driver.
- Fixed time-out in the SAADC driver in abort function.

## [0.8.0] - 2017-12-20
### Added
- Added XIP support in the QSPI driver.
- Implemented Errata 132 in the CLOCK driver.
- Added function for checking if a TIMER instance is enabled.
- Added extended SPIM support.

### Changed
- Updated MDK to 8.15.0. Introduced Segger Embedded Studio startup files.
- Updated drivers: COMP, PWM, QDEC, SAADC, SPIS, TIMER, TWI, TWIS.
- Changed the type used for transfer lengths to 'size_t' in drivers: SPI, SPIM, SPIS, TWI, TWIM, TWIS, UART, UARTE. Introduced checking of EasyDMA transfers limits.
- Updated HALs: COMP, NVMC, UART, UARTE, USBD.
- Updated template files and documentation of configuration options.

### Fixed
- Fixed TWI and TWIM drivers so that they now support GPIOs from all ports.
- Fixed definitions related to compare channels in the TIMER HAL.

### Removed
- Removed the possibility of passing NULL instead of configuration to get default settings during drivers initialization.
- Removed support for UART1 and PRS box #5.

## [0.7.0] - 2017-10-20
### Added
- This CHANGELOG.md file.
- README.md file with simple description and explanations.
- HAL for: ADC, CLOCK, COMP, ECB, EGU, GPIO, GPIOTE, I2S, LPCOMP, NVMC, PDM, POWER, PPI, PWM, QDEC, QSPI, RNG, RTC, SAADC, SPI, SPIM, SPIS, ARM(R) SysTick, TEMP, TIMER, TWI, TWIM, TWIS, UART, UARTE, USBD, WDT.
- Drivers for: ADC, CLOCK, COMP, GPIOTE, I2S, LPCOMP, PDM, POWER, PWM, QDEC, QSPI, RNG, RTC, SAADC, SPI, SPIM, SPIS, ARM(R) SysTick, TIMER, TWI, TWIM, TWIS, UART, UARTE, WDT.
- Allocators for: PPI, SWI/EGU.
- MDK in version 8.14.0.
- Offline documentation for every added driver and simple integration description.
- Template integration files.
