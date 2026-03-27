# BookCanvas — GNU Make 入口：按平台加载 make/<Platform>.mk
#
# 提供: clean, cdb, install, uninstall — 见 README「Install」与仓库内注释
# 需安装: cmake、对应平台工具链（见 README）

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
  PLATFORM := Darwin
else ifeq ($(UNAME_S),Linux)
  PLATFORM := Linux
else ifneq (,$(findstring MINGW,$(UNAME_S)))
  PLATFORM := Windows
else ifneq (,$(findstring MSYS,$(UNAME_S)))
  PLATFORM := Windows
else ifdef OS
  ifeq ($(OS),Windows_NT)
    PLATFORM := Windows
  endif
endif

ifndef PLATFORM
  PLATFORM := Generic
endif

include make/common.mk
include make/$(PLATFORM).mk
