#include <sys/types.h>		//
#include <sys/socket.h>		//socket
#include <arpa/inet.h>		//htons

#include <netinet/in.h> /* for IP Socket data types */
#include <unistd.h> /* for close(), getopt() and fork() */
#include <netdb.h>
#include <string.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <locale>         // std::locale, std::tolower

//////////////////////////////////
/* SOCKET */
//////////////////////////////////

class Socket{
protected:
	int sockfd;
	int port;
	struct sockaddr_in sin;
	struct hostent * ip;
	std::string msg;
	char *pom_str;
public:
	Socket();
	~Socket() { delete[] pom_str; }
	virtual void GetParams(int argc, char* argv[]) {}
	void OpenSocket();
	void CloseSocket();
	virtual void Read() {}
	virtual void Write() {}
};

//Socket contructor with no params
Socket::Socket() : sockfd{ 0 } {
	sin.sin_family = PF_INET;
	pom_str = new char[260];
	port = 0;
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
	int sin_len, ip_len;
	std::string response;
public:
	//Server contructor with no params
	Server() : accfd{ 0 }, sin_len{ sizeof(sin) }, ip_len{ 0 } {
		sin.sin_addr.s_addr = INADDR_ANY;
	}
	virtual void GetParams(int argc, char* argv[]);
	void Bind();
	void Listen(int num);
	void Accept();
	virtual void Read();
	void Search();
	virtual void Write();
	void Close();
};

//Parse parameters
//Params: argc - number of params
//		  argv - array of params
void Server::GetParams(int argc, char* argv[]) {
	if (argc < 3) { throw("Need port number"); }
	if (!strcmp(argv[1], "-p")) {
		this->port = atoi(argv[2]);
		if (port > 65535 || port < 1024) { //or 1 ?
			throw("Need correct port number");
		}
	}
	else {
		throw("Need port number");
	}
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
	std::string rec;
	int num, x;
	msg.clear();	//Clear input msg

	bzero(pom_str, sizeof(pom_str));	//Clear input buffer
	if (read(accfd, pom_str, 255) < 0) { throw("Error receiving message"); }

	rec = pom_str;	//Save content of buffer into rec
	x = rec.find(":");	
	num = atoi(rec.substr(0, x).c_str());	//Get num of msgs client going to send
	msg.append(rec.substr(x + 1));	//Save received msg
	
	//std::cout <<x<< " Left: " << num << " Length of message is " << rec.length() << std::endl;

	//repeat till last msg
	while (num) {
		bzero(pom_str, sizeof(pom_str));	//Clear input buffer
		if (read(accfd, pom_str, 255) < 0) { throw("Error receiving message"); }

		msg.append(pom_str);
		num--;
	}

	//std::cout << std::endl << "MSG: " << msg << std::endl;
}

