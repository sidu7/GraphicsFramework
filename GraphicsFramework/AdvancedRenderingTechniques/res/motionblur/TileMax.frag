#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoord;

layout(binding = 1) uniform sampler2D velocityTex;

layout(binding = 2) uniform TileMaxParams
{
    vec2 TileMaxSize;
    int k;
} tileMaxUbo;

void main()
{
	float maxVal = 0.0f;
	vec2 maxval = vec2(0.0f);
	vec2 xy = vec2(tileMaxUbo.TileMaxSize.x-1,tileMaxUbo.TileMaxSize.y-1);
	for(int u = 0; u < tileMaxUbo.k; ++u)
	{
		for(int v = 0; v < tileMaxUbo.k; ++v)
		{
			vec2 index = vec2(tileMaxUbo.k*TexCoord.x*xy.x + u,tileMaxUbo.k*TexCoord.y*xy.y + v);
			vec2 uv = index / vec2(1280,720);
			vec2 val = texture(velocityTex,uv).xy;
			float len = length(val);
			if(len >= maxVal)
			{
				maxVal = len;
				maxval = val;
			}
		}
	}
	FragColor.xy = maxval;
}
