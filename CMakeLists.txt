cmake_minimum_required (VERSION 2.6)
project (Maradeurs)

include_directories(${PROJECT_BINARY_DIR}
                    /usr/local/include
                    /opt/local/include)
link_directories(/usr/local/lib)

add_definitions(-std=c++14)

add_executable(Server server.cpp UIRenderer.cpp)
target_link_libraries(Server
  SimpleAmqpClient
  tins
  sdl2
  sdl2_gfx
  "-framework OpenGL"
  "-framework GLUT"
)

add_executable(Sniffer sniffer.cpp)
target_link_libraries(Sniffer
  SimpleAmqpClient
  tins
)
