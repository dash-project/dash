Project Description:
--------------------
- dart-shmem-base, dart-shmem-sysv: prototype-implementation of dart.h based on systemV-shared-memory, process-shared-pthreads and FIFOs
- dart-shmem-test: includes unit- AND integration-tests.
  CONVENTIONS:
  - each class contains either unit- or integration-tests, but not both
  - integration-test-method-names start with 'integration_test'
  - integration-tests use only declarations contained in dart.h (in the future we might separate unit- and integration tests, such that we can use the integration tests for other dart-implementations (e.g. based on MPI) as well)

BUILD-process:
- each project is an eclispe cdt-project with automatically generated makefiles
- in order to use eclipse for parallel dev.: File->import->existing projects into workspace
- makefile uses env.-variable 'DASH_LIBS' that should refer to a directory containing the generated shared libs. makefile copies generated lib into this directory.
- compiler must be able to resolve includes (e.g. use CPATH with gcc or adjust makefile or eclipse-project)
- adjust LD_LIBRARY_PATH?
