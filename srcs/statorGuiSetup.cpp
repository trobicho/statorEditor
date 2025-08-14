#include "statorGui.hpp"
#include <GLFW/glfw3.h>
#include <hephaestus/core/hephResult.hpp>
#include <hephaestus/core/hephShaderModule.hpp>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

static void s_callbackWindowSize(GLFWwindow* window, int width, int height) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackWindowSize(window, width, height);
}

static void s_callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackKey(window, key, scancode, action, mods);
}

static void s_callbackCursor(GLFWwindow* window, double x_pos, double y_pos) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackCursor(window, x_pos, y_pos);
}

static void s_callbackMouseButton(GLFWwindow* window, int button, int action, int mod) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackMouseButton(window, button, action, mod);
}

static void s_callbackScroll(GLFWwindow* window, double xoffset, double yoffset) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackScroll(window, xoffset, yoffset);
}

static void s_callbackPathDrop(GLFWwindow* window, int count, const char** paths) {
  StatorGui *olivePtr = static_cast<StatorGui*>(glfwGetWindowUserPointer(window));
  olivePtr->callbackPathDrop(window, count, paths);
}

void  StatorGui::setupCallbackForWindow(GLFWwindow *window) {
  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  glfwSetWindowUserPointer(window, this);
  glfwSetWindowSizeCallback(window, s_callbackWindowSize);
  glfwSetKeyCallback(window, s_callbackKey);
  glfwSetCursorPosCallback(window, s_callbackCursor);
  glfwSetMouseButtonCallback(window, s_callbackMouseButton);
  glfwSetScrollCallback(window, s_callbackScroll);
  glfwSetDropCallback(window, s_callbackPathDrop);
}

HephResult	StatorGui::hephaestusSetup() {
	uint32_t glfwExtCount = 1;
	const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  std::vector<const char*> vkExts;
  for (int i = 0; i < glfwExtCount; i++)
    vkExts.push_back(glfwExts[i]);
  //vkExts.push_back(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME);

	std::vector<HephInstanceExtensionInterface*>	instanceExtensions;
	HephExtensionDebug                            extDebug;
	HephExtensionScreenRendering									extScreenRendering;
	instanceExtensions.push_back(&extDebug);
	instanceExtensions.push_back(&extScreenRendering);

	HephInstanceCreateInfo		instanceCreateInfo = {
		.pApplicationName = "StatorGui",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = "StatorGui",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.hephInstanceDebugInfo = {
			.debug = true,
		},
		.ppHephInstanceExtensions = instanceExtensions.data(),
		.hephInstanceExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
		.ppVkInstanceExtensions =  vkExts.data(),
		.vkInstanceExtensionCount = (uint32_t)vkExts.size(),
	};
	HEPH_CHECK_RESULT(m_hephInstance.create(instanceCreateInfo));

	HephPhysicalDevicesSelectorTest	devicesSelect;
	devicesSelect.selectDevices(m_hephInstance);
	std::vector<HephDeviceExtensionInterface*>	deviceExtensions;
	deviceExtensions.push_back(&extScreenRendering);
	//deviceExtensions.push_back(&extRayTracing);

	std::vector<HephQueueReserveInfo>	queueReserveInfos;
	queueReserveInfos.push_back((HephQueueReserveInfo) {
		.flags = VK_QUEUE_GRAPHICS_BIT,
		.priority = 1.0,
		.count = 1,
	});
	queueReserveInfos.push_back((HephQueueReserveInfo) {
		.flags = VK_QUEUE_TRANSFER_BIT,
		.priority = 1.0,
		.count = 0,
	});
	HephQueueReserveBasic			queueReserveBasic;
	queueReserveBasic.addReserveInfo(queueReserveInfos);
	HephDeviceCreateInfo			deviceCreateInfo = {
		.pQueueReserveInterface = static_cast<HephQueueReserveInterface*>(&queueReserveBasic),
		.ppHephDeviceExtensions = deviceExtensions.data(),
		.hephDeviceExtensionsCount = static_cast<uint32_t>(deviceExtensions.size()),
	};
	HEPH_CHECK_RESULT(m_hephInstance.createDevice(deviceCreateInfo, &m_device));

	return (HephResult());
}

HephResult	StatorGui::setupImGui() {
  IMGUI_CHECKVERSION();

  std::vector<VkDescriptorPoolSize> poolSizes =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1},
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };
  VkDescriptorPoolCreateInfo        poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
	};
  poolInfo.maxSets       = poolSizes.size();
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes    = poolSizes.data();
  vkCreateDescriptorPool(m_device.device, &poolInfo, nullptr, &m_imGuiDescPool);

  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForVulkan(m_mainWindow, true);
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.IniFilename = nullptr;
  io.DisplaySize = ImVec2(static_cast<float>(m_width), static_cast<float>(m_height));
  ImGui_ImplVulkan_InitInfo init_info = {
    .Instance = m_hephInstance.vulkanInstance,
    .PhysicalDevice = m_hephInstance.getPhysicalDevices()[0],
    .Device = m_device.device,
    .QueueFamily = m_device.queues[0].familyIndex,
    .Queue = m_device.queues[0].queue,
    .DescriptorPool = m_imGuiDescPool,
		.RenderPass = m_renderPass,
    .MinImageCount = m_swapchain.getImageCount(),
    .ImageCount = m_swapchain.getImageCount(),
    .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
    .Subpass = 0,
    .CheckVkResultFn = nullptr,
  };
  ImGui_ImplVulkan_Init(&init_info);
  ImGui_ImplVulkan_CreateFontsTexture();
  ImGui_ImplVulkan_DestroyFontsTexture();
  ImGui_ImplVulkan_SetMinImageCount(m_swapchain.getImageCount());
	return (HephResult());
}

HephResult	StatorGui::createHephSwapchain() {
	VkSurfaceFormatKHR	format = (VkSurfaceFormatKHR){VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	HephSurfaceSupportDetails	surfaceSupportDetails(m_device, m_surface);
	if (surfaceSupportDetails.formats[0].format != VK_FORMAT_UNDEFINED)
		format = surfaceSupportDetails.formats[0];
	m_surfaceFormat = format;
	HEPH_CHECK_RESULT(createRenderPass());
	HephSwapchainCreateInfo		swapchainCreateInfo;
	VkSwapchainCreateInfoKHR	swapInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = m_surface,
		.minImageCount = std::min(3u, surfaceSupportDetails.capabilies.minImageCount),
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = VkExtent2D{.width = static_cast<uint32_t>(m_width), .height=static_cast<uint32_t>(m_height)},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.preTransform = surfaceSupportDetails.capabilies.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};
	swapchainCreateInfo.swapchainCreateInfo = swapInfo;
	swapchainCreateInfo.renderPass = m_renderPass;
	HEPH_CHECK_RESULT(m_swapchain.create(m_device, swapchainCreateInfo));
	return (HephResult());
}

HephResult	StatorGui::createRenderPass() {
  VkAttachmentDescription attachmentDescription = {
    .format = m_surfaceFormat.format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };
  VkAttachmentReference   colorAttachmentReference = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription    subpassDescription = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentReference,
  };
  VkSubpassDependency     subpassDependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
      | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  VkRenderPassCreateInfo  renderPassInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &attachmentDescription,
    .subpassCount = 1,
    .pSubpasses = &subpassDescription,
    .dependencyCount = 1,
    .pDependencies = &subpassDependency,
  };
	return (HephResult(vkCreateRenderPass(m_device.device, &renderPassInfo
					, m_device.pAllocationCallbacks, &m_renderPass)
					, "Failed to create RenderPass"));
}
