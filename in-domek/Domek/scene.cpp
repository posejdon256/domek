#include "scene.h"
#include "xfileLoader.h"
#include <sstream>
#include "exceptions.h"
#include <windowsx.h>
#include <direct.h>
#include <dinput.h>
#include <iostream>
#include <fstream>
#include <string>
#include <NuiApi.h>
#include <NuiSensor.h>
#include <NuiImageCamera.h>
#include <NuiSkeleton.h>

using namespace std;
using namespace mini;
using namespace in;
using namespace utils;
using namespace DirectX;


vector<string> elementsI;
bool Keyboard;

bool goW = false;
bool goS = false;
bool goA = false;
bool goD = false;
bool openDoor = false;
bool notAquired = true;

bool turnOnRotation = false;

int mousePositionXPrev = 0;
int mousePositionYPrev = 0;
int mousePositionXNext = 0;
int mousePositionYNext = 0;

LPDIRECTINPUT8 di1;

IDirectInputDevice8* pDevice;
IDirectInputDevice8* pKeyboardI;
IDirectInputDevice8* pMouse;

DIJOYSTATE stateIPrev;
DIJOYSTATE stateI0;

DIMOUSESTATE mouseState;
DIMOUSESTATE mouseStatePrev;
DIMOUSESTATE mouseState0;

BYTE stateKI[256];
BYTE stateKPrevI[256];
BYTE stateK0[256];

const unsigned int GET_STATE_RETRIES = 2;
const unsigned int ACQUIRE_RETRIES = 2;

int frontValue = 0;
int leftValue = 0;

HWND window;
bool callabackWas = false;


