#include <iostream>
#include <string>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void main() {
	// initializing datas for connetion
	string ipAddress = "127.0.0.1";
	int port = 9999;

	// initializing WinSocket
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsResult = WSAStartup(ver, &wsData);

	if (wsResult != 0) {
		cerr << "Can't start WinSocket, Err #" << wsResult << endl;
		return;
	}

	// create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cerr << "Can't create a socket, Err #" << WSAGetLastError() << endl;
		return;
	}

	// fill in a hint structure 
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// connect to server 
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));

	if (connResult == SOCKET_ERROR) {
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}

	// do-while loop to send and receive data
	char buf[4096];
	string userInput;

	do {
		// Prompt the user for some text
		cout << "> ";
		getline(cin, userInput);

		if (userInput.size() > 0) {
			// Send the text
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);

			if (sendResult != SOCKET_ERROR) {
				// Wait for response 
				ZeroMemory(buf, 4096);
				int byteReceived = recv(sock, buf, 4096, 0);

				if (byteReceived > 0) {
					// Echo response to console
					cout << "SERVER > " << string(buf, 0, byteReceived) << endl;
				}
			}
		}
	} while (userInput.size() > 0);

	// gracefully close down everything
	closesocket(sock);
	WSACleanup();

	system("pause");
}