LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
	main.cpp \
	brightness.cpp \
	vnchelper.cpp

LOCAL_CFLAGS  += -Wall -O3

LOCAL_LDLIBS +=  -llog -ldl

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)

LOCAL_MODULE := vnchelper

include $(BUILD_EXECUTABLE)

all: libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so

libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so: $(LOCAL_INSTALLED)
	$(call host-mv, $<, $@)
