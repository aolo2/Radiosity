#ifndef RADIOSITY_CAMERA_H
#define RADIOSITY_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float YAW = 90.0f;
const float PITCH = 0.0f;
const float SPEED = 10.0f;
const float SENSITIVITY = 0.1f;

class camera {
public:
    glm::mat4 view_matrix();

    camera(glm::vec3 pos, glm::vec3 up);

    camera(const camera &other) = default;

    ~camera() = default;

    camera &operator=(camera other) = delete;

    void process_cursor(float x_offset, float y_offset, bool cap_pitch);

    bool process_movement(const bool *keys);
private:
    void update_angles();

    glm::vec3 pos, front, up, right, world_up = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = YAW, pitch = PITCH;
    float movement_speed = SPEED, mouse_sensitivity = SENSITIVITY;
};


#endif //RADIOSITY_CAMERA_H
