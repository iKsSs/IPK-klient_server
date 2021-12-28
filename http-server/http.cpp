#include <sys/types.h>		
#include <sys/socket.h>		//socket
#include <arpa/inet.h>		//htons

#include <netinet/in.h> /* for IP Socket data types */
#include <unistd.h> /* for close(), usleep() */
#include <netdb.h>
#include <string.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>         // std::locale, std::tolower
#include <ctime>
#include <dirent.h>

struct SIG{
	static void int_handler(int x)	// SIGINT handler
	{
		std::cout << "Server is closing.." << std::endl; // after pressing CTRL+C you'll see this message
		throw(-2);
	}
};

//////////////////////////////////
/* SOCKET */
//////////////////////////////////

class Socket{
protected:
	int sockfd;
	unsigned short port;
	struct sockaddr_in sin;
	struct hostent * ip;

	std::ofstream ofs;

	std::string msg;
	std::string chunk;

	std::string rec;
	std::string output;
	std::string header;

	std::string f_in; //communication in ipkHttp(Server|Client)-YYYY-MM-DD:hh:mm:ss.in.log
	std::string f_out; //communication out ipkHttp(Server|Client)-YYYY-MM-DD:hh:mm:ss.out.log

	std::time_t t;
	struct tm * nTime;
	std::string timeStamp;
public:
	Socket();
	virtual void Help() {}
	virtual void GetParams(int argc, char* argv[]) {}
	void OpenSocket();
	void CloseSocket();
	virtual void Read() {}
	virtual void Write() {}
};

//Socket contructor with no params
Socket::Socket() : sockfd{ 0 }, port{ 80 } {
	sin.sin_family = PF_INET;

	t = std::time(0);	//get seconds from 1.1.1970
	nTime = localtime(&t);	//get local time

	//format appropriatelly timestamp
	timeStamp = std::to_string(nTime->tm_year + 1900) + "-";
	timeStamp += ((nTime->tm_mon + 1) >= 10) ? "" : "0";
	timeStamp += std::to_string(nTime->tm_mon + 1) + "-";
	timeStamp += ((nTime->tm_mday) >= 10) ? "" : "0";
	timeStamp += std::to_string(nTime->tm_mday) + ":";
	timeStamp += ((nTime->tm_hour) >= 10) ? "" : "0";
	timeStamp += std::to_string(nTime->tm_hour) + ":";
	timeStamp += ((nTime->tm_min) >= 10) ? "" : "0";
	timeStamp += std::to_string(nTime->tm_min) + ":";
	timeStamp += ((nTime->tm_sec) >= 10) ? "" : "0";
	timeStamp += std::to_string(nTime->tm_sec);

#ifdef DEBUG
	std::cout << timeStamp << std::endl;
#endif

}

//Open new socket
void Socket::OpenSocket() {
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) { throw("Error creating socket"); }
}

//Close socket
void Socket::CloseSocket() {
	if (close(sockfd) < 0) { throw("Error closing socket"); }
}

//////////////////////////////////
/* SERVER */
//////////////////////////////////

class Server : public Socket{
	int accfd;
	int sin_len;

	int chunkMaxSize; //max size of data in chunk
	int chunkTime;

	int cont_len;
	int errPage;

	int chunkSleep;
public:
	//Server contructor with no params
	Server() : accfd{ 0 }, sin_len{ sizeof(sin) }, chunkMaxSize{ -1 }, errPage{ 0 }, chunkSleep{ 0 }, chunkTime{ 0 }, cont_len{ 0 } {
		sin.sin_addr.s_addr = INADDR_ANY;

		f_in = "ipkHttpServer-" + timeStamp + ".in.log";
		f_out = "ipkHttpServer-" + timeStamp + ".out.log";
	}
	~Server() { CloseSocket(); }
	virtual void Help();
	virtual void GetParams(int argc, char* argv[]);
	void Bind();
	void Listen(int num);
	void Accept();
	virtual void Read();
	void Search();
	virtual void Write();
	void Close();
};

