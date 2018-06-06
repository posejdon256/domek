Texture2D diffuseMap : register(t0);
Texture2D specularMap : register(t1);
SamplerState texSampler : register(s0); 

cbuffer materialBuffer : register(b0)
{
	struct Material
	{
		float4 diffuseColor;
		float4 specular;
	} material;
}

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD0;
	float3 viewVec : TEXCOORD1;
	float3 lightVec1 : TEXCOORD2;
	float3 lightVec2 : TEXCOORD3;
};

static const float ka = 0.2;
static const float3 light1Color = float3(0.8f, 0.76f, 0.54f);
static const float3 light2Color = float3(0.8f, 0.8f, 0.8f);

float4 main(PS_INPUT i) : SV_TARGET
{
	float3 viewVec = normalize(i.viewVec);
	float3 normal = normalize(i.norm);
	float3 lightVec = normalize(i.lightVec1);
	float3 halfVec = normalize(viewVec + lightVec);
	float4 diffuseColor = diffuseMap.Sample(texSampler, i.tex);
	float4 specularColor = float4(specularMap.Sample(texSampler, i.tex).xyz , material.specular.a);
	float3 color = diffuseColor.xyz*ka;
	color += light1Color*diffuseColor.xyz * saturate(dot(normal, lightVec)) +
		light1Color*specularColor.xyz * pow(saturate(dot(normal, halfVec)), specularColor.a);
	lightVec = normalize(i.lightVec2);
	halfVec = normalize(viewVec + lightVec);
	color += light2Color*diffuseColor.xyz * saturate(dot(normal, lightVec)) +
		light2Color*specularColor.xyz * pow(saturate(dot(normal, halfVec)), specularColor.a);
	return float4(saturate(color), diffuseColor.a);
}