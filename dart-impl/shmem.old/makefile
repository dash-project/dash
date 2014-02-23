ccd: dart-shmem-test

dart-shmem-base: 
	cd dart-shmem-base/$(BUILD_CONFIG); $(MAKE) ccd

dart-shmem-sysv: dart-shmem-base
	cd dart-shmem-sysv/$(BUILD_CONFIG); $(MAKE) ccd

dart-shmem-test: dart-shmem-sysv dart-shmem-base
	cd dart-shmem-test/$(BUILD_CONFIG); $(MAKE)

clean-all:
	-cd dart-shmem-test/$(BUILD_CONFIG); $(MAKE) clean
	-cd dart-shmem-base/$(BUILD_CONFIG); $(MAKE) clean clean-deploy
	-cd dart-shmem-sysv/$(BUILD_CONFIG); $(MAKE) clean clean-deploy

.PHONY: dart-shmem-base

