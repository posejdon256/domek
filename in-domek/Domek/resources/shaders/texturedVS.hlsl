cbuffer projBuffer : register(b0)
{
	matrix proj;
}

cbuffer viewBuffer : register(b1)
{
	matrix view;
}

cbuffer modelBuffer : register(b2)
{
	matrix model;
	matrix modelit;
}

struct VS_INPUT
{
	float3 pos : POSITION0;
	float3 norm : NORMAL0;
	float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float2 tex : TEXCOORD0;
	float3 viewVec : TEXCOORD1;
	float3 lightVec1 : TEXCOORD2;
	float3 lightVec2 : TEXCOORD3;
};

static const float4 lightpos1 = float4(0, 2.2f, 0, 1.0f);
static const float4 lightpos2 = float4(-50.0f, 100.0f, 50.0f, 1.0f);

PS_INPUT main(VS_INPUT i)
{
	PS_INPUT o = (PS_INPUT)0;
	float4 viewPos = float4(i.pos, 1.0f);
	viewPos = mul(model, viewPos);
	viewPos = mul(view, viewPos);
	o.pos = mul(proj, viewPos);
	o.norm = normalize(mul(view, mul(model, float4(i.norm, 0.0f))).xyz);
	o.viewVec = normalize(-viewPos.xyz);
	o.lightVec1 = normalize((mul(view, lightpos1) - viewPos).xyz);
	o.lightVec2 = normalize((mul(view, lightpos2) - viewPos).xyz);
	o.tex = i.tex;
	return o;
}

