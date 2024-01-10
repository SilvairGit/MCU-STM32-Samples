## Required software and tools:
1. Download and install STM32CubeMX 6.6.1 according to your operating system: https://www.st.com/en/development-tools/stm32cubemx.html
2. Download and install STM32CubeIDE 1.10.1 according to your operating system: https://www.st.com/en/development-tools/stm32cubeide.html
3. If you prefer to use JLink(recommended), download and install the newest version of SEGGER J-link drivers: https://www.segger.com/products/debug-probes/j-link/technology/flash-download/
4. If you prefer to use ST-Link, download and install the newest version of STM32CubeProg https://www.st.com/en/development-tools/stm32cubeprog.html

## In-circuit debugger and programmer 
- You can use JLink as a flasher and project in-circuit debugger and programmer 
- You can use an ST-Link as a flasher and project debugger but we recommend converting ST-Link into J-Link by following instructions: https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/

## Import project to IDE, build and debug
1. Clone this repo to the local machine
2. In STM32CubeIDE import project: 
    - `File/Import/General/Existing Projects into Workspace/Select root directory/Browse...`
    - Select the directory `.../STM32Samples`
    - Do not select: `Copy project into workspace`
    - Press: `Finish`
3. Build project: `Project/Build All` or use `CTRL+B`
4. Look at the `Console` tab if the build is successfully build
5. Debug project: 
    - Make sure that you have connected your debugger to the PC and to the target
    - Go to: `Run/Debug configurations...`
    - Double click on `STM32 C/C++ Application`
    - Go to newly created configuration and select: `Debugger/Debug probe/SEGGER J-LINK` or `ST-LINK (ST-LINK GDB server)` depending on used debugger
    - Select: `Interface/SWD`
    - Select: `Interface/Initial Speed` select max
    - Go to `Main` tab, click `Seatch Project...` and select the `*.elf` target which will be debugg
    - Press apply and debug
    - You should be able to step your code instructions
6. In the `Build Target` tab are the 5 make instructions that can be used during development. Double click to command to execute:
    - `all` - build all project targets
    - `clean` - clean entire project including unit tests
    - `client` - build client project target
    - `erase_jlink` - erase entire MCU flash by using JLink tools
    - `flash_client_jlink` - flash MCU by using JLink tools and client target
    - `flash_server_jlink` - flash MCU by using JLink tools and server target
    - `reset_jlink` - reset MCU by using JLink tools
    - `erase_stlink` - erase entire MCU flash by using ST-Link tools
    - `flash_client_stlink` - flash MCU by using ST-Link tools and client target
    - `flash_server_stlink` - flash MCU by using ST-Link tools and server target
    - `reset_stlink` - reset MCU by using ST-Link tools
    - `test` - run unit test

## Eclipse addons - not required for development
1. If you would like to have an addon to edit README.md install: https://marketplace.eclipse.org/content/markdown-text-editor
2. If you would like to have an addon to git in eclipse follow the instructions: https://www.geeksforgeeks.org/how-to-add-git-credentials-in-eclipse/

## Build from the console
The source code can be built from the console level by using the command: 
- `make` - build the entire project 
- `make all` - build all project targets
- `make clean` - clean entire project including unit tests
- `make client` - build client project target
- `make erase_jlink` - erase entire MCU flash by using JLink tools
- `make flash_client_jlink` - flash MCU by using JLink tools and client target
- `make flash_server_jlink` - flash MCU by using JLink tools and server target
- `make reset_jlink` - reset MCU by using JLink tools
- `make erase_stlink` - erase entire MCU flash by using ST-Link tools
- `make flash_client_stlink` - flash MCU by using ST-Link tools and client target
- `make flash_server_stlink` - flash MCU by using ST-Link tools and server target
- `make reset_stlink` - reset MCU by using ST-Link tools
- `make test` - run unit test

## Potential problems
1. If a toolchain is not found in Eclipse, add a toolchain to the `PATCH`
    - Go to: `Project/Properties/C/C++Build/Environment/`
    - Select `PATCH` then `Edit` and add a patch to your toolchain 
2. If a ST-Link programer is not found in Eclipse, add a ST-Link to the `PATCH`
    - Go to: `Project/Properties/C/C++Build/Environment/`
    - Select `PATCH` then `Edit` and add a patch to your ST-Link `bin` directory location 
3. If a ST-Link programer is not found when calling makefile
    - Add a patch to your ST-Link `bin` directory location to the PATCH  
    - Open: `vi ~/.profile` 
    - Add a line to the file(example directory): `export PATH="/home/user/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin:$PATH"` 
    - Log out and log in again

## Project directory description:
    .
    ├── build                         // build output directory - automatically generated
    ├── common                        // common files directory
    ├── drivers                       // peripheral drivers
    ├── external                      // external source
    │  ├── CMSIS                      // CMSIS source
    │  └── STM32F1xx_HAL_Driver       // STM32 HAL and LL dreiwers source
    ├── hal                           // Hardware Abstraction layer
    ├── features                      // SW features files directory
    ├── gen_pkgs.sh                   // CI package generation script
    ├── LICENSE                       // License file
    ├── Makefile                      // Makefile
    ├── README.md                     // Readme file
    ├── src                           // Main source directory
    │   └── main.c                    // Main file
    ├── stm32f1xx                     // STM32F1 specific file
    │   ├── erase.jlink               // J-Link erase script
    │   ├── flash.jlink               // J-Link flash script
    │   ├── reset.jlink               // J-Link reset script
    │   ├── startup_stm32f103xb.s     // STM32F1 startup file
    │   ├── stm32_assert.h            // STM32F1 HAL asserts file
    │   ├── STM32F103CBUx_FLASH.ld    // STM32F1 linker script
    │   ├── stm32f1xx_conf.h          // STM32F1 LL heder
    │   ├── stm32f1xx_it.c            // STM32F1 system IRQ implementation
    │   ├── stm32f1xx_it.h            // STM32F1 system IRQ implementation
    │   └── system_stm32f1xx.c        // STM32F1 MCU initialization implementation
    └── test                          // Unit tests directory
        └── build                     // Unit tests build output directory - automatically generated
        └── test                      // Test files
        └── config.yml                // Unity/Cmock configuration file
        └── Makefile                  // Unit test Makefile

## Porting guide
To port this source code to another platform you must port the entire `hal` directory. 
The hardware abstraction layer implements firmware interaction with platform-specific peripherals and features.
Rest of the code in directories: `drivers` and `src` is platform independent.
Directories `external` and `stm32f1xx` are platform specific and should not be ported.

## Flags
1. To enable or disable logs use `LOG_ENABLE` in file `Log.h`. There are three types of logs that can be selectively enabled or disabled:
    - Log Error enable by `LOG_ERROR_ENABLE` flag in file `Log.h` - this log type inform about a critical code failures
    - Log Warnings enable by `LOG_WARNING_ENABLE` flag in file `Log.h` - this log type inform about warning situations
    - Log Debug enable by `LOG_DEBUG_ENABLE` flag in file `Log.h` - this log is used for information and debug purpose
2. To enable or disable asserts use `ASSERT_ENABLE` in file `Assert.h`. Disabling assert allows saving Flash memory. It is recommended to keep this flag enabled.
3. To enable or disable logs for all transmitted and received UART frames use `UART_FRAME_LOGGER_ENABLE` in file `UartFrame.h`. Disabling this flag allows saving Flash memory. Enabling this flag can cause a lot of traffic on the logger. It is recommended to use this flag only for debugging purposes.
3. `MCU_CLIENT` and `MCU_SERVER` are flags injected by a makefile during compilation. These flags are defined depending on a selected type of project to build.
