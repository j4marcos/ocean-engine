#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "Form.hpp"
#include <map>
#include <string>
#include <functional>

namespace Ocean
{

    // Forward declarations
    class Entity;
    class Body;

    /**
     * @brief Entity representa a "alma" de um ser vivo
     *
     * É a classe que controla o comportamento de personagens,
     * NPCs, e qualquer coisa que tenha "vida" no mundo.
     */
    class Entity
    {
    protected:
        Body *body;
        std::string name;

    public:
        Entity()
            : body(nullptr), name("Entity")
        {
        }

        virtual ~Entity() = default;

        // Getters
        Body *getBody() const { return body; }
        std::string getName() const { return name; }

        // Setters
        Entity &setBody(Body *b)
        {
            body = b;
            return *this;
        }

        Entity &setName(const std::string &n)
        {
            name = n;
            return *this;
        }

        // Atualização (para ser sobrescrita)
        virtual void update(float deltaTime)
        {
        }

        // Processa input (para ser sobrescrita)
        virtual void processInput(int key, bool pressed) {}
    };

    /**
     * @brief Body é a representação física de uma entidade
     */
    class Body : public Form
    {
    protected:
        Entity *soul;
        float mass;

    public:
        Body() : Form(), soul(nullptr), mass(1.0f) {}
        Body(const Point &pos, float radius)
            : Form(pos, radius), soul(nullptr), mass(1.0f) {}

        // Getters
        Entity *getSoul() const { return soul; }
        float getMass() const { return mass; }

        // Setters
        Body &setSoul(Entity *s)
        {
            soul = s;
            return *this;
        }

        Body &setMass(float m)
        {
            mass = m;
            return *this;
        }

        // Movimento
        void moveForward(float speed)
        {
            Direction dir = Direction::fromAngles(rotation.pitch, rotation.yaw);
            position.x += dir.x * speed;
            position.y += dir.y * speed;
            position.z += dir.z * speed;
        }

        void moveRight(float speed)
        {
            Direction forward = Direction::fromAngles(rotation.pitch, rotation.yaw);
            Direction right = forward.cross(Direction(0, 1, 0)).normalized();
            position.x += right.x * speed;
            position.y += right.y * speed;
            position.z += right.z * speed;
        }

        void moveUp(float speed)
        {
            position.y += speed;
        }

        void render() override
        {
            glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glRotatef(rotation.pitch * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
            glRotatef(rotation.yaw * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
            glRotatef(rotation.roll * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);

            glColor4f(color.r, color.g, color.b, color.a);

            // Renderiza como uma esfera por padrão
            GLUquadric *quad = gluNewQuadric();
            gluQuadricDrawStyle(quad, wireframe ? GLU_LINE : GLU_FILL);
            gluQuadricNormals(quad, GLU_SMOOTH);
            gluSphere(quad, radius, slices, stacks);
            gluDeleteQuadric(quad);

            glPopMatrix();
        }
    };

    /**
     * @brief Player é a entidade controlada pelo jogador
     */
    class Player : public Entity
    {
    private:
        float moveSpeed;
        float turnSpeed;

        // Estado dos controles
        bool movingForward;
        bool movingBackward;
        bool movingLeft;
        bool movingRight;
        bool movingUp;
        bool movingDown;

    public:
        Player()
            : Entity(), moveSpeed(5.0f), turnSpeed(2.0f), movingForward(false), movingBackward(false), movingLeft(false), movingRight(false), movingUp(false), movingDown(false)
        {
            name = "Player";
            body = new Body(Point(0, 0, 0), 0.5f);
            body->setSoul(this);
            body->setColor(RGBA(0.2f, 0.6f, 1.0f));
        }

        ~Player()
        {
            delete body;
        }

        // Getters
        float getMoveSpeed() const { return moveSpeed; }
        float getTurnSpeed() const { return turnSpeed; }

        // Setters
        Player &setMoveSpeed(float speed)
        {
            moveSpeed = speed;
            return *this;
        }

        Player &setTurnSpeed(float speed)
        {
            turnSpeed = speed;
            return *this;
        }

        // Controles - processa teclas (WASD + QE)
        void processInput(int key, bool pressed) override
        {
            switch (key)
            {
            case 'w':
            case 'W':
                movingForward = pressed;
                break;
            case 's':
            case 'S':
                movingBackward = pressed;
                break;
            case 'a':
            case 'A':
                movingLeft = pressed;
                break;
            case 'd':
            case 'D':
                movingRight = pressed;
                break;
            case 'q':
            case 'Q':
                movingDown = pressed;
                break;
            case 'e':
            case 'E':
                movingUp = pressed;
                break;
            }
        }

        // Processa movimento do mouse para rotação
        void processMouseMove(float deltaX, float deltaY)
        {
            if (body)
            {
                Rotation rot = body->getRotation();
                rot.yaw += deltaX * turnSpeed * 0.01f;
                rot.pitch += deltaY * turnSpeed * 0.01f;

                // Limita o pitch para evitar gimbal lock
                if (rot.pitch > M_PI / 2.0f)
                    rot.pitch = M_PI / 2.0f;
                if (rot.pitch < -M_PI / 2.0f)
                    rot.pitch = -M_PI / 2.0f;

                body->setRotation(rot);
            }
        }

        void update(float deltaTime) override
        {
            Entity::update(deltaTime);

            if (!body)
                return;

            float speed = moveSpeed * deltaTime;

            if (movingForward)
                body->moveForward(speed);
            if (movingBackward)
                body->moveForward(-speed);
            if (movingRight)
                body->moveRight(speed);
            if (movingLeft)
                body->moveRight(-speed);
            if (movingUp)
                body->moveUp(speed);
            if (movingDown)
                body->moveUp(-speed);
        }

        // Facilita acesso à posição do body
        Point getPosition() const
        {
            return body ? body->getPosition() : Point();
        }

        void setPosition(const Point &pos)
        {
            if (body)
                body->setPosition(pos);
        }

        void render()
        {
            if (body)
                body->render();
        }
    };

} // namespace Ocean

#endif // ENTITY_HPP
