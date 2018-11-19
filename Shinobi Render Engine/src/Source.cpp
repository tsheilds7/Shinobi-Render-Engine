#define VMA_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <array>
#include <optional>
#include <stdio.h>
#include <map>
#include <set>

#ifdef NDEBUG
const bool validationLayersEnabled = false;
#else
const bool validationLayersEnabled = true;
#endif // NDEBUG

static const char* logger;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};



const int HEIGHT = 600;
const int WIDTH = 900;
const char* TITLE = "Shinobi Render Engine";

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

struct QueueFamilies
{
	static std::optional<uint32_t> graphicsFamily;
};

std::optional<uint32_t> QueueFamilies::graphicsFamily;

class Application
{

private:

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice;
	VkDebugUtilsMessengerEXT callback;
	VkSurfaceKHR surface;
	VkResult result;

	VkQueue GraphicsQueue;
	VkQueue PresentQueue;
	VkQueue TransferQueue;

public:



	Application()
	{

		/*VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = logicalDevice;
		
		VmaAllocator allocator;
		vmaCreateAllocator(&allocatorInfo, &allocator);*/

		glfwInit();


	}

	~Application()
	{

		if (validationLayersEnabled)
		{
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
			std::cout << "Callback Destroyed\n";
		}

		vkDestroyDevice(logicalDevice, nullptr);
		if (validationLayersEnabled)
			std::cout << "Logical Device Destroyed\n";


		vkDestroySurfaceKHR(instance, surface, nullptr);
		if (validationLayersEnabled)
			std::cout << "Surface Destroyed!\n";


		vkDestroyInstance(instance, nullptr);
		if (validationLayersEnabled)
			std::cout << "Instance Destroyed\n";

		glfwDestroyWindow(window);
		if (validationLayersEnabled)
			std::cout << "Glfw window Destroyed!\n";
		
		glfwTerminate();
		
	}

	virtual void createWindow()
	{
		window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
		if(validationLayersEnabled)
		std::cout << "Glfw window created !\n";
	}

	virtual void createWindowSurface()
	{
		if (glfwCreateWindowSurface(instance, window,nullptr,&surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Glfw window surface!");
		}
	}

	void pollEvents()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}


	void run()
	{
		createWindow();
		runVulkan();
		pollEvents();
	}

	void runVulkan()
	{
		createInstance();
		setupDebugCallback();
		createWindowSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;

	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (validationLayersEnabled)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}


		 extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void setupDebugCallback()
	{
		if (!validationLayersEnabled) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;//Optional

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to set up debug callback!");
		}
	}

	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Shinobi";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Shinobi Render Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;
		appInfo.pNext = nullptr;


		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		if (validationLayersEnabled)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
			createInfo.enabledLayerCount = 0;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();


		/*if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error(" Failed to create instance of Vulkan \n");
		}
		else
		{
			std::cout << "Instance created\n";
		}
		*/
		result = vkCreateInstance(&createInfo, nullptr, &instance);

		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create instance of Vulkan!\n");
		else
			if (validationLayersEnabled)
				std::cout << "Instance created \n";

	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(instance, &deviceCount, &physicalDevice);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find a physical device with Vulkan Support !\n");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (auto device : devices)
		{
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties(device, &deviceProps);
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				device = physicalDevice;
				std::cout << deviceProps.deviceName << "\n";
			}
			else if (deviceProps.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				device = physicalDevice;
				std::cout << deviceProps.deviceName << "\n";
			}

			

				for (const auto& queueFamily : queueFamilies)
				{
					bool graphicsQueueFamily = false;
					bool presentQueueFamily = false;
					bool transferQueueFamily = false;

					QueueFamilies families;
					int i = 0;
					if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						graphicsQueueFamily = true;
						families.graphicsFamily = i;
						
						if (validationLayersEnabled)
						std::cout << "Graphics Queue Family found! \n";
					}
					else if(!graphicsQueueFamily)
					{
						throw std::runtime_error("Failed to find suitable queue family with graphics capabilities \n");
					}

					if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
					{
						transferQueueFamily = true;
						if (validationLayersEnabled)
						std::cout << "Transfer Queue Family found!\n";
					}

					VkBool32 presentSupport = false;
					int queueFamilyIndex = 0;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &presentSupport);


					if (queueFamily.queueCount > 0 && presentSupport)
					{
						presentQueueFamily = true;
						if(validationLayersEnabled)
						std::cout << "Present Queue Family Found!\n";
					}

					i++;

				}
			
		}


	}

	void createLogicalDevice()
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		
		QueueFamilies families;
		std::set<uint32_t> queueFamilies = { families.graphicsFamily.value()};
		
		float queuePriority = 1.0f;
		for (int i = 0; i < queueFamilies.size(); i++)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = i;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		auto extensions = getRequiredExtensions();

		createInfo.enabledExtensionCount = 0;

		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		

		result = vkCreateDevice(physicalDevice,&createInfo, nullptr, &logicalDevice);

		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device !\n");
		else
			if (validationLayersEnabled)
				std::cout << "Logical Device created \n";


		
		vkGetDeviceQueue(logicalDevice, families.graphicsFamily.value() , 0, &GraphicsQueue);
		
	}

};

int main()
{
	Application* app = new Application;

	try
	{
		app->run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	delete app;
	return EXIT_SUCCESS();
}