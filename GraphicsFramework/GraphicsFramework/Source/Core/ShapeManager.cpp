#include "ShapeManager.h"

#include "Core/Core.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/IndexBuffer.h"
#include "Rendering/VertexBuffer.h"
#include <algorithm>

ShapeManager::~ShapeManager()
{
	Close();
}

void ShapeManager::Init()
{
	if (mInitialized)
	{
		return;
	}
	mInitialized = true;

	// Clean up any stale data from a previous init
	Close();

	mShapes.resize(SHAPES_NUM);

	// ---- wire primitives (VBO + IBO built manually) ----
	CreateLine();
	CreateWireCube();
	CreatePyramid();

	// ---- solid geometry (built via MakeVAO) ----
	CreateQuad();
	CreateCube();
	CreateSphere();
	CreateTeapot();
}

void ShapeManager::Close()
{
	for (auto& shape : mShapes)
	{
		delete shape.mIBO;
		shape.mIBO = nullptr;
		delete shape.mVBO;
		shape.mVBO = nullptr;
	}
}

// ──────────────────────────────────────────────────────
//  Per-shape helpers (wire primitives)
// ──────────────────────────────────────────────────────

void ShapeManager::CreateLine()
{
	ShapeData& data = mShapes[Shapes::LINE];

	std::vector<glm::vec3> verts;
	verts.emplace_back(0.0f, 0.0f, 0.0f);
	verts.emplace_back(1.0f, 0.0f, 0.0f);

	constexpr uint16_t indices[] = { 0, 1 };

	data.mVBO = RenderingFactory::Instance()->CreateVertexBuffer();
	data.mIBO = RenderingFactory::Instance()->CreateIndexBuffer();

	data.mVBO->SetDataLayout({ VertexFormat::Vec3 });
	data.mVBO->AddData(verts.data(), static_cast<unsigned int>(verts.size()) * sizeof(glm::vec3));
	data.mIBO->AddData(indices, static_cast<unsigned int>(sizeof(indices) / sizeof(indices[0])), IndexType::UInt16);
}

void ShapeManager::CreateWireCube()
{
	ShapeData& data = mShapes[Shapes::WIRECUBE];

	std::vector<glm::vec3> verts = {
		glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f,  0.5f),
	};

	constexpr uint16_t indices[] = {
		// Back face
		0, 1, 1, 2, 2, 3, 3, 0,
		// Front face
		4, 5, 5, 6, 6, 7, 7, 4,
		// Connect front and back
		0, 4, 1, 5, 2, 6, 3, 7
	};

	data.mVBO = RenderingFactory::Instance()->CreateVertexBuffer();
	data.mIBO = RenderingFactory::Instance()->CreateIndexBuffer();

	data.mVBO->SetDataLayout({ VertexFormat::Vec3 });
	data.mVBO->AddData(verts.data(), static_cast<unsigned int>(verts.size()) * sizeof(glm::vec3));
	data.mIBO->AddData(indices, static_cast<unsigned int>(sizeof(indices) / sizeof(indices[0])), IndexType::UInt16);
}

void ShapeManager::CreatePyramid()
{
	ShapeData& data = mShapes[Shapes::PYRAMID];

	std::vector<glm::vec3> verts = {
		glm::vec3( 0.0f,  0.5f,  0.0f),  // apex
		glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f, -0.5f, -0.5f),
		glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f, -0.5f, -0.5f),
	};

	constexpr uint16_t indices[] = {
		1, 0, 2, 2, 0, 3, 3, 0, 4, 4, 0, 1,
		1, 2, 3, 3, 2, 4, 4, 3, 2, 2, 3, 1
	};

	data.mVBO = RenderingFactory::Instance()->CreateVertexBuffer();
	data.mIBO = RenderingFactory::Instance()->CreateIndexBuffer();

	data.mVBO->SetDataLayout({ VertexFormat::Vec3 });
	data.mVBO->AddData(verts.data(), static_cast<unsigned int>(verts.size()) * sizeof(glm::vec3));
	data.mIBO->AddData(indices, static_cast<unsigned int>(sizeof(indices) / sizeof(indices[0])), IndexType::UInt16);
}

