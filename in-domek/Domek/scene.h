#pragma once

#include "texturedEffect.h"
#include "sceneGraph.h"
#include "camera.h"
#include "dxApplication.h"
#include "collision.h"
#include "FW1\FW1FontWrapper.h"
#include "material.h"
#include "constantBuffer.h"
#include "inputLayoutManager.h"
#include <string>

namespace mini
{
	namespace in
	{
		class INScene : public mini::DxApplication
		{
		public:
			explicit INScene(HINSTANCE hInstance,
				int wndWidth = mini::Window::m_defaultWindowWidth,
				int wndHeight = mini::Window::m_defaultWindowHeight,
				std::wstring wndTitle = L"DirectX Window");

			~INScene() override;

		protected:
			void Render() override;
			void Update(const Clock& c) override;
			bool ProcessMessage(mini::WindowMessage& msg) override;

		private:
			//Starts the door opening animation if the door isn't fully open
			void OpenDoor();
			void makeMove(char keyUpDown, char direction);
			void defineMove(int action, int type, int value, int firstValue);
			//Starts the door closing animation if the door isn't fully closed
			void CloseDoor();
			//Toggles between the door opening and closing animation
			void ToggleDoor();
			//Moves the character forward by dx and right by dz in relation to the current camera orientation
			void MoveCharacter(float dx, float dz);
			//Checks if the camera is facing the door
			bool FacingDoor() const;
			//Returns the distance from the character to the door
			float DistanceToDoor() const;

			void InitializeInput();
			void UpdateDoor(float dt);

			void RenderText() const;
			void UpdateModelMatrix(DirectX::XMFLOAT4X4 mtx);

			mini::ConstantBuffer<DirectX::XMFLOAT4X4> m_cbProj;
			mini::ConstantBuffer<DirectX::XMFLOAT4X4> m_cbView;
			mini::ConstantBuffer<DirectX::XMFLOAT4X4, 2> m_cbModel;
			mini::ConstantBuffer<mini::Material::MaterialData> m_cbMaterial;
			TexturedEffect m_texturedEffect;
			std::unique_ptr<mini::SceneGraph> m_sceneGraph;
			mini::InputLayoutManager m_inputLayouts;
			size_t m_attributeID, m_signatureID;

			mini::dx_ptr<IFW1Factory> m_fontFactory;
			mini::dx_ptr<IFW1FontWrapper> m_font;

			mini::FPSCamera m_camera;
			DirectX::XMFLOAT4X4 m_proj;

			unsigned int m_doorNode;
			float m_doorAngle;
			float m_doorAngVel;
			DirectX::XMFLOAT4X4 m_doorTransform;
			CollisionEngine m_collisions;
		};
	}
}
