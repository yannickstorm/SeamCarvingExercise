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

#include "ImageData.h"
#include "CustomImageFilter.h"

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

// load image and format
unsigned char *load_image_and_format(const std::string &path, int &width, int &height,
													int &channels)
{

	unsigned char *pixels =	load_image(path, /*out*/ width, /*out*/ height, /*out*/ channels);
	if (!pixels) {
		spdlog::error("Failed to load image: {}", path.c_str());
	}

	// Compute the format based on number of channels
	if (channels != 3 && channels != 4) {
		spdlog::error("Image is not 3 or 4 channels: {}", path.c_str());
		stbi_image_free(pixels); // if you're using stb_image
		return nullptr;
	}
	return pixels;
}

GLenum get_format_from_channels(int channels) {
	if (channels == 1) return GL_RED;
	if (channels == 2) return GL_RG;
	if (channels == 3) return GL_RGB;
	if (channels == 4) return GL_RGBA;

	spdlog::error("Unsupported number of channels: {}", channels);
	return 0; // Invalid format
}

void sprinkle_red(ImageData &img)
{
    if (!img.valid()) return;

    int total_pixels = img.getWidth() * img.getHeight();
    int count_to_change = total_pixels / 10;

    static std::mt19937 rng(std::random_device{}()); // good seed once
    std::uniform_int_distribution<int> dist(0, total_pixels - 1);

	unsigned int channels = img.getChannels();

    for (int i = 0; i < count_to_change; i++) {
        int idx = dist(rng);
        int offset = idx * channels;

        img.getPixelData()[offset + 0] = 255; // R
        if (channels > 1) img.getPixelData()[offset + 1] = 0;   // G
        if (channels > 2) img.getPixelData()[offset + 2] = 0;   // B
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

	// Create a OpenGL texture identifier
	GLuint seam_carved_image_id;
	glGenTextures(1, &seam_carved_image_id);
	glBindTexture(GL_TEXTURE_2D, seam_carved_image_id);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Set alignment to 1 byte (for width not multiple of 4). Prevents jumbling
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


	// Create a OpenGL texture identifier
	GLuint primitive_resized_image_id;
	glGenTextures(1, &primitive_resized_image_id);
	glBindTexture(GL_TEXTURE_2D, primitive_resized_image_id);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load image from file
	// ----- START HERE -----
	// 1. load image from disk
	const std::string img_path = ASSET_PATH "/schmetterling_mid.jpg";

	// 1. load image
	int img_width = 0, img_height = 0, channels = 0;


	unsigned char *pixels =  load_image_and_format(img_path, /*out*/ img_width, /*out*/ img_height,
		/*out*/ channels);
	if (!pixels) {
		return 1;
	}

	// Initialize based image
	ImageData base_image(img_width, img_height, channels);
	// Copy pixel data into ImageData
	base_image.setPixels(pixels, img_width * img_height * channels);
	// Free all allocated memory
	stbi_image_free(pixels); // free image memory


	// Create a copy of the image pixels for processing
	ImageData primitive_resized_image = base_image; // for now just copy original

	// Initialize seam carved image
	ImageData seam_carved_image = base_image; // for now just copy original

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
			// 2. upload image to gpu

			// Upload pixels into texture
			glBindTexture(GL_TEXTURE_2D, original_image_text_id);
			glTexImage2D(GL_TEXTURE_2D, 0, get_format_from_channels(base_image.getChannels()), base_image.getWidth(), base_image.getHeight(), 0, get_format_from_channels(base_image.getChannels()), GL_UNSIGNED_BYTE, base_image.getPixelData());

			// 3. display image
			// (https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples)
			ImGui::Text("Original");
			ImGui::Image((ImTextureID)(intptr_t)original_image_text_id, ImVec2(img_width, img_height));

			// 4. add a simple imgui slider here to scale image from 0 to 100%
			static float target_scale_perc = 100.0f;
			bool needs_recompute = ImGui::SliderFloat("Scale Image By", &target_scale_perc, 10.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);

			if (needs_recompute) {

				seam_carved_image = base_image; // reset image to original

				ImageData greyscale = base_image;
				// Convert to greyscale first
				CustomImageFilter::toGreyscale(base_image, greyscale);
				// Apply edge detection with sobel
				CustomImageFilter::sobel(greyscale, seam_carved_image);

				ImageData sobelY_result = greyscale;
				CustomImageFilter::sobelY(greyscale, sobelY_result);
				primitive_resized_image = sobelY_result;

				// for (int i = 0; i < (int)((100.0-target_scale_perc)/10); i++)
				// {
				// 	sprinkle_red(seam_carved_image);
				// }

				// 5. Compute image energy
				// auto orig_energy =  calculate_energy(pixels, img_w, img_h, img_chan);
				// 6. find low energy seams
				// auto approx_seams = find_low_energy_seam(orig_energy, img_w, img_h);

				// 7. now remove seems to reduce image size

				// 8. Primitive rescaling using for example bilinear interpolation
				// (horizontal only)
			}

			// Display seam carved image (for now just display original image)
			// Upload pixels into texture
			glBindTexture(GL_TEXTURE_2D, seam_carved_image_id);
			glTexImage2D(GL_TEXTURE_2D, 0,
							get_format_from_channels(seam_carved_image.getChannels()),
							seam_carved_image.getWidth(),
							seam_carved_image.getHeight(),
							0,
							get_format_from_channels(seam_carved_image.getChannels()),
							GL_UNSIGNED_BYTE,
							seam_carved_image.getPixelData());

			ImGui::Text("Processed (Seam Carved)");
			ImGui::Image((ImTextureID)(intptr_t)seam_carved_image_id,
			             ImVec2(seam_carved_image.getWidth(), seam_carved_image.getHeight()));

			// 9. Display resized image (for now just display original image)
			// ImGui::Text("Resized");
			glBindTexture(GL_TEXTURE_2D, primitive_resized_image_id);
			glTexImage2D(GL_TEXTURE_2D, 0, get_format_from_channels(primitive_resized_image.getChannels()), primitive_resized_image.getWidth(), primitive_resized_image.getHeight(), 0, get_format_from_channels(primitive_resized_image.getChannels()), GL_UNSIGNED_BYTE, primitive_resized_image.getPixelData());


			ImGui::Text("Primitive Resized");
			ImGui::Image((ImTextureID)(intptr_t)primitive_resized_image_id,
			             ImVec2(primitive_resized_image.getWidth(), primitive_resized_image.getHeight()));

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