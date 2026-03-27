# 未识别平台：与 Linux 相同策略

cdb:
	cmake -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake -E create_symlink $(BUILD_DIR)/compile_commands.json $(COMPILE_COMMANDS)