INScene::INScene(HINSTANCE hInstance, int wndWidth, int wndHeight, std::wstring wndTitle)
	: DxApplication(hInstance, wndWidth, wndHeight, wndTitle),
	m_cbProj(m_device.CreateConstantBuffer<XMFLOAT4X4>()), m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbModel(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()), m_cbMaterial(m_device.CreateConstantBuffer<Material::MaterialData>()),
	m_inputLayouts(m_device), m_camera(DirectX::XMFLOAT3(0, 1.6f, 0)), m_doorNode(0), m_doorAngle(0), m_doorAngVel(-XM_PIDIV2)
{
	XFileLoader xloader(m_device);
	xloader.Load("house.x");
	m_sceneGraph.reset(new SceneGraph(move(xloader.m_nodes), move(xloader.m_meshes), move(xloader.m_materials)));

	auto vsCode = m_device.LoadByteCode(L"texturedVS.cso");
	auto psCode = m_device.LoadByteCode(L"texturedPS.cso");
	m_signatureID = m_inputLayouts.registerSignatureID(vsCode);
	auto vs = m_device.CreateVertexShader(vsCode);
	auto ps = m_device.CreatePixelShader(psCode);
	D3D11_INPUT_ELEMENT_DESC elem[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	m_attributeID = m_inputLayouts.registerVertexAttributesID(elem);

	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;

	XMStoreFloat4x4(&m_proj, XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), ar, 0.01f, 100.0f));
	m_cbProj.Update(m_device.context(), m_proj);

	m_texturedEffect = TexturedEffect(move(vs), move(ps), m_cbProj, m_cbView, m_cbModel,
		m_cbMaterial, m_device.CreateSamplerState());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_device.context()->IASetInputLayout(m_inputLayouts.getLayout(m_attributeID, m_signatureID).get());

	IFW1Factory *pf;
	auto result = FW1CreateFactory(FW1_VERSION, &pf);
	m_fontFactory.reset(pf);
	if (FAILED(result))
		THROW_WINAPI;
	//IFW1FontWrapper *pfw;
	//MMMMMresult = m_fontFactory->CreateFontWrapper(m_device, L"Calibri", &pfw);
	//MMMMMm_font.reset(pfw);
	//MMMMMif (FAILED(result))
	//MMMMM	THROW_WINAPI;

	m_doorNode = m_sceneGraph->nodeByName("Door");
	m_doorTransform = m_sceneGraph->getNodeTransform(m_doorNode);

	vector<OrientedBoundingRectangle> obstacles;
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, 3.8f), 6.0f, 0.2f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, -4.0f), 6.0f, 0.2f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(2.8f, -3.8f), 0.2f, 7.6f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, -3.8f), 0.2f, 4.85f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, 1.95f), 0.2f, 1.85f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.05f, 1.0f), 0.1f, 1.0f, 0.0f));
	m_collisions.SetObstacles(move(obstacles));

	InitializeInput();

	INuiSensor * pNuiSensor;

	int iSensorCount = 0;
	HRESULT hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			m_pNuiSensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}

	if (NULL != m_pNuiSensor)
	{
		// Initialize the Kinect and specify that we'll be using skeleton
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);
		if (SUCCEEDED(hr))
		{
			// Create an event that will be signaled when skeleton data is available
			m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

			// Open a skeleton stream to receive skeleton data
			hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0);
		}
	}
}
bool INScene::ProcessMessage(WindowMessage& msg)
{
	/*Process windows messages here*/
	/*if message was processed, return true and set value to msg.result*/
	return false;
}
BOOL CALLBACK enumCallback(LPCDIDEVICEINSTANCE instance, LPVOID context)
{
	HRESULT result;
	callabackWas = true;
	// Obtain an interface to the enumerated joystick.
	result = di1->CreateDevice(instance->guidInstance, &pDevice, NULL);

	// If it failed, then we can't use this joystick. (Maybe the user unplugged
	// it while we were in the middle of enumerating it.)
	if (FAILED(result)) {
		THROW_WINAPI;
	}
	result = pDevice->SetDataFormat(&c_dfDIJoystick);
	if (FAILED(result))
		THROW_WINAPI;
	result = pDevice->SetCooperativeLevel(
		window,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		THROW_WINAPI;
	result = pDevice->Acquire();
	if (!FAILED(result))
		notAquired = false;

	// Stop enumeration. Note: we're just taking the first joystick we get. You
	// could store all the enumerated joysticks and let the user pick.
	return DIENUM_STOP;
}
bool GetDeviceState(IDirectInputDevice8* pDevice, unsigned int size, void* ptr, DxDevice m_device)
{
	if (!pDevice) {
		callabackWas = true;
		return true;
	}
	if (notAquired) {
		HRESULT result = pDevice->Acquire();
	}
	if (!m_device)
		return false;
	for (int i = 0; i < GET_STATE_RETRIES; ++i)
	{
		HRESULT result = pDevice->GetDeviceState(size, ptr);
		if (SUCCEEDED(result))
			return true;
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
void INScene::InitializeInput()
{
	ifstream myfile;
	string line;
	myfile.open("elements.txt");
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			elementsI.push_back(line);
		}
		myfile.close();
	}
	if (elementsI.size() == 9) {
		Keyboard = false;
	}
	else {
		Keyboard = true;
	}
	myfile.close();
	HRESULT result;
	window = m_window.getHandle();
	result = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&di1, NULL);
	if (FAILED(result))
		THROW_WINAPI;
	result = di1->EnumDevices(DI8DEVCLASS_GAMECTRL, &enumCallback,
		NULL, DIEDFL_ATTACHEDONLY);
	// Look for the first simple joystick we can find.
	if (FAILED(result)) {
		THROW_WINAPI;
	}
	result = di1->CreateDevice(GUID_SysKeyboard, &pKeyboardI,
		nullptr);
	if (FAILED(result))
		THROW_WINAPI;
	result = pKeyboardI->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
		THROW_WINAPI;
	result = pKeyboardI->SetCooperativeLevel(
		window,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		THROW_WINAPI;
	result = di1->CreateDevice(GUID_SysMouse, &pMouse,
		nullptr);
	if (FAILED(result))
		THROW_WINAPI;
	result = pMouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(result))
		THROW_WINAPI;
	result = pMouse->SetCooperativeLevel(
		window,
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
		THROW_WINAPI;
	GetDeviceState(pKeyboardI, sizeof(BYTE) * 256, stateK0, m_device);
	GetDeviceState(pDevice, sizeof(DIJOYSTATE), &stateIPrev, m_device);
	stateI0 = stateIPrev;
	/*Initialize Direct Input here*/

}
INScene::~INScene()
{
	if (pDevice) {
		pDevice->Unacquire();
		pDevice->Release();
	}

	di1->Release();
	/*Release Direct Input resources here*/

	if (m_pNuiSensor)
	{
		m_pNuiSensor->NuiShutdown();
	}

	if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
	{
		CloseHandle(m_hNextSkeletonEvent);
	}
	if (m_pNuiSensor) m_pNuiSensor->Release();
}
int elementsIContains(string element) {
	for (int i = 0; i < elementsI.size(); i++) {
		if (elementsI[i] == element) {
			return i;
		}
	}
	return -1;
}
void INScene::makeMove(char keyUpDown, char direction) {

	//goW = false;

	switch (keyUpDown)
	{
	case 'D':
		if (direction == 'W')
			goW = true;
		else if (direction == 'S')
			goS = true;
		else if (direction == 'A')
			goA = true;
		else if (direction == 'D')
			goD = true;
		else if (direction == 'E' && FacingDoor())
			openDoor = true;
		break;
	case 'U':
		if (direction == 'W')
			goW = false;
		else if (direction == 'S')
			goS = false;
		else if (direction == 'A')
			goA = false;
		else if (direction == 'D')
			goD = false;
		break;
	case 'K'://kochanie
		turnOnRotation = false;
		mousePositionXPrev = 0;
		mousePositionYPrev = 0;
		mousePositionXNext = 0;
		mousePositionYNext = 0;
		break;
	case 'S'://start
		turnOnRotation = true;
		break;
	default:
		break;

	}
}
void INScene::defineMove(int action, int type, int value, int firstValue) {
	if (value == firstValue) {
		if (action == 4 || action == 5 || action == 6 || action == 7) {
			makeMove('K', ' ');
		}
		if (type == 1) {
			if ((action == 0 || action == 1)) {
				makeMove('U', 'D');
				makeMove('U', 'A');
			}
			else if ((action == 2 || action == 3)) {
				makeMove('U', 'W');
				makeMove('U', 'S');
			}
		}
		else if (type == 0) {
			if (action == 0) {
				makeMove('U', 'D');
			}
			else if (action == 1) {
				makeMove('U', 'A');
			}
			else if (action == 2) {
				makeMove('U', 'W');
			}
			else if (action == 3) {
				makeMove('U', 'S');
			}

		}
	}
	else {
		if (type == 1) {
			if ((action == 0 || action == 1) && firstValue < value) {
				makeMove('D', 'D');
			}
			else if ((action == 0 || action == 1) && firstValue > value) {
				makeMove('D', 'A');
			}
			else if ((action == 2 || action == 3) && firstValue > value) {
				makeMove('D', 'W');
			}
			else if ((action == 2 || action == 3) && firstValue < value) {
				makeMove('D', 'S');
			}
			else if (action == 4 || action == 5) {
				if (!turnOnRotation) {
					makeMove('S', ' ');
					mousePositionXPrev = firstValue;
				} else
					mousePositionXNext = value;
			}
			else if (action == 6 || action == 7) {
				if (!turnOnRotation) {
					makeMove('S', ' ');
					mousePositionYPrev = firstValue;
				} else
					mousePositionYNext = value;
			}
		} 
		else if (type == 0) {
			if (action == 0) {
				makeMove('D', 'D');
			}
			else if (action == 1) {
				makeMove('D', 'A');
			}
			else if (action == 2) {
				makeMove('D', 'W');
			}
			else if (action == 3) {
				makeMove('D', 'S');
			}
			else if (action == 4 || action == 5 || action == 6 || action == 7) {
				if (!turnOnRotation) {
					makeMove('S', ' ');
					mousePositionXPrev = 0;
				}
				if (action == 4)
					mousePositionXNext = 30000;
				else if (action == 5)
					mousePositionXNext = -30000;
				else if (action == 6)
					mousePositionYNext = -30000;
				else if (action == 7)
					mousePositionYNext = 30000;
			}
		}
	}
	if (action == 8 && FacingDoor()) {
		OpenDoor();
	}

}
void INScene::Update(const Clock& c)
{
	if (!callabackWas)
		return;
	/*proccess Direct Input here*/
	if (notAquired && pDevice) {
		HRESULT result = pDevice->Acquire();
		if (!FAILED(result)) {
			notAquired = true;
			GetDeviceState(pDevice, sizeof(DIJOYSTATE), &stateI0, m_device);
			GetDeviceState(pMouse, sizeof(DIMOUSESTATE), &mouseState0, m_device);
			GetDeviceState(pKeyboardI, sizeof(BYTE) * 256, stateK0, m_device);
			notAquired = false;
		}
		else {
			return;
		}

	}
	if (!Keyboard) {
		DIJOYSTATE state;
		if (GetDeviceState(pDevice, sizeof(DIJOYSTATE), &state, m_device))
		{
			if (state.lRx != stateIPrev.lRx && elementsIContains("lRx") != -1) {
				defineMove(elementsIContains("lRx"), 1, state.lRx, stateI0.lRx);
			}
			if (state.lRy != stateIPrev.lRy && elementsIContains("lRy") != -1) {
				defineMove(elementsIContains("lRy"), 1, state.lRy, stateI0.lRy);
			}
			if (state.lRz != stateIPrev.lRz && elementsIContains("lRz") != -1) {
				defineMove(elementsIContains("lRz"), 1, state.lRz, stateI0.lRz);
			}
			if (state.lX != stateIPrev.lX && elementsIContains("lX") != -1) {
				defineMove(elementsIContains("lX"), 1, state.lX, stateI0.lX);
			}
			if (state.lY != stateIPrev.lY && elementsIContains("lY") != -1) {
				defineMove(elementsIContains("lY"), 1, state.lY, stateI0.lY);
			}
			if (state.lZ != stateIPrev.lZ && elementsIContains("lZ") != -1) {
				defineMove(elementsIContains("lZ"), 1, state.lZ, stateI0.lZ);
			}
			for (int i = 0; i < (sizeof(state.rgbButtons) / sizeof(*state.rgbButtons)); i++)
			{
				if (state.rgbButtons[i] != stateIPrev.rgbButtons[i] && elementsIContains("rgbButtons-" + to_string(i)) != -1) {
					defineMove(elementsIContains("rgbButtons-" + to_string(i)), 0, state.rgbButtons[i], stateI0.rgbButtons[i]);
				}
			}
			for (int i = 0; i < (sizeof(state.rgdwPOV) / sizeof(*state.rgdwPOV)); i++) {
				if (state.rgdwPOV[i] != stateIPrev.rgdwPOV[i] && elementsIContains("rgdwPOV-" + to_string(i)) != -1) {
					defineMove(elementsIContains("rgdwPOV-" + to_string(i)), 0, state.rgdwPOV[i], stateI0.rgdwPOV[i]);
				}
			}
			for (int i = 0; i < (sizeof(state.rglSlider) / sizeof(*state.rglSlider)); i++) {
				if (state.rglSlider[i] != stateIPrev.rglSlider[i] && elementsIContains("rglSlider-" + to_string(i)) != -1) {
					defineMove(elementsIContains("rglSlider-" + to_string(i)), 0, state.rglSlider[i], stateI0.rglSlider[i]);
				}
			}
			stateIPrev = state;
		}
	}
	else {
		if (GetDeviceState(pKeyboardI, sizeof(BYTE) * 256, stateKI, m_device))
		{
			for (int i = 0; i < 256; i++) {
				if (stateKI[i] != stateKPrevI[i] && elementsIContains(to_string(i)) != -1) {
					int type = elementsIContains(to_string(i));
					if (type == 4)
						type = 8;
					defineMove(type, 0, stateKI[i], stateK0[i]);
				}
				stateKPrevI[i] = stateKI[i];
			}
					
		}
		if (GetDeviceState(pMouse, sizeof(DIMOUSESTATE), &mouseState, m_device)) {
			if (mouseState.lX != mouseStatePrev.lX && mouseState.rgbButtons[0] != 0) {
				defineMove(4, 1, mouseState.lX*2000, mouseState0.lX);
			}
			if (mouseState.lY != mouseStatePrev.lY && mouseState.rgbButtons[0] != 0) {
				defineMove(6, 1, mouseState.lY*2000, mouseState0.lY);
			}
			if (mouseState.rgbButtons[0] != mouseStatePrev.rgbButtons[0] && mouseState.rgbButtons[0] == 0) {
				makeMove('K', ' ');
			}
			mouseStatePrev = mouseState;
		}
	}
	const int dist = 1;
	if (goW)
		MoveCharacter(0, dist*c.getFrameTime());
	if (goS)
		MoveCharacter(0, -dist*c.getFrameTime());
	if (goA)
		MoveCharacter(-dist*c.getFrameTime(), 0);
	if (goD)
		MoveCharacter(dist*c.getFrameTime(), 0);
	else if (turnOnRotation) {
		m_camera.Rotate(((mousePositionYNext - mousePositionYPrev)*0.000000005 /c.getFrameTime()), (mousePositionXNext - mousePositionXPrev)*0.00000005 / c.getFrameTime());
	}
	if (openDoor) {
		openDoor = false;
		ToggleDoor();
	}

	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0))
	{
		ProcessSkeleton(c);
	}

	XMFLOAT4X4 viewMtx;
	XMStoreFloat4x4(&viewMtx, m_camera.getViewMatrix());
	m_cbView.Update(m_device.context(), viewMtx);

	UpdateDoor(static_cast<float>(c.getFrameTime()));
}

