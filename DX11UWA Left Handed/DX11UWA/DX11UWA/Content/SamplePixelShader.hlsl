// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 normal : NORMAL;
};

texture2D baseTexture : register(t0); // first texture

texture2D detailTexture : register(t1); // second texture

SamplerState filters[2] : register(s0); // filter 0 using CLAMP, filter 1 using WRAP

										// Pixel shader performing multi-texturing with a detail texture on a second UV channel
										// A simple optimization would be to pack both UV sets into a single register
float4 main(PixelShaderInput input) : SV_TARGET
{		 //(float2 baseUV : TEXCOORD0, float2 detailUV : TEXCOORD1, float4 modulate : COLOR) : SV_TARGET
	//float4 baseColor = baseTexture.Sample(filters[0], baseUV) * modulate; // get base color
	//float4 detailColor = detailTexture.Sample(filters[1], detailUV); // get detail effect
	//float4 finalColor = float4(lerp(baseColor.rgb, detailColor.rgb, detailColor.a), baseColor.a);
	//return finalColor; // return a transition based on the detail alpha
	float4 tmp = baseTexture.Sample(filters[1],input.uv);
	if (tmp.a < 0.25f)
	{
		discard;
	}


	return tmp;
}


//// A pass-through function for the (interpolated) color data.
//float4 main(PixelShaderInput input) : SV_TARGET
//{
//	return float4(input.uv, 1.0f);
//}
