ifdef component_compile_rules
    # ESP_OPEN_RTOS
    INC_DIRS += $(cJSON_ROOT)/cJSON

    cJSON_INC_DIR = $(cJSON_ROOT)/cJSON
    cJSON_SRC_DIR = $(cJSON_ROOT)/cJSON

    $(eval $(call component_compile_rules,cJSON))

else
    # ESP_IDF
    COMPONENT_SRCDIRS = cJSON
    COMPONENT_ADD_INCLUDEDIRS = cJSON

endif
