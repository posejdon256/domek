#include "DataWindow.h"
#include "xfileLoader.h"
#include <sstream>
#include "exceptions.h"
#include <windowsx.h>
#include <direct.h>
#include <dinput.h>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace mini;
using namespace in;
using namespace utils;
using namespace DirectX;

bool callbackWasD = false;
bool notAquiredD = true;

MSG KomunikatD;

LPDIRECTINPUT8 di;

IDirectInputDevice8* pDeviceD;
IDirectInputDevice8* pKeybordD;

DIJOYSTATE statePrev;
DIJOYSTATE stateD;

BYTE stateK[256];
BYTE stateKPrev[256];

const unsigned int GET_STATE_RETRIES = 2;
const unsigned int ACQUIRE_RETRIES = 2;

int level = 0;
int option = -1;

HINSTANCE hInstanceD;

HWND hwnd;
HWND button1;
HWND combobox1;
HWND label1;

vector<string> elements;
char  szName[MAX_PATH];

void ShowControls(string elementNumer = "") {
	if (level == 1 && elementNumer != "First") {
		return;
	}
	if (elementNumer != "" && elementNumer != "First") {
		elements.push_back(elementNumer);
	}
	switch (level)
	{
	case 0:
		SetWindowText(label1, L"Wybierz sposób kontroli");
		SendMessage(combobox1, CB_ADDSTRING, 0, (LPARAM)L"Mysz i klawiatura");
		SendMessage(combobox1, CB_ADDSTRING, 0, (LPARAM)L"Joystick");
		break;
	case 1:
		if (option != 0)
			option = ComboBox_GetCurSel(combobox1);
		if (FAILED(option))
			THROW_WINAPI;
		DestroyWindow(button1);
		DestroyWindow(combobox1);
		SetWindowText(label1, L"Prawo");
		break;
	case 2:
		SetWindowText(label1, L"Lewo");
		break;
	case 3:
		SetWindowText(label1, L"Przód");
		break;
	case 4:
		SetWindowText(label1, L"Ty³");
		if (option == 0)
			level += 4;
		break;
	case 5:
		SetWindowText(label1, L"Skrêt prawo");
		break;
	case 6:
		SetWindowText(label1, L"Skrêt lewo");
		break;
	case 7:
		SetWindowText(label1, L"Skrêt góra");
		break;
	case 8:
		SetWindowText(label1, L"Skrêt dó³");
		break;
	case 9:
		SetWindowText(label1, L"Otwieranie drzwi");
		break;
	default:
		break;
	}
	level++;
}
bool GetDeviceStateD(IDirectInputDevice8* pDevice, unsigned int size, void* ptr)
{
	if (notAquiredD) {
		if (!pDevice) {
			option = 0;
			ShowControls("First");
			return true;
		}
		HRESULT result = pDevice->Acquire();
		HRESULT result1 = pKeybordD->Acquire();
		if (!FAILED(result) && !FAILED(result1)) {
			notAquiredD = false;
		}
		else {
			return false;
		}
	}
	for (int i = 0; i < GET_STATE_RETRIES; ++i)
	{
		HRESULT result = pDevice->GetDeviceState(size, ptr);
		if (SUCCEEDED(result))
			return true;
		if (result == E_ACCESSDENIED)
			return false;
		if (result != DIERR_INPUTLOST &&
			result != DIERR_NOTACQUIRED)
			THROW_WINAPI;
		for (int j = 0; j < ACQUIRE_RETRIES; ++j)
		{
			result = pDevice->Acquire();
			if (SUCCEEDED(result))
				break;
			if (result != DIERR_INPUTLOST &&
				result != E_ACCESSDENIED)
				THROW_WINAPI;
		}
	}
	return false;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		if ((HWND)lParam == button1)
			ShowControls("First");
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}
BOOL CALLBACK enumCallbackD(LPCDIDEVICEINSTANCE instance, LPVOID context)
{
	HRESULT result;
	callbackWasD = true;
	// Obtain an interface to the enumerated joystick.
	result = di->CreateDevice(instance->guidInstance, &pDeviceD, NULL);

	// If it failed, then we can't use this joystick. (Maybe the user unplugged
	// it while we were in the middle of enumerating it.)
	if (FAILED(result)) {
		THROW_WINAPI;
	}
	result = pDeviceD->SetDataFormat(&c_dfDIJoystick);
	if (FAILED(result))
		THROW_WINAPI;
	result = pDeviceD->SetCooperativeLevel(
		hwnd,
		DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		THROW_WINAPI;
	//joySetCapture(hwnd, JOYSTICKID1, NULL, false);

	// Stop enumeration. Note: we're just taking the first joystick we get. You
	// could store all the enumerated joysticks and let the user pick.
	return DIENUM_STOP;
}
DataWindow::DataWindow(HINSTANCE hInstance, int cmdShow)
{
	LPCWSTR NazwaKlasy = L"Klasa Okienka";
	hInstanceD = hInstance;

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = NazwaKlasy;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Wysoka Komisja odmawia rejestracji tego okna!", L"Niestety...",
			MB_ICONEXCLAMATION | MB_OK);
		THROW_WINAPI;
	}
	HRESULT result;

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, NazwaKlasy, L"Zbieranie danych", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, hInstance, NULL);
	result = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
	IID_IDirectInput8, (VOID**)&di, NULL);
	ShowWindow(hwnd, cmdShow);
	UpdateWindow(hwnd);
	result = di->EnumDevices(DI8DEVCLASS_GAMECTRL, &enumCallbackD,
		NULL, DIEDFL_ATTACHEDONLY);
	if (FAILED(result))
		THROW_WINAPI;
	button1 = CreateWindowEx(0, L"BUTTON", L"PotwierdŸ", WS_CHILD | WS_VISIBLE,
		60, 150, 150, 30, hwnd, NULL, hInstanceD, NULL);
	label1 = CreateWindowEx(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE |
		SS_LEFT, 60, 60, 120, 50, hwnd, NULL, hInstanceD, NULL);
	combobox1 = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
		CBS_DROPDOWN, 60, 120, 150, 200, hwnd, NULL, hInstanceD, NULL);
	result = di->CreateDevice(GUID_SysKeyboard, &pKeybordD,
		nullptr);
	if (FAILED(result))
		THROW_WINAPI;
	result = pKeybordD->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
		THROW_WINAPI;
	result = pKeybordD->SetCooperativeLevel(
		hwnd,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		THROW_WINAPI;
	ShowControls();
	GetDeviceStateD(pDeviceD, sizeof(DIJOYSTATE), &stateD);
	GetDeviceStateD(pKeybordD, sizeof(BYTE) * 256, stateKPrev);
	MSG msg = { nullptr };
	bool statePushed = false;
	bool stateStillPushed = false;
	statePrev = stateD;
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else if(option == 1)
		{
			GetDeviceStateD(pDeviceD, sizeof(DIJOYSTATE), &stateD);
			if (stateD.lRx != statePrev.lRx)
				if (!statePushed) {
					ShowControls("lRx");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			else if (stateD.lRy != statePrev.lRy)
				if (!statePushed){
					ShowControls("lRy");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			else if (stateD.lRz != statePrev.lRz)
				if (!statePushed) {
					ShowControls("lRz");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			else if (stateD.lX != statePrev.lX)
				if (!statePushed) {
					ShowControls("lX");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			else if (stateD.lY != statePrev.lY)
				if (!statePushed) {
					ShowControls("lY");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			else if (stateD.lZ != statePrev.lZ)
				if (!statePushed) {
					ShowControls("lZ");
					statePushed = true;
					stateStillPushed = true;
				}
				else
					stateStillPushed = true;
			for (int i = 0; i < (sizeof(stateD.rgbButtons) / sizeof(*stateD.rgbButtons)); i++)
				if (stateD.rgbButtons[i] != statePrev.rgbButtons[i])
					if (!statePushed) {
						ShowControls("rgbButtons-" + to_string(i));
						statePushed = true;
						stateStillPushed = true;
					}
					else
						stateStillPushed = true;
			for (int i = 0; i < (sizeof(stateD.rgdwPOV) / sizeof(*stateD.rgdwPOV)); i++)
				if (stateD.rgdwPOV[i] != statePrev.rgdwPOV[i])
					if (!statePushed) {
						ShowControls("rgdwPOV-" + to_string(i));
						statePushed = true;
						stateStillPushed = true;
					}
					else
						stateStillPushed = true;
			for (int i = 0; i < (sizeof(stateD.rglSlider) / sizeof(*stateD.rglSlider)); i++)
				if (stateD.rglSlider[i] != statePrev.rglSlider[i])
					if (!statePushed) {
						ShowControls("rglSlider-" + to_string(i));
						statePushed = true;
						stateStillPushed = true;
					}
					else
						stateStillPushed = true;
			if (!stateStillPushed)
				statePushed = false;
			stateStillPushed = false;
		}
		else if (option == 0) {
			if (GetDeviceStateD(pKeybordD, sizeof(BYTE) * 256, stateK))
			{
				for (int i = 0; i < 256; i++) {
					if (stateK[i] != stateKPrev[i])
						if (!statePushed) {
							ShowControls(to_string(i));
							statePushed = true;
							stateStillPushed = true;
						}
						else
							stateStillPushed = true;
				}
				if (!stateStillPushed)
					statePushed = false;
				stateStillPushed = false;
			}
		}
		if (level == 11) {
			break;
		}

	}
	ofstream myfile;
	myfile.open("elements.txt");
	for (int i = 0; i < elements.size(); i++) {
		myfile << elements[i] + "\n";
	}
	myfile.close();

}

