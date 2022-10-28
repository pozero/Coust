#include "pch.h"

#include "ImGuiLayer.h"
#include "Coust/Application.h"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

namespace Coust
{
	/* these codes are mostly copied from example/example_glfw_opengl3/main.cpp */
	void ImGuiLayer::OnAttach()
	{
   		// Setup Dear ImGui context
   		IMGUI_CHECKVERSION();
   		ImGui::CreateContext();
   		ImGuiIO& io = ImGui::GetIO(); (void)io;
   		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
   		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
   		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
   		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
   		//io.ConfigViewportsNoAutoMerge = true;
   		//io.ConfigViewportsNoTaskBarIcon = true;

		float font_size = 24.0f;
		// tmp file path
		ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Dev\\Coust\\Coust\\third_party\\imgui\\misc\\fonts\\Cousine-Regular.ttf", font_size);
		io.FontDefault = font;

   		// Setup Dear ImGui style
   		ImGui::StyleColorsDark();
   		//ImGui::StyleColorsLight();

   		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
   		ImGuiStyle& style = ImGui::GetStyle();
   		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
   		{
   		    style.WindowRounding = 0.0f;
   		    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
   		}

   		// Setup Platform/Renderer backends
   		ImGui_ImplGlfw_InitForOpenGL(Application::GetInstance().GetWindow().GetWindowHandle(), true);
   		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::OnDetach()
	{
   		ImGui_ImplOpenGL3_Shutdown();
   		ImGui_ImplGlfw_Shutdown();
   		ImGui::DestroyContext();
	}

	void ImGuiLayer::Begin()
	{
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Window& window = Application::GetInstance().GetWindow();
		io.DisplaySize = ImVec2{(float) window.GetWidth(), (float) window.GetHeight()};

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
	}

	void ImGuiLayer::OnUIRender()
	{
		static bool dull = true;
		ImGui::ShowDemoWindow(&dull);
	}
}