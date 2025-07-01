#version 430 core

out vec4 FragColor;

const float pi = 22.0f/7.0f;

in vec2 TexCoord;

layout (binding = 2) uniform sampler2D normaltex;
layout (binding = 3) uniform sampler2D worldpostex;

layout (binding = 3) uniform AOParams
{
	int AOn;
	float AOR;
	float AOscale;
	float contrast;
};

void main()
{
	vec4 worldPosdepth = texture(worldpostex,TexCoord);
	vec3 worldPos = worldPosdepth.xyz;
	float depth = worldPosdepth.w;
	vec3 N = normalize(texture(normaltex,TexCoord).xyz);

	// SSAO
	float S = 0.0f;
	int xp = int(gl_FragCoord.x);
	int yp = int(gl_FragCoord.y);
	int hash = (30*xp ^ yp) + 10*xp*yp;
	for(int i = 0; i < AOn; ++i)
	{
		float AOalpha = (i + 0.5f)/AOn;
		float AOh = AOalpha * AOR / depth;
		float AOtheta = 2 * pi * AOalpha * (7*AOn/9) + hash;
		vec2 AOuv = TexCoord + AOh * vec2(cos(AOtheta),sin(AOtheta));
		vec4 Pidepth = texture(worldpostex,AOuv);
		vec3 Pi = Pidepth.xyz;
		float di = Pidepth.w;
		vec3 wi = Pi - worldPos;
		int Heaviside = 1;
		if((AOR - length(wi)) < 0.0f)
		{
			Heaviside = 0;
		}
		S += max(0,dot(N,wi)-0.001*di) * Heaviside / max(0.1f*AOR*0.1f*AOR,dot(wi,wi));
	}

	S = ((2 * pi * 0.1f * AOR) / AOn) * S;

	float AO = max(0.0f,pow(1 - AOscale*S,contrast));

	//FragColor.xyz = FragColor.xyz * AO;
	FragColor.xyz = vec3(AO);
}