//Search data required by client
void Server::Search() {
	using namespace std;
	locale loc;

	string login, uid;
	string line, token, token_low;
	string args;
	bool x, lf;
	int n;
	vector<string> out;
	vector<string> look;
	vector<string> tst;
	vector<string> input, in_low;
	
	map<char, int> marg;

	marg['L'] = 0;
	marg['U'] = 1;
	marg['G'] = 2;
	marg['N'] = 3;
	marg['H'] = 4;
	marg['S'] = 5;

	lf = (msg[0] == 'l') ? true : false;	//logins or uids

	if (msg.rfind(":") + 1 < msg.size()) {	//any criteria?
		args.append(msg.substr(msg.rfind(":") + 1));	//save criteria to args
	}
	
	if (!msg.substr(1, msg.rfind(":") - 1).empty()) {	//any logins/uids?
		stringstream iss(msg.substr(1, msg.rfind(":") - 1));	//make l/u stream
		while (getline(iss, token, ':')) {

			token_low.clear();
			token_low = token;
			//create token in lowercase
			for (int j = 0; j < token_low.size(); j++) {
				token_low[j] = tolower(token_low[j], loc);
			}
		//	cout << token_low << "." << token << endl;

			x = true;
			//find out if l/u is in input string
			for (std::vector<string>::iterator it = in_low.begin(); it != in_low.end(); ++it) {
				if (*it == token_low) {
					x = false;
					break;
				}
			}
			if (x) {	//l/u isnt in input, add it
				input.push_back(token);

				//create token in lowercase
				in_low.push_back(token_low);
			}
		}
	}

	//open file
	ifstream ifs;
	ifs.open("/etc/passwd", ifstream::in);

	response.clear();

	//no l/u -> send search
	if (input.empty()) { return; }

//	for (int i = 0; i < input.size(); i++)cout << input[i] << endl;

	while (getline(ifs, line)) {
		//LOGIN
		login = line.substr(0, line.find(":"));	//separate login
		for (int i = 0; i < login.size(); i++) {
			login[i] = tolower(login[i], loc);	//tolower login for compare
		}
		//UID
		int first = 1 + line.find(":", 1 + line.find(":"));	//: before uid
		int second = line.find(":", 1 + line.find(":", 1 + line.find(":")));	//: after uid
		uid = line.substr(first, second - first);	//separate uid

		if (lf) {
			for (int i = 0; i < input.size(); i++) {	//every input login copare with actual login
				if (in_low[i] == login) {
					look.push_back(to_string(i) + ":" + line);	//add (order : line)
				}
			}
		}
		else {
			for (int i = 0; i < input.size(); i++) {	//every input uid copare with actual uid
				if (in_low[i] == uid) {
					look.push_back(to_string(i) + ":" + line);	//add (order : line)
				}
			}
		}
	}

	//close file
	ifs.close();

	//for (int i = 0; i < look.size(); i++)cout << look[i] << endl;
	//for (int i = 0; i < input.size(); i++)cout << input[i] << endl;

	//find unknown l/u
	for (int j = 0; j < input.size(); j++) {
		x = true;
		//find out if line number of l/u is occupied (not -> unknown l/u)
		for (int i = 0; i < look.size(); i++) {
			if (atoi(look[i].substr(0, look[i].find(":")).c_str()) == j) {
				x = false;
				break;
			}
		}
		if (x) {	//unknown
			//find out if l/u is in input string
			for (std::vector<string>::iterator it = tst.begin(); it != tst.end(); ++it) {
				if (*it == input[j]) {
					x = false;
					break;
				}
			}
			if (x) {	//l/u isnt in input, add it
				tst.push_back(input[j]);
			}
		}
	}

	//make string from found unknown l/u
	for (std::vector<string>::iterator it = tst.begin(); it != tst.end(); ++it) {
		response.push_back(':');	//first char of line - indicate unknown l/u
		response += *it;
		response.push_back('\n');
	}

	//search for client specified criteria
	for (int j = 0; j < input.size(); j++) {
		//process just known l/u
		for (int i = 0; i < look.size(); i++) {
			//order lines as client want
			if (atoi(look[i].substr(0, look[i].find(":")).c_str()) == j) {
				line = look[i].substr(look[i].find(":") + 1); //line without index num
				int k = 0;
				stringstream iss(line);
				out.clear();
				//separate every "item" in line
				while (getline(iss, token, ':')) {
					++k;
					if (k == 2) { continue; } //preskoceni druheho parametru radku
					out.push_back(token);
				}
				//save "item" if is in criteria string
				for (int i = 0; i < args.size(); i++) {
					response += out[marg[args[i]]] + " ";
				}
				//nothing added to response - cant pop_back
				if (!args.empty()) {
					response.pop_back();
				}
				//add new line after every l/u
				response += "\n";
			}
		}
	}
	//remove new line after last l/u
	response.pop_back();
}

//Write to client
void Server::Write() {
	int pom;
	std::string send;

	pom = (response.length() / 250) + 0.5;	//Get num of msgs going to be send

	send = std::to_string(pom);	//create header of msg (pom:)
	send.push_back(':');

	//repeat till last msg
	while (pom >= 0) {
		send.append(response.substr(0, 250));	//append body
		response.erase(0, 250);	//erase sended content

		//std::cout << send << std::endl;

		if (write(accfd, send.c_str(), send.length() + 1) < 0) { throw("Error sending message"); }

		send.clear();	//clear sended msg
		pom--;

		usleep(500);
	}
}

//Close connection
void Server::Close() {
	if (close(accfd) < 0) { throw("Error closing connection"); }
}

//////////////////////////////////
/* CLIENT */
//////////////////////////////////

class Client : public Socket{
	std::string name;
	std::string items;
	std::string crits;
	char log;
	bool c_ok;
public:
	//Client contructor with no params
	Client() : log{ 0 }, c_ok{ false } {}
	virtual void GetParams(int argc, char* argv[]);
	void case_param(std::string params, unsigned int *i);
	void SetPort();
	void DNSip();
	void Connect();
	virtual void Write();
	virtual void Read();
	void Print();
	void CheckConnection() { if (!c_ok) std::cerr << "Server unexpectedly closed connection"; }
};

