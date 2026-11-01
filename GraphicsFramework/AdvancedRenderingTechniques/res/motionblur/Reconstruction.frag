#version 450

layout(location = 0) out vec4 FragColor;

const float SOFT_Z_EXTENT = 1.0f;

layout(location = 0) in vec2 TexCoord;

layout(binding = 1) uniform sampler2D VelocityDepth;
layout(binding = 2) uniform sampler2D NeighbourMax;
layout(binding = 3) uniform sampler2D Color;

layout(binding = 4) uniform ReconstructionParams
{
    int k;
    int S;
} reconUbo;

highp float rand(vec2 co)
{
	highp float a = 12.9898;
	highp float b = 78.233;
	highp float c = 43758.5453;
	highp float dt = dot(co.xy, vec2(a, b));
	highp float sn = mod(dt, 3.14);
	return fract(sin(sn) * c);
}

float softDepthCompare(float Za, float Zb)
{
	return clamp(1-(Za-Zb)/SOFT_Z_EXTENT,0.0f,1.0f);
}

float cone(vec2 X, vec2 Y, vec2 v)
{
	return clamp(1-length(X-Y)/length(v),0.0f,1.0f);
}

float cylinder(vec2 X, vec2 Y, vec2 v)
{
	float lv = length(v);
	return 1.0 - smoothstep(0.95f * lv,1.05f * lv,length(X-Y));
}

void main()
{
	vec2 X = gl_FragCoord.xy;
	ivec2 NSize = textureSize(NeighbourMax,0);
	ivec2 SSize = textureSize(VelocityDepth,0);

	vec2 vn = texture(NeighbourMax,floor(X/reconUbo.k/NSize)).xy;

	vec3 Cx = texture(Color,TexCoord).xyz;
	if(length(vn) <= 0.01f + 0.5f)
	{
		FragColor.xyz = Cx; // No Blur
	}
	else
	{
		vec2 Vx = texture(VelocityDepth,TexCoord).xy;
		float Zx = texture(VelocityDepth,TexCoord).z;

		// Sample current point
		float weight = 1.0f / length(Vx);

		vec3 sum = Cx * weight;

		// Take S - 1 additional neighbour samples
		float j = rand(TexCoord) - 0.5f;
		for(int i = 0; i < reconUbo.S; ++i)
		{
			if(i == (reconUbo.S-1)/2)
			{
				continue;
			}

			float t = mix(-1.0f,1.0f,(i+j+1.0f)/(reconUbo.S+1.0f));
			vec2 Y = floor(X + vn*t + 0.5f);

			vec2 Vy = texture(VelocityDepth,Y/SSize).xy;
			float Zy = texture(VelocityDepth,Y/SSize).z;

			float f = softDepthCompare(Zx,Zy);
			float b = softDepthCompare(Zy,Zx);

			float Ay = f * cone(Y,X,Vy) + b * cone(X,Y,Vx) + cylinder(Y,X,Vy) * cylinder(X,Y,Vx) * 2;

			// Accumulate
			weight += Ay;
			vec3 Cy = texture(Color,Y/SSize).xyz;
			sum += Ay * Cy;
		}

		FragColor.xyz = sum / weight;
	}
}
