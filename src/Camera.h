#pragma once

#include <glm/glm.hpp>
#include "Device.h"

struct CameraBufferObject {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
};


class Camera {
private:
    Device* device;
    
    CameraBufferObject cameraBufferObject;
    
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    void* mappedData;

    float r, theta, phi;
	glm::vec3 eye;

public:
    Camera(Device* device, float aspectRatio);
    Camera(Device* device, float aspectRatio, glm::vec3 eye, float nearPlane, float farPlane);
    ~Camera();

    VkBuffer GetBuffer() const;
    
    void UpdateOrbit(float deltaX, float deltaY, float deltaZ);
	void TranslateCamera(glm::vec3 translation);
};