//Parse parameters
//Params: argc - number of params
//		  argv - array of params
void Client::GetParams(int argc, char* argv[]) {
	using namespace std;
	if (argc < 6) { throw("Need host and port number and uid(s) or login(s)"); }

	string params;
	string logins;
	string uids;
	int x, log_l = 0, log_u = 0;

	//parse params and store it into string
	for (int i = 1; i < argc; i++) {
		params.append(argv[i]);
		params.push_back(' '); //separate by space
	}

	if (params.empty()) { throw("Wrong parameter"); }
	
	//finite automata
	for (unsigned int i = 0; i < params.size(); ++i) {
		if (params[i] == ' ') { continue; }		//skip whitespace
		if (params[i] == '-') {		//every param must start with -
			switch (params[++i]) {	//next char switch
				case ' ':				//after - cant be whitespace
					throw("Wrong parameter");

				case 'h':
					if (!name.empty()) { throw("Repetition of parameter -h"); }	//not set yet
					if (params[++i] != ' ') { throw("Wrong parameter"); }	//after h must be whitespace
					++i;
					if ((x = params.substr(i).find(" ")) != string::npos) { //find whitespace after string
						name = params.substr(i, x);
						i += x;	//i = position of whitespace after string
					}
					else {
						name = params.substr(i);
						i = -1; //i = "inf"
					}
					break;

				case 'p':
					if (port) { throw("Repetition of parameter -p"); }	//not set yet
					if (params[++i] != ' ') { throw("Wrong parameter"); }	//after p must be whitespace
					++i;
					if ((x = params.substr(i).find(" ")) != string::npos) {	//find whitespace after string
						port = atoi(params.substr(i, x).c_str());
						i += x;	//i = position of whitespace after string
					}
					else {
						port = atoi(params.substr(i).c_str());
						i = -1; //i = "inf"
					}
					break;

				case 'l':
					log_l = i;	//save position of -l
					x = 0;
					if (!logins.empty()) { throw("Repetition of parameter -l"); }	//not set yet
					if (params[++i] != ' ') { throw("Wrong parameter"); } //after l must be whitespace
					++i;
					//do till end or next param
					while (i < params.size() && params[i] != '-') { 

						if ((x = params.substr(i).find(" ")) != string::npos) {
							logins += params.substr(i, x);	//add login
							i += x + 1;	//i = position of whitespace after string + 1
						}
						else {
							logins += params.substr(i);	//add (last) login
							i = -1;	//i = "inf"
						}
						logins.push_back(':');	//separator
					}
					if (!logins.empty()) {	//inserted any login?
						logins.pop_back();	//remove last :
					}
					i--;
					break;

				case 'u':
					log_u = i;	//save position of -u
					x = 0;
					if (!uids.empty()) { throw("Repetition of parameter -u"); }	//not set yet
					if (params[++i] != ' ') { throw("Wrong parameter"); } //after u must be whitespace
					++i;
					//do till end or next param
					while (i < params.size() && params[i] != '-') {

						if ((x = params.substr(i).find(" ")) != string::npos) {
							uids += params.substr(i, x);	//add uid
							i += x + 1;	//i = position of whitespace after string + 1
						}
						else {
							uids += params.substr(i);	//add (last) uid
							i = -1;	//i = "inf"
						}
						uids.push_back(':');	//separator
					}
					if (!uids.empty()) {	//inserted any uid?
						uids.pop_back();	//remove last :
					}
					i--;
					break;

				case 'G':
				case 'N':
				case 'H':
				case 'S':
				case 'U':
				case 'L':
					case_param(params, &i); //call auxiliary function
					break;

				default:
					throw("Wrong parameter");
			}
		}
		else{ throw("Wrong parameter"); }
	}

	//Neither -l nor -u was set
	if (log_l == log_u) {
		throw("Parameter error");
	}
	else if (log_l > log_u) {	//-l was set later
		this->items.append("l");
		this->items += logins;
		log = 'l';
	}
	else {	//-u was set later
		this->items.append("u");
		this->items += uids;
		log = 'u';
	}
}

