#include "render/generated/ui_pass.vtx.hlsl"

float4 main(PS_INPUT input) : SV_TARGET {
  float3 modulated = input.col * (0.5 + 0.5 * sin(time * 2.0));
  return float4(modulated, 1.0);
};


