#ifndef SHAPE_HPP
#define SHAPE_HPP

#include "Element.hpp"
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>

namespace Ocean {

/**
 * @brief Classe base para figuras 2D
 */
class Shape : public Element {
protected:
    int segments; // Número de segmentos para desenhar curvas

public:
    Shape() : Element(), segments(32) {}
    Shape(const Point& pos, float radius) : Element(pos, radius), segments(32) {}

    Shape& setSegments(int s) {
        segments = s;
        return *this;
    }

    int getSegments() const { return segments; }
};

/**
 * @brief Círculo 2D
 */
class Circle : public Shape {
public:
    Circle() : Shape() {}
    Circle(const Point& center, float diameter) : Shape(center, diameter / 2.0f) {}

    Circle& setDiameter(float d) {
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

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            glVertex3f(x, y, 0.0f);
        }
        glEnd();

        glPopMatrix();
    }

    // Desenha círculo preenchido
    void renderFilled() {
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glColor4f(color.r, color.g, color.b, color.a);

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f); // Centro
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float x = radius * std::cos(angle);
            float y = radius * std::sin(angle);
            glVertex3f(x, y, 0.0f);
        }
        glEnd();

        glPopMatrix();
    }
};

/**
 * @brief Anel 2D (diferença entre dois círculos)
 */
class Ring : public Shape {
private:
    float innerRadius;

public:
    Ring() : Shape(), innerRadius(0.5f) {}
    Ring(const Point& center, float outerRadius, float innerRadius)
        : Shape(center, outerRadius), innerRadius(innerRadius) {}

    Ring& setInnerRadius(float r) {
        innerRadius = r;
        return *this;
    }

    Ring& setOuterRadius(float r) {
        radius = r;
        return *this;
    }

    float getInnerRadius() const { return innerRadius; }
    float getOuterRadius() const { return radius; }

    void render() override {
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(rotation.pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
        glRotatef(rotation.yaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
        glRotatef(rotation.roll * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
        glScalef(scale.x, scale.y, scale.z);

        glColor4f(color.r, color.g, color.b, color.a);

        // Desenha o anel como uma série de quadriláteros
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * M_PI * i / segments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            
            glVertex3f(innerRadius * cosA, innerRadius * sinA, 0.0f);
            glVertex3f(radius * cosA, radius * sinA, 0.0f);
        }
        glEnd();

        glPopMatrix();
    }
};

} // namespace Ocean

#endif // SHAPE_HPP