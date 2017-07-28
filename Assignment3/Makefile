
sim :  sim.o pagetable.o swap.o rand.o clock.o lru.o fifo.o opt.o
	gcc -Wall -g -o sim $^

%.o : %.c pagetable.h sim.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o sim *~
