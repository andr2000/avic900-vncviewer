LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
	main.cpp \
	brightness.cpp

LOCAL_CFLAGS  += -Wall -O3

LOCAL_LDLIBS +=  -llog -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)

LOCAL_MODULE := brightness

include $(BUILD_EXECUTABLE)

$(LOCAL_BUILT_MODULE), $(LOCAL_SHARED_LIBRARIES): post_build
define gen_post_build
post_build:
	mv $(2) $(1)/lib$(LOCAL_MODULE).so
endef

$(eval $(call gen_post_build, $(dir $(LOCAL_SHARED_LIBRARIES)), $(LOCAL_BUILT_MODULE)))
