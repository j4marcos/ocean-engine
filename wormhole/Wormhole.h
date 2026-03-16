#ifndef WORMHOLE_H
#define WORMHOLE_H

#include <GLFW/glfw3.h>
#include "Math.h"

struct WormholeMouth {
    vec3 position;
    double mass;
    double radius;
    double r_s;

    WormholeMouth(vec3 pos, float m) : position(pos), mass(m) {
        r_s = 2.0 * G * mass / (c * c);
    }
    
    void draw(float r, float g, float b) {
        glPushMatrix();
        glTranslatef(position.x, position.y, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(r, g, b);               // Color for the wormhole
        glVertex2f(0.0f, 0.0f);           // Center
        for(int i = 0; i <= 100; i++) {
            float angle = 2.0f * M_PI * i / 100;
            float x = r_s * cos(angle);
            float y = r_s * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
        glPopMatrix();
    }
};

struct Wormhole {
    WormholeMouth mouthA;
    WormholeMouth mouthB;
    
    Wormhole(vec3 posA, vec3 posB, float m) 
        : mouthA(posA, m), mouthB(posB, m) {}
        
    void draw() {
        mouthA.draw(0.0f, 0.5f, 1.0f); // Blueish
        mouthB.draw(1.0f, 0.5f, 0.0f); // Orangeish
    }
};

#endif
