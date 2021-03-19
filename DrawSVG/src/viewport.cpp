#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float centerX, float centerY, float vspan ) {

  // Task 5 (part 2): 
  // Set svg coordinate to normalized device coordinate transformation. Your input
  // arguments are defined as normalized SVG canvas coordinates.
  this->centerX = centerX;
  this->centerY = centerY;
  this->vspan = vspan; 

  Matrix3x3 translationMatrix = Matrix3x3::identity();
  Matrix3x3 scaleMatrix = Matrix3x3::identity();
  Matrix3x3 positionMatrix = Matrix3x3::identity();

  translationMatrix(0,2) = -centerX;
  translationMatrix(1,2) = -centerY;

  scaleMatrix(0,0) = 1 / (2 * vspan);
  scaleMatrix(1,1) = 1 / (2 * vspan);

  positionMatrix(0,2) = 0.5;
  positionMatrix(1,2) = 0.5;

  set_svg_2_norm(positionMatrix * scaleMatrix * translationMatrix);
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->centerX -= dx;
  this->centerY -= dy;
  this->vspan *= scale;
  set_viewbox( centerX, centerY, vspan );
}

} // namespace CMU462
