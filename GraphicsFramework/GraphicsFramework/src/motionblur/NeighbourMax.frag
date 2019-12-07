#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D TileMax;
uniform vec2 NMaxSize;
uniform int k;

void main()
{
	float max = 0.0f;
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
				if(len >= max)
				{
					max = len;
					maxval = val;
				}
			}
		}
	}

	FragColor.xy = maxval;
}