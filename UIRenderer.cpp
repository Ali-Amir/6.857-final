#include "UIRenderer.hpp"

#include <iostream>

namespace ui {

UIRenderer::~UIRenderer() {
  SDL_DestroyRenderer(_ren);
  SDL_DestroyWindow(_win);
  SDL_Quit();
}

void UIRenderer::drawElements() {
  for (const auto &kv : _circles) {
    const auto &cir = kv.second;
    filledCircleColor(_ren, cir.x, cir.y, cir.rad, cir.color);
  }
}

void UIRenderer::drawScene() {
  // Set color that will be used for clearing.
  SDL_SetRenderDrawColor(_ren, 255, 255, 255, 255);
  //First clear the renderer
  SDL_RenderClear(_ren);
  drawElements();
}

void UIRenderer::_Init(int argc, char **argv) {
  //First we need to start up SDL, and make sure it went ok
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
    exit(1);
  }

  //Now create a window with title "Hello World" at 100, 100 on the screen with w:640 h:480 and show it
  _win = SDL_CreateWindow("Hello World!", 100, 100, 1024, 840, SDL_WINDOW_SHOWN);
  //Make sure creating our window went ok
  if (_win == nullptr) {
    std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    exit(1);
  }

  //Create a renderer that will draw to the window, -1 specifies that we want to load whichever
  //video driver supports the flags we're passing
  //Flags: SDL_RENDERER_ACCELERATED: We want to use hardware accelerated rendering
  //SDL_RENDERER_PRESENTVSYNC: We want the renderer's present function (update screen) to be
  //synchornized with the monitor's refresh rate
  _ren = SDL_CreateRenderer(_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (_ren == nullptr) {
    SDL_DestroyWindow(_win);
    std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(1);
  }
}

void UIRenderer::update() {
  drawScene();
  SDL_RenderPresent(_ren);
}

} // namespace ui
