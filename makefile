
all: dart dash-lib dart-shmem-base dart-shmem-sysv dash-lib-test dart-shmem-test

dart:
	cd dart; $(MAKE)

dart-shmem-base: dart
	cd dart-shmem/dart-shmem-base/Debug; $(MAKE)

dart-shmem-sysv: dart-shmem-base
	cd dart-shmem/dart-shmem-sysv/Debug; $(MAKE)

dash-lib: dart-shmem-base
	cd dash-lib/Debug; $(MAKE)

dash-lib-test: dash-lib
	cd dash-lib-test/Debug; $(MAKE)

dart-shmem-test: dart-shmem-base
	cd dart-shmem/dart-shmem-test/Debug; $(MAKE)

clean:
	-cd dart-shmem/dart-shmem-base/Debug; $(MAKE) clean
	-cd dart-shmem/dart-shmem-sysv/Debug; $(MAKE) clean
	-cd dash-lib/Debug; $(MAKE) clean
	-cd dash-lib-test/Debug; $(MAKE) clean
	-cd dart-shmem/dart-shmem-test/Debug; $(MAKE) clean

.PHONY: all dart dash-lib dart-shmem-base dart-shmem-sysv dash-lib-test dart-shmem-test clean
