#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_VertexID              0   x           0   VERTID    uint   x   
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Position              0   xyzw        0      POS   float   xyzw
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
vs_4_0
dcl_input_sgv v0.x, vertex_id
dcl_output_siv o0.xyzw, position
dcl_output o1.xy
dcl_temps 1
ishl r0.x, v0.x, l(1)
and r0.x, r0.x, l(2)
utof r0.x, r0.x
mad o0.x, r0.x, l(2.000000), l(-1.000000)
and r0.z, v0.x, l(2)
utof r0.y, r0.z
mad o0.y, r0.y, l(-2.000000), l(1.000000)
mov o1.xy, r0.xyxx
mov o0.zw, l(0,0,0,1.000000)
ret 
// Approximately 10 instruction slots used
#endif

const BYTE gVSL[] =
{
     68,  88,  66,  67,   3,  34, 
    134,  52, 179, 241,   3, 211, 
    118, 241, 117,  23,  12, 123, 
     64, 241,   1,   0,   0,   0, 
    200,   2,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    128,   0,   0,   0, 180,   0, 
      0,   0,  12,   1,   0,   0, 
     76,   2,   0,   0,  82,  68, 
     69,  70,  68,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     28,   0,   0,   0,   0,   4, 
    254, 255,   0,   1,   0,   0, 
     28,   0,   0,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  49,  48,  46, 
     49,   0,  73,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   1,   1,   0,   0, 
     83,  86,  95,  86, 101, 114, 
    116, 101, 120,  73,  68,   0, 
     79,  83,  71,  78,  80,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  68,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      3,  12,   0,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171, 171, 171,  83,  72, 
     68,  82,  56,   1,   0,   0, 
     64,   0,   1,   0,  78,   0, 
      0,   0,  96,   0,   0,   4, 
     18,  16,  16,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
    103,   0,   0,   4, 242,  32, 
     16,   0,   0,   0,   0,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3,  50,  32,  16,   0, 
      1,   0,   0,   0, 104,   0, 
      0,   2,   1,   0,   0,   0, 
     41,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,  16,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   2,   0, 
      0,   0,  86,   0,   0,   5, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  18,  32,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  64,   1,  64,   0,   0, 
      0,   0, 128, 191,   1,   0, 
      0,   7,  66,   0,  16,   0, 
      0,   0,   0,   0,  10,  16, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   2,   0, 
      0,   0,  86,   0,   0,   5, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      0,   0,   0,   0,  50,   0, 
      0,   9,  34,  32,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0, 192,   1,  64,   0,   0, 
      0,   0, 128,  63,  54,   0, 
      0,   5,  50,  32,  16,   0, 
      1,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   8, 194,  32, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    128,  63,  62,   0,   0,   1, 
     83,  84,  65,  84, 116,   0, 
      0,   0,  10,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   2,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
