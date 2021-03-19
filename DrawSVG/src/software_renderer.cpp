#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = svg_2_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
    //transformation = svg_2_screen;
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  
  int sampleBufferSize = sample_rate * sample_rate * 4 * target_h * target_w;
  supersample_buffer.resize(sampleBufferSize);

  // Set supersamplebuffer as default
  memset(&supersample_buffer[0], 255, sampleBufferSize);

  supersample_h = sample_rate * target_h;
  supersample_w = sample_rate * target_w;

  //std::cout << "size of super sample buffer is: " << this->supersample_buffer.size() << std::endl;

}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  
  // Resize super sample buffer and initialize it
  supersample_w = sample_rate * width;
  supersample_h = sample_rate * height;
  supersample_buffer.resize(4 * supersample_w * supersample_h);

  // Set supersamplebuffer as default
  memset(&supersample_buffer[0], 255, 4 * supersample_w * supersample_h);

  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack
  Matrix3x3 oldTrans = transformation;
  transformation = transformation * element->transform;

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }
  transformation = oldTrans;
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {
  //std::cout << sample_rate << std::endl;
  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // Get premultiplied color
  if (sample_rate == 1)
  {
    float r = render_target[4 * (sx + sy * target_w)    ] / 255.f;
    float g = render_target[4 * (sx + sy * target_w) + 1] / 255.f;
    float b = render_target[4 * (sx + sy * target_w) + 2] / 255.f;
    float a = render_target[4 * (sx + sy * target_w) + 3] / 255.f;
    Color premultipliedColor = Color(r, g, b, a);

    // Modify input color
    Color inputColor = Color(color.a * color.r, color.a * color.g, color.a * color.b, color.a);

    // Calculate for premultiplied color
    Color compositeColor = inputColor + (1 - inputColor.a) * premultipliedColor;

    render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (compositeColor.r / compositeColor.a * 255);
    render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (compositeColor.g / compositeColor.a * 255);
    render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (compositeColor.b / compositeColor.a * 255);
    render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (compositeColor.a * 255);
  }
  else
  {
    // Add color into supersample buffer
    int count = 0;                            // num of samples in one pixel
    for (int countX = 0; countX < sample_rate; countX++)
    {
      for (int countY = 0; countY < sample_rate; countY++)
      {
        // Fill all supersample buffer position
        int screenPointIdx = sample_rate * sample_rate * 4 * (sx + sy * target_w);
        int supersampleIdx = count * 4;   // Each sample point will take 4 memory address
        
        float r = supersample_buffer[screenPointIdx + supersampleIdx + 0] / 255.f;
        float g = supersample_buffer[screenPointIdx + supersampleIdx + 1] / 255.f;
        float b = supersample_buffer[screenPointIdx + supersampleIdx + 2] / 255.f;
        float a = supersample_buffer[screenPointIdx + supersampleIdx + 3] / 255.f;
        Color premultipliedColor = Color(r, g, b, a);

        // Modify input color
        Color inputColor = Color(color.a * color.r, color.a * color.g, color.a * color.b, color.a);

        // Calculate for premultiplied color
        Color compositeColor = inputColor + (1 - inputColor.a) * premultipliedColor;

        supersample_buffer[screenPointIdx + supersampleIdx + 0] = (uint8_t) (compositeColor.r / compositeColor.a * 255);
        supersample_buffer[screenPointIdx + supersampleIdx + 1] = (uint8_t) (compositeColor.g / compositeColor.a * 255);
        supersample_buffer[screenPointIdx + supersampleIdx + 2] = (uint8_t) (compositeColor.b / compositeColor.a * 255);
        supersample_buffer[screenPointIdx + supersampleIdx + 3] = (uint8_t) (compositeColor.a * 255);

        count++;
      }
    }
  }
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 2: 
  // Implement line rasterization

  // First calculate the slope
  float dx = x1 - x0;
  float dy = y1 - y0;

  if (dy / dx >= 0 && dy / dx <= 1)
  {
    if (y1 <= y0 && x1 <= x0)
    {
      float tempx = x0;
      x0 = x1;
      x1 = tempx;
      
      float tempy = y0;
      y0 = y1;
      y1 = tempy;
      
      dx = x1 - x0;
      dy = y1 - y0;
    }
    int eps = 0;
    //int y = (int) floor(y0);
    float y = y0;

    for (int i = x0; i < x1; i++)
    {
      rasterize_point(i, y, color);
      if (2 * (eps + dy) < dx)
      {
        eps += dy;
      }
      else {
        y++;
        eps += dy - dx;
      }
    }
  }
  else if (dy / dx > 1)
  {
    if (x1 < x0)
    {
      float tempx = x0;
      x0 = x1;
      x1 = tempx;
      
      float tempy = y0;
      y0 = y1;
      y1 = tempy;
            
      dx = x1 - x0;
      dy = y1 - y0;
    }

    int eps = 0;
    float x = x0;

    for (int y = y0; y < y1; y++)
    {
      rasterize_point(x, y, color);
      if (2 * (eps + dx) < dy)
      {
        eps += dx;
      }
      else {
        x++;
        eps += dx - dy;
      }
    }
  }
  // For slop is negative
  else if (dy / dx < 0 && dy / dx >= -1)
  {
    if (y1 > y0)
    {
      float tempx = x0;
      x0 = x1;
      x1 = tempx;
      
      float tempy = y0;
      y0 = y1;
      y1 = tempy;
            
      dx = x1 - x0;
      dy = y1 - y0;
    }

    int eps = 0;
    float y = y0;

    for (int x = x0; x < x1; x++)
    {
      rasterize_point(x, y, color);
      if (2 * (eps + dy) > -dx)
      {
        eps += dy;
      }
      else {
        y--;
        eps += dy + dx;
      }
    }
  }
  else if (dy / dx < -1)
  {
    if (x1 >= x0)
    {
      float tempx = x0;
      x0 = x1;
      x1 = tempx;
      
      float tempy = y0;
      y0 = y1;
      y1 = tempy;
            
      dx = x1 - x0;
      dy = y1 - y0;
    }

    int eps = 0;
    //int x = (int) floor(x0);
    float x = x0;

    for (int y = y0; y < y1; y++)
    {
      rasterize_point(x, y, color);
      if (2 * (eps + dx) > -dy)
      {
        eps += dx;
      }
      else {
        x--;
        eps += dx + dy;
      }
    }
  }
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization

  // Calculate edge vectors of the triangle
  double vec1_x = x1 - x0, vec1_y = y1 - y0;
  double vec2_x = x2 - x1, vec2_y = y2 - y1;
  double vec3_x = x0 - x2, vec3_y = y0 - y2;

  // Get the bouding box coordinate for this triangle
  int min_x = max(0.f, min(x0, min(x1, x2)));
  int max_x = min((float) target_w, max(x0, max(x1, x2)));
  int min_y = max(0.f, min(y0, min(y1, y2)));
  int max_y = min((float) target_h, max(y0, max(y1, y2)));

  // For each pixel coord in the triangle bouding box
  for (int x = min_x; x <= max_x; x++)
  {
    for (int y = min_y; y <= max_y; y++)
    {
      float unit = 1.f / (2.f * sample_rate);   // Unit coord for each sample
      int count = 0;                            // num of samples in one pixel

      // For each sample point in this pixel coord
      for (int countX = 0; countX < sample_rate; countX++)
      {
        for (int countY = 0; countY < sample_rate; countY++)
        {
          //std::cout << "Sample point: (" << x + unit + 2 * countX * unit << "," << y + unit + 2 * countY * unit << ")" << std::endl;
          float sampleX = x + unit + 2 * countX * unit;
          float sampleY = y + unit + 2 * countY * unit;

          // Calculate sample - vertex vector
          float u1 = sampleX - x0, u2 = sampleY - y0;
          float v1 = sampleX - x1, v2 = sampleY - y1;
          float w1 = sampleX - x2, w2 = sampleY - y2;

          int screenPointIdx = sample_rate * sample_rate * 4 * (x + y * target_w);
          int supersampleIdx = count * 4;   // Each sample point will take 4 memory address

          // Use right hand rule to check if sample point is inside of the triangle
          if ((vec1_x * u2 - vec1_y * u1 > 0 && 
              vec2_x * v2 - vec2_y * v1 > 0 &&
              vec3_x * w2 - vec3_y * w1 > 0) ||
              (u1 * vec1_y - u2 * vec1_x > 0 &&
              v1 * vec2_y - v2 * vec2_x > 0 &&
              w1 * vec3_y - w2 * vec3_x > 0))
          {
            if (sample_rate == 1)
            {
              rasterize_point(x, y, color);
            }
            else
            {
              // Compute composite color
              float r = supersample_buffer[screenPointIdx + supersampleIdx + 0] / 255.f;
              float g = supersample_buffer[screenPointIdx + supersampleIdx + 1] / 255.f;
              float b = supersample_buffer[screenPointIdx + supersampleIdx + 2] / 255.f;
              float a = supersample_buffer[screenPointIdx + supersampleIdx + 3] / 255.f;
              Color premultipliedColor = Color(r, g, b, a);

              // Modify input color
              Color inputColor = Color(color.a * color.r, color.a * color.g, color.a * color.b, color.a);

              // Calculate for premultiplied color
              Color compositeColor = inputColor + (1 - inputColor.a) * premultipliedColor;

              supersample_buffer[screenPointIdx + supersampleIdx + 0] = (uint8_t) (compositeColor.r / compositeColor.a * 255);
              supersample_buffer[screenPointIdx + supersampleIdx + 1] = (uint8_t) (compositeColor.g / compositeColor.a * 255);
              supersample_buffer[screenPointIdx + supersampleIdx + 2] = (uint8_t) (compositeColor.b / compositeColor.a * 255);
              supersample_buffer[screenPointIdx + supersampleIdx + 3] = (uint8_t) (compositeColor.a * 255);
            }
          }
          count++;
        }
      }
    }
  }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization
  float imageWidth = x1 - x0;
  float imageHeight = y1 - y0;

  for (float x = floor(x0) + 0.5; x <= x1; x++)
  {
    for (float y = floor(y0) + 0.5; y <= y1; y++)
    {
      float u = (x - x0) / imageWidth;
      float v = (y - y0) / imageHeight;
      //Color color = sampler->sample_nearest(tex, u, v, 0);
      //Color color = sampler->sample_bilinear(tex, u, v, 0);

      Color color = sampler->sample_trilinear(tex, u, v, imageWidth, imageHeight);
      rasterize_point(x, y, color);
    }
  }

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".

  if (sample_rate == 1)
  {
    return;
  }

  // Now need to calculate the average color value for supersample buffer
  for (int x = 0; x < target_w; x++)
  {
    for (int y = 0; y < target_h; y++)
    {
      float r = 0.f, g = 0.f, b = 0.f, a = 0.f;
      int count = 0;
      for (int xSample = 0; xSample < sample_rate; xSample++)
      {
        for (int ySample = 0; ySample < sample_rate; ySample++)
        {
          r += supersample_buffer[sample_rate * sample_rate * 4 * (x + y * target_w) + count * 4 + 0];
          g += supersample_buffer[sample_rate * sample_rate * 4 * (x + y * target_w) + count * 4 + 1];
          b += supersample_buffer[sample_rate * sample_rate * 4 * (x + y * target_w) + count * 4 + 2];
          a += supersample_buffer[sample_rate * sample_rate * 4 * (x + y * target_w) + count * 4 + 3];

          count++;
        }
      }

      // Calculate final color
      r = r / pow(sample_rate, 2);
      g = g / pow(sample_rate, 2);
      b = b / pow(sample_rate, 2);
      a = a / pow(sample_rate, 2);

      // Push back to render target 
      render_target[4 * (x + y * target_w)    ] = (uint8_t) r;
      render_target[4 * (x + y * target_w) + 1] = (uint8_t) g;
      render_target[4 * (x + y * target_w) + 2] = (uint8_t) b;
      render_target[4 * (x + y * target_w) + 3] = (uint8_t) a;
    }
  }

  // Clean up supersample buffer
  memset(&supersample_buffer[0], 255, 4 * supersample_w * supersample_h);

  return;
}

} // namespace CMU462
