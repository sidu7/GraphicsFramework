#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec2 TexCoord;

const float pi = 3.14159265f;

layout(binding = 1) uniform sampler2D normaltex;
layout(binding = 2) uniform sampler2D worldpostex;
layout(binding = 3) uniform sampler2D diffusetex;
layout(binding = 4) uniform sampler2D specularalpha;
layout(binding = 5) uniform sampler2D shadowmap;

layout(binding = 6) uniform LightingParams
{
    vec3 lightPos;
    vec3 Light;
    int GBufferShow;
    mat4 inverseview;
    mat4 shadowmat;
    float biasAlpha;
    float exposure;
    float contrast;
} lightingUbo;

void main()
{	
	vec3 worldPos = texture(worldpostex,TexCoord).rgb;
	vec3 lightVec = lightingUbo.lightPos - worldPos;
	vec3 eyePos = (lightingUbo.inverseview * vec4(0,0,0,1)).xyz;
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
	vec3 Ii = lightingUbo.Light;

	vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
	float D = (alp+2) * pow(NH,alp) / (2*pi);

	vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

	FragColor = vec4(Ii * NL * BRDF,1.0);
}
