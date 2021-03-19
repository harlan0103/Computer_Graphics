#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width; 
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors (JUST AS A PLACEHOLDER)
  //Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    //std::cout << "mipmap level: " << i << "; size: " << tex.mipmap[i].width << " x " << tex.mipmap[i].height << std::endl;
    //Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    // For each texel point at current level
    for (size_t x = 0; x < mip.width; x++)
    {
      for (size_t y = 0; y < mip.height; y++)
      {
        // Find indx of corresponding 4 texels from the upper level
        int idx00 = 4 * ((x * 2) + (y * 2) * tex.mipmap[i - 1].width);
        int idx10 = 4 * ((x * 2 + 1) + (y * 2) * tex.mipmap[i - 1].width);
        int idx01 = 4 * ((x * 2) + (y * 2 + 1) * tex.mipmap[i - 1].width);
        int idx11 = 4 * ((x * 2 + 1) + (y * 2 + 1) * tex.mipmap[i - 1].width);

        int idx[4] = { idx00, idx10, idx01, idx11 };

        Color c = Color(0, 0, 0, 0);

        // Get r g b a on each upper level texel
        for (int j = 0; j < 4; j++)
        {
          c.r += tex.mipmap[i - 1].texels[idx[j] + 0];
          c.g += tex.mipmap[i - 1].texels[idx[j] + 1];
          c.b += tex.mipmap[i - 1].texels[idx[j] + 2];
          c.a += tex.mipmap[i - 1].texels[idx[j] + 3];
        }

        // Mix them and push back to current level texel buffer
        //c = c / 4.f;
        Color texelColor = Color(c.r/4.f, c.g/4.f, c.b/4.f, c.a/4.f);

        mip.texels[4 * (x + y * mip.width) + 0] = texelColor.r;
        mip.texels[4 * (x + y * mip.width) + 1] = texelColor.g;
        mip.texels[4 * (x + y * mip.width) + 2] = texelColor.b;
        mip.texels[4 * (x + y * mip.width) + 3] = texelColor.a;
      }
    }
  }
}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 6: Implement nearest neighbour interpolation
  
  // Find the nearest point
  clamp<float>(u, 0.f, 1.f);
  clamp<float>(v, 0.f, 1.f);

  if (u < 0.f) u = 0.f;
  if (v < 0.f) v = 0.f;
  
  int x = u * tex.mipmap[level].width;
  int y = v * tex.mipmap[level].height;

  if (x > tex.mipmap[level].width || y > tex.mipmap[level].height)
  {
    return Color(1, 0, 1, 1);
  }

  //std::cout << "x, y" << x << ", " << y << std::endl;

  MipLevel& mip = tex.mipmap[level];

  // Get the color from nearest point
  float r = mip.texels[4 * (x + y * mip.width)    ] / 255.f;
  float g = mip.texels[4 * (x + y * mip.width) + 1] / 255.f;
  float b = mip.texels[4 * (x + y * mip.width) + 2] / 255.f;
  float a = mip.texels[4 * (x + y * mip.width) + 3] / 255.f;

  return Color(r, g, b, a);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 6: Implement bilinear filtering
  // Find Bilinear interpolation

  clamp<float>(u, 0.f, 1.f);
  clamp<float>(v, 0.f, 1.f);

  if (u < 0.f) u = 0.f;
  if (v < 0.f) v = 0.f;

  float texU = u * tex.mipmap[level].width;
  float texV = v * tex.mipmap[level].height;

  // Calculate 4 nearest points
  int x0 = (int) floor(texU);
  int y0 = (int) floor(texV);

  int x1 = (texU - x0 > 0.5) ? x0 + 1 : x0 - 1;
  int y1 = (texV - y0 > 0.5) ? y0 + 1 : y0 - 1;

  if (x1 < 0) x1 = 0;
  else if (x1 > tex.mipmap[level].width - 1) x1 = tex.mipmap[level].width - 1;
  if (y1 < 0) y1 = 0;
  else if (y1 > tex.mipmap[level].height - 1) y1 = tex.mipmap[level].height - 1;

  //std::cout << x0 << "." << y0 << "." << x1 << "." << y1 << std::endl;

  float s = texU - (x0 + 0.5);
  float t = texV - (y0 + 0.5);

  if (s < 0) s *= -1.f;
  if (t < 0) t *= -1.f;

  MipLevel& mip = tex.mipmap[level];

  // Find four neighbor colors
  int idx00 = 4 * (x0 + y0 * mip.width);
  int idx10 = 4 * (x1 + y0 * mip.width);
  int idx01 = 4 * (x0 + y1 * mip.width);
  int idx11 = 4 * (x1 + y1 * mip.width);
  
  int idx[4] = { idx00, idx10, idx01, idx11 };

  Color colors[4] = { Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0) };
  for (int i = 0; i < 4; i++) 
  {
    colors[i].r = mip.texels[idx[i] + 0] / 255.f;
    colors[i].g = mip.texels[idx[i] + 1] / 255.f;
    colors[i].b = mip.texels[idx[i] + 2] / 255.f;
    colors[i].a = mip.texels[idx[i] + 3] / 255.f;
  }

  // colors[ c00, c10, c01, c11 ]
  // Perform interpolation
  Color color = (1 - t) * ((1 - s) * colors[0] + s * colors[1]) + t * ((1 - s) * colors[2] + s * colors[3]);

  return color;
}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering

  // du/dx = (x2 - x1) / imageWidth * mipmapWidth => mipmapwidth / imageWidth
  // dv/dx = 0
  // du/dy = 0
  // dv/dy = (y2 - y1) / imageHeight * mipmapHeight => mipmapHeight / imageHeight
  float L = max(tex.width / u_scale, tex.height / v_scale);
  float level = log2f(L);

  // Set the level inside of the boundary
  if (level < 0)
  {
    return sample_bilinear(tex, u, v, 0);
  }
  else if (level > tex.mipmap.size() - 1)
  {
    level = tex.mipmap.size() - 2;
  }

  //std::cout << level << std::endl;

  int level1 = (int) floor(level);
  int level2 = level1 + 1;
  float h = level - level1;

  // Perform bilinear interpolation on each level and linear interpolation between levels
  Color texelColor = (1 - h) * sample_bilinear(tex, u, v, level1) + h * sample_bilinear(tex, u, v, level2);

  return texelColor;
}

} // namespace CMU462
