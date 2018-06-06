#include "texturedEffect.h"
#include <dxptr.h>

using namespace std;
using namespace mini;
using namespace in;
using namespace DirectX;


TexturedEffect::TexturedEffect(dx_ptr<ID3D11VertexShader>&& vs, dx_ptr<ID3D11PixelShader>&& ps,
							   const ConstantBuffer<XMFLOAT4X4>& cbProj,
							   const ConstantBuffer<XMFLOAT4X4>& cbView,
							   const ConstantBuffer<XMFLOAT4X4, 2>& cbModel,
							   const ConstantBuffer<Material::MaterialData>& cbMaterial,
							   dx_ptr<ID3D11SamplerState>&& sampler)
	: StaticEffect(BasicEffect(move(vs), move(ps)), VSConstantBuffers{cbProj, cbView, cbModel}, PSConstantBuffers{cbMaterial}, PSSamplers(sampler), PSShaderResources{})
{ 
}