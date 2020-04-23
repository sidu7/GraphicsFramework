// HeightField.h
// -- example height field
// cs561 2/20

#ifndef CS561_HEIGHTFIELD_H
#define CS561_HEIGHTFIELD_H


#include "Grid.h"
#include <complex>
#include <glm/vec2.hpp>

struct HeightField {
  HeightField(int sz=64);

  void initialize(void);
  void update(double dt);
  void handle_input(glm::vec2 position);

  Grid<std::complex<float>> grid0,
              grid;
  float max_height;
  double time;
  float wave_speed;
  float wave_dampness;
  //glm::vec2 spacial_frequency;
};



#endif