// ──────────────────────────────────────────────────────
//  Per-shape helpers (solid geometry via MakeVAO)
// ──────────────────────────────────────────────────────

void ShapeManager::CreateQuad()
{
	std::vector<ShapeVertex> vertices;
	std::vector<uint16_t> indices;

	for (int i = 0; i < 4; ++i)
	{
		ShapeVertex v;
		int x = (i == 0 || i == 3) ? -1 : 1;
		int y = (i < 2) ? -1 : 1;
		v.position  = glm::vec3(0.5f * x, 0.5f * y, 0.0f);
		v.normal    = glm::vec3(0.0f, 0.0f, 1.0f);
		v.texCoord  = glm::vec2(x == -1 ? 0.0f : 1.0f, y == -1 ? 0.0f : 1.0f);
		vertices.push_back(v);
	}

	indices = { 0, 1, 2, 2, 3, 0 };

	MakeVAO(vertices, indices, Shapes::QUAD);
}

void ShapeManager::CreateCube()
{
	std::vector<ShapeVertex> vertices;
	vertices.reserve(32);
	std::vector<uint16_t> indices;

	// 6 faces × 4 vertices = 24 vertices (with per-face normals)
	// Index layout per face: 0,1,2, 0,2,3
	constexpr int faceOffsets[] = {
		 0,  // front   (z+):  0 1 2 3
		 4,  // right   (x+):  4 5 6 7
		 8,  // back    (z-):  8 9 10 11
		12,  // left    (x-):  12 13 14 15
		16,  // upper   (y+):  16 17 18 19
		20   // bottom  (y-):  20 21 22 23
	};

	constexpr glm::vec3 faceNormals[] = {
		glm::vec3( 0, 0, 1),   // front
		glm::vec3( 1, 0, 0),   // right
		glm::vec3( 0, 0,-1),   // back
		glm::vec3(-1, 0, 0),   // left
		glm::vec3( 0, 1, 0),   // upper
		glm::vec3( 0,-1, 0),   // bottom
	};

	constexpr glm::vec2 faceTexCoords[4] = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f },
	};

	constexpr float facePositions[6][4][3] = {
		// front   (z=+1)
		{ { -1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1} },
		// right   (x=+1)
		{ { 1, 1, 1}, { 1, 1,-1}, { 1,-1,-1}, { 1,-1, 1} },
		// back    (z=-1)
		{ {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1} },
		// left    (x=-1)
		{ {-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1} },
		// upper   (y=+1)
		{ { 1, 1, 1}, {-1, 1, 1}, {-1, 1,-1}, { 1, 1,-1} },
		// bottom  (y=-1)
		{ {-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1} },
	};

	for (int f = 0; f < 6; ++f)
	{
		for (int v = 0; v < 4; ++v)
		{
			ShapeVertex vertex;
			vertex.position  = glm::vec3(facePositions[f][v][0], facePositions[f][v][1], facePositions[f][v][2]);
			vertex.normal    = faceNormals[f];
			vertex.texCoord  = faceTexCoords[v];
			vertices.push_back(vertex);
		}

		// Two triangles per face: (0,1,2) and (0,2,3) relative to the face's
		// first vertex, matching the 4 vertices pushed above.
		const uint16_t base = static_cast<uint16_t>(faceOffsets[f]);
		indices.push_back(base);
		indices.push_back(static_cast<uint16_t>(base + 1));
		indices.push_back(static_cast<uint16_t>(base + 2));
		indices.push_back(base);
		indices.push_back(static_cast<uint16_t>(base + 2));
		indices.push_back(static_cast<uint16_t>(base + 3));
	}

	MakeVAO(vertices, indices, Shapes::CUBE);
}