//Auxiliary function for parse arguments
//Params: params - program input params
//		  *i - index number
void Client::case_param(std::string params, unsigned int *i) {
	switch (params[*i]) {	//switch criteria
		case 'G':
		case 'N':
		case 'H':
		case 'S':
		case 'U':
		case 'L':
			if (crits.find(params[*i]) != std::string::npos) { //check repetition
				crits.clear();	//clear crits (error dont care of content)
				crits = "Repetition of parameter "; 
				crits += params[*i];  //create err msg
				throw(crits.c_str()); 
			}
			crits += params[*i]; //add criteria
			++(*i);	//next char
			case_param(params, i); //sticked criteria?
			break;
		default:
			break;
	}
}

//Set connection port to server
void Client::SetPort() {
	sin.sin_port = htons(port);
}

//DNS translate - name to IP
void Client::DNSip() {
	if ((ip = gethostbyname(name.c_str())) == NULL) { throw("Error hostname"); }
}

//Connect to server
void Client::Connect() {
	memcpy(&sin.sin_addr, ip->h_addr, ip->h_length);
	if (connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) { throw("Error connection"); }
}

//Write to server
void Client::Write() {
	int pom;
	std::string msg = items + ":" + crits;	//send logins/uids and criteria
	std::string send;

	pom = (msg.length() / 250) + 0.5;	//Get num of msgs going to be send

	send = std::to_string(pom);	//create header of msg (num:)
	send.push_back(':');

	//repeat till last msg
	while (pom >= 0) {
		send.append(msg.substr(0, 250));	//append body
		msg.erase(0, 250);	//erase sended content

		//std::cout << send << std::endl;

		if (write(sockfd, send.c_str(), send.length() + 1) < 0) { throw("Error sending message"); }

		send.clear();	//clear sended msg
		pom--;

		usleep(500);
	} 
}

//Read from server
void Client::Read() {
	std::string rec;
	int num, x;
	msg.clear();	//Clear input msg

	bzero(pom_str, sizeof(pom_str));	//Clear input buffer
	if (read(sockfd, pom_str, 255) < 0) { throw("Error receiving message"); }

	rec = pom_str;	//Save content of buffer into rec

	if (!rec.empty()) { c_ok = true; }

	x = rec.find(":");
	num = atoi(rec.substr(0, x).c_str());	//Get num of msgs client going to send
	msg.append(rec.substr(x + 1));	//Save received msg
	
	//std::cout << x << " Left: " << num << " Length of message is " << rec.length() << std::endl;

	//repeat till last msg
	while (num) {
		bzero(pom_str, sizeof(pom_str));	//Clear input buffer
		if (read(sockfd, pom_str, 255) < 0) { throw("Error receiving message"); }

		msg.append(pom_str);	//Save(append) received msg
		num--;
	}

	//std::cout << std::endl << "MSG: " << msg << std::endl;
}

//Print data from the server
void Client::Print() {
	std::string out;
	std::stringstream iss(msg);
	//std::cout << "." << msg << "." << std::endl;
	while (getline(iss, out, '\n')) { //process every line separately
		if (out[0] == ':') {	//line starting with : indicates unknown login/uid
			if (log == 'l') {
				std::cerr << "Error: unknown login " << out.substr(1) << std::endl; //followed by login
			}
			else {
				std::cerr << "Error: unknown uid " << out.substr(1) << std::endl; //followed by uid
			}
		}
		else {
			if (!out.empty()) {
				std::cout << out << std::endl; //other line just print
			}
		}
	}
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

	c.Print();

	c.CloseSocket();

	c.CheckConnection();
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

		pid_t pid = fork(); //create child - concurrent server

		if (pid == -1) {
			throw ("Fork error");
		}
		else if (pid > 0) { //parent (server)
			continue;
		}
		else { //child (proces)
			s.Read();

			s.Search();

			s.Write();

			s.Close();

			exit(0);
		}
	}

	s.CloseSocket();
}

//////////////////////////////////
/* MAIN */
//////////////////////////////////

int main(int argc, char* argv[]) {
	try {
#ifdef SERV
		StartServer(argc, argv);
#else
		StartClient(argc, argv);
#endif
	}
	catch (const char* e) { //print my err msg to STDERR
		std::cerr << e << std::endl;
		exit(-1);
	}
	catch (...) { //print just ERROR to STDERR if unknown exception
		std::cerr << "Unspecified ERROR" << std::endl;
		exit(-2);
	}
}
