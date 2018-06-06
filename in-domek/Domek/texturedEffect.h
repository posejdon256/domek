#pragma once

#include "effect.h"
#include "constantBuffer.h"
#include "material.h"

namespace mini
{
	namespace in
	{
		class TexturedEffect : public mini::StaticEffect<mini::BasicEffect, mini::VSConstantBuffers, mini::PSConstantBuffers, mini::PSSamplers, mini::PSShaderResources>
		{
		public:
			enum VSConstanbBufferSlots
			{
				ProjMtx,
				ViewMtx,
				WorldMtx
			};

			enum PSConstantBufferSlots
			{
				Material
			};

			enum PSSamplerSlots
			{
				Sampler
			};

			enum PSTextureSlots
			{
				DiffuseMap,
				SpecularMap
			};

			TexturedEffect() = default;

			TexturedEffect(mini::dx_ptr<ID3D11VertexShader>&& vs,
				mini::dx_ptr<ID3D11PixelShader>&& ps,
				const mini::ConstantBuffer<DirectX::XMFLOAT4X4>& cbProj,
				const mini::ConstantBuffer<DirectX::XMFLOAT4X4>& cbView,
				const mini::ConstantBuffer<DirectX::XMFLOAT4X4, 2>& cbModel,
				const mini::ConstantBuffer<mini::Material::MaterialData>& cbMaterial,
				dx_ptr<ID3D11SamplerState>&& sampler
			);

			TexturedEffect& operator=(TexturedEffect&& other) = default;

			void SetProjMtxBuffer(const ConstantBuffer<DirectX::XMFLOAT4X4>& buffer) { SetVSConstantBuffer(ProjMtx, buffer); }
			void SetViewMtxBuffer(const ConstantBuffer<DirectX::XMFLOAT4X4>& buffer) { SetVSConstantBuffer(ViewMtx, buffer); }
			void SetWorldMtxBuffer(const ConstantBuffer<DirectX::XMFLOAT4X4, 2>& buffer) { SetVSConstantBuffer(WorldMtx, buffer); }
			void SetMaterialBuffer(const ConstantBuffer<Material::MaterialData>& buffer) { SetPSConstantBuffer(Material, buffer); }
			void SetSampler(dx_ptr<ID3D11SamplerState>&& sampler) { SetPSSampler(Sampler, std::move(sampler)); }
			void SetDiffuseMap(const dx_ptr<ID3D11ShaderResourceView>& texture) { SetPSShaderResource(DiffuseMap, texture); }
			void SetSpecularMap(const dx_ptr<ID3D11ShaderResourceView>& texture) { SetPSShaderResource(SpecularMap, texture); }

		private:
			mini::ConstantBuffer<DirectX::XMFLOAT4X4> m_cbProj;
			mini::ConstantBuffer<DirectX::XMFLOAT4X4> m_cbView;
			mini::ConstantBuffer<DirectX::XMFLOAT4X4, 2> m_cbModel;
			mini::ConstantBuffer<mini::Material::MaterialData> m_cbMaterial;
		};
	}
}