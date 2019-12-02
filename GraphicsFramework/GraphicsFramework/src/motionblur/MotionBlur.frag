#version 330

const int MAX_SAMPLES = 50;

out vec4 FragColor;

uniform sampler2D Color;
uniform sampler2D Velocity;

void main()
{
	vec2 texelSize = 1.0/ vec2(textureSize(Color,0));
	vec2 screenTexCoords = gl_FragCoord.xy * texelSize;

	vec2 velocity = texture(Velocity,screenTexCoords).rg;

	float speed = length(velocity/texelSize);
	int nSamples = clamp(int(speed),1,MAX_SAMPLES);

	vec3 result = texture(Color,screenTexCoords).xyz;
	for(int i = 1; i < nSamples; ++i)
	{
		vec2 offset = velocity * (float(i)/float(nSamples - 1) - 0.5);
		result += texture(Color,screenTexCoords + offset).xyz;
	}

	result /= float(nSamples);

	FragColor.xyz = result;
}