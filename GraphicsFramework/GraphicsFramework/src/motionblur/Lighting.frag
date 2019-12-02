#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 TexCoord;

const float pi = 22.0f/7.0f;

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
uniform float biasAlpha;
uniform float exposure;
uniform float contrast;

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

	FragColor = vec4(Ii * NL * BRDF,1.0);
}