#
# Copyright (C) YuqiaoZhang(HanetakaChou)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

HIDE := @

LOCAL_PATH := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
ifeq (true, $(APP_DEBUG))
	BIN_DIR := $(LOCAL_PATH)/bin/debug
	OBJ_DIR := $(LOCAL_PATH)/obj/debug
else
	BIN_DIR := $(LOCAL_PATH)/bin/release
	OBJ_DIR := $(LOCAL_PATH)/obj/release
endif
SOURCE_DIR := $(LOCAL_PATH)/../source
THIRD_PARTY_DIR := $(LOCAL_PATH)/../thirdparty

CC := clang++

C_FLAGS := 
C_FLAGS += -Wall -Werror=return-type
# C_FLAGS += -fvisibility=hidden
C_FLAGS += -fPIE -fPIC
C_FLAGS += -pthread
ifeq (true, $(APP_DEBUG))
	C_FLAGS += -g -O0 -UNDEBUG
else
	C_FLAGS += -O2 -DNDEBUG
endif
C_FLAGS += -Dbrx_init_unknown_device=brx_init_vk_device
C_FLAGS += -Dbrx_destroy_unknown_device=brx_destroy_vk_device
C_FLAGS += -I$(LOCAL_PATH)/../spirv
ifeq (true, $(APP_DEBUG))
	C_FLAGS += -I$(LOCAL_PATH)/../spirv/debug
else
	C_FLAGS += -I$(LOCAL_PATH)/../spirv/release
endif
C_FLAGS += -DPAL_STDCPP_COMPAT=1
C_FLAGS += -I$(THIRD_PARTY_DIR)/CoreRT/src/Native/inc/unix
C_FLAGS += -I$(THIRD_PARTY_DIR)/DirectXMath/Inc

LD_FLAGS := 
LD_FLAGS += -pthread
LD_FLAGS += -Wl,--no-undefined
LD_FLAGS += -Wl,--enable-new-dtags 
LD_FLAGS += -Wl,-rpath,'$$ORIGIN'
ifneq (true, $(APP_DEBUG))
	LD_FLAGS += -s
endif

all :  \
	$(BIN_DIR)/glTF-Viewer

# Link
ifeq (true, $(APP_DEBUG))
$(BIN_DIR)/glTF-Viewer: \
	$(OBJ_DIR)/Demo-support-camera_controller.o \
	$(OBJ_DIR)/Demo-support-main.o \
	$(OBJ_DIR)/Demo-support-renderer.o \
	$(OBJ_DIR)/Demo-support-tick_count.o \
	$(OBJ_DIR)/Demo-demo.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o \
	$(BIN_DIR)/libBRX.so \
	$(BIN_DIR)/libVkLayer_khronos_validation.so \
	$(BIN_DIR)/VkLayer_khronos_validation.json \
	$(LOCAL_PATH)/Demo-Linux.map
else
$(BIN_DIR)/glTF-Viewer: \
	$(OBJ_DIR)/Demo-support-camera_controller.o \
	$(OBJ_DIR)/Demo-support-main.o \
	$(OBJ_DIR)/Demo-support-renderer.o \
	$(OBJ_DIR)/Demo-support-tick_count.o \
	$(OBJ_DIR)/Demo-demo.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o \
	$(BIN_DIR)/libBRX.so \
	$(LOCAL_PATH)/Demo-Linux.map
endif
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) $(CC) -pie $(LD_FLAGS) \
		-Wl,--version-script=$(LOCAL_PATH)/Demo-Linux.map \
		$(OBJ_DIR)/Demo-support-camera_controller.o \
		$(OBJ_DIR)/Demo-support-main.o \
		$(OBJ_DIR)/Demo-support-renderer.o \
		$(OBJ_DIR)/Demo-support-tick_count.o \
		$(OBJ_DIR)/Demo-demo.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o \
		$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o \
		-L$(BIN_DIR) -lBRX \
		-lxcb \
		-o $(BIN_DIR)/glTF-Viewer

# Compile
$(OBJ_DIR)/Demo-support-camera_controller.o: $(SOURCE_DIR)/support/camera_controller.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/support/camera_controller.cpp -MD -MF $(OBJ_DIR)/Demo-support-camera_controller.d -o $(OBJ_DIR)/Demo-support-camera_controller.o

