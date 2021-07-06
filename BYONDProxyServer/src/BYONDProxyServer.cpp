
/*
* BYOND Proxy Server by ZackBleus
*
* Datapipe code written by Jeff Lawson <jlawson@bovine.net>
* inspired by code originally by Todd Vierling, 1995.ssss
*/


#include "RUNSUB.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <string>
#include <conio.h>
#include <vector>
#include <intrin.h>
#include <sstream>
#include <iomanip>
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#define bzero(p, l) memset(p, 0, l)
#define bcopy(s, t, l) memmove(t, s, l)
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#define recv(x,y,z,a) read(x,y,z)
#define send(x,y,z,a) write(x,y,z)
#define closesocket(s) close(s)
typedef int SOCKET;
#endif

#include <d3d11.h>
#include <tchar.h>

#include "PacketFrame.h"
#include "ImGui/imgui.h"
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif


struct client_t
{
	int inuse;
	SOCKET csock, osock;
	time_t activity;
};

#define MAXCLIENTS 20
#define IDLETIMEOUT 300

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


const char ident[] = "$Id: datapipe.c,v 1.8 1999/01/29 01:21:54 jlawson Exp $";

int main(int argc, char* argv[])
{
	SOCKET lsock;
	char buf[0xFFFF];
	struct sockaddr_in laddr, oaddr;
	int i;
	struct client_t clients[MAXCLIENTS];
	int cptrOff = 0;
	int sptrOff = 0;
	bool seq = false;


	int NEST_NORMAL = 0;
	int NEST_CTOS_HANDSHAKE = 1;
	int NEST_STOC_HANDSHAKE = 2;
	int state = 1;

	u_int encryptionKey = 0;


	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Byond Proxy Server"), NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Byond Proxy Server"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Main loop
	bool isExit = false;


#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32)
	/* Winsock needs additional startup activities */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(1, 1), &wsadata);
