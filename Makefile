
all :
	make -f Makefile.cpu
debug-cpu:
	make -f Makefile.cpu DBG=true
debug-gpu:
	make -f Makefile.gpu DBG=true
gpu :
	make -f Makefile.gpu
cpu :
	make -f Makefile.cpu
clean:
	find . -name "*.o" | xargs rm -vf
	find . -name "*.ii" | xargs rm -vf
	find . -name "*.s" | xargs rm -vf
	rm -vf *~ *#
