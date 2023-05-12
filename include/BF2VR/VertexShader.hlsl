// VertexShader.hlsl - Shader for converting DirectX framebuffer to OpenXR compatible texture.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
};

VS_OUTPUT VS(uint id : SV_VertexID) {
    VS_OUTPUT output;
    output.TexCoords = float2((id << 1) & 2, id & 2);
    output.Position = float4(output.TexCoords.x * 2 - 1, -output.TexCoords.y * 2 + 1, 0, 1);

    return output;
}