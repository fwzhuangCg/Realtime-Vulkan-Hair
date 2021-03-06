#include <vulkan/vulkan.h>
#include <sstream>
#include "Instance.h"
#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Image.h"
#include <iostream>
#include "ObjLoader.h"


Device* device;
SwapChain* swapChain;
Renderer* renderer;
Camera* camera;
Camera* shadowCamera;

bool WDown = false;
bool ADown = false;
bool SDown = false;
bool DDown = false;
bool QDown = false;
bool EDown = false;


namespace {
    void resizeCallback(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        vkDeviceWaitIdle(device->GetVkDevice());
        swapChain->Recreate();
        renderer->RecreateFrameResources();
    }

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    double previousX = 0.0;
    double previousY = 0.0;

	void keyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_W) {
			if (action == GLFW_PRESS) {
				WDown = true;
			}
			else if (action == GLFW_RELEASE) {
				WDown = false;
			}
		}
		else if (key == GLFW_KEY_A) {
			if (action == GLFW_PRESS) {
				ADown = true;
			}
			else if (action == GLFW_RELEASE) {
				ADown = false;
			}
		}
		else if (key == GLFW_KEY_S) {
			if (action == GLFW_PRESS) {
				SDown = true;
			}
			else if (action == GLFW_RELEASE) {
				SDown = false;
			}
		}
		else if (key == GLFW_KEY_D) {
			if (action == GLFW_PRESS) {
				DDown = true;
			}
			else if (action == GLFW_RELEASE) {
				DDown = false;
			}
		}
		else if (key == GLFW_KEY_Q) {
			if (action == GLFW_PRESS) {
				QDown = true;
			}
			else if (action == GLFW_RELEASE) {
				QDown = false;
			}
		}
		else if (key == GLFW_KEY_E) {
			if (action == GLFW_PRESS) {
				EDown = true;
			}
			else if (action == GLFW_RELEASE) {
				EDown = false;
			}
		}
	}

    void mouseDownCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                leftMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                leftMouseDown = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                rightMouseDown = true;
                glfwGetCursorPos(window, &previousX, &previousY);
            }
            else if (action == GLFW_RELEASE) {
                rightMouseDown = false;
            }
        }
    }

    void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition) {
        if (leftMouseDown) {
            double sensitivity = 0.5;
            float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
            float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);

            camera->UpdateOrbit(deltaX, deltaY, 0.0f);

            previousX = xPosition;
            previousY = yPosition;
        } else if (rightMouseDown) {
            double deltaZ = static_cast<float>((previousY - yPosition) * 0.05);

            camera->UpdateOrbit(0.0f, 0.0f, deltaZ);

            previousY = yPosition;
        }
    }
}


void moveSphere(VkCommandPool commandPool) {
	float delta = 0.005;
	glm::vec3 translation(0.0);
	if (WDown) {
		translation += glm::vec3(0.0, delta, 0.0);
	}
	if (ADown) {
		translation += glm::vec3(-delta, 0.0, 0.0);
	}
	if (SDown) {
		translation += glm::vec3(0.0, -delta, 0.0);
	}
	if (DDown) {
		translation += glm::vec3(delta, 0.0, 0.0);	
	}
	if (QDown) {
		translation += glm::vec3(0.0, 0.0, -delta);		
	}
	if (EDown) {
		translation += glm::vec3(0.0, 0.0, delta);
	}

	renderer->scene->translateSphere(translation);
	shadowCamera->TranslateCamera(translation);
}


