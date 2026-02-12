#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Element.hpp"
#include <GL/gl.h>
#include <GL/glu.h>

namespace Ocean
{

    /**
     * @brief Câmera para visualização da cena
     *
     * A câmera pode seguir elementos e olhar para direções ou objetos específicos.
     */
    class Camera : public Element
    {
    private:
        Element *followTarget;   // Elemento que a câmera segue
        Element *lookAtTarget;   // Elemento para o qual a câmera olha
        Direction lookDirection; // Direção para onde a câmera olha
        Direction upVector;      // Vetor "para cima" da câmera

        Point followOffset; // Offset em relação ao alvo seguido
        bool isFollowing;
        bool isLookingAtElement;

        // Parâmetros de projeção
        float fov; // Campo de visão
        float nearPlane;
        float farPlane;
        float aspectRatio;

        // Ângulos de rotação orbital (quando olhando para um alvo)
        float orbitalYaw;   // Rotação horizontal em torno do alvo
        float orbitalPitch; // Rotação vertical em torno do alvo
        float orbitalDistance; // Distância do alvo

    public:
        Camera()
            : Element(), followTarget(nullptr), lookAtTarget(nullptr), lookDirection(0.0f, 0.0f, -1.0f), upVector(0.0f, 1.0f, 0.0f), followOffset(0.0f, 2.0f, 10.0f), isFollowing(false), isLookingAtElement(false), fov(45.0f), nearPlane(0.1f), farPlane(1000.0f), aspectRatio(16.0f / 9.0f), orbitalYaw(0.0f), orbitalPitch(0.3f), orbitalDistance(15.0f)
        {
        }

        Camera(const Point &pos)
            : Element(pos, 0.0f), followTarget(nullptr), lookAtTarget(nullptr), lookDirection(0.0f, 0.0f, -1.0f), upVector(0.0f, 1.0f, 0.0f), followOffset(0.0f, 2.0f, 10.0f), isFollowing(false), isLookingAtElement(false), fov(45.0f), nearPlane(0.1f), farPlane(1000.0f), aspectRatio(16.0f / 9.0f), orbitalYaw(0.0f), orbitalPitch(0.3f), orbitalDistance(15.0f)
        {
        }

        // Configura a câmera para seguir um elemento
        Camera &follow(Element *element)
        {
            if (element && !isFollowing)
            {
                // Calcula o offset inicial baseado na posição atual
                followOffset = position - element->getPosition();
            }
            followTarget = element;
            isFollowing = (element != nullptr);
            return *this;
        }

        // Define o offset de seguimento
        Camera &setFollowOffset(const Point &offset)
        {
            followOffset = offset;
            return *this;
        }

        // Para de seguir
        Camera &stopFollowing()
        {
            followTarget = nullptr;
            isFollowing = false;
            return *this;
        }

        // Olha para uma direção específica
        Camera &lookAt(const Direction &dir)
        {
            lookDirection = dir.normalized();
            isLookingAtElement = false;
            lookAtTarget = nullptr;
            return *this;
        }

        // Olha para um elemento específico
        Camera &lookAt(Element *element)
        {
            lookAtTarget = element;
            isLookingAtElement = (element != nullptr);
            
            // Calcula ângulos e distância inicial
            if (element) {
                Point targetPos = element->getPosition();
                Direction toTarget(
                    targetPos.x - position.x,
                    targetPos.y - position.y,
                    targetPos.z - position.z
                );
                orbitalDistance = std::sqrt(
                    toTarget.x * toTarget.x + 
                    toTarget.y * toTarget.y + 
                    toTarget.z * toTarget.z
                );
                
                // Calcula ângulos
                orbitalYaw = std::atan2(toTarget.x, toTarget.z);
                orbitalPitch = std::atan2(toTarget.y, std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z));
            }
            
            return *this;
        }

