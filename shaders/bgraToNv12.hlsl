Texture2D<float4> inputTex : register(t0);
RWTexture2D<float> outputY : register(u0);
RWTexture2D<float2> outputUV : register(u1);

[numthreads(16, 16, 1)]
void main(uint2 pos : SV_DispatchThreadID)
{
    uint width, height;
    inputTex.GetDimensions(width, height);
    if (pos.x >= width || pos.y >= height)
        return;

    float3 c0 = saturate(inputTex[pos].rgb);
    outputY[pos] = saturate(dot(c0, float3(0.182586, 0.614231, 0.062007)) + (16.0 / 255.0));

    if (all((pos & 1) == 0))
    {
        uint2 maxPos = uint2(width - 1, height - 1);
        uint2 p1 = uint2(min(pos.x + 1, maxPos.x), pos.y);
        uint2 p2 = uint2(pos.x, min(pos.y + 1, maxPos.y));
        uint2 p3 = min(pos + 1, maxPos);

        float3 avg = (c0 + saturate(inputTex[p1].rgb) + saturate(inputTex[p2].rgb) + saturate(inputTex[p3].rgb)) * 0.25;

        float u = dot(avg, float3(-0.100644, -0.338572, 0.439216)) + (128.0 / 255.0);
        float v = dot(avg, float3(0.439216, -0.398942, -0.040274)) + (128.0 / 255.0);

        outputUV[pos / 2] = saturate(float2(u, v));
    }
}