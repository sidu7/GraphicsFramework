#version 430 core

out vec4 FragColor;

in vec2 TexCoord;

const float pi = 22.0f/7.0f;

layout (binding = 2) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 inverseview;
};

layout (binding = 1) uniform sampler2D normaltex;
layout (binding = 2) uniform sampler2D worldpostex;
layout (binding = 3) uniform sampler2D diffusetex;
layout (binding = 4) uniform sampler2D specularalpha;

uniform vec3 lightPos;
uniform vec3 Light;
uniform int GBufferShow;
uniform float lightRadius;

void main()
{
	vec2 TexC = vec2(gl_FragCoord.x / 1280, gl_FragCoord.y / 720);
	vec3 worldPos = texture(worldpostex,TexC).rgb;
	float d = length(worldPos - lightPos);
	if(d > lightRadius)
	{
		FragColor = vec4(0,0,0,1);
	}
	else
	{
		vec3 lightVec = lightPos - worldPos;
		vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
		vec3 eyeVec = eyePos - worldPos;

		vec3 N = normalize(texture(normaltex,TexC).rgb);
		vec3 L = normalize(lightVec);
		vec3 V = normalize(eyeVec);
		vec3 H = normalize(L + V);
		float NL = max(dot(L,N),0.0);
		float NH = max(dot(N,H),0.0);
		float LH = max(dot(L,H),0.0);

		vec3 Kd = texture(diffusetex,TexC).rgb;
		vec4 value = texture(specularalpha,TexC);
		vec3 Ks = value.rgb;
	
		float alp = value.w;
		vec3 Ii = Light;

		vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
		float D = (alp+2) * pow(NH,alp) / (2*pi);

		vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

		float attn = clamp(1.0 - d*d /(lightRadius * lightRadius),0.0,1.0);

		FragColor = vec4(attn * Ii * NL*BRDF,1.0);
	}
	//FragColor = vec4(0,1,0,1);
}