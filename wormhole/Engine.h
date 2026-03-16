#ifndef ENGINE_H
#define ENGINE_H

#include <GLFW/glfw3.h>
#include "Math.h"

struct Engine {
    GLFWwindow* window;
    int WIDTH = 1024;
    int HEIGHT = 768;
    float width = 100000000000.0f;
    float height = 75000000000.0f;

    float offsetX = 0.0f, offsetY = 0.0f;

    Engine() {
        if (!glfwInit()) {
            cerr << "Failed to initialize GLFW" << endl;
            exit(EXIT_FAILURE);
        }
        window = glfwCreateWindow(WIDTH, HEIGHT, "2D Wormhole Simulation", NULL, NULL);
        if (!window) {
            cerr << "Failed to create GLFW window" << endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
        glViewport(0, 0, WIDTH, HEIGHT);
    }

    void run() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double left   = -width + offsetX;
        double right  =  width + offsetX;
        double bottom = -height + offsetY;
        double top    =  height + offsetY;
        glOrtho(left, right, bottom, top, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
    
    ~Engine() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

extern Engine engine;

#endif
