#include "../includes/camera.h"

#include <GLFW/glfw3.h>
#include <iostream>

camera::camera(glm::vec3 pos, glm::vec3 up) {
    this->pos = pos;
    this->world_up = normalize(up);

    this->pitch = PITCH;
    this->yaw = YAW;

    this->update_angles();
}

glm::mat4 camera::view_matrix() {
    return glm::lookAt(this->pos, this->pos + this->front, this->up);
}

bool camera::process_movement(const bool *keys) {
    bool updated = false;

    if (keys[GLFW_KEY_W]) {
        this->pos += this->front * movement_speed;
        updated = true;
    }

    if (keys[GLFW_KEY_A]) {
        this->pos -= this->right * movement_speed;
        updated = true;
    }

    if (keys[GLFW_KEY_S]) {
        this->pos -= this->front * movement_speed;
        updated = true;
    }

    if (keys[GLFW_KEY_D]) {
        this->pos += this->right * movement_speed;
        updated = true;
    }

    if (updated) {
        this->update_angles();
    }

    return updated;
}

void camera::process_cursor(float x_offset, float y_offset, bool cap_pitch) {
    x_offset *= this->mouse_sensitivity;
    y_offset *= this->mouse_sensitivity;

    this->yaw += x_offset;
    this->pitch += y_offset;

    if (cap_pitch) {
        if (this->pitch > 89.0f) {
            this->pitch = 89.0f;
        }
        if (this->pitch < -89.0f) {
            this->pitch = -89.0f;
        }
    }

    this->update_angles();
}

void camera::update_angles() {
    glm::vec3 new_front;

    new_front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
    new_front.y = sin(glm::radians(this->pitch));
    new_front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

    this->front = glm::normalize(new_front);
    this->right = glm::normalize(glm::cross(this->front, this->world_up));
    this->up = glm::normalize(glm::cross(this->right, this->front));
}