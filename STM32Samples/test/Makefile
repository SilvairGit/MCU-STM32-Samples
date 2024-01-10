# Shell tools and definitions
CLEANUP = rm -rf
MKDIR = mkdir -p
CP = cp
MV = mv
CC = gcc
GREP = grep

RUNNER_GEN_CONFIG = config.yml

# Target definition - depends on platform
ifeq ($(OS),Windows_NT)
	TARGET_EXTENSION = .exe
else
	TARGET_EXTENSION = .elf
endif

# Phony
.PHONY: clean

# Unity patch
FRAMEWORK = ../../../unittest_framework
UNITY_SRC_DIR = $(FRAMEWORK)/unity/src
CMOCK_LIB_DIR = $(FRAMEWORK)/cmock/lib
CMOCK_SRC_DIR = $(FRAMEWORK)/cmock/src

# List of all src directories - do not add here external sources, hal and main.c file
SOURCE_CODE_DIR  = common
SOURCE_CODE_DIR += features
SOURCE_CODE_DIR += simulators

SOURCE_HAL_DIR = ../hal

# Include directories
C_INCLUDES  = -I$(UNITY_SRC_DIR) 
C_INCLUDES += -I$(BUILD_MOCK_DIR) 
C_INCLUDES += -I$(CMOCK_SRC_DIR)
C_INCLUDES += -I../common
C_INCLUDES += -I../src
C_INCLUDES += -I../features
C_INCLUDES += -I../hal
C_INCLUDES += -I../stm32f1xx
C_INCLUDES += $(addprefix -I../,$(SOURCE_CODE_DIR))

# Compilation flags - the same as in STM project
CFLAGS = -std=c99
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wextra
CFLAGS += -Wswitch-default
CFLAGS += -fdata-sections 
CFLAGS += -ffunction-sections 
CFLAGS += -fstack-usage
CFLAGS += -Wdouble-promotion
CFLAGS += -Wfloat-conversion
CFLAGS += -Wno-discarded-qualifiers
CFLAGS += -g3

# Compilation flags - extra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
CFLAGS += -fno-strict-aliasing
CFLAGS += -fno-builtin 
CFLAGS += -fshort-enums 

# Libraries
LIBS = -lm 

# Linker flags
LDFLAGS = 

# Compilation symbols
SYMBOLS  = -DUNITY_INCLUDE_FLOAT
SYMBOLS += -DUNITY_FLOAT_VERBOSE 
SYMBOLS += -DUNITY_USE_COMMAND_LINE_ARGS
SYMBOLS += -DCMAKE_UNIT_TEST
SYMBOLS += -DUNITY_FLOAT_PRECISION
SYMBOLS += -DMCU_SERVER=1


# Variable definitions
BUILD_DIR = build
BUILD_RUNNER_DIR = $(BUILD_DIR)/runner
BUILD_LOG_DIR = $(BUILD_DIR)/log
BUILD_MOCK_DIR = $(BUILD_DIR)/mocks
# List off test catalog structure
BUILD_TEST_SRC_DIR = $(addprefix $(BUILD_DIR)/,$(SOURCE_CODE_DIR))

