bank: bank.o string_parser.o
	gcc -pthread bank.o string_parser.o -o bank -lpthread
	
bank.o: bank.c string_parser.h account.h

clean:
	rm -f $(PROGRAMS) *.o
