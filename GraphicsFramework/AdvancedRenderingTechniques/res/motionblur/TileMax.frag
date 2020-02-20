#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D velocityTex;
uniform vec2 TileMaxSize;
uniform int k;

void main()
{
	float max = 0.0f;
	vec2 maxval = vec2(0.0f);
	vec2 xy = vec2(TileMaxSize.x-1,TileMaxSize.y-1);
	for(int u = 0; u < k; ++u)
	{
		for(int v = 0; v < k; ++v)
		{
			vec2 index = vec2(k*TexCoord.x*xy.x + u,k*TexCoord.y*xy.y + v);
			vec2 uv = index / vec2(1280,720);
			vec2 val = texture(velocityTex,uv).xy;
			float len = length(val);
			if(len >= max)
			{
				max = len;
				maxval = val;
			}
		}
	}
	FragColor.xy = maxval;
}