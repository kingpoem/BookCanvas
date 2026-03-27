# Windows（MSYS2/Git Bash 等 GNU Make 环境）：与 README 中 VS 2022 一致

CMAKE_INSTALL_CONFIG := --config Release

cdb:
	cmake -B $(BUILD_DIR) -G "Visual Studio 17 2022" -A x64 \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake -E copy_if_different $(BUILD_DIR)/compile_commands.json $(COMPILE_COMMANDS)
