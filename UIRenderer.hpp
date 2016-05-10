#ifndef __UIRENDERER_HPP__
#define __UIRENDERER_HPP__

#include "Utils.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <cstring>
#include <memory>
#include <map>

namespace ui {

class UIRenderer {

public:

  UIRenderer(int width, int height, int argc, char **argv) : _width(width), _height(height), _win(nullptr), _ren(nullptr) {
    _Init(argc, argv);
  }

  ~UIRenderer();

  void set(const std::string &mac, const utils::Circle &circle) {
    _circles[mac] = circle;
  }

  void _Init(int argc, char **argv);
  void drawScene();
  void update();

private:

  void drawElements();

private:

  static void drawSceneStatic() {
    _staticThis->drawScene();
  }

  static UIRenderer *_staticThis;

private:

  int _width;
  int _height;
  std::map<std::string, utils::Circle> _circles;

  // SDL stuff.
  SDL_Window* _win;
  SDL_Renderer* _ren;

};

} // namespace ui

#endif // __UIRENDERER_HPP__
