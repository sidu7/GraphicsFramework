#include "ShapeManager.h"
#include "../opengl/VertexArray.h"
#include "../opengl/ElementArrayBuffer.h"
#include "../opengl/VertexBuffer.h"

void ShapeManager::Init()
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	//Quad
	for (unsigned int i = 0; i < 4; ++i)
	{
		Vertex v;
		int x = i == 0 || i == 3 ? -1 : 1;
		int y = i < 2 ? -1 : 1;
		v.position = glm::vec3(0.5f * x,0.5f * y,0.0f);
		v.texCoord = glm::vec2(x == -1 ? 0.0f : 1.0f, y == -1 ? 0.0f : 1.0f);
		vertices.push_back(v);
	}

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	MakeVAO(vertices, indices, QUAD);

	vertices.clear();
	indices.clear();

	//Cube

	//Sphere
	const float PI = 3.141592f;
	int n = 32;
	float d = 2.0f * PI / float(n * 2);
	for (int i = 0; i <= n * 2; i++)
	{
		float s = i * 2.0f * PI / float(n * 2);
		for (int j = 0; j <= n; j++)
		{
			float t = j * PI / float(n);
			float x = cos(s) * sin(t);
			float y = sin(s) * sin(t);
			float z = cos(t);
			// Create vertex
			Vertex v;
			v.position = glm::vec3(x, y, z);
			v.normal = glm::vec3(x, y, z);
			v.texCoord = glm::vec2(s / (2.0f * PI), t / PI);
			//v.tangent = glm::vec3(-sinf(s), cosf(s), 0.0);

			vertices.push_back(v);

			//mPoints.push_back(glm::vec3(x, y, z));
			//mNormals.push_back(glm::vec3(x, y, z));

			// Add indices
			if (i > 0 && j > 0)
			{
				// i, j, k
				indices.push_back((i - 1) * (n + 1) + (j - 1));
				indices.push_back((i - 1) * (n + 1) + j);
				indices.push_back((i) * (n + 1) + j);
				// i, k, l
				indices.push_back((i - 1) * (n + 1) + (j - 1));
				indices.push_back((i) * (n + 1) + j);
				indices.push_back((i) * (n + 1) + (j - 1));
			}
		}
	}

	MakeVAO(vertices,indices,SPHERE);

	vertices.clear();
	indices.clear();

	//Teapot
	// Created with n x n grid of quads
	n = 12;

	static unsigned int TeapotIndex[][16] = {
	1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
	4, 17, 18, 19,  8, 20, 21, 22, 12, 23, 24, 25, 16, 26, 27, 28,
	19, 29, 30, 31, 22, 32, 33, 34, 25, 35, 36, 37, 28, 38, 39, 40,
	31, 41, 42,  1, 34, 43, 44,  5, 37, 45, 46,  9, 40, 47, 48, 13,
	13, 14, 15, 16, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
	16, 26, 27, 28, 52, 61, 62, 63, 56, 64, 65, 66, 60, 67, 68, 69,
	28, 38, 39, 40, 63, 70, 71, 72, 66, 73, 74, 75, 69, 76, 77, 78,
	40, 47, 48, 13, 72, 79, 80, 49, 75, 81, 82, 53, 78, 83, 84, 57,
	57, 58, 59, 60, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
	60, 67, 68, 69, 88, 97, 98, 99, 92,100,101,102, 96,103,104,105,
	69, 76, 77, 78, 99,106,107,108,102,109,110,111,105,112,113,114,
	78, 83, 84, 57,108,115,116, 85,111,117,118, 89,114,119,120, 93,
	121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,
	124,137,138,121,128,139,140,125,132,141,142,129,136,143,144,133,
	133,134,135,136,145,146,147,148,149,150,151,152, 69,153,154,155,
	136,143,144,133,148,156,157,145,152,158,159,149,155,160,161, 69,
	162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,
	165,178,179,162,169,180,181,166,173,182,183,170,177,184,185,174,
	174,175,176,177,186,187,188,189,190,191,192,193,194,195,196,197,
	177,184,185,174,189,198,199,186,193,200,201,190,197,202,203,194,
	204,204,204,204,207,208,209,210,211,211,211,211,212,213,214,215,
	204,204,204,204,210,217,218,219,211,211,211,211,215,220,221,222,
	204,204,204,204,219,224,225,226,211,211,211,211,222,227,228,229,
	204,204,204,204,226,230,231,207,211,211,211,211,229,232,233,212,
	212,213,214,215,234,235,236,237,238,239,240,241,242,243,244,245,
	215,220,221,222,237,246,247,248,241,249,250,251,245,252,253,254,
	222,227,228,229,248,255,256,257,251,258,259,260,254,261,262,263,
	229,232,233,212,257,264,265,234,260,266,267,238,263,268,269,242,
	270,270,270,270,279,280,281,282,275,276,277,278,271,272,273,274,
	270,270,270,270,282,289,290,291,278,286,287,288,274,283,284,285,
	270,270,270,270,291,298,299,300,288,295,296,297,285,292,293,294,
	270,270,270,270,300,305,306,279,297,303,304,275,294,301,302,271 };

	static glm::vec3 TeapotPoints[] = {
		glm::vec3(1.4,0.0,2.4), glm::vec3(1.4,-0.784,2.4), glm::vec3(0.784,-1.4,2.4),
		glm::vec3(0.0,-1.4,2.4), glm::vec3(1.3375,0.0,2.53125),
		glm::vec3(1.3375,-0.749,2.53125), glm::vec3(0.749,-1.3375,2.53125),
		glm::vec3(0.0,-1.3375,2.53125), glm::vec3(1.4375,0.0,2.53125),
		glm::vec3(1.4375,-0.805,2.53125), glm::vec3(0.805,-1.4375,2.53125),
		glm::vec3(0.0,-1.4375,2.53125), glm::vec3(1.5,0.0,2.4), glm::vec3(1.5,-0.84,2.4),
		glm::vec3(0.84,-1.5,2.4), glm::vec3(0.0,-1.5,2.4), glm::vec3(-0.784,-1.4,2.4),
		glm::vec3(-1.4,-0.784,2.4), glm::vec3(-1.4,0.0,2.4),
		glm::vec3(-0.749,-1.3375,2.53125), glm::vec3(-1.3375,-0.749,2.53125),
		glm::vec3(-1.3375,0.0,2.53125), glm::vec3(-0.805,-1.4375,2.53125),
		glm::vec3(-1.4375,-0.805,2.53125), glm::vec3(-1.4375,0.0,2.53125),
		glm::vec3(-0.84,-1.5,2.4), glm::vec3(-1.5,-0.84,2.4), glm::vec3(-1.5,0.0,2.4),
		glm::vec3(-1.4,0.784,2.4), glm::vec3(-0.784,1.4,2.4), glm::vec3(0.0,1.4,2.4),
		glm::vec3(-1.3375,0.749,2.53125), glm::vec3(-0.749,1.3375,2.53125),
		glm::vec3(0.0,1.3375,2.53125), glm::vec3(-1.4375,0.805,2.53125),
		glm::vec3(-0.805,1.4375,2.53125), glm::vec3(0.0,1.4375,2.53125),
		glm::vec3(-1.5,0.84,2.4), glm::vec3(-0.84,1.5,2.4), glm::vec3(0.0,1.5,2.4),
		glm::vec3(0.784,1.4,2.4), glm::vec3(1.4,0.784,2.4), glm::vec3(0.749,1.3375,2.53125),
		glm::vec3(1.3375,0.749,2.53125), glm::vec3(0.805,1.4375,2.53125),
		glm::vec3(1.4375,0.805,2.53125), glm::vec3(0.84,1.5,2.4), glm::vec3(1.5,0.84,2.4),
		glm::vec3(1.75,0.0,1.875), glm::vec3(1.75,-0.98,1.875), glm::vec3(0.98,-1.75,1.875),
		glm::vec3(0.0,-1.75,1.875), glm::vec3(2.0,0.0,1.35), glm::vec3(2.0,-1.12,1.35),
		glm::vec3(1.12,-2.0,1.35), glm::vec3(0.0,-2.0,1.35), glm::vec3(2.0,0.0,0.9),
		glm::vec3(2.0,-1.12,0.9), glm::vec3(1.12,-2.0,0.9), glm::vec3(0.0,-2.0,0.9),
		glm::vec3(-0.98,-1.75,1.875), glm::vec3(-1.75,-0.98,1.875),
		glm::vec3(-1.75,0.0,1.875), glm::vec3(-1.12,-2.0,1.35), glm::vec3(-2.0,-1.12,1.35),
		glm::vec3(-2.0,0.0,1.35), glm::vec3(-1.12,-2.0,0.9), glm::vec3(-2.0,-1.12,0.9),
		glm::vec3(-2.0,0.0,0.9), glm::vec3(-1.75,0.98,1.875), glm::vec3(-0.98,1.75,1.875),
		glm::vec3(0.0,1.75,1.875), glm::vec3(-2.0,1.12,1.35), glm::vec3(-1.12,2.0,1.35),
		glm::vec3(0.0,2.0,1.35), glm::vec3(-2.0,1.12,0.9), glm::vec3(-1.12,2.0,0.9),
		glm::vec3(0.0,2.0,0.9), glm::vec3(0.98,1.75,1.875), glm::vec3(1.75,0.98,1.875),
		glm::vec3(1.12,2.0,1.35), glm::vec3(2.0,1.12,1.35), glm::vec3(1.12,2.0,0.9),
		glm::vec3(2.0,1.12,0.9), glm::vec3(2.0,0.0,0.45), glm::vec3(2.0,-1.12,0.45),
		glm::vec3(1.12,-2.0,0.45), glm::vec3(0.0,-2.0,0.45), glm::vec3(1.5,0.0,0.225),
		glm::vec3(1.5,-0.84,0.225), glm::vec3(0.84,-1.5,0.225), glm::vec3(0.0,-1.5,0.225),
		glm::vec3(1.5,0.0,0.15), glm::vec3(1.5,-0.84,0.15), glm::vec3(0.84,-1.5,0.15),
		glm::vec3(0.0,-1.5,0.15), glm::vec3(-1.12,-2.0,0.45), glm::vec3(-2.0,-1.12,0.45),
		glm::vec3(-2.0,0.0,0.45), glm::vec3(-0.84,-1.5,0.225), glm::vec3(-1.5,-0.84,0.225),
		glm::vec3(-1.5,0.0,0.225), glm::vec3(-0.84,-1.5,0.15), glm::vec3(-1.5,-0.84,0.15),
		glm::vec3(-1.5,0.0,0.15), glm::vec3(-2.0,1.12,0.45), glm::vec3(-1.12,2.0,0.45),
		glm::vec3(0.0,2.0,0.45), glm::vec3(-1.5,0.84,0.225), glm::vec3(-0.84,1.5,0.225),
		glm::vec3(0.0,1.5,0.225), glm::vec3(-1.5,0.84,0.15), glm::vec3(-0.84,1.5,0.15),
		glm::vec3(0.0,1.5,0.15), glm::vec3(1.12,2.0,0.45), glm::vec3(2.0,1.12,0.45),
		glm::vec3(0.84,1.5,0.225), glm::vec3(1.5,0.84,0.225), glm::vec3(0.84,1.5,0.15),
		glm::vec3(1.5,0.84,0.15), glm::vec3(-1.6,0.0,2.025), glm::vec3(-1.6,-0.3,2.025),
		glm::vec3(-1.5,-0.3,2.25), glm::vec3(-1.5,0.0,2.25), glm::vec3(-2.3,0.0,2.025),
		glm::vec3(-2.3,-0.3,2.025), glm::vec3(-2.5,-0.3,2.25), glm::vec3(-2.5,0.0,2.25),
		glm::vec3(-2.7,0.0,2.025), glm::vec3(-2.7,-0.3,2.025), glm::vec3(-3.0,-0.3,2.25),
		glm::vec3(-3.0,0.0,2.25), glm::vec3(-2.7,0.0,1.8), glm::vec3(-2.7,-0.3,1.8),
		glm::vec3(-3.0,-0.3,1.8), glm::vec3(-3.0,0.0,1.8), glm::vec3(-1.5,0.3,2.25),
		glm::vec3(-1.6,0.3,2.025), glm::vec3(-2.5,0.3,2.25), glm::vec3(-2.3,0.3,2.025),
		glm::vec3(-3.0,0.3,2.25), glm::vec3(-2.7,0.3,2.025), glm::vec3(-3.0,0.3,1.8),
		glm::vec3(-2.7,0.3,1.8), glm::vec3(-2.7,0.0,1.575), glm::vec3(-2.7,-0.3,1.575),
		glm::vec3(-3.0,-0.3,1.35), glm::vec3(-3.0,0.0,1.35), glm::vec3(-2.5,0.0,1.125),
		glm::vec3(-2.5,-0.3,1.125), glm::vec3(-2.65,-0.3,0.9375),
		glm::vec3(-2.65,0.0,0.9375), glm::vec3(-2.0,-0.3,0.9), glm::vec3(-1.9,-0.3,0.6),
		glm::vec3(-1.9,0.0,0.6), glm::vec3(-3.0,0.3,1.35), glm::vec3(-2.7,0.3,1.575),
		glm::vec3(-2.65,0.3,0.9375), glm::vec3(-2.5,0.3,1.125), glm::vec3(-1.9,0.3,0.6),
		glm::vec3(-2.0,0.3,0.9), glm::vec3(1.7,0.0,1.425), glm::vec3(1.7,-0.66,1.425),
		glm::vec3(1.7,-0.66,0.6), glm::vec3(1.7,0.0,0.6), glm::vec3(2.6,0.0,1.425),
		glm::vec3(2.6,-0.66,1.425), glm::vec3(3.1,-0.66,0.825), glm::vec3(3.1,0.0,0.825),
		glm::vec3(2.3,0.0,2.1), glm::vec3(2.3,-0.25,2.1), glm::vec3(2.4,-0.25,2.025),
		glm::vec3(2.4,0.0,2.025), glm::vec3(2.7,0.0,2.4), glm::vec3(2.7,-0.25,2.4),
		glm::vec3(3.3,-0.25,2.4), glm::vec3(3.3,0.0,2.4), glm::vec3(1.7,0.66,0.6),
		glm::vec3(1.7,0.66,1.425), glm::vec3(3.1,0.66,0.825), glm::vec3(2.6,0.66,1.425),
		glm::vec3(2.4,0.25,2.025), glm::vec3(2.3,0.25,2.1), glm::vec3(3.3,0.25,2.4),
		glm::vec3(2.7,0.25,2.4), glm::vec3(2.8,0.0,2.475), glm::vec3(2.8,-0.25,2.475),
		glm::vec3(3.525,-0.25,2.49375), glm::vec3(3.525,0.0,2.49375),
		glm::vec3(2.9,0.0,2.475), glm::vec3(2.9,-0.15,2.475), glm::vec3(3.45,-0.15,2.5125),
		glm::vec3(3.45,0.0,2.5125), glm::vec3(2.8,0.0,2.4), glm::vec3(2.8,-0.15,2.4),
		glm::vec3(3.2,-0.15,2.4), glm::vec3(3.2,0.0,2.4), glm::vec3(3.525,0.25,2.49375),
		glm::vec3(2.8,0.25,2.475), glm::vec3(3.45,0.15,2.5125), glm::vec3(2.9,0.15,2.475),
		glm::vec3(3.2,0.15,2.4), glm::vec3(2.8,0.15,2.4), glm::vec3(0.0,0.0,3.15),
		glm::vec3(0.0,-0.002,3.15), glm::vec3(0.002,0.0,3.15), glm::vec3(0.8,0.0,3.15),
		glm::vec3(0.8,-0.45,3.15), glm::vec3(0.45,-0.8,3.15), glm::vec3(0.0,-0.8,3.15),
		glm::vec3(0.0,0.0,2.85), glm::vec3(0.2,0.0,2.7), glm::vec3(0.2,-0.112,2.7),
		glm::vec3(0.112,-0.2,2.7), glm::vec3(0.0,-0.2,2.7), glm::vec3(-0.002,0.0,3.15),
		glm::vec3(-0.45,-0.8,3.15), glm::vec3(-0.8,-0.45,3.15), glm::vec3(-0.8,0.0,3.15),
		glm::vec3(-0.112,-0.2,2.7), glm::vec3(-0.2,-0.112,2.7), glm::vec3(-0.2,0.0,2.7),
		glm::vec3(0.0,0.002,3.15), glm::vec3(-0.8,0.45,3.15), glm::vec3(-0.45,0.8,3.15),
		glm::vec3(0.0,0.8,3.15), glm::vec3(-0.2,0.112,2.7), glm::vec3(-0.112,0.2,2.7),
		glm::vec3(0.0,0.2,2.7), glm::vec3(0.45,0.8,3.15), glm::vec3(0.8,0.45,3.15),
		glm::vec3(0.112,0.2,2.7), glm::vec3(0.2,0.112,2.7), glm::vec3(0.4,0.0,2.55),
		glm::vec3(0.4,-0.224,2.55), glm::vec3(0.224,-0.4,2.55), glm::vec3(0.0,-0.4,2.55),
		glm::vec3(1.3,0.0,2.55), glm::vec3(1.3,-0.728,2.55), glm::vec3(0.728,-1.3,2.55),
		glm::vec3(0.0,-1.3,2.55), glm::vec3(1.3,0.0,2.4), glm::vec3(1.3,-0.728,2.4),
		glm::vec3(0.728,-1.3,2.4), glm::vec3(0.0,-1.3,2.4), glm::vec3(-0.224,-0.4,2.55),
		glm::vec3(-0.4,-0.224,2.55), glm::vec3(-0.4,0.0,2.55), glm::vec3(-0.728,-1.3,2.55),
		glm::vec3(-1.3,-0.728,2.55), glm::vec3(-1.3,0.0,2.55), glm::vec3(-0.728,-1.3,2.4),
		glm::vec3(-1.3,-0.728,2.4), glm::vec3(-1.3,0.0,2.4), glm::vec3(-0.4,0.224,2.55),
		glm::vec3(-0.224,0.4,2.55), glm::vec3(0.0,0.4,2.55), glm::vec3(-1.3,0.728,2.55),
		glm::vec3(-0.728,1.3,2.55), glm::vec3(0.0,1.3,2.55), glm::vec3(-1.3,0.728,2.4),
		glm::vec3(-0.728,1.3,2.4), glm::vec3(0.0,1.3,2.4), glm::vec3(0.224,0.4,2.55),
		glm::vec3(0.4,0.224,2.55), glm::vec3(0.728,1.3,2.55), glm::vec3(1.3,0.728,2.55),
		glm::vec3(0.728,1.3,2.4), glm::vec3(1.3,0.728,2.4), glm::vec3(0.0,0.0,0.0),
		glm::vec3(1.5,0.0,0.15), glm::vec3(1.5,0.84,0.15), glm::vec3(0.84,1.5,0.15),
		glm::vec3(0.0,1.5,0.15), glm::vec3(1.5,0.0,0.075), glm::vec3(1.5,0.84,0.075),
		glm::vec3(0.84,1.5,0.075), glm::vec3(0.0,1.5,0.075), glm::vec3(1.425,0.0,0.0),
		glm::vec3(1.425,0.798,0.0), glm::vec3(0.798,1.425,0.0), glm::vec3(0.0,1.425,0.0),
		glm::vec3(-0.84,1.5,0.15), glm::vec3(-1.5,0.84,0.15), glm::vec3(-1.5,0.0,0.15),
		glm::vec3(-0.84,1.5,0.075), glm::vec3(-1.5,0.84,0.075), glm::vec3(-1.5,0.0,0.075),
		glm::vec3(-0.798,1.425,0.0), glm::vec3(-1.425,0.798,0.0), glm::vec3(-1.425,0.0,0.0),
		glm::vec3(-1.5,-0.84,0.15), glm::vec3(-0.84,-1.5,0.15), glm::vec3(0.0,-1.5,0.15),
		glm::vec3(-1.5,-0.84,0.075), glm::vec3(-0.84,-1.5,0.075), glm::vec3(0.0,-1.5,0.075),
		glm::vec3(-1.425,-0.798,0.0), glm::vec3(-0.798,-1.425,0.0),
		glm::vec3(0.0,-1.425,0.0), glm::vec3(0.84,-1.5,0.15), glm::vec3(1.5,-0.84,0.15),
		glm::vec3(0.84,-1.5,0.075), glm::vec3(1.5,-0.84,0.075), glm::vec3(0.798,-1.425,0.0),
		glm::vec3(1.425,-0.798,0.0) };


	int npatches = sizeof(TeapotIndex) / sizeof(TeapotIndex[0]);
	const int nv = npatches * (n + 1) * (n + 1);
	int nq = npatches * n * n;

	for (int p = 0; p < npatches; ++p) // For each patch
	{
		for (int i = 0; i <= n; ++i) // Grid in u direction
		{
			float u = float(i) / n;
			for (int j = 0; j <= n; ++j) // Grid in v direction
			{
				float v = float(j) / n;

				// Four u weights
				float u0 = (1.0f - u) * (1.0f - u) * (1.0f - u);
				float u1 = 3.0f * (1.0f - u) * (1.0f - u) * u;
				float u2 = 3.0f * (1.0f - u) * u * u;
				float u3 = u * u * u;

				// Three du weights
				float du0 = (1.0f - u) * (1.0f - u);
				float du1 = 2.0f * (1.0f - u) * u;
				float du2 = u * u;

				// Four v weights
				float v0 = (1.0f - v) * (1.0f - v) * (1.0f - v);
				float v1 = 3.0f * (1.0f - v) * (1.0f - v) * v;
				float v2 = 3.0f * (1.0f - v) * v * v;
				float v3 = v * v * v;

				// Three dv weights
				float dv0 = (1.0f - v) * (1.0f - v);
				float dv1 = 2.0f * (1.0f - v) * v;
				float dv2 = v * v;

				// Grab the 16 control points for Bezier patch.
				glm::vec3* p00 = &TeapotPoints[TeapotIndex[p][0] - 1];
				glm::vec3* p01 = &TeapotPoints[TeapotIndex[p][1] - 1];
				glm::vec3* p02 = &TeapotPoints[TeapotIndex[p][2] - 1];
				glm::vec3* p03 = &TeapotPoints[TeapotIndex[p][3] - 1];
				glm::vec3* p10 = &TeapotPoints[TeapotIndex[p][4] - 1];
				glm::vec3* p11 = &TeapotPoints[TeapotIndex[p][5] - 1];
				glm::vec3* p12 = &TeapotPoints[TeapotIndex[p][6] - 1];
				glm::vec3* p13 = &TeapotPoints[TeapotIndex[p][7] - 1];
				glm::vec3* p20 = &TeapotPoints[TeapotIndex[p][8] - 1];
				glm::vec3* p21 = &TeapotPoints[TeapotIndex[p][9] - 1];
				glm::vec3* p22 = &TeapotPoints[TeapotIndex[p][10] - 1];
				glm::vec3* p23 = &TeapotPoints[TeapotIndex[p][11] - 1];
				glm::vec3* p30 = &TeapotPoints[TeapotIndex[p][12] - 1];
				glm::vec3* p31 = &TeapotPoints[TeapotIndex[p][13] - 1];
				glm::vec3* p32 = &TeapotPoints[TeapotIndex[p][14] - 1];
				glm::vec3* p33 = &TeapotPoints[TeapotIndex[p][15] - 1];

				// Evaluate the Bezier patch at (u,v)
				glm::vec3 V =
					u0 * v0 * (*p00) + u0 * v1 * (*p01) + u0 * v2 * (*p02) + u0 * v3 * (*p03) +
					u1 * v0 * (*p10) + u1 * v1 * (*p11) + u1 * v2 * (*p12) + u1 * v3 * (*p13) +
					u2 * v0 * (*p20) + u2 * v1 * (*p21) + u2 * v2 * (*p22) + u2 * v3 * (*p23) +
					u3 * v0 * (*p30) + u3 * v1 * (*p31) + u3 * v2 * (*p32) + u3 * v3 * (*p33);

				// Create vertex
				Vertex vert;
				vert.position = glm::vec3(V[0], V[1], V[2]);

				//mPoints.push_back(glm::vec3(V[0], V[1], V[2]));// , 1.0));

				// Evaluate the u-tangent of the Bezier patch at (u,v)
				glm::vec3 du =
					du0 * v0 * (*p10 - *p00) + du0 * v1 * (*p11 - *p01) + du0 * v2 * (*p12 - *p02) + du0 * v3 * (*p13 - *p03) +
					du1 * v0 * (*p20 - *p10) + du1 * v1 * (*p21 - *p11) + du1 * v2 * (*p22 - *p12) + du1 * v3 * (*p23 - *p13) +
					du2 * v0 * (*p30 - *p20) + du2 * v1 * (*p31 - *p21) + du2 * v2 * (*p32 - *p22) + du2 * v3 * (*p33 - *p23);
				//vert.tangent = du;

				// Evaluate the v-tangent of the Bezier patch at (u,v)
				glm::vec3 dv =
					u0 * dv0 * (*p01 - *p00) + u0 * dv1 * (*p02 - *p01) + u0 * dv2 * (*p03 - *p02) +
					u1 * dv0 * (*p11 - *p10) + u1 * dv1 * (*p12 - *p11) + u1 * dv2 * (*p13 - *p12) +
					u2 * dv0 * (*p21 - *p20) + u2 * dv1 * (*p22 - *p21) + u2 * dv2 * (*p23 - *p22) +
					u3 * dv0 * (*p31 - *p30) + u3 * dv1 * (*p32 - *p31) + u3 * dv2 * (*p33 - *p32);

				// Calculate the surface normal as the cross product of the two tangents.
				//mNormals.push_back(glm::cross(dv, du));
				vert.normal = glm::vec3(glm::cross(dv, du));

				// Get texture coordinates
				vert.texCoord = glm::vec2(u, v);
				vertices.push_back(vert);

				// Create a quad for all but the first edge vertices
				if (i > 0 && j > 0)
				{
					indices.push_back(p * (n + 1) * (n + 1) + (i - 1) * (n + 1) + (j - 1));
					indices.push_back(p * (n + 1) * (n + 1) + (i - 1) * (n + 1) + (j));
					indices.push_back(p * (n + 1) * (n + 1) + (i) * (n + 1) + (j));
					
					indices.push_back(p * (n + 1) * (n + 1) + (i - 1) * (n + 1) + (j - 1));
					indices.push_back(p * (n + 1) * (n + 1) + (i) * (n + 1) + (j));
					indices.push_back(p * (n + 1) * (n + 1) + (i) * (n + 1) + (j - 1));
				}
			}
		}
	}

	MakeVAO(vertices, indices, TEAPOT);
}

void ShapeManager::MakeVAO(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Shapes shape)
{
	VertexArray* mpVAO = new VertexArray();
	ElementArrayBuffer* mpEBO = new ElementArrayBuffer();
	VertexBuffer mpVBO;

	mpVAO->Bind();

	// Send vertex information to VBO
	mpVBO.AddData(&vertices[0], vertices.size() * sizeof(Vertex));

	// Set up index buffer EBO
	mpEBO->AddData(&indices[0], indices.size(),sizeof(unsigned int));

	// Position
	mpVAO->Push(3, GL_FLOAT, sizeof(float));
	// Normal
	mpVAO->Push(3, GL_FLOAT, sizeof(float));
	// Texture coordinates
	mpVAO->Push(2, GL_FLOAT, sizeof(float));
	// Tangent
	//mVAO.Push(3, GL_FLOAT, sizeof(float));
	mpVAO->AddLayout();
	// Unbind VAO
	mpVAO->Unbind();

	mShapes[shape] = std::make_pair(mpVAO, mpEBO);
}