#endif


	bool my_tool_active = true;
	//float* my_color;

	char sAddr[32] = "192.168.1.127";
	char sPort[32] = "9543";
	char cAddr[32] = "104.248.109.159";
	char cPort[32] = "8192";

	bool inputAddresses = true;

	while (inputAddresses)
	{

		// Poll and handle messages (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Input IP Information", &my_tool_active, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar())
		{
			ImGui::EndMenuBar();
		}

		ImGui::Text("Local Address: "); ImGui::SameLine(); ImGui::InputText("1", sAddr, 32);
		ImGui::Text("Local Port: "); ImGui::SameLine(); ImGui::InputText("2", sPort, 32);
		ImGui::Separator();
		ImGui::Text("Remote Address: "); ImGui::SameLine(); ImGui::InputText("3", cAddr, 32);
		ImGui::Text("Remote Port: "); ImGui::SameLine(); ImGui::InputText("4", cPort, 32);

		if (ImGui::Button("Start"))
		{
			inputAddresses = false;
		}

		ImGui::End();

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync
	}


	/* reset all of the client structures */
	for (i = 0; i < MAXCLIENTS; i++)
		clients[i].inuse = 0;


	/* determine the listener address and port */
	bzero(&laddr, sizeof(struct sockaddr_in));
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons((unsigned short)atol(sPort));
	laddr.sin_addr.s_addr = inet_addr(sAddr);

	if (!laddr.sin_port) {
		fprintf(stderr, "invalid listener port\n");
		return 20;
	}
	if (laddr.sin_addr.s_addr == INADDR_NONE) {
		struct hostent* n;
		if ((n = gethostbyname(sAddr)) == NULL) {
			perror("gethostbyname");
			return 20;
		}
		bcopy(n->h_addr, (char*)&laddr.sin_addr, n->h_length);
	}



	/* determine the outgoing address and port */
	bzero(&oaddr, sizeof(struct sockaddr_in));
	oaddr.sin_family = AF_INET;
	oaddr.sin_port = htons((unsigned short)atol(cPort));
	if (!oaddr.sin_port) {
		fprintf(stderr, "invalid target port\n");
		return 25;
	}
	oaddr.sin_addr.s_addr = inet_addr(cAddr);
	if (oaddr.sin_addr.s_addr == INADDR_NONE) {
		struct hostent* n;
		if ((n = gethostbyname(cAddr)) == NULL) {
			perror("gethostbyname");
			return 25;
		}
		bcopy(n->h_addr, (char*)&oaddr.sin_addr, n->h_length);
	}


	/* create the listener socket */
	if ((lsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return 20;
	}
	if (bind(lsock, (struct sockaddr*) & laddr, sizeof(laddr))) {
		perror("bind");
		fprintf(stderr, "%u", WSAGetLastError());
		return 20;
	}
	if (listen(lsock, 5)) {
		perror("listen");
		return 20;
	}


	/* change the port in the listener struct to zero, since we will
	 * use it for binding to outgoing local sockets in the future. */
	laddr.sin_port = htons(0);


	/* fork off into the background. */
#if !defined(__WIN32__) && !defined(WIN32) && !defined(_WIN32)
	if ((i = fork()) == -1) {
		perror("fork");
		return 20;
	}
	if (i > 0)
		return 0;
	setsid();
#endif

	PacketFrame pfC = PacketFrame();
	PacketFrame pfS = PacketFrame();

	std::vector<std::string> packetDisplayBuffer;
	packetDisplayBuffer.resize(64);

	/* main polling loop. */
	while (!isExit)
	{

		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				isExit = true;
		}
		if (isExit)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Packet Stream", &my_tool_active, ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
				if (ImGui::MenuItem("Close", "Ctrl+W")) { my_tool_active = false; }
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// Display contents in a scrolling region
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Packets");
		ImGui::BeginChild("Scrolling");
		for (int n = 0; n < 64; n++)
			ImGui::Text(packetDisplayBuffer[n].c_str(), n);
		ImGui::EndChild();
		ImGui::End();

		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync


		fd_set fdsr;
		int maxsock;
		struct timeval tv = { 1,0 };
		time_t now = time(NULL);

		/* build the list of sockets to check. */
		FD_ZERO(&fdsr);
		FD_SET(lsock, &fdsr);
		maxsock = (int)lsock;
		for (i = 0; i < MAXCLIENTS; i++)
			if (clients[i].inuse) {
				FD_SET(clients[i].csock, &fdsr);
				if ((int)clients[i].csock > maxsock)
					maxsock = (int)clients[i].csock;
				FD_SET(clients[i].osock, &fdsr);
				if ((int)clients[i].osock > maxsock)
					maxsock = (int)clients[i].osock;
			}
		if (select(maxsock + 1, &fdsr, NULL, NULL, &tv) < 0) {
			return 30;
		}


		/* check if there are new connections to accept. */
		if (FD_ISSET(lsock, &fdsr))
		{
			SOCKET csock = accept(lsock, NULL, 0);

			for (i = 0; i < MAXCLIENTS; i++)
				if (!clients[i].inuse) break;
			if (i < MAXCLIENTS)
			{
				/* connect a socket to the outgoing host/port */
				SOCKET osock;
				if ((osock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
					perror("socket");
					closesocket(csock);
				}
				else if (bind(osock, (struct sockaddr*) & laddr, sizeof(laddr))) {
					perror("bind");
					closesocket(csock);
					closesocket(osock);
				}
				else if (connect(osock, (struct sockaddr*) & oaddr, sizeof(oaddr))) {
					perror("connect");
					closesocket(csock);
					closesocket(osock);
				}
				else {
					clients[i].osock = osock;
					clients[i].csock = csock;
					clients[i].activity = now;
					clients[i].inuse = 1;
				}
			}
			else {
				fprintf(stderr, "too many clients\n");
				closesocket(csock);
			}
		}

		cptrOff = 0;
		sptrOff = 0;

		/* service any client connections that have waiting data. */
		for (i = 0; i < MAXCLIENTS; i++)
		{
			int nbyt, closeneeded = 0;
			if (!clients[i].inuse) {
				continue;
			}

			if (FD_ISSET(clients[i].csock, &fdsr)) {
				if ((nbyt = recv(clients[i].csock, buf, 0xFFFF, 0)) <= 0 ||
					send(clients[i].osock, buf, nbyt, 0) <= 0) closeneeded = 1;
				else clients[i].activity = now;

				pfC.read(buf); // Read buffer into packet frame


				RUNSUB::decrypt(&pfC.data[0], 0, pfC.len, encryptionKey); // Decrypt

				std::stringstream packetStringStream;

				//if (pfC.hasSeq)
				//	packetStringStream << std::setfill('0') << std::setw(4) << std::hex << pfC.sequence;


				packetStringStream << std::setfill('0') << std::setw(4) << std::hex << pfC.type;
				packetStringStream << "		";

				for (size_t n = 0; n != pfC.len - 1; ++n)
				{
					packetStringStream << " " << std::setw(2) << std::setfill('0') << std::hex << (int)pfC.data[n];
				}

				packetDisplayBuffer.pop_back();
				packetDisplayBuffer.insert(packetDisplayBuffer.begin(), "Client -> " + packetStringStream.str());

				if (state == NEST_CTOS_HANDSHAKE && pfC.type == 1)
				{
					u_int byondVersion = 0;
					u_int minVersion = 0;

					state = NEST_STOC_HANDSHAKE;

					pfC.hasSeq = true;

					memcpy(&encryptionKey, &pfC.data[8], sizeof(u_int));
					memcpy(&byondVersion, &pfC.data[0], sizeof(u_int));
					memcpy(&minVersion, &pfC.data[4], sizeof(u_int));
					encryptionKey += byondVersion + (minVersion * 0x10000);
				}

				fprintf(stderr, "\n\n");
			}

			if (FD_ISSET(clients[i].osock, &fdsr)) {
				if ((nbyt = recv(clients[i].osock, buf, 0xFFFF, 0)) <= 0 ||
					send(clients[i].csock, buf, nbyt, 0) <= 0) closeneeded = 1;
				else clients[i].activity = now;

				pfS.read(buf);


				RUNSUB::decrypt(&pfS.data[0], 0, pfS.len, encryptionKey);

				std::stringstream packetStringStream;



				packetStringStream << std::setfill('0') << std::setw(4) << std::hex << pfS.type;
				packetStringStream << "		";

				for (size_t n = 0; n != pfS.len - 1; ++n)
				{
					packetStringStream << " " << std::setw(2) << std::setfill('0') << std::hex << (int)pfS.data[n];
				}

				packetDisplayBuffer.pop_back();
				packetDisplayBuffer.insert(packetDisplayBuffer.begin(), "Server -> " + packetStringStream.str());




				if (state == NEST_STOC_HANDSHAKE && pfS.type == 1)
				{
					int ptr = 15;
					while (true) {
						int v;
						memcpy(&v, &buf[ptr], sizeof(int));
						ptr += 4;
						v += 0x71bd632f;
						v &= 0x04008000;
						if (v == 0)
							break;
					}
					int addToKey = 0;
					memcpy(&addToKey, &buf[ptr], sizeof(int));
					encryptionKey += addToKey;
					// right, done
					state = NEST_NORMAL;
				}


				fprintf(stderr, "\n\n");
			}
			else if (now - clients[i].activity > IDLETIMEOUT) {
				closeneeded = 1;
			}
			if (closeneeded) {
				closesocket(clients[i].csock);
				closesocket(clients[i].osock);
				clients[i].inuse = 0;
			}
		}

	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
	}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}