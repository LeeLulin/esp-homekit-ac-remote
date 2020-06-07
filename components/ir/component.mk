INC_DIRS += $(ir_ROOT)

ir_INC_DIR = $(ir_ROOT)
ir_SRC_DIR = $(ir_ROOT)/ir

$(eval $(call component_compile_rules,ir))

EXTRA_CFLAGS += -DINCLUDE_xTimerPendFunctionCall=1

ifeq ($(IR_DEBUG),1)
ir_CFLAGS += -DIR_DEBUG
endif
