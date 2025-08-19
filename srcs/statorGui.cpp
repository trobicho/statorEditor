#include "statorGui.hpp"
#include "stator/data.hpp"
#include "thread"
#include <GLFW/glfw3.h>
#include <boost/json/parse.hpp>
#include <filesystem>
#include <fstream>
#include <hephaestus/core/hephResult.hpp>
#include <hephaestus/memory/hephMemoryAllocator.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

void  StatorGui::callbackWindowSize(GLFWwindow* window, int width, int height) {
	HEPH_PRINT_RESULT(HephResult(vkQueueWaitIdle(m_device.queues[0].queue), "Error waiting for queue {{}}!"));
  m_width = width;
  m_height = height;

	m_swapchain.destroy();
  vkDestroySurfaceKHR(m_hephInstance.vulkanInstance, m_surface, m_device.pAllocationCallbacks);
  HEPH_PRINT_RESULT(HephResult(glfwCreateWindowSurface(m_hephInstance.vulkanInstance, m_mainWindow
        , m_device.pAllocationCallbacks, &m_surface), "Failed to create Surface {{}} !"));
	auto surfaceOld = m_surface;
  m_swapchain.recreate(VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, m_surface);

  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.DisplaySize = ImVec2(static_cast<float>(m_width), static_cast<float>(m_height));
  m_imageCurrent = 0;

  updateLayout();
}

void  StatorGui::callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
  }

	if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
  }

	if (mods & GLFW_MOD_CONTROL && key == GLFW_KEY_Q) {
		m_quit = true;
	}

	if (mods & GLFW_MOD_CONTROL && key == GLFW_KEY_D) {
	}
}

void  StatorGui::callbackCursor(GLFWwindow* window, double xpos, double ypos) {

}

void  StatorGui::callbackMouseButton(GLFWwindow* window, int button, int action, int mod) {
  if (button == GLFW_MOUSE_BUTTON_1) {
    if (action == GLFW_PRESS) {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
    }
  }
}

void  StatorGui::callbackScroll(GLFWwindow* window, double xoffset, double yoffset) {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  float factor = 1.0 + 0.1 * yoffset;
  ImVec2  pos;
  pos.x = xpos;
  pos.y = ypos;
}

void  StatorGui::callbackPathDrop(GLFWwindow* window, int count, const char** paths) {
	std::filesystem::path	filePath;

  for (int i = 0; i < count; i++) {
		filePath = paths[i];
		std::string extension = filePath.extension();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  }
}

StatorGui::StatorGui(std::string partsJsonPath, std::string recipesJsonPath) {
  glfwInit();
  std::ifstream jsonPartsFile(partsJsonPath);
  json::value docParts = json::parse(jsonPartsFile);
  for (auto& part: docParts.as_object()["parts"].as_array()) {
    partsGlobalArray.push_back(Part(part.as_object()));
  }

  std::ifstream jsonRecipesFile(recipesJsonPath);
  json::value docRecipes = json::parse(jsonRecipesFile);
  for (auto& recipe: docRecipes.as_object()["recipes"].as_array()) {
    recipesGlobalArray.push_back(Recipe(recipe.as_object()));
  }

  for (auto& recipe: recipesGlobalArray) {
    for (auto& out: recipe.outputs) {
      for (auto& part: partsGlobalArray) {
        if (part.name == out.name) {
          part.addRecipe(recipe);
        }
      }
    }
  }
}

HephResult	StatorGui::create() {
	HEPH_CHECK_RESULT(hephaestusSetup());

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  m_mainWindow = glfwCreateWindow(1920, 1080, "Stator", NULL, NULL);
  glfwGetWindowSize(m_mainWindow, &m_width, &m_height);
  m_winLayout.setMain(GuiWindowInfo(ImVec2(0, 0), ImVec2(m_width, m_height)));
  setupCallbackForWindow(m_mainWindow);

  HEPH_CHECK_RESULT(HephResult(glfwCreateWindowSurface(m_hephInstance.vulkanInstance, m_mainWindow
        , m_device.pAllocationCallbacks, &m_surface), "Failed to create Surface {{}} !"));
	HephCommandPoolCreateInfo commandPoolCreateInfo = {
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_device.queues[0].familyIndex,
	};
	HEPH_CHECK_RESULT(m_commandPool.create(m_device, commandPoolCreateInfo).errorFormat("Failed to create CommandPool {{}} !"));
	HEPH_CHECK_RESULT(m_allocator.create(m_device).errorFormat("Failed to create Memory Allocator {{}} !"));
	HEPH_CHECK_RESULT(createHephSwapchain());
	HEPH_CHECK_RESULT(HephResult("Empty swapchain!", (m_swapchain.getImageCount() > 0)));
	m_commandBuffers.resize(m_swapchain.getImageCount());
	HEPH_CHECK_RESULT(m_commandPool.allocate(m_commandBuffers.size(), m_commandBuffers.data()));

	HEPH_CHECK_RESULT(setupImGui().errorFormat("Failed to setup ImGui! {}"));

	return (HephResult());
}

void  StatorGui::updateLayout() {
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  GuiWindowInfo main;
	main.pos = ImVec2(0, 0);
	if (m_showTopBar)
		main.pos.y = m_windowInfoTopBar.size.y;
	if (m_showPartSelectorPanel)
		main.pos.x = m_windowInfoPartSelectorPanel.size.x;
	main.size = ImVec2(io.DisplaySize.x - main.pos.x, io.DisplaySize.y - main.pos.y);
  m_winLayout.setMain(main);
}