//Print help message
void Server::Help() {
	std::string hlp{ "\
###########################################################\n\
Project: HTTP Server - Server\n\
Author: Jakub Pastuszek - xpastu00\n\
\tVUT FIT - April 2015\n\
Description:\n\
The program acts as http server servicing client requests.\n\
\nParameters:\n\
-h\t\t\tshow this help message\n\
-c chunk-max-size\tset up chunked transfer encoding with max size defined\n\
-p port\t\t\tset up server TCP port for listening\n\
-t min-chunk-time\tmin wait time before sending next chunk - max is this num doubled\
\nExamples:\n\
\tipkhttpserver\n\
\tipkhttpserver -p 8080\n\
\tipkhttpserver -c 1000\n\
\tipkhttpserver -c 500 -t 100\n\
--------------------------------------------------------" };
	std::cout << hlp << std::endl;
	exit(0);
}

//Parse parameters
//Params: argc - number of params
//		  argv - array of params
void Server::GetParams(int argc, char* argv[]) {
	int opt;

	while ((opt = getopt(argc, argv, "hc:p:t:")) != -1) {	//c, p, t need value
		switch (opt) {
		case 'h':		//display help
			this->Help();
			break;
		case 'c':		//set chunk max size
			chunkMaxSize = atoi(optarg);
			break;
		case 'p':		//set port
			port = atoi(optarg);
			break;
		case 't':		//set min chunk time
			chunkTime = atoi(optarg) * 1000;	//usleep is in micro seconds
			break;
		default:		//any other param is wrong
			throw("Wrong parameter - use: [-h] [-c chunk-max-size] [-p port] [-t min-chunk-time]");
		}
	}

#ifdef DEBUG
	printf("%s: %d\n", "port", port);
	printf("%s: %d\n", "chunk", chunkMaxSize);
#endif

}

//Bind server port
void Server::Bind() {
	sin.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) { throw("Error bind"); }
}

//Server listen
//Param: num - max simultaneous connected clients
void Server::Listen(int num) {
	if (listen(sockfd, num) != 0) { throw("Error listen"); }
}

//Server accept connection
void Server::Accept() {
	if ((accfd = accept(sockfd, (struct sockaddr *)&sin, (socklen_t *)&sin_len)) < 0) { throw("Error open connection"); }
}

//Read from client
void Server::Read() {
	char c;
	bool a = false, b = false;

	rec.clear();		//empty rec string
	while (1) {			//read until \n\r\n
		if (read(accfd, &c, 1) < 0) { throw("Error receiving message"); }

		rec.push_back(c);	//read char by char and save it

		if (c == '\n') {
			a = true;
			if (b) {
				break;
			}
		}
		else if (c == '\r' && a) {
			b = true;
			a = false;
		}
		else {
			a = b = false;
		}
	}

	//in file
	ofs.open(f_in, std::ifstream::app);
	ofs << rec;
	ofs.close();

#ifdef DEBUG
	std::cout << std::endl << "REQ: " << rec << std::endl;
#endif
}

//Search data required by client
void Server::Search() {
	std::ifstream ifs;
	std::string path;
	int x, y;

	msg.clear();
	cont_len = 0;

	x = rec.find("GET") + 4;
	y = rec.find("HTTP/1") - 1;

	path.append("./www/");			//main web file
	path += rec.substr(x, y - x);	//get file with path

	// Access function 0 determines if an object exits, whether file or directory
	if (access(path.c_str(), 0) != 0) {
		errPage = 404;
		return;
	}

	// Try to open the object as a directory stream
	DIR *dir = opendir(path.c_str());
	if (dir != NULL)
	{
		// Object successfully opened as a directory stream
		// Close the open directory stream
		closedir(dir);
		errPage = 403;
		return;
	}

	//open input file
	ifs.open(path.c_str(), std::ifstream::in);
	if (!ifs.is_open()) {
		errPage = 404;
		return;
	}

	//file to string
	msg.append((std::istreambuf_iterator<char>(ifs)),
		std::istreambuf_iterator<char>());

	ifs.close();

#ifdef DEBUG
	std::cout << output << std::endl;
#endif

	cont_len = msg.length();	//length of file is legth of string
}

