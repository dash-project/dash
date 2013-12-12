include config.defs

all: dart dash-lib dash-lib-test dart-impl

dart:
	cd dart-if/$(DART_IF_VERSION); $(MAKE) deploy

dart-impl: dart
	-cd dart-impl/$(DART_IMPL); $(MAKE) ccd BUILD_CONFIG=$(BUILD_CONFIG)

dash-lib: dart-impl
	cd dash/dash-lib/$(BUILD_CONFIG); $(MAKE) ccd

dash-lib-test: dash-lib
	cd dash/dash-lib-test/$(BUILD_CONFIG); $(MAKE)

clean-all:
	-cd dart-if/$(DART_IF_VERSION); $(MAKE) clean-deploy
	-cd dash/dash-lib/$(BUILD_CONFIG); $(MAKE) clean clean-deploy
	-cd dash/dash-lib-test/$(BUILD_CONFIG); $(MAKE) clean
	-cd dart-impl/$(DART_IMPL); $(MAKE) clean-all BUILD_CONFIG=$(BUILD_CONFIG)

.PHONY: dart

