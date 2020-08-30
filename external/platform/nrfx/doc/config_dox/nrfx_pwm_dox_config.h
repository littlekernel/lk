/**
 *
 * @defgroup nrfx_pwm_config PWM peripheral driver configuration
 * @{
 * @ingroup nrfx_pwm
 */
/** @brief 
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_ENABLED

/** @brief Enable PWM0 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM0_ENABLED

/** @brief Enable PWM1 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM1_ENABLED

/** @brief Enable PWM2 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM2_ENABLED

/** @brief Enable PWM3 instance
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM3_ENABLED

/** @brief Interrupt priority
 *
 *  Following options are available:
 * - 0 - 0 (highest)
 * - 1 - 1
 * - 2 - 2
 * - 3 - 3
 * - 4 - 4
 * - 5 - 5
 * - 6 - 6
 * - 7 - 7
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY

/** @brief Enables logging in the module.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_CONFIG_LOG_ENABLED

/** @brief Default Severity level
 *
 *  Following options are available:
 * - 0 - Off
 * - 1 - Error
 * - 2 - Warning
 * - 3 - Info
 * - 4 - Debug
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_CONFIG_LOG_LEVEL

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_CONFIG_INFO_COLOR

/** @brief ANSI escape code prefix.
 *
 *  Following options are available:
 * - 0 - Default
 * - 1 - Black
 * - 2 - Red
 * - 3 - Green
 * - 4 - Yellow
 * - 5 - Blue
 * - 6 - Magenta
 * - 7 - Cyan
 * - 8 - White
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_CONFIG_DEBUG_COLOR

/** @brief Enables nRF52 Anomaly 109 workaround for PWM.
 *
 * The workaround uses interrupts to wake up the CPU and ensure
 * it is active when PWM is about to start a DMA transfer. For
 * initial transfer, done when a playback is started via PPI,
 * a specific EGU instance is used to generate the interrupt.
 * During the playback, the PWM interrupt triggered on SEQEND
 * event of a preceding sequence is used to protect the transfer
 * done for the next sequence to be played.
 *
 *  Set to 1 to activate.
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED

/** @brief EGU instance used by the nRF52 Anomaly 109 workaround for PWM.
 *
 *  Following options are available:
 * - 0 - EGU0
 * - 1 - EGU1
 * - 2 - EGU2
 * - 3 - EGU3
 * - 4 - EGU4
 * - 5 - EGU5
 *
 * @note This is an NRF_CONFIG macro.
 */
#define NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE



/** @} */
