SERVER          = ipkhttpserver
CLIENT          = ipkhttpclient
SOURCE          = http.cpp
TARGET          = SERV

CC = g++
STD = -std=c++11
MAKRO = -D
MAKRO2 =-D_GLIBCXX_USE_C99
LIB = -static-libstdc++
DEBUG = $(MAKRO)DEBUG

TEST_S = testserver
TEST_C = testclient

.PHONY: all rebuild test test1 test2 test3 test4 pause1 pause2 pack clean

all: $(SERVER) $(CLIENT)

rebuild:	clean all

$(SERVER):
	$(CC) $(STD) $(MAKRO)$(TARGET) $(MAKRO2) $(LIB) $(SOURCE) -o $@

$(CLIENT):
	$(CC) $(STD) $(LIB) $(SOURCE) -o $@

$(TEST_S):
	./ipkhttpserver -p 5024

$(TEST_C):
	./ipkhttpclient localhost:5024/index.php
	
test:	test1 pause1 test2 test3 pause2 test4

test1:
	@echo "Server bez podpory chunk encoding"
	./ipkhttpserver -p 5024 &
	@echo
	@echo "Klient pozaduje soubor index.php z root adresare serveru - server odpovida kodem 200"
	./ipkhttpclient http://localhost:5024/index.php
	@echo
	@echo "Klient pozaduje root adresar serveru - server odpovida kodem 403"
	./ipkhttpclient http://127.0.0.1:5024
	@echo
	@echo "Vypnuti serveru signalem SIGINT"
	@killall -2 ipkhttpserver
	@echo
	
test2:
	@echo "Server s podporou chunk encoding"
	./ipkhttpserver -p 5025 -c 2200 -t 50 &
	@echo
	@echo "Klient pozaduje soubor long.txt z root adresare serveru - server odpovida kodem 200"
	./ipkhttpclient http://127.0.0.1:5025/long.txt
	@echo
	@echo "Klient pozaduje neexistujici soubor nenalezeno - server odpovida kodem 404"
	./ipkhttpclient http://localhost:5025/nenalezeno
	@echo
	@echo "Vypnuti serveru signalem SIGINT"
	@killall -2 ipkhttpserver
	@echo

test3:
	@echo "Server vytiskne help"
	./ipkhttpserver -h
	@echo
	@echo "Klient vytiskne help"
	./ipkhttpclient -h
	@echo

test4:
	@echo "Klient se snazi pripojit server www.seznam.cz a ziskat soubor index.html - server (externi) odpovida kodem 200"
	./ipkhttpclient http://www.seznam.cz/index.html
	@echo
	@echo "Klient se snazi pripojit na spatnou URI (bez protokolu http) - skonci chybou make (klient konci exit -2)"
	./ipkhttpclient 127.0.0.1
	@echo
	
pause1:
	@sleep 1

pause2:
	@sleep 2
	
pack:
	tar -czf xpastu00.tgz http.cpp Makefile README www

clean:
	rm -rf *~ *.core $(SERVER) $(CLIENT) *.log *.header *.payload
