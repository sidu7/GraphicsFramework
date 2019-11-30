#include "math.h"
#include <vector>
#include <iostream>
#include <string.h>
#include <assert.h> 
using namespace std;

#include "rgbe.h"

const float PI = 3.1415927;

// Read an HDR image in .hdr (RGBE) format.
void read(const string inName, std::vector<float>& image, 
          int& width, int& height)
{
    rgbe_header_info info;
    char errbuf[100] = {0};
    
    // Open file and read width and height from the header
    FILE* fp = fopen(inName.c_str(), "rb");
    if (!fp) {
        printf("Can't open file: %s\n", inName.c_str());
       return; }
    int rc = RGBE_ReadHeader(fp, &width, &height, &info, errbuf);
    if (rc != RGBE_RETURN_SUCCESS) {
        printf("RGBE read error: %s\n", errbuf);
        return; }

    // Allocate enough memory
    image.resize(3*width*height);

    // Read the pixel data and close the file
    rc = RGBE_ReadPixels_RLE(fp, &image[0], width, height, errbuf);
    if (rc != RGBE_RETURN_SUCCESS) {
        printf("RGBE read error: %s\n", errbuf);
        return; }
    fclose(fp);
    
    printf("Read %s (%dX%d)\n", inName.c_str(), width, height);
}

// Write an HDR image in .hdr (RGBE) format.
void write(const string outName, std::vector<float>& image, 
           const int width, const int height)
{
    rgbe_header_info info;
    char errbuf[100] = {0};

    // Open file and rite width and height to the header
    FILE* fp  =  fopen(outName.c_str(), "wb");
    int rc = RGBE_WriteHeader(fp, width, height, NULL, errbuf);
    if (rc != RGBE_RETURN_SUCCESS) {
        printf("RGBE write error: %s\n", errbuf);
        return; }

    // Writ the pixel data and close the file
    rc = RGBE_WritePixels_RLE(fp, &image[0], width,  height, errbuf);
    if (rc != RGBE_RETURN_SUCCESS) {
        printf("RGBE write error: %s\n", errbuf);
        return; }
    fclose(fp);
    
    printf("Wrote %s (%dX%d)\n", outName.c_str(), width, height);
}

struct vec3
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
};

float Y_Function(float x, float y, float z, int index)
{
	switch (index)
	{
	case 0:
		return 0.28209f;
		break;
	case 1:
		return 0.48860f * y;
		break;
	case 2:
		return 0.48860f * z;
		break;
	case 3:
		return 0.48860f * x;
		break;
	case 4:
		return 1.09254f * x * y;
		break;
	case 5:
		return 1.09254f * y * z;
		break;
	case 6:
		return 0.31539f * (3 * z * z - 1);
		break;
	case 7:
		return 1.09254f * x * z;
		break;
	case 8:
		return 0.54627f * (x * x - y * y);
		break;
	}
}

int main()
{    
    // Read in-file name from command line, create out-file name
    string inName = "skyDome.hdr";
    string outName = inName.substr(0,inName.length()-4) + "-irradiance.hdr";

    std::vector<float> image;
    int width, height;
    read(inName, image, width, height);

	float A_term[9] = { 3.1415f, 
						2.0943f, 2.0943f, 2.0943f,
						0.7853f, 0.7853f, 0.7853f, 0.7853f, 0.7853f };

	vec3 Elm[9];
	vec3 Llm[9];
	float test = 0.0f;
    // For no good reason other than to demonstrate the manipulation
    // of an image, I'll Gamma correct the image to linear color
    // space.  Use gamma=2.2 if you have no specific gamma
    // information. Skip this step if you know the image is already in
    // linear color space.

    // This is included to demonstrate the magic of OpenMP: This
    // pragma turns the following loop into a multi-threaded loop,
    // making use of all the cores your machine may have.
    //#pragma omp parallel for schedule(dynamic, 1) // Magic: Multi-thread y loop
    for (int j=0;  j<height;  j++) 
	{
        for (int i=0;  i<width; i++) 
		{
            int p = (j*width+i);
			vec3 Lij;
			Lij.x = image[3 * p];
			Lij.y = image[3 * p + 1];
			Lij.z = image[3 * p + 2];
			float theta = ((j + 0.5f) * PI) / height;
			float fi = ((i + 0.5f) * 2.0f * PI) / width;
			float x = sin(theta) * cos(fi);
			float y = sin(theta) * sin(fi);
			float z = cos(theta);
			float sint = sin(theta);
			test += sint * (PI / height) * ((2 * PI) / width);
			for (int e = 0; e < 9; ++e)
			{
				float Y_Term = Y_Function(x, y, z, e);

				Llm[e].x += Lij.x * Y_Term * sint * (PI / height) * ((2 * PI) / width);
				Llm[e].y += Lij.y * Y_Term * sint * (PI / height) * ((2 * PI) / width);
				Llm[e].z += Lij.z * Y_Term * sint * (PI / height) * ((2 * PI) / width);
				/*for (int c = 0; c < 3; c++)
				{
					image[3 * p + c] *= pow(image[3 * p + c], 1.8);
				}*/
			}
		} 
	}

	for (int i = 0; i < 9; ++i)
	{
		Elm[i].x = A_term[i] * Llm[i].x;
		Elm[i].y = A_term[i] * Llm[i].y;
		Elm[i].z = A_term[i] * Llm[i].z;
	}

	std::vector<float> irrImage;
	int irrWidth = 400; int irrHeight = 200;

	for (int j = 0; j < irrHeight; ++j)
	{
		for (int i = 0; i < irrWidth; ++i)
		{
			float theta = ((j + 0.5f) * PI) / irrHeight;
			float fi = ((i+0.5f) * 2.0f * PI) / irrWidth;
			float x = sin(theta) * cos(fi);
			float y = sin(theta) * sin(fi);
			float z = cos(theta);
			vec3 res;
			for (int k = 0; k < 9; ++k)
			{
				float Y_Term = Y_Function(x, y, z, k);
				res.x += Elm[k].x * Y_Term;
				res.y += Elm[k].y * Y_Term;
				res.z += Elm[k].z * Y_Term;
			}
			irrImage.push_back(res.x);
			irrImage.push_back(res.y);
			irrImage.push_back(res.z);
		}
	}

    // Write out the processed image.
    write(outName, irrImage, irrWidth, irrHeight);
}