        // Rotaciona a câmera (em torno do alvo se estiver olhando para um elemento)
        Camera &rotate(float deltaYaw, float deltaPitch)
        {
            if (isLookingAtElement && lookAtTarget) {
                // Rotação orbital ao redor do alvo
                orbitalYaw += deltaYaw;
                orbitalPitch += deltaPitch;
                
                // Limita pitch para evitar gimbal lock
                const float maxPitch = M_PI / 2.0f - 0.1f;
                if (orbitalPitch > maxPitch) orbitalPitch = maxPitch;
                if (orbitalPitch < -maxPitch) orbitalPitch = -maxPitch;
                
                // Recalcula posição da câmera
                Point targetPos = lookAtTarget->getPosition();
                position.x = targetPos.x + orbitalDistance * std::sin(orbitalYaw) * std::cos(orbitalPitch);
                position.z = targetPos.z + orbitalDistance * std::cos(orbitalYaw) * std::cos(orbitalPitch);
                position.y = targetPos.y + orbitalDistance * std::sin(orbitalPitch);
                
            } else {
                // Rotação livre da direção de visão
                // Converte lookDirection para ângulos
                float currentYaw = std::atan2(lookDirection.x, lookDirection.z);
                float currentPitch = std::atan2(lookDirection.y, std::sqrt(lookDirection.x * lookDirection.x + lookDirection.z * lookDirection.z));
                
                currentYaw += deltaYaw;
                currentPitch += deltaPitch;
                
                // Limita pitch
                const float maxPitch = M_PI / 2.0f - 0.1f;
                if (currentPitch > maxPitch) currentPitch = maxPitch;
                if (currentPitch < -maxPitch) currentPitch = -maxPitch;
                
                // Reconstrói lookDirection
                lookDirection.x = std::sin(currentYaw) * std::cos(currentPitch);
                lookDirection.z = std::cos(currentYaw) * std::cos(currentPitch);
                lookDirection.y = std::sin(currentPitch);
                lookDirection = lookDirection.normalized();
            }
            
            return *this;
        }

        // Ajusta a distância orbital (zoom)
        Camera &zoom(float delta)
        {
            orbitalDistance += delta;
            if (orbitalDistance < 1.0f) orbitalDistance = 1.0f;
            
            if (isLookingAtElement && lookAtTarget) {
                // Recalcula posição
                Point targetPos = lookAtTarget->getPosition();
                position.x = targetPos.x + orbitalDistance * std::sin(orbitalYaw) * std::cos(orbitalPitch);
                position.z = targetPos.z + orbitalDistance * std::cos(orbitalYaw) * std::cos(orbitalPitch);
                position.y = targetPos.y + orbitalDistance * std::sin(orbitalPitch);
            }
            
            return *this;
        }

        // Configura os parâmetros de projeção
        Camera &setProjection(float fieldOfView, float aspect, float near, float far)
        {
            fov = fieldOfView;
            aspectRatio = aspect;
            nearPlane = near;
            farPlane = far;
            return *this;
        }

        Camera &setFOV(float fieldOfView)
        {
            fov = fieldOfView;
            return *this;
        }

        Camera &setAspectRatio(float aspect)
        {
            aspectRatio = aspect;
            return *this;
        }

        Camera &setUpVector(const Direction &up)
        {
            upVector = up;
            return *this;
        }

        // Getters
        float getFOV() const { return fov; }
        float getAspectRatio() const { return aspectRatio; }
        Point getFollowOffset() const { return followOffset; }
        Direction getLookDirection() const { return lookDirection; }

        // Atualiza a posição da câmera
        void update(float deltaTime) override
        {
            // Se está olhando para um elemento em modo orbital
            if (isLookingAtElement && lookAtTarget)
            {
                Point targetPos = lookAtTarget->getPosition();
                
                // Usa ângulos orbitais para posicionar a câmera
                position.x = targetPos.x + orbitalDistance * std::sin(orbitalYaw) * std::cos(orbitalPitch);
                position.z = targetPos.z + orbitalDistance * std::cos(orbitalYaw) * std::cos(orbitalPitch);
                position.y = targetPos.y + orbitalDistance * std::sin(orbitalPitch);
                
                // Atualiza a direção de visão
                lookDirection = Direction(
                                    targetPos.x - position.x,
                                    targetPos.y - position.y,
                                    targetPos.z - position.z)
                                    .normalized();
            }
            // Se está apenas seguindo (sem olhar para elemento), usa offset fixo
            else if (isFollowing && followTarget)
            {
                Point targetPos = followTarget->getPosition();
                position = targetPos + followOffset;
            }
        }

        // Calcula o ponto para onde a câmera está olhando
        Point calculateLookPoint() const
        {
            if (isLookingAtElement && lookAtTarget)
            {
                return lookAtTarget->getPosition();
            }
            else
            {
                return Point(
                    position.x + lookDirection.x,
                    position.y + lookDirection.y,
                    position.z + lookDirection.z);
            }
        }

        // Aplica a câmera ao contexto OpenGL
        void apply()
        {
            // Configura a projeção
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(fov, aspectRatio, nearPlane, farPlane);

            // Configura a view matrix
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            Point lookPoint = calculateLookPoint();

            gluLookAt(
                position.x, position.y, position.z,    // Posição da câmera
                lookPoint.x, lookPoint.y, lookPoint.z, // Ponto de foco
                upVector.x, upVector.y, upVector.z     // Vetor up
            );
        }

        // Renderização vazia (câmera não é visível)
        void render() override
        {
            }
    };

} // namespace Ocean

#endif // CAMERA_HPP
