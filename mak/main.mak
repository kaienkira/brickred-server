#==============================================================================
#-*- DEFAULT VALUES -*-
#==============================================================================
CC_ ?= gcc
CXX_ ?= g++
LINKER_ ?= g++
AR_ ?= ar cr
RM_ ?= rm -f
SED_ ?= sed

CFLAGS ?= -Wall -c -std=c11 $(C_FLAG)
CPPFLAGS ?= -Wall -c -std=c++17 $(CPP_FLAG)
ELFLAGS ?= $(EL_FLAG)
DLFLAGS ?= -shared -fPIC
INCLUDES += $(INCLUDE)
LIBS += $(LIB)
DEPFILES += $(DEPFILE)

#==============================================================================
#-*- FUNCTIONS -*-
#==============================================================================
define ECHO
	@printf "\033[;32m%s\033[0m\n" $1
endef

define make_c_rule
$(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .o, $(basename $1)))): $1
	@$(call ECHO, "[compiling $$@ ...]")
	@$(CC_) -o $$@ $$(CFLAGS) $$(INCLUDES) $$<
$(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .d, $(basename $1)))): $1
	@$(CC_) -M $$(CFLAGS) $$(INCLUDES) $$< | \
		$(SED_) '1s,^[^:]*:,$$@ $$@:,' | $(SED_) '1s/\.d/\.o/' >$$@
	@if [ ! -s $$@ ]; then $(RM_) $$@; fi
endef

define make_cc_rule
$(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .o, $(basename $1)))): $1
	@$(call ECHO, "[compiling $$@ ...]")
	@$(CXX_) -o $$@ $$(CPPFLAGS) $$(INCLUDES) $$<
$(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .d, $(basename $1)))): $1
	@$(CXX_) -M $$(CPPFLAGS) $$(INCLUDES) $$< | \
		$(SED_) '1s,^[^:]*:,$$@ $$@:,' | $(SED_) '1s/\.d/\.o/' >$$@
	@if [ ! -s $$@ ]; then $(RM_) $$@; fi
endef

#==============================================================================
#-*- MAIN -*-
#==============================================================================
ifeq ($(LINK_TYPE), exec)
	FINAL_TARGET = $(TARGET)
endif

ifeq ($(LINK_TYPE), dynamic)
	FINAL_TARGET = $(TARGET).so
    CFLAGS += -fPIC
    CPPFLAGS += -fPIC
endif

ifeq ($(LINK_TYPE), static)
    FINAL_TARGET = $(TARGET).a
endif

.PHONY: debug release profile clean

debug: CFLAGS += -g
debug: CPPFLAGS += -g
debug: $(FINAL_TARGET)

release: CFLAGS += -g -O2 -fno-strict-aliasing
release: CPPFLAGS += -g -O2
release: $(FINAL_TARGET)

profile: CFLAGS += -g -pg
profile: CPPFLAGS += -g -pg
profile: DLFLAGS += -pg
profile: ELFLAGS += -pg
profile: $(FINAL_TARGET)

ifeq ($(BUILD_DIR),)
BUILD_DIR = ./
else
BUILD_DIR := $(BUILD_DIR)/
endif

OBJS = $(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .o, $(basename $(SRCS)))))
DEPS = $(addprefix $(BUILD_DIR), $(subst /,_, $(addsuffix .d, $(basename $(SRCS)))))

$(FINAL_TARGET): $(OBJS) $(DEPFILES)
ifeq ($(LINK_TYPE), exec)
	@$(call ECHO, "[linking exec $(TARGET) ...]")
	@$(LINKER_) -o $(FINAL_TARGET) $(ELFLAGS) $(OBJS) $(LIBS)
else ifeq ($(LINK_TYPE), static)
	@$(call ECHO, "[linking static $(FINAL_TARGET) ...]")
	@$(AR_) $(FINAL_TARGET) $(OBJS)
else ifeq ($(LINK_TYPE), dynamic)
	@$(call ECHO, "[linking dynamic $(FINAL_TARGET) ...]")
	@$(LINKER_) -o $(FINAL_TARGET) $(DLFLAGS) $(OBJS) $(LIBS)
endif

$(foreach SRC, $(filter %.c, $(SRCS)), $(eval $(call make_c_rule, $(SRC))))
$(foreach SRC, $(filter %.cc, $(SRCS)), $(eval $(call make_cc_rule, $(SRC))))

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

clean:
	@$(call ECHO, "[cleaning $(FINAL_TARGET)...]")
	@$(RM_) $(OBJS) $(DEPS) $(FINAL_TARGET)
