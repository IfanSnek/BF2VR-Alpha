cbuffer EyeBuffer : register(b0) {
    bool leftEye;
}

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
};

VS_OUTPUT VS(uint id : SV_VertexID) {
    VS_OUTPUT output;
    output.TexCoords = float2((id << 1) & 2, id & 2);

    float scaleFactor = 1.25;

    // Right eye from top right
    float shift = (scaleFactor - 1) * output.TexCoords.x;
    output.Position = float4(output.TexCoords.x * 2 - 1 + shift, -output.TexCoords.y * 2 + 1, 0, 1);

    return output;
}