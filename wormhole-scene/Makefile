# Run from this directory: make
# Or from repo root: make -C wormhole-scene

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
LDFLAGS  := -lGL -lGLU -lglut -ldl

INCLUDES := -I. -Iaudio -Isimulation -Itexture -Ioverlay -Iraster -Iraycast -Iscene

SRCS := \
	audio/wormhole3d_audio.cpp \
	wormhole3d_globals.cpp \
	scene/scene_gpu.cpp \
	scene/scene_prefabs.cpp \
	scene/scene_world.cpp \
	scene/scene_moving.cpp \
	simulation/simulation.cpp \
	texture/wormhole3d_init.cpp \
	raster/wormhole3d_raster.cpp \
	raster/wormhole3d_portals.cpp \
	raycast/mode_cpu.cpp \
	raycast/mode_gpu.cpp \
	raycast/raycast.cpp \
	overlay/wormhole3d_ui.cpp \
	wormhole3d_main.cpp

OBJS := $(SRCS:.cpp=.o)

TARGET := wormhole3d_demo

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
