# macOS：与 README Build 一致，QT 通过 brew 的 qt 传递。

QT_SDK_DIR := $(shell brew --prefix qt 2>/dev/null)

.PHONY: reinstall

cdb:
	cmake -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) \
		$(if $(QT_SDK_DIR),-DQT_SDK_DIR=$(QT_SDK_DIR),) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

reinstall:
	sudo $(MAKE) clean
	cmake -B $(BUILD_DIR) -G $(CMAKE_GENERATOR) \
		$(if $(QT_SDK_DIR),-DQT_SDK_DIR=$(QT_SDK_DIR),) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR)
	cmake --build $(BUILD_DIR) --config Release --parallel
	$(MAKE) install
