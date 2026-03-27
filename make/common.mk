BUILD_DIR        := build
BOOKSIM2_SRC     := 3rdpart/booksim2/src
# make clean 时删除的本地可选前缀目录（仅在 cmake 中手动 -DCMAKE_INSTALL_PREFIX 指向此处时使用）
INSTALL_DIR      := $(abspath $(CURDIR)/Install)

NINJA_BIN := $(shell command -v ninja 2>/dev/null)
ifeq ($(NINJA_BIN),)
  CMAKE_GENERATOR := Unix Makefiles
else
  CMAKE_GENERATOR := Ninja
endif

CMAKE_INSTALL_CONFIG :=

CLANG_FORMAT ?= clang-format
FORMAT_DIRS    := src

.PHONY: clean clean-booksim cdb format install uninstall

format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "未找到 $(CLANG_FORMAT)，请先安装 LLVM/clang-format"; exit 1; }
	@find $(FORMAT_DIRS) -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.c' -o -name '*.cc' -o -name '*.cxx' \) -exec $(CLANG_FORMAT) -i {} +

clean: clean-booksim
	rm -rf $(BUILD_DIR)
	rm -rf $(INSTALL_DIR)

clean-booksim:
	$(MAKE) -C $(BOOKSIM2_SRC) clean

# 使用 $(BUILD_DIR)/CMakeCache.txt 中的 CMAKE_INSTALL_PREFIX，不写 --prefix
install:
	@test -f $(BUILD_DIR)/cmake_install.cmake || { echo "请先配置并编译工程（存在 $(BUILD_DIR)/cmake_install.cmake）"; exit 1; }
	cmake --install $(BUILD_DIR) $(CMAKE_INSTALL_CONFIG)

# 按 CMake 安装清单删除「上一次 cmake --install」写入的路径（如 /usr/local 等系统前缀）
# 需要仍保留 $(BUILD_DIR)；若已 make clean，则清单丢失，无法再用本目标卸载
uninstall:
	@found=0; \
	for m in `find $(BUILD_DIR) -maxdepth 1 -name 'install_manifest*.txt' 2>/dev/null | sort`; do \
		[ -f "$$m" ] || continue; \
		found=1; \
		echo "卸载清单: $$m"; \
		while IFS= read -r f || [ -n "$$f" ]; do \
			[ -z "$$f" ] && continue; \
			if [ -e "$$f" ] || [ -L "$$f" ]; then \
				echo "  rm -rf $$f"; \
				rm -rf -- "$$f"; \
			fi; \
		done < "$$m"; \
		rm -f -- "$$m"; \
	done; \
	if [ "$$found" -eq 0 ]; then \
		echo "未找到 $(BUILD_DIR)/install_manifest*.txt。请先执行 cmake --install（或保留上次的 build 目录）。"; \
		exit 1; \
	fi