void INScene::ProcessSkeleton(const Clock& c)
{
	NUI_SKELETON_FRAME skeletonFrame = { 0 };

	HRESULT hr = m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame);
	if (FAILED(hr))
	{
		return;
	}

	// smooth out the skeleton data
	m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

	for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
	{
		NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;

		if (NUI_SKELETON_TRACKED == trackingState)
		{
			auto wl = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_WRIST_LEFT];
			auto el = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT];
			auto sl = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT];

			auto wr = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_WRIST_RIGHT];
			auto er = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT];
			auto sr = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT];

			float speed = 10;

			if (wl.y > sl.y) MoveCharacter(0, speed * c.getFrameTime());
			if (wl.y < el.y) MoveCharacter(0, speed * -c.getFrameTime());

			if (wr.y > sr.y) MoveCharacter(speed * c.getFrameTime(), 0);
			if (wr.y < er.y) MoveCharacter(speed *-c.getFrameTime(), 0);


			auto hd = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD];
			auto sc = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];

			float dz = hd.z - sc.z;
	
			float dx = hd.x - sc.x;
			dz += 0.05;
			if (abs(dz) < 0.05) dz = 0;
			if (abs(dx) < 0.05) dx = 0;

			dx *= 0.3;
			dz *= 0.3;


			m_camera.Rotate(-dz, dx);
		}
	}
}