int main() {
    static constexpr char* applicationName = "Realtime Vulkan Hair";
	const float windowWidth = 1080.f;
	const float windowHeight = 720.f;
	InitializeWindow((int)windowWidth, (int)windowHeight, applicationName);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    Instance* instance = new Instance(applicationName, glfwExtensionCount, glfwExtensions);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance->GetVkInstance(), GetGLFWWindow(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    instance->PickPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, surface);

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    device = instance->CreateDevice(QueueFlagBit::GraphicsBit | QueueFlagBit::TransferBit | QueueFlagBit::ComputeBit | QueueFlagBit::PresentBit, deviceFeatures);

    swapChain = device->CreateSwapChain(surface, 5);

    camera = new Camera(device, windowWidth / windowHeight);
	shadowCamera = new Camera(device, windowWidth / windowHeight, glm::vec3(2.f, 0.f, 1.f), 1.f, 10.f);

    VkCommandPoolCreateInfo transferPoolInfo = {};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.queueFamilyIndex = device->GetInstance()->GetQueueFamilyIndices()[QueueFlags::Transfer];
    transferPoolInfo.flags = 0;

    VkCommandPool transferCommandPool;
    if (vkCreateCommandPool(device->GetVkDevice(), &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

	VkImage mannequinDiffuseImage;
	VkDeviceMemory mannequinDiffuseImageMemory;
	Image::FromFile(device,
		transferCommandPool,
		"images/mannequin_diffuse.png",
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mannequinDiffuseImage,
		mannequinDiffuseImageMemory
	);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	ObjLoader::LoadObj("models/collisionTest.obj", vertices, indices);
	Model* collisionSphere = new Model(device, transferCommandPool, vertices, indices, glm::scale(glm::vec3(0.98f)));
	collisionSphere->SetTexture(mannequinDiffuseImage);

	ObjLoader::LoadObj("models/mannequin.obj", vertices, indices);
	Model* mannequin = new Model(device, transferCommandPool, vertices, indices, glm::scale(glm::vec3(0.98f)));
	mannequin->SetTexture(mannequinDiffuseImage);

	Hair* hair = new Hair(device, transferCommandPool, "models/mannequin_segment.obj");

	// trans, rot, scale
	Collider sphereCollider = Collider(glm::vec3(2.0, 0.0, 1.0), glm::vec3(0.0), glm::vec3(1.0));
	//Collider faceCollider = Collider(glm::vec3(0.0, 2.511, 0.915), glm::vec3(0, 0.0, 0.0), glm::vec3(0.561, 0.749, 0.615));
	Collider headCollider = Collider(glm::vec3(0.0, 2.64, 0.08), glm::vec3(-38.270, 0.0, 0.0), glm::vec3(0.817, 1.158, 1.01));
	Collider neckCollider = Collider(glm::vec3(0.0, 1.35, -0.288), glm::vec3(18.301, 0.0, 0.0), glm::vec3(0.457, 1.0, 0.538));
	Collider bustCollider = Collider(glm::vec3(0.0, -0.380, -0.116), glm::vec3(-17.260, 0.0, 0.0), glm::vec3(1.078, 1.683, 0.974));
	Collider shoulderRCollider = Collider(glm::vec3(-0.698, 0.087, -0.36), glm::vec3(-20.254, 13.144, 34.5), glm::vec3(0.721, 1.0, 0.724));
	Collider shoulderLCollider = Collider(glm::vec3(0.698, 0.087, -0.36), glm::vec3(-20.254, 13.144, -34.5), glm::vec3(0.721, 1.0, 0.724));

	std::vector<Collider> colliders = { sphereCollider, /*faceCollider,*/ headCollider, neckCollider, bustCollider, shoulderRCollider, shoulderLCollider };

	std::vector<Model*> models = { collisionSphere, mannequin };

    Scene* scene = new Scene(device, transferCommandPool, colliders, models);
    scene->AddHair(hair);

	vkDestroyCommandPool(device->GetVkDevice(), transferCommandPool, nullptr);

    renderer = new Renderer(device, swapChain, scene, camera, shadowCamera);

    glfwSetWindowSizeCallback(GetGLFWWindow(), resizeCallback);
    glfwSetMouseButtonCallback(GetGLFWWindow(), mouseDownCallback);
    glfwSetCursorPosCallback(GetGLFWWindow(), mouseMoveCallback);
	glfwSetKeyCallback(GetGLFWWindow(), keyPressCallback);

	double fps = 0;
	double timebase = 0;
	int frame = 0;

    while (!ShouldQuit()) {
        glfwPollEvents();
       /* scene->UpdateTime();
		double previousTime = glfwGetTime();
		moveSphere(transferCommandPool);

        renderer->Frame();
		double currentTime = glfwGetTime();*/

		frame++;
		double time = glfwGetTime();

		if (time - timebase > 1.0) {
			fps = frame / (time - timebase);
			timebase = time;
			frame = 0;
		}

		std::ostringstream ss;
		ss << "[";
		ss.precision(1);
		ss << std::fixed << fps;
		ss << " fps] ";
		glfwSetWindowTitle(GetGLFWWindow(), ss.str().c_str());

		scene->UpdateTime();
		renderer->Frame();
		moveSphere(transferCommandPool);
    }

    vkDeviceWaitIdle(device->GetVkDevice());

	vkDestroyImage(device->GetVkDevice(), mannequinDiffuseImage, nullptr);
	vkFreeMemory(device->GetVkDevice(), mannequinDiffuseImageMemory, nullptr);

    delete scene;
	delete collisionSphere;
	delete mannequin;
    delete hair;
    delete camera;
    delete shadowCamera;
    delete renderer;
    delete swapChain;
    delete device;
    delete instance;
    DestroyWindow();
    return 0;
}
