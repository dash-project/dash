all: buildall

buildall:
	@echo "***** Invoking dart-mpi/makefile"
	(cd dart-mpi; $(MAKE))
	@echo "***** Invoking dart/makefile"
	(cd dart; $(MAKE))
	@echo "***** Invoking test/makefile"
	(cd test; $(MAKE))


cleanall: 
	-rm -f app
	@echo "****** Invoking dart-mpi/makefile"
	@(cd dart-mpi; $(MAKE) cleanall)
	@echo "****** Invoking dart/makefile"
	@(cd dart; $(MAKE) cleanall)
	@echo "****** Invoking test/makefile"
	@(cd test; $(MAKE) cleanall)
