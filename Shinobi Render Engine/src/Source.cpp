#define VMA_IMPLEMENTATION
#define VK_USE_PLATFORM_WIN32_KHR
#include "GLFW/glfw3.h"
#include "vk_mem_alloc.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <array>
#include <optional>
#include <stdio.h>
#include <map>

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

class Application
{

private:

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkDebugUtilsMessengerEXT callback;

	VkResult result;

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