void  StatorGui::destroy() {
  std::cout << "DESTROY" << std::endl;

	vkDeviceWaitIdle(m_device.device);
	{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_device.device, m_imGuiDescPool, m_device.pAllocationCallbacks);
  }
  {
    m_swapchain.destroy();
    vkDestroySurfaceKHR(m_hephInstance.vulkanInstance, m_surface, m_device.pAllocationCallbacks);
    vkDestroyRenderPass(m_device.device, m_renderPass, m_device.pAllocationCallbacks);
    m_commandPool.destroy();
  }
  glfwDestroyWindow(m_mainWindow);
  glfwTerminate();
  std::cout << "Everything was successfully destroyed" << std::endl;
}

void  StatorGui::run() {
  ImGuiIO& io = ImGui::GetIO();
	double lastUpdateTime = 0;

  while(!glfwWindowShouldClose(m_mainWindow) && !m_quit) {
		double now = glfwGetTime();
		double deltaTime = now - lastUpdateTime;
    glfwPollEvents();
    HEPH_PRINT_RESULT(render());
		if (deltaTime < m_framerate) {
			//std::cout << "expect: " <<  m_framerate << "delta: " << deltaTime << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(1000.0 * (m_framerate - deltaTime))));
		}
		lastUpdateTime = now;
  }
}

HephResult	StatorGui::renderGui() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	drawTopBar();
  updateLayout();
  drawPartSelector();

	if (ImGui::Begin("NodeEditor", &m_winLayout.showNodeEditor, ImGuiWindowFlags_NoDecoration)) {
		m_winLayout.main.set();
    m_factoryEditor.draw();
  }
	ImGui::End();
	return (HephResult());
}

HephResult	StatorGui::render() {
	HephSwapchainPresentData	presentData;

	HephResult	result = m_swapchain.acquireNextImage(presentData);
  if (result.vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    return (HephResult());
  if (!result.valid())
    return (HephResult(result.vkResult, "problem acquiring next frame ({}) !!"));

  auto& commandBuffer = m_commandBuffers[presentData.imageCurrent];
  vkResetCommandBuffer(commandBuffer, 0);
  VkCommandBufferBeginInfo  beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr,
  };

  renderGui();

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  {
    VkClearValue clearValue = (VkClearValue){0.2f, 0.2f, 0.2f, 1.0f};
    VkRenderPassBeginInfo     renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = m_renderPass,
      .framebuffer = presentData.image.framebuffer,
      .renderArea = (VkRect2D) {
        .offset = (VkOffset2D){0, 0},
        .extent = presentData.extent,
      },
      .clearValueCount = 1,
      .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    VkRect2D		scissor = {
      .offset = {0, 0},
      .extent = presentData.extent,
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
  }
  VkPipelineStageFlags  waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo  submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &presentData.syncObject.semaphoreAvailable,
    .pWaitDstStageMask = waitStage,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &presentData.syncObject.semaphoreFinish,
  };
	VkQueue queue = m_device.queues[0].queue;
  HEPH_CHECK_RESULT(HephResult(vkQueueSubmit(queue, 1, &submitInfo, presentData.syncObject.fence)
				, "Failed to submit Post"));
  VkPresentInfoKHR  presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &presentData.syncObject.semaphoreFinish,
    .swapchainCount = 1,
    .pSwapchains = &presentData.swapchain,
    .pImageIndices = &presentData.imageIndex,
    .pResults = nullptr,
  };
  vkQueuePresentKHR(queue, &presentInfo);
	return (HephResult());
}

void  StatorGui::drawTopBar() {
  if (m_showTopBar) {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open...")) {
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Prefences")) {
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Tools")) {
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Help")) {
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("About")) {
        ImGui::EndMenu();
      }
			m_windowInfoTopBar.getInfo();
      ImGui::EndMainMenuBar();
    }
  }
  else {
    m_windowInfoTopBar.size = ImVec2(0, 0);
  }
}

void  StatorGui::drawPartSelector() {
  m_windowInfoPartSelectorPanel.size = ImVec2(0, 0);
  if (m_showPartSelectorPanel) {
    ImGui::SetNextWindowPos(ImVec2(0, m_windowInfoTopBar.size.y));
    ImGui::SetNextWindowSize(ImVec2(0, m_height - m_windowInfoTopBar.size.y));
    if (ImGui::Begin("File selector", &m_showPartSelectorPanel, ImGuiWindowFlags_NoDecoration)) {
      for (auto& part : partsGlobalArray) {
        if (ImGui::BeginMenu(part.name.c_str())) {
          if (ImGui::Selectable(part.name.c_str()))
            m_factoryEditor.placeNodeAt<PartNode>({300, 100}, part);
          int i = 1;
          for (auto& recipe: part.recipes) {
            std::string name = "recipe " + std::to_string(i);
            if (ImGui::BeginMenu(name.c_str())) {
              if (ImGui::Selectable("ADD")) {
                m_factoryEditor.placeNodeAt<RecipeNode>({300, 100}, *recipe);
              }
              recipe->drawPopUp();
              ImGui::EndMenu();
            }
            i++;
          }
          ImGui::EndMenu();
        }
      }
      m_windowInfoPartSelectorPanel.getInfo();
      ImGui::End();
    }
  }
}
