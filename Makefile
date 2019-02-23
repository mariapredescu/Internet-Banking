LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SRV=server
SRV=server
CLT=client

all: $(SRV) $(CLT)

$(SRV):$(SRV).c
	gcc -o $(SRV) $(LIBSOCKET) $(SRV).c

$(CLT):	$(CLT).c
	gcc -o $(CLT) $(LIBSOCKET) $(CLT).c

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)


