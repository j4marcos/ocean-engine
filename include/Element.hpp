#ifndef ELEMENT_HPP
#define ELEMENT_HPP

#include "Math3D.hpp"

namespace Ocean
{

    /**
     * @brief Classe base para qualquer objeto no espaço 3D
     *
     * Element é a classe fundamental da engine. Qualquer coisa que existe
     * no mundo do jogo herda de Element.
     */
    class Element
    {
    protected:
        Point position;
        float radius;
        Rotation rotation;
        Point scale;
        float velocity;
        Direction movementDirection;
        RGBA color;

    public:
        Element()
            : position(originPoint), radius(1.0f), rotation(), scale(Point(1.0f, 1.0f, 1.0f)), velocity(0.0f), movementDirection(), color()
        {
        }

        Element(const Point &pos, float r = 1.0f)
            : position(pos), radius(r), rotation(), scale(Point(1.0f, 1.0f, 1.0f)), velocity(0.0f), movementDirection(), color()
        {
        }

        virtual ~Element() = default;

        // Getters
        Point getPosition() const { return position; }
        float getRadius() const { return radius; }
        Rotation getRotation() const { return rotation; }
        Point getScale() const { return scale; }
        float getVelocity() const { return velocity; }
        Direction getMovementDirection() const { return movementDirection; }

        // Setters fluentes (retornam referência para encadeamento)
        Element &setPosition(const Point &pos)
        {
            position = pos;
            return *this;
        }

        Element &setRadius(float r)
        {
            radius = r;
            return *this;
        }

        Element &setColor(const RGBA &c)
        {
            color = c;
            return *this;
        }

        Element &setRotation(const Rotation &rot)
        {
            rotation = rot;
            return *this;
        }

        Element &setRotation(float pitch, float yaw, float roll)
        {
            rotation = Rotation(pitch, yaw, roll);
            return *this;
        }

        Element &setScale(const Point &s)
        {
            scale = s;
            return *this;
        }

        Element &setScale(float x, float y, float z)
        {
            scale = Point(x, y, z);
            return *this;
        }

        Element &setVelocity(float v)
        {
            velocity = v;
            return *this;
        }

        Element &setMovementDirection(const Direction &dir)
        {
            movementDirection = dir;
            return *this;
        }

        // Aplica movimento ao elemento
        void applyMovement(const Direction &direction)
        {
            movementDirection = movementDirection + direction;
        }

        // Atualiza a posição baseado na velocidade e direção
        virtual void update(float deltaTime)
        {
            if (velocity != 0)
            {
                Direction normalizedDir = movementDirection.normalized();
                position.x += normalizedDir.x * velocity * deltaTime;
                position.y += normalizedDir.y * velocity * deltaTime;
                position.z += normalizedDir.z * velocity * deltaTime;
            }
        }

        // Método virtual puro para renderização
        virtual void render() = 0;
    };

} // namespace Ocean

#endif // ELEMENT_HPP
