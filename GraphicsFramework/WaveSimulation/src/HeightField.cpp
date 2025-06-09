// HeightField.cpp
// -- example height field
// cs561 2/20

#include <cstdlib>
#include <cmath>
#include "HeightField.h"
#include "FFT2.h"
#include <glm/gtc/integer.hpp>
using namespace std;

//#define BOUNDARY

const float PI = 22.0f/7.0f;
HeightField::HeightField(int sz)
    : grid0(sz,sz),
      grid(sz,sz),
      max_height(1),
      wave_speed(0.1f),
      wave_dampness(1.7f) {
};


void HeightField::initialize(void) {
  for (int j=0; j < grid.height(); ++j)
    for (int i=0; i < grid.width(); ++i) {
      grid0(i,j) = 0.0f;
      grid(i,j) = 0.0f;
    }
  time = 0;
}


void HeightField::update(double dt)
{
	if (dt == 0.0f)
		return;

	float h2 = pow(dt,2);
	float c1 = 1.0f / (1.0f/h2 + wave_dampness/(2*dt));
	float c2 = 2.0f/ h2;
	float c3 = wave_dampness / (2.0f * dt) - 1 / h2;

	//FFT2<float> fft(log(grid.width()),log(grid.height()));
	//fft(grid);
	//fft(grid0);
	
	for (int j=0; j < grid.height(); ++j)
	{
		for (int i=0; i < grid.width(); ++i)
		{
#ifdef BOUNDARY
			if(i < grid.width()/3 && (j > grid.height()/3 || j < 2*grid.height()/3))
			{
				continue;
			}

			float u_inext = i == grid.width() - 1 ? 0.0f : grid(i + 1, j);
			float u_iprev = i == 0 || (i == grid.width() / 3 && (j > grid.height() / 3 || j < 2 * grid.height() / 3)) ? 0.0f : grid(i - 1, j);
			float u_jnext = j == grid.height() - 1 || (j == grid.width() / 3 && i < grid.width() / 3 )? 0.0f : grid(i, j + 1);
			float u_jprev = j == 0 || (j == 2 * grid.width() / 3 && i < grid.width() / 3) ? 0.0f : grid(i, j - 1);
#else	
			std::complex<float> u_inext = i == grid.width() - 1 ? 0.0f : grid(i+1,j);
			std::complex<float> u_iprev = i == 0 ? 0.0f : grid(i-1,j);
			std::complex<float> u_jnext = j == grid.height() - 1 ? 0.0f : grid(i,j+1);
			std::complex<float> u_jprev = j == 0 ? 0.0f : grid(i,j-1);
#endif				
			std::complex<float> inside = (u_inext - std::complex<float>(2.0f,0.0f) * grid(i,j) + u_iprev) / std::complex<float>(pow(grid.cellWidth(),2)) + (u_jnext - std::complex<float>(2.0f, 0.0f) * grid(i,j) + u_jprev) / std::complex<float>(pow(grid.cellHeight(),2));
			std::complex<float> temp = grid(i,j);
			grid(i,j) = c1 * (c2*grid(i,j) + c3*grid0(i,j) + wave_speed * wave_speed * inside);
			grid0(i,j) = temp;
		}
	}

	//IFFT2<float> ifft(log(grid.width()), log(grid.height()));
	//ifft(grid);
	//ifft(grid0);
	
	time += dt;
}

void HeightField::handle_input(glm::vec2 position)
{
	position *= glm::vec2(grid.width(), grid.height());
	grid(position.x, position.y) = 0.5f;
}

