#ifndef CONFIG_H
#define CONFIG_H

#ifndef MCU_SERVER
#define MCU_SERVER 0
#endif

#ifndef MCU_CLIENT
#define MCU_CLIENT 0
#endif

#if MCU_SERVER == 1 && MCU_CLIENT == 1
#error "MCU_SERVER and MCU_CLIENT cannot be enabled at the same time"
#endif

#if MCU_SERVER == 1
#define ENABLE_CLIENT 0     /**< Enable Client support */
#define ENABLE_LC 1         /**< Enable LC support */
#define ENABLE_CTL 0        /**< Enable CTL support */
#define ENABLE_PIRALS 1     /**< Enable PIR and ALS support */
#define ENABLE_ENERGY 1     /**< Enable energy monitoring support */
#define ENABLE_EMG_L_TEST 1 /**< Enable Emergency Lighting Testing support */

#define DFU_VALIDATION_STRING "MCU_Srv" /**< Defines string to be expected in app data */
#endif

#if MCU_CLIENT == 1
#define ENABLE_CLIENT 1     /**< Enable Client support */
#define ENABLE_LC 0         /**< Enable LC support */
#define ENABLE_CTL 0        /**< Enable CTL support */
#define ENABLE_PIRALS 0     /**< Enable PIR and ALS support */
#define ENABLE_ENERGY 0     /**< Enable energy monitoring support */
#define ENABLE_EMG_L_TEST 0 /**< Enable Emergency Lighting Testing support */

#define DFU_VALIDATION_STRING "MCU_Cli" /**< Defines string to be expected in app data */
#endif

#if ENABLE_CTL == 1 && ENABLE_LC == 1
#error "Features CTL and LC cannot be enabled at the same time"
#endif

#ifndef BUILD_NUMBER
#define BUILD_NUMBER "x.x.x" /**< Defines firmware build number. */
#endif

#endif