//Write to client
void Server::Write() {
	std::string http_code;
	std::string http_q;

	//out file
	ofs.open(f_out, std::ifstream::app);

	if (errPage == 404) {
		http_code = "404";
		http_q = "Not Found";
	}
	else if (errPage == 403) {
		http_code = "403";
		http_q = "Forbidden";
	}
	else {
		http_code = "200";
		http_q = "OK";
	}

	header.clear();
	//create header
	header.append("HTTP/1.1 ");
	header.append(http_code);
	header.append(" ");
	header.append(http_q);
	header.append("\r\n");
	header.append("Server: IPK-server/1.3\r\n");
	header.append("Content-Type: text/plain\r\n");
	if (chunkMaxSize != -1) {	//if chunk is enabled
		header.append("Transfer-Encoding: chunked");
	}
	else {
		header.append("Content-Length: ");
		header.append(std::to_string(cont_len));
	}
	header.append("\r\n");
	header.append("Connection: close\r\n");
	header.append("\r\n");

	ofs << header;

#ifdef DEBUG
	std::cout << header << std::endl;	//check output
#endif

	if (write(accfd, header.c_str(), header.length()) < 0) { throw("Error sending header"); }

	output.clear();

	if (chunkMaxSize == -1) {	//if chunk is NOT enabled
		if (write(accfd, msg.c_str(), msg.length()) < 0) { throw("Error sending body"); }

		ofs << msg;
	}
	else {
		char buffer[10];
		bool is_last = false, last = false;
		int send_len, msg_len;

		while (1) {		//till all chunks are send
			msg_len = msg.length();
			chunk.clear();

			if (is_last) {		//last chunk is 0
				sprintf(buffer, "%x", 0);
				last = true;
			}

			if (msg_len <= chunkMaxSize) {		//msg is shorter than chunk
				sprintf(buffer, "%x", msg_len);	//get size in hex
				is_last = true;
				send_len = msg_len;
			}
			else {				//msg is longer than chunk
				sprintf(buffer, "%x", chunkMaxSize);
				send_len = chunkMaxSize;
			}

			chunk.append(buffer);	//chunk num in hex
			chunk.append("\r\n");	//and new line

			if (write(accfd, chunk.c_str(), chunk.length()) < 0) { throw("Error sending chunked num"); }
			//write chunk

			ofs << chunk;

#ifdef DEBUG
			std::cout << std::endl << "Chunk: " << chunk << std::endl;
#endif
			if (!last) {
				if (write(accfd, msg.c_str(), send_len) < 0) { throw("Error sending chunked body"); }
				//write data

				ofs << msg.substr(0, send_len);
				msg.erase(0, send_len);		//erase written data from input
#ifdef DEBUG
				std::cout << std::endl << "MSG: " << msg.substr(0, send_len) << std::endl;
#endif
			}
			if (write(accfd, "\r\n", 2) < 0) { throw("Error sending chunked end"); }
			//write end new line

			ofs << "\r\n";

			if (last) {	//last chunk -> end
				break;
			}

			if (chunkTime) {	//is defined min delay
				chunkSleep = rand() % chunkTime + chunkTime;	//from min to 2*min random
				usleep(chunkSleep);
			}
		}
	}
	ofs.close();
}

//Close connection
void Server::Close() {
	if (close(accfd) < 0) { throw("Error closing connection"); }
}

//////////////////////////////////
/* CLIENT */
//////////////////////////////////

class Client : public Socket{
	unsigned int len;

	char * cs;

	std::string url;
	std::string body;

	std::string f_header; //ipkResp-YYYY-MM-DD:hh:mm:ss.header
	std::string f_payload; //ipkResp-YYYY-MM-DD:hh:mm:ss.payload
public:
	//Client contructor with no params
	Client() : len{ 0 }, cs{ 0 } {
		f_in = "ipkHttpClient-" + timeStamp + ".in.log";
		f_out = "ipkHttpClient-" + timeStamp + ".out.log";

		f_header = "ipkResp-" + timeStamp + ".header";
		f_payload = "ipkResp-" + timeStamp + ".payload";
	}
	virtual void Help();
	virtual void GetParams(int argc, char* argv[]);
	void SetPort();
	void DNSip();
	void Connect();
	virtual void Write();
	virtual void Read();
};

