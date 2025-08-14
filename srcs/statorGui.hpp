#pragma once

#include <hephaestus/hephaestus.hpp>
#include <hephaestus/extensions/hephExtensionDebug.hpp>
#include <hephaestus/extensions/hephExtensionScreenRendering.hpp>
#include <hephaestus/memory/hephMemoryAllocator.hpp>
#include <memory>
#include <array>

#include "ImNodeFlow.h"
#include "guiInfo.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "stator/statorNode.hpp"
#include "stator/factory.hpp"

#define	FRAMERATE	(1.0 / 60.0)


struct  StatorGuiWindowLayout {
  void  setMain(GuiWindowInfo info) {
    main = info;
    updateLayout();
  }
  void  updateLayout() {
  }

  GuiWindowInfo main;

  bool          showNodeEditor = true;
};

class	  StatorGui {
	public:
		StatorGui(std::string partsJsonPath, std::string recipesJsonPath);

		HephResult	create();
    void  			run();
    void  			destroy();

    void  callbackWindowSize(GLFWwindow* window, int width, int height);
    void  callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    void  callbackCursor(GLFWwindow* window, double x_pos, double y_pos);
    void  callbackMouseButton(GLFWwindow* window, int button, int action, int mod);
    void  callbackScroll(GLFWwindow* window, double xoffset, double yoffset);
    void  callbackPathDrop(GLFWwindow* window, int count, const char** paths);

	private:
    void          updateLayout();
		HephResult		hephaestusSetup();
    HephResult		createHephSwapchain();
    HephResult		createRenderPass();
    HephResult		setupImGui();
    void  				setupCallbackForWindow(GLFWwindow *window);

    HephResult		render();
    HephResult		renderGui();
		
    void					drawTopBar();
    void          drawPartSelector();

		GLFWwindow*		m_mainWindow;
    int						m_width, m_height;
		HephInstance	m_hephInstance;
		HephDevice		m_device;
		bool					m_quit = false;
		double				m_framerate = FRAMERATE;
		
		bool					m_showTopBar = true;
		GuiWindowInfo	m_windowInfoTopBar;

		bool					m_showPartSelectorPanel = true;
		GuiWindowInfo	m_windowInfoPartSelectorPanel;

    StatorGuiWindowLayout         m_winLayout;

    //Vulkan Stuff
		HephSwapchain									m_swapchain;
    VkSurfaceKHR      						m_surface;
		VkSurfaceFormatKHR						m_surfaceFormat;
    VkRenderPass                  m_renderPass = VK_NULL_HANDLE;
    HephCommandPool               m_commandPool;
		HephMemoryAllocator						m_allocator;
    uint32_t                      m_imageCount = 0;
    uint32_t                      m_imageCurrent = 0;
    std::vector<VkFence>          m_fences;
    std::vector<VkCommandBuffer>  m_commandBuffers;

    //GUI
    VkDescriptorPool      				m_imGuiDescPool = VK_NULL_HANDLE;
    FactoryEditor                 m_factoryEditor;
};