void INScene::RenderText() const
{
	wstringstream str;
	str << L"FPS: " << getClock().getFPS();
	m_font->DrawString(m_device.context().get(), str.str().c_str(), 20.0f, 10.0f, 10.0f, 0xff0099ff,
		FW1_RESTORESTATE | FW1_NOGEOMETRYSHADER);
	if (DistanceToDoor() < 1.0f && FacingDoor())
	{
		wstring prompt(L"(E) Otwórz/Zamknij");
		FW1_RECTF layout;
		auto rect = m_font->MeasureString(prompt.c_str(), L"Calibri", 20.0f, &layout, FW1_NOWORDWRAP);
		auto width = rect.Right - rect.Left;
		auto height = rect.Bottom - rect.Top;
		auto clSize = m_window.getClientSize();
		m_font->DrawString(m_device.context().get(), prompt.c_str(), 20.0f, (clSize.cx - width) / 2, (clSize.cy - height) / 2,
			0xff00ff99, FW1_RESTORESTATE | FW1_NOGEOMETRYSHADER);
	}
}
void INScene::UpdateModelMatrix(DirectX::XMFLOAT4X4 mtx)
{
	XMFLOAT4X4 m[2] = { mtx };
	XMVECTOR det;
	XMStoreFloat4x4(&m[1], XMMatrixTranspose(XMMatrixInverse(&det, XMLoadFloat4x4(&m[0]))));
	m_cbModel.Update(m_device.context(), m);
}
void INScene::Render()
{
	DxApplication::Render();
	for (unsigned int i = 0; i < m_sceneGraph->meshCount(); ++i)
	{
		auto& m = m_sceneGraph->getMesh(i);
		auto& material = m_sceneGraph->getMeshMaterial(i);
		if (!material.getDiffuseTexture())
			continue;
		m_cbMaterial.Update(m_device.context(), material.getMaterialData());
		m_texturedEffect.SetDiffuseMap(material.getDiffuseTexture());
		m_texturedEffect.SetSpecularMap(material.getSpecularTexture());
		m_texturedEffect.Begin(m_device.context());
		UpdateModelMatrix(m.getTransform());
		m.Render(m_device.context());
	}
	//MMMMRenderText();
}
void INScene::OpenDoor()
{
	if (m_doorAngVel < 0)
		m_doorAngVel = -m_doorAngVel;
}
void INScene::CloseDoor()
{
	if (m_doorAngVel > 0)
		m_doorAngVel = -m_doorAngVel;
}
void INScene::ToggleDoor()
{
	m_doorAngVel = -m_doorAngVel;
}
void INScene::UpdateDoor(float dt)
{
	if ((m_doorAngVel > 0 && m_doorAngle < XM_PIDIV2) || (m_doorAngVel < 0 && m_doorAngle > 0))
	{
		m_doorAngle += dt*m_doorAngVel;
		if (m_doorAngle < 0)
			m_doorAngle = 0;
		else if (m_doorAngle > XM_PIDIV2)
			m_doorAngle = XM_PIDIV2;
		XMFLOAT4X4 doorTransform;
		auto mtx = XMLoadFloat4x4(&m_doorTransform);
		auto v = XMVectorSet(0.000004f, 0.0f, -1.08108f, 1.0f);
		v = XMVector3TransformCoord(v, mtx);
		XMStoreFloat4x4(&doorTransform, mtx * XMMatrixTranslationFromVector(-v) * XMMatrixRotationZ(m_doorAngle) *
			XMMatrixTranslationFromVector(v));
		m_sceneGraph->setNodeTransform(m_doorNode, doorTransform);
		auto tr = m_collisions.MoveObstacle(5, OrientedBoundingRectangle(XMFLOAT2(-3.05f, 1.0f), 0.1f, 1.0f,
			m_doorAngle));
		m_camera.MoveTarget(XMFLOAT3(tr.x, 0, tr.y));
	}
}
void INScene::MoveCharacter(float dx, float dz)
{
	auto forward = m_camera.getForwardDir();
	auto right = m_camera.getRightDir();
	XMFLOAT3 temp;
	XMStoreFloat3(&temp, forward*dz + right*dx);
	auto tr = XMFLOAT2(temp.x, temp.z);
	m_collisions.MoveCharacter(tr);
	m_camera.MoveTarget(XMFLOAT3(tr.x, 0, tr.y));
}
bool INScene::FacingDoor() const
{
	auto rect = m_collisions.getObstacle(5);
	XMFLOAT2 p[4] = { rect.getP1(), rect.getP2(), rect.getP3(), rect.getP4() };
	XMVECTOR points[4] = { XMLoadFloat2(p), XMLoadFloat2(p + 1), XMLoadFloat2(p + 2), XMLoadFloat2(p + 3) };
	auto forward = XMVectorSwizzle(m_camera.getForwardDir(), 0, 2, 1, 3);
	XMFLOAT4 target = m_camera.getTarget();
	auto camera = XMVectorSwizzle(XMLoadFloat4(&target), 0, 2, 1, 3);
	unsigned int max_i = 0, max_j = 0;
	auto max_a = 0.0f;
	for (unsigned int i = 0; i < 4; ++i)
		for (auto j = i + 1; j < 4; ++j)
		{
			auto a = XMVector2AngleBetweenVectors(points[i] - camera, points[j] - camera).m128_f32[0];
			if (a > max_a)
			{
				max_i = i;
				max_j = j;
				max_a = a;
			}
		}
	return XMScalarNearEqual(XMVector2AngleBetweenVectors(forward, points[max_i] - camera).m128_f32[0]
		+ XMVector2AngleBetweenVectors(forward, points[max_j] - camera).m128_f32[0], max_a, 0.001f);
}
float INScene::DistanceToDoor() const
{
	return m_collisions.DistanceToObstacle(5);
}