void ShapeManager::CreateSphere()
{
	const float PI = 3.141592f;
	int n = 32;
	float d = 2.0f * PI / static_cast<float>(n * 2);

	std::vector<ShapeVertex> vertices;
	std::vector<uint16_t> indices;
	vertices.reserve((n * 2 + 1) * (n + 1));
	indices.reserve(n * n * 6);

	for (int i = 0; i <= n * 2; ++i)
	{
		float s = i * 2.0f * PI / static_cast<float>(n * 2);
		for (int j = 0; j <= n; ++j)
		{
			float t = j * PI / static_cast<float>(n);

			ShapeVertex v;
			v.position = glm::vec3(cos(s) * sin(t), sin(s) * sin(t), cos(t));
			v.normal   = v.position;  // unit sphere: position == normal
			v.texCoord = glm::vec2(s / (2.0f * PI), t / PI);
			vertices.push_back(v);

			if (i > 0 && j > 0)
			{
				int row = i * (n + 1) + j;
				int col = i * (n + 1) + (j - 1);

				// two triangles
				indices.push_back(static_cast<uint16_t>(col));
				indices.push_back(static_cast<uint16_t>(row));
				indices.push_back(static_cast<uint16_t>(row - 1));

				indices.push_back(static_cast<uint16_t>(col));
				indices.push_back(static_cast<uint16_t>(row - 1));
				indices.push_back(static_cast<uint16_t>(col - 1));
			}
		}
	}

	MakeVAO(vertices, indices, Shapes::SPHERE);
}