//Print help message
void Client::Help() {
	std::string hlp{ "\
###########################################################\n\
Project: HTTP Server - Client\n\
Author: Jakub Pastuszek - xpastu00\n\
\tVUT FIT - April 2015\n\
Description:\n\
The program acts as http client requesting server services.\n\
\nParameters:\n\
-h\t\tshow this help message\n\
URI\t\tserver address - required\n\
\nExamples:\n\
\tipkhttpclient http://localhost:8080\n\
\tipkhttpclient http://www.seznam.cz/\n\
--------------------------------------------------------" };
	std::cout << hlp << std::endl;
	exit(0);
}

//Parse parameters
//Params: argc - number of params
//		  argv - array of params
void Client::GetParams(int argc, char* argv[]) {
	unsigned int n, x;

	if (argc < 2) {			//at least URI is needed
		throw("Wrong parameter - use [-h] URI");
	}
	//parse params and store it into string
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			this->Help();		//print out a help msg
		}
		else {
			if (!url.empty()) {	//redefine URI (3rd param) -> invalid
				throw("Wrong parameter - use [-h] URI");
			}
			url = argv[i];		//save URI
		}
	}

	//http protocol is correct
	if (url.find("http://") != -1) {
		url.erase(0, url.find("//") + 2);	//erase that
	}
	else {			//other protocol is not supported
		throw("Wrong protocol type");
	}
	if (url.find("@") != -1) {				//any login informations are deleted
		url.erase(0, url.find("@") + 1);
	}
	if ((n = url.find(":")) != -1) {		//address contains port num
		x = url.find("/", n + 1);
		port = std::atoi(url.substr(n + 1, x - n).c_str());	//get port num
	}

#ifdef DEBUG
	std::cout << url << std::endl;	//check output
#endif

}

//Set connection port to server
void Client::SetPort() {
	sin.sin_port = htons(port);
}

//DNS translate - name to IP
void Client::DNSip() {
	unsigned int x, y;
	x = url.find(":");
	y = url.find("/");
	if (x > y) { x = y; }
	if ((ip = gethostbyname(url.substr(0, x).c_str())) == NULL) { throw("Error hostname"); }
}

//Connect to server
void Client::Connect() {
	memcpy(&sin.sin_addr, ip->h_addr, ip->h_length);
	if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) { throw("Error connection"); }
}

//Write to server
void Client::Write() {
	std::string file, root;

	if (url.find("/") != -1) {			//get file with path from URI
		file = url.substr(url.find("/"));	//file
		root = url.substr(0, url.find("/"));//host
	}
	else {				//not file or dir
		root = url;
		file.push_back('/');
	}

	header.clear();
	//create header
	header.append("GET ");
	header.append(file);
	header.append(" HTTP/1.1\r\n");
	header.append("User-Agent: IPK-browser/0.2\r\n");
	header.append("Host: ");
	header.append(root);
	header.append("\r\nConnection: close\r\n");
	header.append("Accept-language: cs\r\n");
	header.append("\r\n");

	//out file
	ofs.open(f_out, std::ifstream::app);
	ofs << header;
	ofs.close();

#ifdef DEBUG
	std::cout << header << std::endl;	//check output
#endif

	if (write(sockfd, header.c_str(), header.length()) < 0) { throw("Error sending message"); }
	//send whole header to server
}

