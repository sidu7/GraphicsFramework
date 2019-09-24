/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

// These definitions agree with the ObjectIds enum in scene.h
const int     nullId	= 0;
const int     skyId	= 1;
const int     seaId	= 2;
const int     groundId	= 3;
const int     roomId	= 4;
const int     boxId	= 5;
const int     frameId	= 6;
const int     lPicId	= 7;
const int     rPicId	= 8;
const int     teapotId	= 9;
const int     spheresId	= 10;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec4 shadowCoord;

uniform int objectId;
uniform vec3 diffuse; //Kd
uniform vec3 specular; //Ks
uniform float shininess; //alpha exponent

uniform vec3 Light; //Ii
uniform vec3 Ambient; //Ia
uniform bool UseBRDF; //Lighting flag

uniform sampler2D shadowMap;

const float pi = 22/7;

vec2 shadowIndex;

void main()
{
    vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
    vec3 V = normalize(eyeVec);
    vec3 H = normalize(L + V);
    float NL = max(dot(L,N),0.0);
    float NH = max(dot(N,H),0.0);
	float LH = max(dot(L,H),0.0);

    vec3 Kd = diffuse; 
	vec3 Ks;
	if(UseBRDF)
		Ks = specular/10.0;
	else
		Ks = specular;
    float alp = shininess;
    vec3 Ii = Light;
    vec3 Ia = Ambient;

    if (objectId==groundId || objectId==seaId) {
        ivec2 uv = ivec2(floor(200.0*texCoord));
        if ((uv[0]+uv[1])%2==0)
            Kd *= 0.9; }

	vec3 F = Ks + (vec3(1,1,1)-Ks)*pow((1-LH),5);
	float D = (alp+2) * pow(NH,alp) / (2*pi);

	vec3 BRDF = (Kd/pi) + ((F*D)/(4*pow(LH,2)));

	if(shadowCoord.w > 0.0)	
	{	shadowIndex = shadowCoord.xy/shadowCoord.w;
	if(shadowIndex.x >= 0.0 && shadowIndex.y >= 0.0 && shadowIndex.x <= 1.0 && shadowIndex.y <= 1.0)
	{
		float lightDepth = texture(shadowMap,shadowIndex).w;
		float pixelDepth = shadowCoord.w;
		if(pixelDepth > lightDepth + 0.009)
			gl_FragColor.xyz = Ia*Kd ;
		else
		{
			if(!UseBRDF)
				gl_FragColor.xyz = Ia*Kd + Ii*Kd*NL + Ii*Ks*pow(NH,alp); // Phong lighting
			else //BRDF Lighting
				gl_FragColor.xyz = Ia*Kd + Ii * NL * BRDF;
		}
	}
	else
	{
		gl_FragColor.xyz = Ia*Kd + Ii * NL * BRDF;
	}
	}
	else
		gl_FragColor.xyz = Ia*Kd + Ii * NL * BRDF;
}