void ShapeManager::CreateTeapot()
{
	const int n = 12;

	// 16 control points per Bezier patch × 32 patches (standard Newell teapot)
	static constexpr unsigned int TeapotIndex[32][16] = {
		{ 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16 },
		{ 4, 17, 18, 19,  8, 20, 21, 22, 12, 23, 24, 25, 16, 26, 27, 28 },
		{ 19, 29, 30, 31, 22, 32, 33, 34, 25, 35, 36, 37, 28, 38, 39, 40 },
		{ 31, 41, 42,  1, 34, 43, 44,  5, 37, 45, 46,  9, 40, 47, 48, 13 },
		{ 13, 14, 15, 16, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60 },
		{ 16, 26, 27, 28, 52, 61, 62, 63, 56, 64, 65, 66, 60, 67, 68, 69 },
		{ 28, 38, 39, 40, 63, 70, 71, 72, 66, 73, 74, 75, 69, 76, 77, 78 },
		{ 40, 47, 48, 13, 72, 79, 80, 49, 75, 81, 82, 53, 78, 83, 84, 57 },
		{ 57, 58, 59, 60, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96 },
		{ 60, 67, 68, 69, 88, 97, 98, 99, 92,100,101,102, 96,103,104,105 },
		{ 69, 76, 77, 78, 99,106,107,108,102,109,110,111,105,112,113,114 },
		{ 78, 83, 84, 57,108,115,116, 85,111,117,118, 89,114,119,120, 93 },
		{121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136 },
		{124,137,138,121,128,139,140,125,132,141,142,129,136,143,144,133 },
		{133,134,135,136,145,146,147,148,149,150,151,152, 69,153,154,155 },
		{136,143,144,133,148,156,157,145,152,158,159,149,155,160,161, 69 },
		{162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177 },
		{165,178,179,162,169,180,181,166,173,182,183,170,177,184,185,174 },
		{174,175,176,177,186,187,188,189,190,191,192,193,194,195,196,197 },
		{177,184,185,174,189,198,199,186,193,200,201,190,197,202,203,194 },
		{204,204,204,204,207,208,209,210,211,211,211,211,212,213,214,215 },
		{204,204,204,204,210,217,218,219,211,211,211,211,215,220,221,222 },
		{204,204,204,204,219,224,225,226,211,211,211,211,222,227,228,229 },
		{204,204,204,204,226,230,231,207,211,211,211,211,229,232,233,212 },
		{212,213,214,215,234,235,236,237,238,239,240,241,242,243,244,245 },
		{215,220,221,222,237,246,247,248,241,249,250,251,245,252,253,254 },
		{222,227,228,229,248,255,256,257,251,258,259,260,254,261,262,263 },
		{229,232,233,212,257,264,265,234,260,266,267,238,263,268,269,242 },
		{270,270,270,270,279,280,281,282,275,276,277,278,271,272,273,274 },
		{270,270,270,270,282,289,290,291,278,286,287,288,274,283,284,285 },
		{270,270,270,270,291,298,299,300,288,295,296,297,285,292,293,294 },
		{270,270,270,270,300,305,306,279,297,303,304,275,294,301,302,271 },
	};

	// Control points (1-based indexing in TeapotIndex, converted to 0-based below)
	static constexpr glm::vec3 TeapotPoints[] = {
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
		glm::vec3(1.425,-0.798,0.0)
	};

	constexpr int npatches = static_cast<int>(sizeof(TeapotIndex) / sizeof(TeapotIndex[0]));
	const int vertsPerPatch = (n + 1) * (n + 1);

	std::vector<ShapeVertex> vertices;
	std::vector<uint16_t> indices;
	vertices.reserve(npatches * vertsPerPatch);
	indices.reserve(npatches * n * n * 6);  // 6 indices per grid cell (2 triangles)

	for (int p = 0; p < npatches; ++p)
	{
		for (int i = 0; i <= n; ++i)
		{
			const float u = static_cast<float>(i) / n;

			// u basis & derivative weights (pre-computed)
			const float u0 = (1.0f - u) * (1.0f - u) * (1.0f - u);
			const float u1 = 3.0f * (1.0f - u) * (1.0f - u) * u;
			const float u2 = 3.0f * (1.0f - u) * u * u;
			const float u3 = u * u * u;
			const float du0 = (1.0f - u) * (1.0f - u);
			const float du1 = 2.0f * (1.0f - u) * u;
			const float du2 = u * u;

			for (int j = 0; j <= n; ++j)
			{
				const float v = static_cast<float>(j) / n;

				// v basis weights
				const float v0 = (1.0f - v) * (1.0f - v) * (1.0f - v);
				const float v1 = 3.0f * (1.0f - v) * (1.0f - v) * v;
				const float v2 = 3.0f * (1.0f - v) * v * v;
				const float v3 = v * v * v;

				// v derivative weights
				const float dv0 = (1.0f - v) * (1.0f - v);
				const float dv1 = 2.0f * (1.0f - v) * v;
				const float dv2 = v * v;

				// Grab 16 control points for this Bezier patch (1-based → 0-based)
				const unsigned int* idx = TeapotIndex[p];
				const glm::vec3* const pts[] = {
					&TeapotPoints[idx[0] - 1], &TeapotPoints[idx[1] - 1], &TeapotPoints[idx[2] - 1], &TeapotPoints[idx[3] - 1],
					&TeapotPoints[idx[4] - 1], &TeapotPoints[idx[5] - 1], &TeapotPoints[idx[6] - 1], &TeapotPoints[idx[7] - 1],
					&TeapotPoints[idx[8] - 1], &TeapotPoints[idx[9] - 1], &TeapotPoints[idx[10] - 1], &TeapotPoints[idx[11] - 1],
					&TeapotPoints[idx[12] - 1], &TeapotPoints[idx[13] - 1], &TeapotPoints[idx[14] - 1], &TeapotPoints[idx[15] - 1],
				};

				// Evaluate surface position
				const glm::vec3 V =
					u0 * v0 * *pts[0] + u0 * v1 * *pts[1] + u0 * v2 * *pts[2] + u0 * v3 * *pts[3] +
					u1 * v0 * *pts[4] + u1 * v1 * *pts[5] + u1 * v2 * *pts[6] + u1 * v3 * *pts[7] +
					u2 * v0 * *pts[8] + u2 * v1 * *pts[9] + u2 * v2 * *pts[10] + u2 * v3 * *pts[11] +
					u3 * v0 * *pts[12] + u3 * v1 * *pts[13] + u3 * v2 * *pts[14] + u3 * v3 * *pts[15];

				// Evaluate u-tangent
				const glm::vec3 du =
					du0 * v0 * (*pts[4] - *pts[0]) + du0 * v1 * (*pts[5] - *pts[1]) + du0 * v2 * (*pts[6] - *pts[2]) + du0 * v3 * (*pts[7] - *pts[3]) +
					du1 * v0 * (*pts[8] - *pts[4]) + du1 * v1 * (*pts[9] - *pts[5]) + du1 * v2 * (*pts[10] - *pts[6]) + du1 * v3 * (*pts[11] - *pts[7]) +
					du2 * v0 * (*pts[12] - *pts[8]) + du2 * v1 * (*pts[13] - *pts[9]) + du2 * v2 * (*pts[14] - *pts[10]) + du2 * v3 * (*pts[15] - *pts[11]);

				// Evaluate v-tangent
				const glm::vec3 dv =
					u0 * dv0 * (*pts[1] - *pts[0]) + u0 * dv1 * (*pts[2] - *pts[1]) + u0 * dv2 * (*pts[3] - *pts[2]) +
					u1 * dv0 * (*pts[5] - *pts[4]) + u1 * dv1 * (*pts[6] - *pts[5]) + u1 * dv2 * (*pts[7] - *pts[6]) +
					u2 * dv0 * (*pts[9] - *pts[8]) + u2 * dv1 * (*pts[10] - *pts[9]) + u2 * dv2 * (*pts[11] - *pts[10]) +
					u3 * dv0 * (*pts[13] - *pts[12]) + u3 * dv1 * (*pts[14] - *pts[13]) + u3 * dv2 * (*pts[15] - *pts[14]);

				ShapeVertex vert;
				vert.position  = V;
				vert.normal    = glm::normalize(glm::cross(dv, du));
				vert.texCoord  = glm::vec2(u, v);
				vertices.push_back(vert);

				// Add quad (2 triangles) for all but first edge
				if (i > 0 && j > 0)
				{
					const uint16_t base = static_cast<uint16_t>(p * vertsPerPatch);
					const uint16_t a = base + (i - 1) * (n + 1) + (j - 1);
					const uint16_t b = base + (i - 1) * (n + 1) + j;
					const uint16_t c = base + i * (n + 1) + j;
					const uint16_t d = base + i * (n + 1) + (j - 1);

					indices.push_back(a); indices.push_back(b); indices.push_back(c);
					indices.push_back(a); indices.push_back(c); indices.push_back(d);
				}
			}
		}
	}

	MakeVAO(vertices, indices, Shapes::TEAPOT);
}

// ──────────────────────────────────────────────────────
//  Shared VAO builder
// ──────────────────────────────────────────────────────

void ShapeManager::MakeVAO(const std::vector<ShapeVertex>& vertices, const std::vector<uint16_t>& indices, Shapes shape)
{
	ShapeData& data = mShapes[shape];
	data.mVBO = RenderingFactory::Instance()->CreateVertexBuffer();
	data.mIBO = RenderingFactory::Instance()->CreateIndexBuffer();

	data.mVBO->SetDataLayout({
		VertexFormat::Vec3,  // Position
		VertexFormat::Vec3,  // Normal
		VertexFormat::Vec2,  // Texture Coordinates
	});

	data.mVBO->AddData(vertices.data(), static_cast<unsigned int>(vertices.size()) * sizeof(ShapeVertex));
	data.mIBO->AddData(indices.data(), static_cast<unsigned int>(indices.size()), IndexType::UInt16);
}
