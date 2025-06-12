#version 430 core

out vec4 FragColor;

const float pi = 22.0f/7.0f;

uniform HBlock
{ 
	float Num;
	float Numbers[2 * 100];
};

in vec2 TexCoord;

uniform vec3 Ambient;
uniform mat4 inverseview;
uniform sampler2D diffusetex;
uniform sampler2D normaltex;
uniform sampler2D speculartex;
uniform sampler2D worldpostex;
uniform sampler2D irradiance;
uniform sampler2D skydome;
uniform float exposure;
uniform float contrast;
uniform bool showDiffuse;
uniform bool showSpecular;

vec3 TexRead(vec3 w,sampler2D tex, float level)
{
	vec2 uv = vec2(0.5 - atan(w.z,w.x)/(2*pi),  acos(-w.y)/pi);
	return textureLod(tex,uv,level).xyz;
}

vec3 Fresnel(vec3 L, vec3 H, vec3 Ks)
{
	return Ks + (vec3(1) - Ks)*pow((1 - dot(L,H)),5);
}

float Distribution(vec3 N, vec3 H, float alpha)
{
	return (alpha + 2) * pow(max(dot(N,H),0.0),alpha) / (2*pi);
}

void main()
{
	vec3 Kd = texture(diffusetex,TexCoord).xyz;
	vec4 val = texture(speculartex,TexCoord);
	float alpha = val.w;
	vec3 Ks = val.xyz;
	vec4 worldPosdepth = texture(worldpostex,TexCoord);
	vec3 worldPos = worldPosdepth.xyz;
	float depth = worldPosdepth.w;
	vec3 eyePos = (inverseview * vec4(0,0,0,1)).xyz;
	vec3 eyeVec = eyePos - worldPos;
	vec3 V = normalize(eyeVec);
	vec3 N = normalize(texture(normaltex,TexCoord).xyz);
	vec3 R = normalize(2.0f*N*dot(V,N) - V);
	vec3 IrrAmbient = TexRead(-N,irradiance,0);
	//vec3 IrrAmbient = IrradianceMap(N);
	ivec2 skydomesize = textureSize(skydome,0);

	vec3 A = normalize(vec3(R.z,0,-R.x));
	vec3 B = normalize(cross(R,A));

	vec3 specular = vec3(0.0f);
	
	for (int i = 0, j = 0; i < Num; ++i, j+= 2)
	{
		float u = Numbers[j];
		float v = Numbers[j+1];

		v = acos(pow(v,1.0f/(alpha+1)))/pi;
		vec3 LDir = vec3(cos(2*pi*(0.5-u))*sin(pi*v),cos(pi*v),sin(2*pi*(0.5-u))*sin(pi*v));
		vec3 wk = normalize(LDir.x*A+LDir.y*R+LDir.z*B);
		vec3 H = normalize(wk + V);
		float level = max(0.5f * log2(1.0f * skydomesize.x * skydomesize.y / Num) - 0.5f * log2(Distribution(N,H,alpha)),0.0f);
		vec2 Xuv = vec2(0.5 + atan(wk.z,-wk.x)/(2*pi),  acos(-wk.y)/pi);
		vec3 Li = textureLod(skydome,Xuv,level).xyz;
		specular +=  (1/pow(max(dot(wk,H),0.0),2)) * Fresnel(wk,H,Ks)/4.0f * Li * max(dot(wk,N),0.0f);
	}

	specular /= Num;

	vec3 diffuse = IrrAmbient * Kd / pi;
	vec3 color = vec3(0);
	if(showDiffuse)
	color += diffuse;
	if(showSpecular)
	color += specular;

	vec4 OutColor = vec4(color,1.0);

	// Tone Mapping and Gamma Correction
	vec4 base = exposure * OutColor / (exposure * OutColor + vec4(1,1,1,1));

	FragColor = pow(base,vec4(contrast/2.2));
}