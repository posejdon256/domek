#pragma once

#include "windowApplication.h"
#include "dxDevice.h"
#include "clock.h"
#include "effect.h"

namespace mini
{
	class DxApplication : public WindowApplication
	{
	public:
		explicit DxApplication(HINSTANCE hInstance,
			int wndWidth = Window::m_defaultWindowWidth,
			int wndHeight = Window::m_defaultWindowHeight,
			std::wstring wndTitle = L"DirectX Window");

	protected:
		int MainLoop() override;

		virtual void Update(const Clock& c) { }
		virtual void Render();

		const RenderTargetsEffect& getDefaultRenderTarget() const { return m_renderTarget; }
		const Clock& getClock() const { return m_clock; }

		DxDevice m_device;

	private:
		RenderTargetsEffect m_renderTarget;
		Clock m_clock;
	};
}
