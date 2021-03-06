#include <nes/nes.hpp>

#define VIDEO_CPP
namespace NES {

Video video;

unsigned Video::palette30(
  unsigned n, double saturation, double hue,
  double contrast, double brightness, double gamma
) {
  signed color = (n & 0x0f), level = color < 0xe ? (n >> 4) & 3 : 1;

  static const double black = 0.518, white = 1.962, attenuation = 0.746;
  static const double levels[8] = {
    0.350, 0.518, 0.962, 1.550,
    1.094, 1.506, 1.962, 1.962,
  };

  double lo_and_hi[2] = {
    levels[level + 4 * (color == 0x0)],
    levels[level + 4 * (color <  0xd)],
  };

  double y = 0.0, i = 0.0, q = 0.0;
  auto wave = [](signed p, signed color) { return (color + p + 8) % 12 < 6; };
  for(signed p = 0; p < 12; p++) {
    double spot = lo_and_hi[wave(p, color)];

    if(((n & 0x040) && wave(p, 12))
    || ((n & 0x080) && wave(p,  4))
    || ((n & 0x100) && wave(p,  8))
    ) spot *= attenuation;

    double v = (spot - black) / (white - black);

    v = (v - 0.5) * contrast + 0.5;
    v *= brightness / 12.0;

    y += v;
    i += v * std::cos((3.141592653 / 6.0) * (p + hue));
    q += v * std::sin((3.141592653 / 6.0) * (p + hue));
  }

  i *= saturation;
  q *= saturation;

  auto gammaAdjust = [=](double f) { return f < 0.0 ? 0.0 : std::pow(f, 2.2 / gamma); };
  return (uclamp<10>(1023.0 * gammaAdjust(y +  0.946882 * i +  0.623557 * q)) << 20)
       + (uclamp<10>(1023.0 * gammaAdjust(y + -0.274788 * i + -0.635691 * q)) << 10)
       + (uclamp<10>(1023.0 * gammaAdjust(y + -1.108545 * i +  1.709007 * q)) <<  0);
}

void Video::generate(Format format) {
  for(unsigned n = 0; n < (1 << 9); n++) palette[n] = palette30(n, 2.0, 0.0, 1.0, 1.0, 1.8);

  if(format == Format::RGB24) {
    for(unsigned n = 0; n < (1 << 9); n++) {
      unsigned color = palette[n];
      palette[n] = ((color >> 6) & 0xff0000) + ((color >> 4) & 0x00ff00) + ((color >> 2) & 0x0000ff);
    }
  }

  if(format == Format::RGB16) {
    for(unsigned n = 0; n < (1 << 9); n++) {
      unsigned color = palette[n];
      palette[n] = ((color >> 14) & 0xf800) + ((color >> 9) & 0x07e0) + ((color >> 5) & 0x001f);
    }
  }

  if(format == Format::RGB15) {
    for(unsigned n = 0; n < (1 << 9); n++) {
      unsigned color = palette[n];
      palette[n] = ((color >> 15) & 0x7c00) + ((color >> 10) & 0x03e0) + ((color >> 5) & 0x001f);
    }
  }
}

Video::Video() {
  palette = new unsigned[1 << 9];
}

Video::~Video() {
  delete[] palette;
}

}
