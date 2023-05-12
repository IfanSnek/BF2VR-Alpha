// PixelShader.hlsl - Shader for converting DirectX framebuffer to OpenXR compatible texture.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

SamplerState s0;
Texture2D tex0;

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
};

float4 PS(PS_INPUT input) : SV_Target0
{
    return tex0.Sample(s0, input.TexCoords);
}