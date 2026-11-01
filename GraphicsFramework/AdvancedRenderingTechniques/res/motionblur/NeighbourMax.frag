#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoord;

layout(binding = 1) uniform sampler2D TileMax;

layout(binding = 2) uniform NeighbourMaxParams
{
    vec2 NMaxSize;
    int k;
} neighbourMaxUbo;

void main()
{
	float maxVal = 0.0f;
	vec2 maxval = vec2(0.0f);

	for(int u = -1; u < 2; ++u)
	{
		for(int v = -1; v < 2; ++v)
		{
			vec2 uv = TexCoord + vec2(u,v);
			if(uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f)
			{
				vec2 val = texture(TileMax,uv).xy;
				float len = length(val);
				if(len >= maxVal)
				{
					maxVal = len;
					maxval = val;
				}
			}
		}
	}

	FragColor.xy = maxval;
}
