all: buildall

buildall:
	@echo "***** Invoking dart-mpi/makefile"
	(cd dart-mpi; $(MAKE))
	@echo "***** Invoking test/makefile"
	(cd test; $(MAKE))


cleanall: 
	-rm -f app
	@echo "****** Invoking dart-mpi/makefile"
	@(cd dart-mpi; $(MAKE) cleanall)
	@echo "****** Invoking test/makefile"
	@(cd test; $(MAKE) cleanall)
