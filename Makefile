all:src dest bin

src:src.c crypt.o
	gcc src.c crypt.o -o src -lssl -lcrypto
dest:dest.c crypt.o
	gcc dest.c crypt.o -o dest -lssl -lcrypto -pthread
crypt.o:crypt.c
	gcc -c crypt.c -o crypt.o
	
bin:app
	objcopy -O binary app bin
app:app.o
	ld -Ttext 0x90000000 app.o -o app
app.o:app.c
	gcc -c app.c -o app.o
	
clean:
	rm -f src dest bin crypt.o app app.o
