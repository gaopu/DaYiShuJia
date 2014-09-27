client:client.o func.o readline.o readline.h client.h
	gcc client.o func.o readline.o -o client -lreadline
	make clean
client.o:
func.o:
readline.o:
.PHONY:clean
clean:
	rm *.o -rf
