#include "Variations.h"
struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
};

VS_OUTPUT VS(uint id : SV_VertexID) {
    VS_OUTPUT output;
    output.TexCoords = float2((id << 1) & 2, id & 2);

    float scaleFactor = 1;

#ifdef OCULUSFIX
        scaleFactor = 1.15;
#endif

    // Shift to correct crosseyes
    float shift = (scaleFactor - 1);
    output.Position = float4(output.TexCoords.x * 2 - 1 + shift, -output.TexCoords.y * 2 + 1, 0, 1);

    return output;
}