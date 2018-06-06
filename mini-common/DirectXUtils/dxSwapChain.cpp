#include "dxSwapChain.h"
#include "exceptions.h"

using namespace mini;

dx_ptr<ID3D11Texture2D> DxSwapChain::GetBuffer(unsigned idx) const
{
	ID3D11Texture2D* tmp;
	auto hr = m_swapChain->GetBuffer(idx, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tmp));
	dx_ptr<ID3D11Texture2D> result(tmp);
	if (FAILED(hr))
		THROW_DX(hr);
	return result;
}

bool DxSwapChain::Present(unsigned syncInterval, unsigned flags) const
{
	return SUCCEEDED(m_swapChain->Present(syncInterval, flags));
}