$(OBJ_DIR)/Demo-support-main.o: $(SOURCE_DIR)/support/main.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/support/main.cpp -MD -MF $(OBJ_DIR)/Demo-support-main.d -o $(OBJ_DIR)/Demo-support-main.o

$(OBJ_DIR)/Demo-support-renderer.o: $(SOURCE_DIR)/support/renderer.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/support/renderer.cpp -MD -MF $(OBJ_DIR)/Demo-support-renderer.d -o $(OBJ_DIR)/Demo-support-renderer.o

$(OBJ_DIR)/Demo-support-tick_count.o: $(SOURCE_DIR)/support/tick_count.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/support/tick_count.cpp -MD -MF $(OBJ_DIR)/Demo-support-tick_count.d -o $(OBJ_DIR)/Demo-support-tick_count.o

$(OBJ_DIR)/Demo-demo.o: $(SOURCE_DIR)/demo.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/demo.cpp -MD -MF $(OBJ_DIR)/Demo-demo.d -o $(OBJ_DIR)/Demo-demo.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_asset_input_stream_file.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_asset_input_stream_file.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_asset_input_stream_memory.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_asset_input_stream_memory.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_image_asset_dds.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_image_asset_dds.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_image_asset_pvr.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_image_asset_pvr.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset_gltf_cgltf.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset_gltf_cgltf.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset_gltf.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset_gltf.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o

$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o: $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset.cpp
	$(HIDE) mkdir -p $(OBJ_DIR)
	$(HIDE) $(CC) -c $(C_FLAGS) $(SOURCE_DIR)/../thirdparty/ImportAsset/source/import_scene_asset.cpp -MD -MF $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.d -o $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o

# Copy
ifeq (true, $(APP_DEBUG))
CONFIG_NAME := debug
else
CONFIG_NAME := release
endif
$(BIN_DIR)/libBRX.so: $(THIRD_PARTY_DIR)/Brioche/build-linux/bin/$(CONFIG_NAME)/libBRX.so
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) cp -f $(THIRD_PARTY_DIR)/Brioche/build-linux/bin/$(CONFIG_NAME)/libBRX.so $(BIN_DIR)/libBRX.so


$(BIN_DIR)/libVkLayer_khronos_validation.so: $(THIRD_PARTY_DIR)/Brioche/thirdparty/Vulkan-ValidationLayers/bin/linux/x64/libVkLayer_khronos_validation.so
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) cp -f $(THIRD_PARTY_DIR)/Brioche/thirdparty/Vulkan-ValidationLayers/bin/linux/x64/libVkLayer_khronos_validation.so $(BIN_DIR)/libVkLayer_khronos_validation.so

$(BIN_DIR)/VkLayer_khronos_validation.json: $(THIRD_PARTY_DIR)/Brioche/thirdparty/Vulkan-ValidationLayers/bin/linux/x64/VkLayer_khronos_validation.json
	$(HIDE) mkdir -p $(BIN_DIR)
	$(HIDE) cp -f $(THIRD_PARTY_DIR)/Brioche/thirdparty/Vulkan-ValidationLayers/bin/linux/x64/VkLayer_khronos_validation.json $(BIN_DIR)/VkLayer_khronos_validation.json

-include \
	$(OBJ_DIR)/Demo-support-camera_controller.d \
	$(OBJ_DIR)/Demo-support-main.d \
	$(OBJ_DIR)/Demo-support-renderer.d \
	$(OBJ_DIR)/Demo-support-tick_count.d \
	$(OBJ_DIR)/Demo-demo.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.d \
	$(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.d

clean:
	$(HIDE) rm -f $(BIN_DIR)/glTF-Viewer
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-camera_controller.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-main.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-renderer.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-tick_count.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-demo.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.o
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-camera_controller.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-main.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-renderer.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-support-tick_count.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-demo.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_file.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_asset_input_stream_memory.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_dds.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_image_asset_pvr.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf_cgltf.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset_gltf.d
	$(HIDE) rm -f $(OBJ_DIR)/Demo-thirdparty-ImportAsset-import_scene_asset.d
	$(HIDE) rm -f $(BIN_DIR)/libBRX.so
ifeq (true, $(APP_DEBUG))
	$(HIDE) rm -f $(BIN_DIR)/libVkLayer_khronos_validation.so
	$(HIDE) rm -f $(BIN_DIR)/VkLayer_khronos_validation.json
endif

.PHONY : \
	all \
	clean