all:
	find . -name "test.*" -type d -exec make -C  {} \;


clean : 
	rm -f *~
	find . -name "test.*" -type d -exec make -C {} clean \;
