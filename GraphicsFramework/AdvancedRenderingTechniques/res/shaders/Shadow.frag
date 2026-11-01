#version 420

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 position;

void main()
{
	float depthsqr = position.w*position.w;
	float depthcube = depthsqr*position.w;
	float depthquad = depthcube*position.w;
	FragColor = vec4(position.w,depthsqr,depthcube,depthquad);
   //FragColor = vec4(position.w/350,position.w/350,position.w/350,1);
}