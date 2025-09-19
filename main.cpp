#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>

#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <fmt/format.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <random>

static void glfw_error_callback(int error, const char *description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// load image
unsigned char *load_image(const std::string &path, int &width, int &height,
													int &channels) {
	unsigned char *pixels =
			stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);
	if (!pixels) {
		spdlog::error("Failed to load image: {}", path.c_str());
	}
	return pixels;
}

void sprinkle_red(unsigned char* pixels, int width, int height, int channels)
{
    if (!pixels) return;

    int total_pixels = width * height;
    int count_to_change = total_pixels / 10;

    static std::mt19937 rng(std::random_device{}()); // good seed once
    std::uniform_int_distribution<int> dist(0, total_pixels - 1);

    for (int i = 0; i < count_to_change; i++) {
        int idx = dist(rng);
        int offset = idx * channels;

        pixels[offset + 0] = 255; // R
        if (channels > 1) pixels[offset + 1] = 0;   // G
        if (channels > 2) pixels[offset + 2] = 0;   // B
    }
}

int main(int, char **) {
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char *glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char *glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
	// only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow *window = glfwCreateWindow(
			1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform
	// windows can look identical to regular ones.
	ImGuiStyle &style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		spdlog::error("Failed to initialize GLAD");
		return -1;
	}
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	static bool show_demo_window = false;

	ImVec4 clear_color = ImVec4(0.168f, 0.394f, 0.534f, 1.00f);






	// Create a OpenGL texture identifier
	GLuint original_image_text_id;
	glGenTextures(1, &original_image_text_id);
	glBindTexture(GL_TEXTURE_2D, original_image_text_id);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiWindowFlags window_flags =
				ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_DockNodeHost;
		ImGui::DockSpaceOverViewport(0, nullptr,
																 ImGuiDockNodeFlags_PassthruCentralNode);

		// 1. Show the big demo window (Most of the sample code is in
		// ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
		// ImGui!).
		if (show_demo_window) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		// 2. Simple window that we create ourselves. We use a Begin/End pair
		// to create a named window.
		{
			ImGui::Begin("Settings");
			ImGui::Text("Configure the App below."); // Display some text (you can use
																							 // a format strings too)
			ImGui::Checkbox(
					"Demo Window",
					&show_demo_window); // Edit bools storing our window open/close state

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
									1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		// 3. Show image window
		{
			ImGui::Begin("Image Window");
			// ----- START HERE -----
			// 1. load image from disk
			const std::string img_path = ASSET_PATH "/schmetterling_mid.jpg";

			// 1. load image
			int img_width = 0, img_height = 0, img_channels = 0;
			unsigned char *pixels =  load_image(img_path, /*out*/ img_width, /*out*/ img_height,
									/*out*/ img_channels);
						
			if (!pixels) {
				return 1;
			}

			sprinkle_red(pixels, img_width, img_height, img_channels);

			// Determine format based on number of channels
			GLint internalFormat;
			GLenum format;

			switch (img_channels) {
				case 1:
					internalFormat = GL_RED;
					format = GL_RED;
					break;
				case 3:
					internalFormat = GL_RGB;
					format = GL_RGB;
					break;
				case 4:
					internalFormat = GL_RGBA;
					format = GL_RGBA;
					break;
				default:
					spdlog::error("Failed to initialize GLAD");
					stbi_image_free(pixels); // if you're using stb_image
					return 1;
			}
			// 2. upload image to gpu

			// Upload pixels into texture			
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, img_width, img_height, 0, format, GL_UNSIGNED_BYTE, pixels);

			stbi_image_free(pixels); // free image memory

			// 3. display image
			// (https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples)
			ImGui::Text("Original");
			ImGui::Image((ImTextureID)(intptr_t)original_image_text_id, ImVec2(img_width, img_height));

			// 4. add a simple imgui slider here to scale image from 0 to 100%
			// if (ImGui::SliderFloat("Scale Image By", &target_scale_perc, 10.0f,
			//                        100.0f)) {
			// }
			bool needs_recompute = false;
			if (needs_recompute) {
				// 5. Compute image energy
				// auto orig_energy =  calculate_energy(pixels, img_w, img_h, img_chan);
				// 6. find low energy seams
				// auto approx_seams = find_low_energy_seam(orig_energy, img_w, img_h);

				// 7. now remove seems to reduce image size

				// 8. Primitive rescaling using for example bilinear interpolation
				// (horizontal only)
			}

			ImGui::Text("Processed (Seam Carved)");
			// ImGui::Image((ImTextureID)(intptr_t)image_processed_tex_id,
			//              ImVec2(working_width, img_h));

			ImGui::Text("Primitive Resized");
			// ImGui::Image((ImTextureID)(intptr_t)primitive_tex_id,
			//              ImVec2(target_scale, img_h));

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
								 clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we
		// save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call
		//  glfwMakeContextCurrent(window) directly)
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow *backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

	// Cleanup
	// TODO glDeleteTextures(1, &image1_tex_id);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}