//Read from server
void Client::Read() {
	char c;
	bool a = false, b = false;

	rec.clear();		//empty rec string
	while (1) {			//read until \n\r\n
		if (read(sockfd, &c, 1) < 0) { throw("Error receiving message"); }

		rec.push_back(c);		//read char by char and save it

		if (c == '\n') {
			a = true;
			if (b) {
				break;
			}
		}
		else if (c == '\r' && a) {
			b = true;
			a = false;
		}
		else {
			a = b = false;
		}
	}

	//header file
	ofs.open(f_header, std::ifstream::app);
	ofs << rec;
	ofs.close();

	//in file
	ofs.open(f_in, std::ifstream::app);
	ofs << rec;
#ifdef DEBUG
	std::cout << std::endl << "HEAD: " << std::endl << rec << std::endl;
#endif
	int pos = rec.find("Content-Length:");	//get know if length or chunk

	body.clear();
	if (pos != -1) {	//Content-Length
		len = std::atoi(rec.substr(pos + 15, rec.find("\r\n", pos)).c_str());	//pos - position of Content-Length
		//16 is its length, and substr to EOL
		if (len != 0) {					//any msg
			cs = new char[len + 1];		//char array
			cs[len] = '\0';				//last char must be nul
			if (read(sockfd, cs, len) < 0) { throw("Error receiving message"); }	//read entire stream of data
			ofs << cs;
			body.append(cs);
			delete[] cs;
		}
	}
	else {				//Chunked
		while (1) {		//till last chunk
			a = false;

			chunk.clear();
			//get know length of chunk in hex
			while (1) {			//read until \r\n
				if (read(sockfd, &c, 1) < 0) { throw("Error receiving message"); }

				chunk.push_back(c);		//char by char

				if (c == '\r') {
					a = true;
				}
				else if (c == '\n' && a) {
					break;
				}
				else {
					a = false;
				}
			}

			ofs << chunk;

#ifdef DEBUG
			std::cout << "Chunk: " << chunk;	//check output
#endif
			sscanf(chunk.c_str(), "%x", &len);	//read num in hex
#ifdef DEBUG
			std::cout << "Len: " << len << std::endl;
#endif
			if (len == 0) {			//chunk length is zero -> last chunk
				read(sockfd, &c, 1);	//read last \r
				read(sockfd, &c, 1);	//read last \n
				ofs << "\r\n";
				break;
			}

			/*
			cs = new char[len + 2];		//char array
			cs[len + 1] = '\0';				//last char must be nul

			if (read(sockfd, cs, len) < 0) { throw("Error receiving message"); }

#ifdef DEBUG
			std::cout <<  "Msg: " << cs << std::endl;
#endif
			body.append(cs);		//char by char
			ofs << cs;
			delete[] cs;
			*/

			chunk.clear();
			int n = 0;
			while (n < len) {
				if (read(sockfd, &c, 1) < 0) { throw("Error receiving message"); }
				chunk.push_back(c);		//char by char
				n++;
			}

#ifdef DEBUG
			std::cout << "Msg: " << chunk << std::endl;
#endif

			body.append(chunk);		//char by char
			ofs << chunk;

			read(sockfd, &c, 1);	//read last \r
			read(sockfd, &c, 1);	//read last \n
			ofs << "\r\n";
		}
	}

	ofs.close();

	//payload file
	ofs.open(f_payload, std::ifstream::app);
	ofs << body;
	ofs.close();

#ifdef DEBUG
	std::cout << std::endl << "MSG: " << std::endl << body << std::endl;
#endif
}

//////////////////////////////////
/* FUNCTIONS */
//////////////////////////////////

//Client side
void StartClient(int argc, char* argv[]) {
	Client c;

	c.GetParams(argc, argv);

	c.OpenSocket();
	c.SetPort();

	c.DNSip();

	c.Connect();

	c.Write();

	c.Read();

	c.CloseSocket();
}

//Server side
void StartServer(int argc, char* argv[]) {
	Server s;

	s.GetParams(argc, argv);

	s.OpenSocket();
	s.Bind();
	s.Listen(5);

	while (1) {
		s.Accept();

		s.Read();

		s.Search();

		s.Write();

		s.Close();
	}

	s.CloseSocket();
}

//////////////////////////////////
/* MAIN */
//////////////////////////////////

int main(int argc, char* argv[]) {
	try {
#ifdef SERV
		signal(SIGINT, SIG::int_handler);
		StartServer(argc, argv);
#else
		StartClient(argc, argv);
#endif
	}
	catch (const char* e) { //print my err msg to STDERR
		std::cerr << e << std::endl;
		exit(-1);
	}
	catch (int e) { //catched SIGINT
		//nothing to do
	}
	catch (...) { //print just ERROR to STDERR if unknown exception
		std::cerr << "Unspecified ERROR" << std::endl;
		exit(-2);
	}
}
