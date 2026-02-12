#ifndef FORM_HPP
#define FORM_HPP

#include "Element.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>

namespace Ocean {

/**
 * @brief Classe base para formas 3D
 */
class Form : public Element {
protected:
    int slices;  // Subdivisões em longitude
    int stacks;  // Subdivisões em latitude
    bool wireframe;

public:
    Form() : Element(), slices(16), stacks(16), wireframe(false) {}
    Form(const Point& pos, float radius) 
        : Element(pos, radius), slices(16), stacks(16), wireframe(false) {}

    Form& setSlices(int s) {
        slices = s;
        return *this;
    }

    Form& setStacks(int s) {
        stacks = s;
        return *this;
    }

    Form& setWireframe(bool w) {
        wireframe = w;
        return *this;
    }

    int getSlices() const { return slices; }
    int getStacks() const { return stacks; }
    bool isWireframe() const { return wireframe; }
};

/**
 * @brief Esfera 3D
 */
class Sphere : public Form {
public:
    Sphere() : Form() {}
    Sphere(const Point& center, float diameter) : Form(center, diameter / 2.0f) {}

    Sphere& setDiameter(float d) {
        radius = d / 2.0f;
        return *this;
    }

    float getDiameter() const { return radius * 2.0f; }

    void render() override {
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(rotation.pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
        glRotatef(rotation.yaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        glRotatef(rotation.roll * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        glColor4f(color.r, color.g, color.b, color.a);

        GLUquadric* quad = gluNewQuadric();
        if (wireframe) {
            gluQuadricDrawStyle(quad, GLU_LINE);
        } else {
            gluQuadricDrawStyle(quad, GLU_FILL);
        }
        gluQuadricNormals(quad, GLU_SMOOTH);
        gluSphere(quad, radius, slices, stacks);
        gluDeleteQuadric(quad);

        glPopMatrix();
    }
};

/**
 * @brief Cubo/Caixa 3D
 */
class Box : public Form {
private:
    float width, height, depth;

public:
    Box() : Form(), width(1.0f), height(1.0f), depth(1.0f) {}
    Box(const Point& pos, float w, float h, float d) 
        : Form(pos, 1.0f), width(w), height(h), depth(d) {}

    Box& setDimensions(float w, float h, float d) {
        width = w;
        height = h;
        depth = d;
        return *this;
    }

    void render() override {
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(rotation.pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
        glRotatef(rotation.yaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        glRotatef(rotation.roll * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        glColor4f(color.r, color.g, color.b, color.a);

        float hw = width / 2.0f;
        float hh = height / 2.0f;
        float hd = depth / 2.0f;

        if (wireframe) {
            glBegin(GL_LINE_LOOP);
        } else {
            glBegin(GL_QUADS);
        }

        // Frente
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-hw, -hh, hd);
        glVertex3f(hw, -hh, hd);
        glVertex3f(hw, hh, hd);
        glVertex3f(-hw, hh, hd);

        // Trás
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-hw, -hh, -hd);
        glVertex3f(-hw, hh, -hd);
        glVertex3f(hw, hh, -hd);
        glVertex3f(hw, -hh, -hd);

        // Topo
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-hw, hh, -hd);
        glVertex3f(-hw, hh, hd);
        glVertex3f(hw, hh, hd);
        glVertex3f(hw, hh, -hd);

        // Base
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-hw, -hh, -hd);
        glVertex3f(hw, -hh, -hd);
        glVertex3f(hw, -hh, hd);
        glVertex3f(-hw, -hh, hd);

        // Direita
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(hw, -hh, -hd);
        glVertex3f(hw, hh, -hd);
        glVertex3f(hw, hh, hd);
        glVertex3f(hw, -hh, hd);

        // Esquerda
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-hw, -hh, -hd);
        glVertex3f(-hw, -hh, hd);
        glVertex3f(-hw, hh, hd);
        glVertex3f(-hw, hh, -hd);

        glEnd();
        glPopMatrix();
    }
};

/**
 * @brief Torus 3D (anel 3D - anel de saturno por exemplo)
 */
class Torus : public Form {
private:
    float innerRadius;

public:
    Torus() : Form(), innerRadius(0.3f) {
        radius = 1.0f;
    }
    
    Torus(const Point& center, float outerRadius, float tubeRadius)
        : Form(center, outerRadius), innerRadius(tubeRadius) {}

    Torus& setInnerRadius(float r) {
        innerRadius = r;
        return *this;
    }

    Torus& setOuterRadius(float r) {
        radius = r;
        return *this;
    }

    void render() override {
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(rotation.pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
        glRotatef(rotation.yaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        glRotatef(rotation.roll * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        glColor4f(color.r, color.g, color.b, color.a);

        // Desenha o torus manualmente
        for (int i = 0; i < slices; ++i) {
            glBegin(wireframe ? GL_LINE_LOOP : GL_QUAD_STRIP);
            for (int j = 0; j <= stacks; ++j) {
                for (int k = 0; k <= 1; ++k) {
                    float s = (i + k) % slices;
                    float t = j % stacks;

                    float theta = s * 2.0f * M_PI / slices;
                    float phi = t * 2.0f * M_PI / stacks;

                    float x = (radius + innerRadius * std::cos(phi)) * std::cos(theta);
                    float y = (radius + innerRadius * std::cos(phi)) * std::sin(theta);
                    float z = innerRadius * std::sin(phi);

                    glNormal3f(std::cos(phi) * std::cos(theta),
                              std::cos(phi) * std::sin(theta),
                              std::sin(phi));
                    glVertex3f(x, y, z);
                }
            }
            glEnd();
        }

        glPopMatrix();
    }
};

} // namespace Ocean

#endif // FORM_HPP
