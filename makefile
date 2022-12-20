all: Receiver Sender
	
server: Receiver.c
	gcc Receiver.c -o Receiver
	
client: Sender.c
	gcc Sender.c -o Sender
	
.PHONY: clean all
	
clean: 
	rm -rf *.o Receiver Sender