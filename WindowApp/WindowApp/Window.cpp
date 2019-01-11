#include <windows.h>
#include <iostream>
#include <string>
#include <atlstr.h>
#include <fstream>

#pragma warning(disable: 4996)

#pragma comment(lib, "ws2_32.lib")

// IP:
// 192.168.1.14 
// 127.0.0.1

// PORT
int port = 9999;

// API
#define ID_EDIT 2
#define ID_BUTTON1 0
#define ID_BUTTON2 1
//#define ID_PORT 3

// SOCKETS
WSADATA WsaData;
SOCKET Socket;
struct hostent *host;
SOCKADDR_IN SockAddr;

// WIN32
MSG msg;
HWND hwnd;
WNDCLASSW clas;

std::string readFile(LPSTR file);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CenterWindow(HWND hwnd);
std::string OpenDialog(HWND);
void LoadFile(LPSTR);
void OpenDialog2(HWND hwnd, std::string str);
void writeFile(LPSTR file, std::string str);

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
	hwnd = CreateWindowW(clas.lpszClassName, L"Client", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, 800, 400, 
		NULL, NULL, hInstance, NULL);

	// Loop for keep the Window front to user
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	closesocket(Socket);
	WSACleanup();
	CloseHandle(Thread);
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static HWND hwndEdit;
	//static HWND hwndPort;

	HWND hwndButton;
	HWND hwndButton1;

	switch (msg) {
	case WM_CREATE: {
		CenterWindow(hwnd);

		//hwndPort = CreateWindowW(L"Edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 199, 50, 80, 20, hwnd, (HMENU) ID_PORT, NULL, NULL);
		hwndEdit = CreateWindowW(L"Edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 630, 300, 150, 20, hwnd, (HMENU) ID_EDIT, NULL, NULL);
		/*ghwndEdit = CreateWindowW(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE,
			-50, -50, 10, 10,
			hwnd, (HMENU)1, NULL, NULL);*/
		hwndButton = CreateWindowW(L"Button", L"Connect", WS_VISIBLE | WS_CHILD, 700, 330, 80, 25, hwnd, (HMENU)ID_BUTTON1, NULL, NULL);
		hwndButton1 = CreateWindowW(L"Button", L"Send", WS_VISIBLE | WS_CHILD, 600, 330, 80, 25, hwnd, (HMENU)ID_BUTTON2, NULL, NULL);

		break;
	}
	case WM_COMMAND: {
		if (wParam == ID_BUTTON2) {
			std::string str = OpenDialog(hwnd);
			send(Socket, str.c_str(), str.size() + 1, 0);
		}
		if (wParam == ID_BUTTON1) {
			// Initialise Winsock
			int iResult = WSAStartup(MAKEWORD(2, 2), &WsaData);
			if (iResult != 0)
			{
				WSACleanup();
				exit(0);
			}

			// Create our socket
			Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (Socket == INVALID_SOCKET)
			{
				WSACleanup();
				exit(0);
			}

			// Getting ipAddress
			TCHAR szTemp[128];
			GetDlgItemText(hwnd, ID_EDIT, szTemp, 128);

			// Resolve IP address for hostname
			host = gethostbyname(szTemp);

			if (host == NULL)
			{
				MessageBoxW(hwnd, L"Can't find a host", L"Error with host", MB_OK);
				WSACleanup();
				exit(0);
			}
			else {
				MessageBoxW(hwnd, L"Connecting...", L"Connecting...", MB_OK);
			}

			// Setup our socket address structure
			SockAddr.sin_port = htons(port);
			SockAddr.sin_family = AF_INET;
			SockAddr.sin_addr.S_un.S_addr = *((unsigned long*)host->h_addr);;

			Thread = CreateThread(0, 0, ThreadFn, 0, 0, &threadID);
		}

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
	//MessageBoxW(hwnd, L"In THREAD!", L"MESSAGE!", 0);

	// Attempt to connect to server
	int connResult = connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr));

	if (connResult == SOCKET_ERROR)
	{
		MessageBoxW(hwnd, L"Can't connect", L"Error with Connect", MB_OK);
		closesocket(Socket);
		WSACleanup();
	}
	else {
		MessageBoxW(hwnd, L"Connected!", L"Connected!", MB_OK);

		// do - while loop to send and receive data
		char buffer[4096];

		while (true) {
			int byteReceived = recv(Socket, buffer, 4096, 0);

			if (byteReceived > 0) {
				// Echo response to console
				MessageBoxW(hwnd, L"We got one file!", L"MESSAGE!", MB_OK);
				std::string str = std::string(buffer, 0, byteReceived);
				OpenDialog2(hwnd, str);
			}
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
	RECT rc = {0};

	GetWindowRect(hwnd, &rc);

	int win_w = rc.right - rc.left;
	int win_h = rc.bottom - rc.top;

	int screen_w = GetSystemMetrics(SM_CXSCREEN);
	int screen_h = GetSystemMetrics(SM_CYSCREEN);

	SetWindowPos(hwnd, HWND_TOP, (screen_w - win_w)/2, (screen_h - win_h)/2, 0, 0, SWP_NOSIZE);
}