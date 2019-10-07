#version 330 core

out vec4 FragColor;

in vec3 normalVec;
in vec3 worldPos;
in vec3 eyePos;

uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform vec3 lightPos;
uniform vec3 light;

const float pi = 22/7;

void main()
{
	vec3 lightVec = lightPos - worldPos;
	vec3 eyeVec = eyePos - worldPos;

	vec3 N = normalize(normalVec);
	vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);
	vec3 H = normalize(L + V);
	float NL = max(dot(L,N),0.0);
	float NH = max(dot(N,H),0.0);
	float LH = max(dot(L,H),0.0);

	vec3 Kd = diffuse;
	vec3 Ks = specular;
	
	float alp = shininess;
	vec3 Ii = light;

	vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
	float D = (alp+2) * pow(NH,alp) / (2*pi);

	vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

	FragColor = vec4(Ii * NL * BRDF,1);
}