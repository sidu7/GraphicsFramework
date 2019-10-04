#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

const float pi = 22/7;

uniform sampler2D normaltex;
uniform sampler2D worldpostex;
uniform sampler2D diffusetex;
uniform sampler2D specularalpha;
uniform sampler2D shadowmap;

uniform vec3 lightPos;
uniform vec3 Light;
uniform int GBufferShow;
uniform mat4 inverseview;
uniform mat4 shadowmat;

void main()
{	
	vec3 worldPos = texture(worldpostex,TexCoord).rgb;
	vec3 lightVec = lightPos - worldPos;
	vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
	vec3 eyeVec = eyePos - worldPos;

	vec3 N = normalize(texture(normaltex,TexCoord).rgb);
	vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);
	vec3 H = normalize(L + V);
	float NL = max(dot(L,N),0.0);
	float NH = max(dot(N,H),0.0);
	float LH = max(dot(L,H),0.0);

	vec3 Kd = texture(diffusetex,TexCoord).rgb;
	vec4 value = texture(specularalpha,TexCoord);
	vec3 Ks = value.rgb;
	
	float alp = value.w;
	vec3 Ii = Light;

	vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
	float D = (alp+2) * pow(NH,alp) / (2*pi);

	vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

	vec4 shadowCoord = shadowmat * vec4(worldPos,1.0);
	
	if(shadowCoord.w > 0.0)	
	{	
		vec2 shadowIndex = shadowCoord.xy/shadowCoord.w;
		if(shadowIndex.x >= 0.0 && shadowIndex.y >= 0.0 && shadowIndex.x <= 1.0 && shadowIndex.y <= 1.0)
		{
			float lightDepth = texture(shadowmap,shadowIndex).w;
			float pixelDepth = shadowCoord.w;
			if(pixelDepth > lightDepth + 0.009)
				FragColor = vec4(0,0,0,0);
			else
			{
				FragColor = vec4(Ii * NL * BRDF,1.0);
			}
		}
		else
			FragColor = vec4(Ii * NL * BRDF,1.0);
	}
	else
		FragColor = vec4(0,0,0,0);

	switch(GBufferShow)
	{
		case 1: FragColor = vec4(texture(normaltex,TexCoord).rgb,1.0);
				break;
		case 2: FragColor = vec4(worldPos,1.0);
				break;
		case 3: FragColor = vec4(Kd,1.0);
				break;
		case 4: FragColor = vec4(Ks,1.0);
				break;
	}
}