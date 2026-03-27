# Linux：与 CI 一致（有 Ninja 则用 Ninja，否则 Unix Makefiles）

cdb:
	cmake -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
