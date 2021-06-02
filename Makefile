scheduler: scheduler.c
	gcc -pthread -g -o scheduler scheduler.c -lm

clean:
	rm -fr scheduler *dSYM
