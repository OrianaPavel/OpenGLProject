#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        //TODO - Update the rest of camera parameters

    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    glm::vec3 Camera::getCameraPosition() {
        return cameraPosition;
    }

    glm::vec3 Camera::getCameraTarget() {
        return cameraTarget;
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        //TODO
        glm::vec3 cameraForwardDir = glm::normalize(cameraTarget - cameraPosition);
        glm::vec3 cameraRightDir = glm::normalize(glm::cross(cameraForwardDir, cameraUpDirection));

        if (direction == MOVE_FORWARD) {
            this->cameraPosition += speed * cameraForwardDir;
            this->cameraTarget += speed * cameraForwardDir;
        }
        if (direction == MOVE_BACKWARD) {
            this->cameraPosition -= speed * cameraForwardDir;
            this->cameraTarget -= speed * cameraForwardDir;
        }
        if (direction == MOVE_RIGHT) {
            this->cameraPosition += speed * cameraRightDir;
            this->cameraTarget += speed * cameraRightDir;
        }
        if (direction == MOVE_LEFT) {
            this->cameraPosition -= speed * cameraRightDir;
            this->cameraTarget -= speed * cameraRightDir;
        }
        /*
        if (direction == MOVE_UP) {
            this->cameraPosition.y += speed;
            this->cameraTarget.y += speed;
        }
        if (direction == MOVE_DOWN) {
            this->cameraPosition.y -= speed;
            this->cameraTarget.y -= speed;
        }
        */
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        //TODO
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        this->cameraTarget.x = cameraPosition.x + front.x;
        this->cameraTarget.y = cameraPosition.y + front.y;
        this->cameraTarget.z = cameraPosition.z + front.z;

        glm::vec3 Right = glm::normalize(glm::cross(front, glm::vec3(0.0,1.0,0.0)));
        this->cameraUpDirection = glm::normalize(glm::cross(Right, front));
    }
}