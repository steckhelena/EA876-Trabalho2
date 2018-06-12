main:
	gcc -othreads threads.c imageprocessing.c -I./ -lfreeimage -lpthread
	gcc -omultiprocessos multiprocesso.c imageprocessing.c -I./ -lfreeimage -lpthread

test: main
	./test.sh

clean:
	rm multiprocessos
	rm threads
