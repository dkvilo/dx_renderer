#include "render/generated/ui_pass.vtx.hlsl"

PS_INPUT main(VS_INPUT input) {
  PS_INPUT output;

  float3 rotatedPos = input.pos;
  float angle = time;

  float cosA = cos(angle);
  float sinA = sin(angle);

  rotatedPos.x = input.pos.x * cosA - input.pos.y * sinA;
  rotatedPos.y = input.pos.x * sinA + input.pos.y * cosA;

  output.pos = float4(rotatedPos * scale, 1.0);
  output.col = input.col;

  return output;
};
