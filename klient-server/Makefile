SERVER          = server
CLIENT          = client
SOURCE          = ipk.cpp
TARGET          = SERV

CC = g++49
STD = -std=c++11
MAKRO = -D
MAKRO2 =-D_GLIBCXX_USE_C99
LIB = -static-libstdc++

TEST = testserver

all: $(SERVER) $(CLIENT)

rebuild:	clean all

$(SERVER):
	$(CC) $(STD) $(MAKRO)$(TARGET) $(MAKRO2) $(LIB) $(SOURCE) -o $@

$(CLIENT):
	$(CC) $(STD) $(MAKRO2) $(LIB) $(SOURCE) -o $@

$(TEST):
	./server -p 6655

pack:
	tar -czf xpastu00.tar.gz ipk.cpp protokol.pdf Makefile

clean:
	rm -rf *~ $(SERVER) $(CLIENT)