# List of all source code directories
SRC_DIR = $(addprefix ../,$(SOURCE_CODE_DIR))
SRC_DIR_C_FILES = $(wildcard $(addsuffix /*.c,$(SRC_DIR)))

# List of all source code headers
HEADER_TO_MOCK = $(wildcard $(addsuffix /*.h,$(SRC_DIR) $(SOURCE_HAL_DIR)))
HAL_TO_MOCK = $(wildcard $(addsuffix /*.h,$(SOURCE_HAL_DIR)))

# List of all test c files
TEST_SRC_C_FILES = $(wildcard $(addsuffix /*.c,$(SOURCE_CODE_DIR)))

# Runner source files and output files lists
RUNNER_SRC = $(TEST_SRC_C_FILES)
RUNNER_FILES = $(basename $(notdir $(TEST_SRC_C_FILES)))
RUNNER_OUT = $(addsuffix _Runner.c,$(addprefix $(BUILD_RUNNER_DIR)/,$(RUNNER_FILES)))

# Mock source file list
MOCK_SRC = $(addsuffix .c,$(addprefix $(BUILD_MOCK_DIR)/Mock,$(basename $(notdir $(HEADER_TO_MOCK)))))
MOCK_HAL_OBJECT = $(addsuffix .o,$(addprefix $(BUILD_MOCK_DIR)/Mock,$(basename $(notdir $(HAL_TO_MOCK)))))

# Test logs result output file list
LOG_RESULTS_FILES = $(addsuffix .txt,$(addprefix $(BUILD_LOG_DIR)/,$(RUNNER_FILES)))

# Test targets output files
TEST_TARGET_FILES = $(addsuffix $(TARGET_EXTENSION),$(addprefix $(BUILD_DIR)/,$(RUNNER_FILES)))

# List of all *.c files to compile
C_SOURCES  = $(TEST_SRC_C_FILES)
C_SOURCES += $(SRC_DIR_C_FILES)
C_SOURCES += $(wildcard $(UNITY_SRC_DIR)/*.c)
C_SOURCES += $(wildcard $(CMOCK_SRC_DIR)/*.c)
C_SOURCES += $(MOCK_SRC)
C_SOURCES += $(RUNNER_OUT)

# List of all *.o files
OBJECTS_SRC  = $(addprefix $(BUILD_DIR)/,$(TEST_SRC_C_FILES)) 
OBJECTS_SRC += $(subst ../,$(BUILD_DIR)/,$(SRC_DIR_C_FILES))
OBJECTS_SRC += $(addprefix $(BUILD_DIR)/,$(notdir $(wildcard $(UNITY_SRC_DIR)/*.c)))
OBJECTS_SRC += $(addprefix $(BUILD_DIR)/,$(notdir $(wildcard $(CMOCK_SRC_DIR)/*.c)))
OBJECTS_SRC += $(MOCK_SRC)
OBJECTS_SRC += $(RUNNER_OUT)

OBJECTS = $(OBJECTS_SRC:.c=.o)

FILTER_OUT = $(foreach v,$(2),$(if $(findstring $(1),$(v)),,$(v)))

OBJECTS_NO_RUNNER = $(filter-out %_Runner.o,$(OBJECTS))
OBJECTS_RUNNER = $(filter %_Runner.o,$(OBJECTS))
OBJECTS_NO_RUNNER_NO_TEST = $(call FILTER_OUT,/Test,$(OBJECTS))
OBJECTS_NO_RUNNER_NO_TEST_NO_MOCK = $(call FILTER_OUT,/Mock,$(OBJECTS_NO_RUNNER_NO_TEST))

UNIQ = $(if $1,$1 $(call uniq,$(filter-out $1,$1)))

# Clean target
clean: 
	$(CLEANUP) $(BUILD_DIR)

# Create all necessery directories
makedir:
	@$(MKDIR) $(BUILD_DIR) $(BUILD_RUNNER_DIR) $(BUILD_LOG_DIR) $(BUILD_MOCK_DIR) $(BUILD_TEST_SRC_DIR) 
	
# Print UT results
test: $(LOG_RESULTS_FILES)
	@echo --------------------------
	@echo --------UT Results--------
	@echo --------------------------

	@ruby $(UNITY_SRC_DIR)/../auto/unity_test_summary.rb $(BUILD_LOG_DIR) 
	@test ! -f $(BUILD_LOG_DIR)/*.testfail || exit 1

# Combine UT result
$(BUILD_LOG_DIR)/%.txt: $(TEST_TARGET_FILES)
	$(CLEANUP) $@.txt $@.testpass $@.testfail
	@./$(patsubst $(BUILD_LOG_DIR)/Test%.txt,$(BUILD_DIR)/Test%$(TARGET_EXTENSION),$@ ) > $@.testpass 2>&1 || ($(CP) $@.testpass $@ && $(MV) $@.testpass $@.testfail)

$(TEST_TARGET_FILES): $(MOCK_SRC) $(RUNNER_OUT) $(OBJECTS) 
	$(eval MOCK_LIST := $(shell echo $(OBJECTS_NO_RUNNER_NO_TEST_NO_MOCK) | $(GREP) "include" $(filter %$(basename $(notdir $@)).c,$(C_SOURCES)) | $(GREP) -o "Mock.*h" || true))	
	$(eval MOCK_LIST := $(addprefix $(BUILD_MOCK_DIR)/,$(MOCK_LIST:.h=.o)))
	$(eval MOCK_REMOVE_LIST := $(notdir $(subst Mock,,$(MOCK_LIST))))
	$(eval MOCK_TO_LINK  := $(filter-out $(foreach v,$(MOCK_REMOVE_LIST),$(filter %/$(v),$(OBJECTS_NO_RUNNER_NO_TEST_NO_MOCK))),$(OBJECTS_NO_RUNNER_NO_TEST_NO_MOCK)))
	$(eval C_INCLUDE_LIST := $(shell echo $(OBJECTS_NO_RUNNER_NO_TEST_NO_MOCK) | $(GREP) "include" $(filter %$(basename $(notdir $@)).c,$(C_SOURCES)) | $(GREP) -o "[a-zA-Z]*\.c" || true))
	$(eval C_INCLUDE_LIST_O := $(C_INCLUDE_LIST:.c=.o))
	$(eval FILES_TO_LINK := $(filter-out $(foreach v,$(C_INCLUDE_LIST_O),$(filter %/$(v),$(MOCK_TO_LINK))),$(MOCK_TO_LINK)))
	$(eval MOCK_LIST += $(MOCK_HAL_OBJECT))	
	$(eval MOCK_LIST := $(sort $(MOCK_LIST)))		
	$(eval MOCK_LIST := $(call UNIQ,$(MOCK_LIST)))	
	
	$(CC) $(BUILD_RUNNER_DIR)/$(notdir $(basename $@))_Runner.o $(filter %$(notdir $(basename $@)).o,$(OBJECTS)) $(MOCK_LIST) $(FILES_TO_LINK) $(LDFLAGS) $(LIBS) $(C_INCLUDES) $(SYMBOLS) -o $@

# Generate Mock of all header files - avoid rebuilding each execution
$(BUILD_MOCK_DIR)/%.c: $(BUILD_DIR)/gen_mocks.flag
	@:
	
# Generate Mock of all header files
$(BUILD_DIR)/gen_mocks.flag: $(HEADER_TO_MOCK)
	@$(MAKE) makedir	
	echo flag > $(BUILD_DIR)/gen_mocks.flag
	ruby $(CMOCK_LIB_DIR)/cmock.rb -o$(RUNNER_GEN_CONFIG) $(HEADER_TO_MOCK)	
	
# Compile entire source code
$(BUILD_DIR)/%.o: $(BUILD_DIR)/gen_mocks.flag $(C_SOURCES)
	@$(MAKE) makedir
	$(CC) -c $(CFLAGS) $(C_INCLUDES) $(SYMBOLS) $(filter %/$(subst .o,.c,$(notdir $@)),$(C_SOURCES)) -o $@

# Generate runners
$(BUILD_RUNNER_DIR)/%_Runner.c: $(TEST_SRC_C_FILES)
	@$(MAKE) makedir
	ruby $(UNITY_SRC_DIR)/../auto/generate_test_runner.rb $(RUNNER_GEN_CONFIG) $(filter %$(notdir $(subst _Runner.c,.c, $@)),$(RUNNER_SRC)) $@
