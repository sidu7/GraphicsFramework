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
uniform float biasAlpha;
uniform float exposure;
uniform float contrast;

vec3 Cholesky(float m11, float m12, float m13, float m22, float m23, float m33, float z1, float z2, float z3)
{
	float a = sqrt(m11);
	float b = m12 / a;
	float c = m13 / a;
	float d = sqrt(m22 - (b*b));
	float e = (m23 - b*c) / d;
	float f = sqrt(m33 - c*c - e*e);

	float c1p = z1 / a;
	float c2p = (z2 - b*c1p) / d;
	float c3p = (z3 - c*c1p - e*c2p) / f;

	float c3 = c3p / f;
	float c2 = (c2p - e*c3) / d;
	float c1 = (c1p - b*c2 - c*c3) / a;

	return vec3(c1,c2,c3);
}

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
	
	float G;
	if(shadowCoord.w > 0.0)	
	{	
		vec2 shadowIndex = shadowCoord.xy/shadowCoord.w;
		if(shadowIndex.x >= 0.0 && shadowIndex.y >= 0.0 && shadowIndex.x <= 1.0 && shadowIndex.y <= 1.0)
		{
			vec4 b = texture(shadowmap,shadowIndex);
			float zf = shadowCoord.w;			

			vec4 bprime = (1 - biasAlpha) * b + biasAlpha * vec4(0.5);

			vec3 c = Cholesky(1.0f, bprime.x, bprime.y, bprime.y, bprime.z, bprime.w, 1.0f, zf, zf*zf);

			float b4ac = sqrt(c.y*c.y - 4*c.x*c.z);

			float z2 = (-c.y + b4ac) / (2 * c.z);
			float z3 = (-c.y - b4ac) / (2 * c.z);

			if(z2 > z3)
			{
				float tempz = z3;
				z3 = z2;
				z2 = tempz;
			}

			if (zf <= z2)
			{
				G = 0.0f;
			}
			else if(zf <= z3)
			{
				G = (zf*z3 - bprime.x*(zf+z3) + bprime.y)/((z3-z2)*(zf-z2));
			}
			else
			{
				G = 1.0f - ((z2*z3 - bprime.x*(z2+z3) + bprime.y)/((zf-z2)*(zf-z3)));				
			}
		}
		else
			G = 0.0f;
	}
	else
		G = 1.0f;

	vec4 OutColor = (1-G) * vec4(Ii * NL * BRDF,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = exposure * OutColor / (exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(contrast/2.2));

	FragColor = (1-G) * vec4(Ii * NL * BRDF,1.0);

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