# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
#
LOCAL_PATH := $(call my-dir)

TARGET_ARCH_ABI := armeabi-v7a
include $(CLEAR_VARS)

LOCAL_MODULE            := curl_static
LOCAL_SRC_FILES         := curl/libcurl.a
LOCAL_EXPORT_C_INCLUDES := curl/include


include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := hook

include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog
LOCAL_MODULE    := vrapihook
LOCAL_SRC_FILES := main.cpp fakes.cpp ../shared/utils/utils.c ../shared/inline-hook/inlineHook.c ../shared/inline-hook/relocate.c
LOCAL_STATIC_LIBRARIES += curl_static
include $(BUILD_SHARED_LIBRARY)
