// Grid.h
// -- basic 2D grid class
// cs561 2/20

#ifndef CS561_GRID_H
#define CS561_GRID_H


#include <vector>
#include <sstream>
#include <stdexcept>


template <class T>
class Grid {
  public:
    Grid(int width, int height,
         float physical_width=1.0f, float physical_height=1.0f)
        : grid_width(width), grid_height(height),
          cell_width(physical_width/width),
          cell_height(physical_height/height),
          grid_data(width*height) { }
    int width(void) const  { return grid_width; }
    int height(void) const { return grid_height; }
    float cellWidth(void) const { return cell_width; }
    float cellHeight(void) const { return cell_height; }
    T operator()(int i) const;
    T& operator()(int i);
    T& operator()(int i, int j);
    T operator()(int i, int j) const;
  private:
    int grid_width,
        grid_height;
    float cell_width,
          cell_height;
    std::vector<T> grid_data;
};



/////////////////////////////////////////////////////////////////
// non-inline implementations
/////////////////////////////////////////////////////////////////

template <class T>
T Grid<T>::operator()(int i) const {
  #ifndef NDEBUG
  if (i < 0 || i >= grid_width*grid_height) {
    std::stringstream ss;
    ss << "out-of-bounds error: (" << i << ")";
    throw std::out_of_range(ss.str().c_str());
  }
  #endif
  return grid_data[i];
}


template <class T>
T& Grid<T>::operator()(int i) {
  #ifndef NDEBUG
  if (i < 0 || i >= grid_width*grid_height) {
    std::stringstream ss;
    ss << "out-of-bounds error: (" << i << ")";
    throw std::out_of_range(ss.str().c_str());
  }
  #endif
  return grid_data[i];
}


template <class T>
T& Grid<T>::operator()(int x, int y) {
  #ifndef NDEBUG
  if (x < 0 || x >= grid_width || y < 0 || y >= grid_height) {
    std::stringstream ss;
    ss << "out-of-bounds error: (" << x << "," << y << ")";
    //throw std::out_of_range(ss.str().c_str());
  }
  #endif
  return grid_data[grid_width*y+x];
}
  

template <class T>
T Grid<T>::operator()(int x, int y) const {
  #ifndef NDEBUG
  if (x < 0 || x >= grid_width || y < 0 || y >= grid_height) {
    std::stringstream ss;
    ss << "out-of-bounds error: (" << x << "," << y << ")";
    //throw std::out_of_range(ss.str().c_str());
  }
  #endif
  return grid_data[grid_width*y+x];
}



#endif

