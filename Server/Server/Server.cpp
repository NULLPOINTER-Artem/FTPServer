#include <windows.h>
#include <iostream>
#include <string>
#include <atlstr.h>
#include <fstream>

#pragma warning(disable: 4996)

#pragma comment(lib, "ws2_32.lib")

// IP:
// 168.192.1.14 
// 127.0.0.1

// PORT
int port = 9999;

// API
#define ID_BUTTON1 0
#define ID_BUTTON2 1

// SOCKETS
WSADATA WsaData;
SOCKET Socket;
//struct hostent *host;
SOCKADDR_IN SockAddr;

// Client
sockaddr_in client;
int clientSize = sizeof(client);
SOCKET clientSocket;

// WIN32
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CenterWindow(HWND hwnd);
std::string OpenDialog(HWND);
void LoadFile(LPSTR);
void OpenDialog2(HWND hwnd, std::string str);
void writeFile(LPSTR file, std::string str);
std::string readFile(LPSTR file);

// MAIN WINDOW
MSG msg;
HWND hwnd;
WNDCLASSW clas;

// Thread
HANDLE Thread = NULL;
DWORD WINAPI ThreadFn(LPVOID vpParam);
DWORD threadID = NULL;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	// Registration a Window
	clas.style = CS_HREDRAW | CS_VREDRAW;
	clas.cbClsExtra = 0;
	clas.cbWndExtra = 0;
	clas.lpszClassName = L"The Window";
	clas.hInstance = hInstance;
	clas.hbrBackground = GetSysColorBrush(COLOR_3DDKSHADOW);
	clas.lpszMenuName = NULL;
	clas.lpfnWndProc = WndProc;
	clas.hCursor = LoadCursor(NULL, IDC_ARROW);
	clas.hIcon = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassW(&clas);

	// Creating the Window
	hwnd = CreateWindowW(clas.lpszClassName, L"Server", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, 800, 400,
		NULL, NULL, hInstance, NULL);

	// Loop for keep the Window front to user
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	/*GetMessage(&msg, NULL, NULL, NULL)*/

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	closesocket(Socket);
	closesocket(clientSocket);
	CloseHandle(Thread);
	WSACleanup();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	HWND hwndButton1;
	HWND hwndButton2;

	switch (msg) {
	case WM_CREATE: {
		CenterWindow(hwnd);

		hwndButton1 = CreateWindowW(L"Button", L"Send", WS_VISIBLE | WS_CHILD, 700, 330, 80, 25, hwnd, (HMENU)ID_BUTTON1, NULL, NULL);
		hwndButton2 = CreateWindowW(L"Button", L"Start!", WS_VISIBLE | WS_CHILD, 600, 330, 80, 25, hwnd, (HMENU)ID_BUTTON2, NULL, NULL);

		break;
	}
	case WM_COMMAND: {
		if (wParam == ID_BUTTON1) {
			std::string str = OpenDialog(hwnd);
			send(clientSocket, str.c_str(), str.size() + 1, 0);
		}
		if (wParam == ID_BUTTON2) {
			if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
			{
				MessageBoxW(hwnd, L"Error with makeWord", L"Error", 0);
				WSACleanup();
				exit(0);
			}

			Socket = socket(AF_INET, SOCK_STREAM, 0);
			if (Socket == INVALID_SOCKET)
			{
				MessageBoxW(hwnd, L"Error with creation socket", L"Error", 0);
				WSACleanup();
				exit(0);
			}

			SockAddr.sin_family = AF_INET;
			SockAddr.sin_addr.s_addr = INADDR_ANY;
			SockAddr.sin_port = htons(port);

			// Reading and Writing
			Thread = CreateThread(0, 0, ThreadFn, 0, 0, &threadID);
		}
		break;
	}
	case WM_CLOSE: {
		DestroyWindow(hwnd);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI ThreadFn(LPVOID vpParam) {
	// Binding
	if (bind(Socket, (sockaddr*)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR) {
		MessageBoxW(hwnd, L"Error with bind", L"Error", 0);
		WSACleanup();
		exit(0);
	}

	listen(Socket, SOMAXCONN);

	// Wait for a connetion client!
	clientSocket = accept(Socket, (sockaddr*)&client, &clientSize);

	// Close listening socket 
	closesocket(Socket);

	// While loop: accept and echo message back to client 
	char buffer[4096];

	// Wait for client to send data
	while (true) {
		int byteReceived = recv(clientSocket, buffer, 4096, 0);

		if (byteReceived == SOCKET_ERROR) {
			MessageBoxW(hwnd, L"Client Disconnected.", L"Disconnect", 0);
			break;
		}

		if (byteReceived > 0) {
			/*MessageBoxW(hwnd, (wchar_t*)std::string(buffer, 0, byteReceived).c_str(), L"MESSAGE!", 0);*/
			MessageBoxW(hwnd, L"We got one file!", L"MESSAGE!", MB_OK);
			std::string str = std::string(buffer, 0, byteReceived);
			OpenDialog2(hwnd, str);
		}
	}

	return 0;
}

std::string OpenDialog(HWND hwnd) {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.hwndOwner = hwnd;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = TEXT("All files(*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFileTitle = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	std::string res = "";

	if (GetOpenFileName(&ofn)) {
		LoadFile(ofn.lpstrFile);
		res = readFile(ofn.lpstrFile);
	}

	return res;
}

void OpenDialog2(HWND hwnd, std::string str) {
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.hwndOwner = hwnd;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = TEXT("All files(*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFileTitle = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		//LoadFile(ofn.lpstrFile);
		writeFile(ofn.lpstrFile, str);
	}
}

void writeFile(LPSTR file, std::string str) {
	std::string result = "";
	std::string path = file;

	std::fstream file1(path, std::fstream::out);

	file1 << str;

	file1.close();
}

std::string readFile(LPSTR file) {
	std::string result = "";
	std::string path = file;

	std::fstream file1(path, std::fstream::in);

	std::string str = "";
	while (!file1.eof()) {
		str = "";
		std::getline(file1, str);
		result += str + "\n";
	}

	file1.close();
	return result;
}

void LoadFile(LPSTR file) {
	HANDLE hFile;
	DWORD dwSize;
	DWORD dw;

	LPBYTE lpBuffer = NULL;

	hFile = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	dwSize = GetFileSize(hFile, NULL);
	lpBuffer = (LPBYTE)HeapAlloc(GetProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS, dwSize + 1);
	ReadFile(hFile, (LPWSTR)lpBuffer, dwSize, &dw, NULL);
	CloseHandle(hFile);
	lpBuffer[dwSize] = 0;
	//SetWindowText(ghwndEdit, (LPSTR)lpBuffer);
	HeapFree(GetProcessHeap(), 0, lpBuffer);
}

void CenterWindow(HWND hwnd) {
	RECT rc = { 0 };

	GetWindowRect(hwnd, &rc);

	int win_w = rc.right - rc.left;
	int win_h = rc.bottom - rc.top;

	int screen_w = GetSystemMetrics(SM_CXSCREEN);
	int screen_h = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, (screen_w - win_w) / 2, (screen_h - win_h) / 2, 0, 0, SWP_NOSIZE);
}