## Required software and tools:
1. To be able to run the test, UT framework is required: https://github.com/ThrowTheSwitch/Unity
2. Additional tool for mock generator is required: https://github.com/ThrowTheSwitch/CMock 
3. Update the path for UT framework in `Makefile` by modifying the parameter `FRAMEWORK`
4. Now UT can be run by command: `make test`

## Run from the console
The source code can be built from the console level by using the command: 
- `make clean` - clean entire project
- `make test` - run unit test

## Project directory description:
    .
    ├── build              // Build directory created after test run
    │  ├── ...             // Build artifacts directory and files
    │  ├── log             // Log directory contain log from indiwidual logs
    │  ├── *.elf           // *.elf files in this location are exuecutable files of each UT
    ├── common             // Features UT directory
    │  ├── ...             // UT files
    ├── config.yml         // Unity configuration file
    ├── features           // Features UT directory
    │  └── ...             // UT files
    ├── Makefile           // Makefile
    └── README.md          // Readme file
    