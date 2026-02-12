#ifndef MATH3D_HPP
#define MATH3D_HPP

#include <cmath>
#include <iostream>

namespace Ocean
{

    /**
     * @brief Representa um ponto no espaço 3D
     */
    class Point
    {
    public:
        float x, y, z;

        Point() : x(0.0f), y(0.0f), z(0.0f) {}
        Point(float x, float y, float z) : x(x), y(y), z(z) {}

        Point operator+(const Point &other) const
        {
            return Point(x + other.x, y + other.y, z + other.z);
        }

        Point operator-(const Point &other) const
        {
            return Point(x - other.x, y - other.y, z - other.z);
        }

        Point operator*(float scalar) const
        {
            return Point(x * scalar, y * scalar, z * scalar);
        }

        Point &operator+=(const Point &other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        float distanceTo(const Point &other) const
        {
            float dx = x - other.x;
            float dy = y - other.y;
            float dz = z - other.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        void print() const
        {
            std::cout << "Point(" << x << ", " << y << ", " << z << ")" << std::endl;
        }
    };

    /**
     * @brief Representa uma direção/vetor no espaço 3D
     */
    class Direction
    {
    public:
        float x, y, z;

        Direction() : x(0.0f), y(0.0f), z(1.0f) {}
        Direction(float x, float y, float z) : x(x), y(y), z(z) {}

        // Cria direção a partir de ângulos (pitch, yaw, roll em radianos)
        static Direction fromAngles(float pitch, float yaw, float roll = 0.0f)
        {
            Direction dir;
            dir.x = std::cos(pitch) * std::sin(yaw);
            dir.y = std::sin(pitch);
            dir.z = std::cos(pitch) * std::cos(yaw);
            return dir;
        }

        float magnitude() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }

        Direction normalized() const
        {
            float mag = magnitude();
            if (mag == 0)
                return Direction(0, 0, 1);
            return Direction(x / mag, y / mag, z / mag);
        }

        Direction operator+(const Direction &other) const
        {
            return Direction(x + other.x, y + other.y, z + other.z);
        }

        Direction operator*(float scalar) const
        {
            return Direction(x * scalar, y * scalar, z * scalar);
        }

        // Produto escalar
        float dot(const Direction &other) const
        {
            return x * other.x + y * other.y + z * other.z;
        }

        // Produto vetorial
        Direction cross(const Direction &other) const
        {
            return Direction(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
        }

        void print() const
        {
            std::cout << "Direction(" << x << ", " << y << ", " << z << ")" << std::endl;
        }
    };

    /**
     * @brief Representa uma cor RGB
     */
    struct RGBA
    {
        float r, g, b, a;

        RGBA() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
        RGBA(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

        static RGBA white() { return RGBA(1.0f, 1.0f, 1.0f); }
        static RGBA black() { return RGBA(0.0f, 0.0f, 0.0f); }
        static RGBA red() { return RGBA(1.0f, 0.0f, 0.0f); }
        static RGBA green() { return RGBA(0.0f, 1.0f, 0.0f); }
        static RGBA blue() { return RGBA(0.0f, 0.0f, 1.0f); }
        static RGBA yellow() { return RGBA(1.0f, 1.0f, 0.0f); }
        static RGBA orange() { return RGBA(1.0f, 0.5f, 0.0f); }
    };

    /**
     * @brief Representa uma rotação em 3D (ângulos de Euler)
     */
    struct Rotation
    {
        float pitch; // Rotação em X
        float yaw;   // Rotação em Y
        float roll;  // Rotação em Z

        Rotation() : pitch(0.0f), yaw(0.0f), roll(0.0f) {}
        Rotation(float pitch, float yaw, float roll)
            : pitch(pitch), yaw(yaw), roll(roll) {}
    };

    // Ponto de origem global
    static Point originPoint(0.0f, 0.0f, 0.0f);

} // namespace Ocean

#endif // MATH3D_HPP
