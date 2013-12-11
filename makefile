BUILD_CONFIG = Generic 

all: dart dash-lib dart-shmem-base dart-shmem-sysv dash-lib-test dart-shmem-test

dart:
	cd dart; $(MAKE) deploy

dart-shmem-base: dart
	cd dart-shmem/dart-shmem-base/$(BUILD_CONFIG); $(MAKE) ccd

dart-shmem-sysv: dart-shmem-base
	cd dart-shmem/dart-shmem-sysv/$(BUILD_CONFIG); $(MAKE) ccd

dash-lib: dart-shmem-base
	cd dash-lib/$(BUILD_CONFIG); $(MAKE) ccd

dash-lib-test: dash-lib
	cd dash-lib-test/$(BUILD_CONFIG); $(MAKE)

dart-shmem-test: dart-shmem-base
	cd dart-shmem/dart-shmem-test/$(BUILD_CONFIG); $(MAKE)


clean-all:
	-cd dart; $(MAKE) clean-deploy
	-cd dart-shmem/dart-shmem-base/$(BUILD_CONFIG); $(MAKE) clean clean-deploy
	-cd dart-shmem/dart-shmem-sysv/$(BUILD_CONFIG); $(MAKE) clean clean-deploy
	-cd dash-lib/$(BUILD_CONFIG); $(MAKE) clean clean-deploy
	-cd dash-lib-test/$(BUILD_CONFIG); $(MAKE) clean
	-cd dart-shmem/dart-shmem-test/$(BUILD_CONFIG); $(MAKE) clean

.PHONY: dart
