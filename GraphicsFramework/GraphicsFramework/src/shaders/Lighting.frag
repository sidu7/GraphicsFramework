#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

const float pi = 22/7;

uniform sampler2D normaltex;
uniform sampler2D worldpostex;
uniform sampler2D diffusetex;
uniform sampler2D specularalpha;

uniform vec3 lightPos;
uniform vec3 eyePos;
uniform vec3 Light;
uniform vec3 Ambient;

void main()
{	
	vec3 worldPos = texture(worldpostex,TexCoord).rgb;
	vec3 lightVec = lightPos - worldPos;
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
	vec3 Ks = value.rgb/10.0;
	
	float alp = value.w;
	vec3 Ii = Light;
	vec3 Ia = Ambient;

	vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
	float D = (alp+2) * pow(NH,alp) / (2*pi);

	vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

    FragColor = vec4(Ia*Kd + Ii * NL * BRDF,1.0);
	//FragColor = vec4(texture(specularalpha,TexCoord).xyz,1.0);
}