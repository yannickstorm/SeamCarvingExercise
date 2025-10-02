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
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <cfloat>

#include "ImageData.h"
#include "CustomImageFilter.h"
#include "SobelShader.h"

// Seam carving background job state + worker (extracted from main)
struct SeamCarveJobState {
	std::atomic<unsigned int> target_image_width{0};
	std::atomic<bool> compute_request{false};
	std::atomic<bool> is_busy{false};
	std::atomic<bool> result_available{false};
	std::atomic<bool> stop_request{false};
	std::atomic<unsigned int> progress_percent{100}; // 0..100 progress of current task
	std::mutex mtx; // protects result and sobel_result
	std::condition_variable cv;
	ImageData result;
	ImageData sobel_result;
};

// Worker thread entry point.
// Repeatedly waits for a carving request, then performs:
//  1. Reset working copy and compute initial greyscale.
//  2. Iteratively compute energy (Sobel), DP minimal energy map, seam, and remove it.
//  3. Adapts to slider changes mid-process by re-reading target width.
//  4. Publishes the final carved image + last Sobel energy when target reached.
// Notes:
//  - Starts from the original image each request (stateless approach keeps logic simple).
//  - Thread-safe publication guarded by mutex; atomics signal availability/state.
static void seamCarveWorker(const ImageData &base_image, SeamCarveJobState &job) {
	while (!job.stop_request.load()) {
		// 1. Wait until there's a new request (or stop signaled)
		std::unique_lock<std::mutex> lk(job.mtx);
		job.cv.wait(lk, [&]() { return job.compute_request.load() || job.stop_request.load(); });
		if (job.stop_request.load()) break; // graceful shutdown

		// Capture current target & transition to working state
		unsigned int target = job.target_image_width.load();
		job.compute_request.store(false);
		job.is_busy.store(true);

		// Prepare working copies (fresh start each request)
		ImageData seam_carved = base_image;
		ImageData greyscale_image = CustomImageFilter::toGreyscale(seam_carved);
		const unsigned int original_width = seam_carved.getWidth();

		// Release lock during heavy processing (only needed for publishing results)
		lk.unlock();

		ImageData sobel_image = greyscale_image; // To prevent empty image in case of no carving
		// 2. Seam removal loop until desired width or stop
		while (!job.stop_request.load() && seam_carved.getWidth() > target) {
			// If user moves the slider, adapt target without restarting
			unsigned int latestTarget = job.target_image_width.load();
			if (latestTarget != target) target = latestTarget;

			// (a) Compute contrast image with Sobel
			sobel_image = CustomImageFilter::sobel(greyscale_image);

			// (b) Dynamic programming minimal energy path map
			std::vector<unsigned int> minimalEnergyPathMap = CustomImageFilter::computeMinimalEnergyPathMap(sobel_image);

			// (c) Extract minimal energy seam
			std::vector<unsigned int> seam = CustomImageFilter::identityMinEnergySeam( minimalEnergyPathMap, sobel_image.getWidth(), sobel_image.getHeight());

			// (d) Remove seam from working + greyscale versions
			CustomImageFilter::removeSeam(seam_carved, seam);
			CustomImageFilter::removeSeam(greyscale_image, seam);

			// Update progress
			if ((original_width - target) != 0) {
				unsigned int completion_percentage = (original_width - seam_carved.getWidth()) * 100u / (original_width - target);
				if (completion_percentage > 100u) completion_percentage = 100u;
				job.progress_percent.store(completion_percentage);
			}
		}

		// Publish result (lock to prevent race conditions)
		std::lock_guard<std::mutex> lk2(job.mtx);
		job.result       = seam_carved;
		job.sobel_result = sobel_image;
		job.result_available.store(true);
		job.progress_percent.store(100);
		
		// Mark worker idle after finishing current request
		job.is_busy.store(false);
	}
}

// Draw some int value as text in the center of image
static void DrawTextOverlay(const ImVec2& image_pos, const ImVec2& image_size, unsigned int value) {
	ImVec2 center(image_pos.x + image_size.x * 0.5f, image_pos.y + image_size.y * 0.5f);

	// Percentage text
	std::string value_text = fmt::format("{}%", value);
	ImFont* font = ImGui::GetFont();
	float big_size = font->FontSize * 2.4f;
	ImVec2 text_size = font->CalcTextSizeA(big_size, FLT_MAX, 0.0f, value_text.c_str());
	ImVec2 text_pos(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImU32 color  = IM_COL32(255,0,255,255);
	draw_list->AddText(font, big_size, text_pos, color, value_text.c_str());
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

void load_ImageData_to_GLTexture(const ImageData &image, GLuint texture_id) {
	if (image.pixels.empty()) {
		spdlog::error("ImageData has no pixel data.");
		return;
	}

	GLenum format = image.getGLFormat();
	if (format == 0) {
		spdlog::error("Unsupported number of channels: {}", image.getChannels());
		return;
	}

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, image.getWidth(), image.getHeight(), 0,
				 format, GL_UNSIGNED_BYTE, image.pixels.data());
}

int main(int, char **) {
	// Setup window
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


	// Setup texture for displaying base image
	GLuint original_image_text_id;
	glGenTextures(1, &original_image_text_id);
	glBindTexture(GL_TEXTURE_2D, original_image_text_id);
	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Setup texture for displaying seam carved image
	GLuint seam_carved_image_id;
	glGenTextures(1, &seam_carved_image_id);
	glBindTexture(GL_TEXTURE_2D, seam_carved_image_id);
	// Filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Set alignment to 1 byte (for width not multiple of 4). Prevents jumbling
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Setup texture for displaying primitive resized image
	GLuint primitive_resized_image_id;
	glGenTextures(1, &primitive_resized_image_id);
	glBindTexture(GL_TEXTURE_2D, primitive_resized_image_id);
	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Set alignment to 1 byte (for width not multiple of 4). Prevents jumbling
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Setup texture for displaying debug (e.g. Sobel) image
	GLuint debug_tex;
	glGenTextures(1, &debug_tex);
	glBindTexture(GL_TEXTURE_2D, debug_tex);
	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Set alignment to 1 byte (for width not multiple of 4). Prevents jumbling
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load image from file
	// ----- START HERE -----
	// 1. load image from disk
	const std::string img_path = ASSET_PATH "/interesting_image.jpg";
	int img_width = 0, img_height = 0, channels = 0;
	unsigned char *pixels =  load_image(img_path, /*out*/ img_width, /*out*/ img_height, /*out*/ channels);
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
	ImageData sobel_image = base_image; // energy visualization

	// Background job state for seam carving
	SeamCarveJobState job;

	job.result = base_image;
	job.sobel_result = sobel_image;
	job.target_image_width = base_image.getWidth();

	// Launch worker thread
	std::thread worker(seamCarveWorker, std::cref(base_image), std::ref(job));

	int display_w, display_h;
	// Main loop
	while (!glfwWindowShouldClose(window)) {


		// Restore viewport for ImGui
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);

		
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
			ImGui::Text("Configure the App below.");
			ImGui::Checkbox("Demo Window", &show_demo_window);
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("Seam Carving: %s", job.is_busy.load() ? "Working" : "Idle");
			ImGui::Image((ImTextureID)(intptr_t)debug_tex,
				ImVec2(seam_carved_image.getWidth(), seam_carved_image.getHeight()));
			ImGui::End();
		}

		// 3. Show image window
		{
			ImGui::Begin("Image Window");
			// 2. upload image to gpu

			// Upload pixels into texture
			load_ImageData_to_GLTexture(base_image, original_image_text_id);

			// 3. display image
			// (https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples)
			ImGui::Text("Original");
			ImGui::Image((ImTextureID)(intptr_t)original_image_text_id, ImVec2(img_width, img_height));

			// Slider to trigger an image width reduction
			static float target_scale_perc = 100.0f;
			bool slider_changed = ImGui::SliderFloat("Scale Image By", &target_scale_perc, 10.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
			unsigned int target_width = static_cast<unsigned int>(base_image.getWidth() * (target_scale_perc / 100.0f));

			if (slider_changed) {
				if (target_width < 1) target_width = 1;
				// Set parameter for the Seam Carving thread
				job.target_image_width.store(target_width);
				// Set a compute request
				job.compute_request.store(true);
				job.progress_percent.store(0);
				// Notify the thread that it can check for task to do
				job.cv.notify_one();
			}

			// Check if the processing thread has finished working on the picture.
			// Pull the results if finished.
			if (job.result_available.load()) {
				//Require the lock to prevent race conditions.
				std::lock_guard<std::mutex> lk(job.mtx);
				// Copy data result
				seam_carved_image = job.result;
				sobel_image = job.sobel_result;
				// Reset flag
				job.result_available.store(false);
				// Update debug texture
				load_ImageData_to_GLTexture(sobel_image, debug_tex);
			}

			// Display seam carved image (for now just display original image)
			// Upload pixels into texture
			load_ImageData_to_GLTexture(seam_carved_image, seam_carved_image_id);
			ImGui::Text("Processed (Seam Carved)");
			// Record position to overlay process progress if busy
			ImVec2 image_pos = ImGui::GetCursorScreenPos();
			ImVec2 image_size((float)seam_carved_image.getWidth(), (float)seam_carved_image.getHeight());
			ImGui::Image((ImTextureID)(intptr_t)seam_carved_image_id, image_size);


			if (job.is_busy.load()) {
				DrawTextOverlay(image_pos, image_size, job.progress_percent.load());
			}

			// 9. Display resized image (for now just display original image)
			load_ImageData_to_GLTexture(primitive_resized_image, primitive_resized_image_id);
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

	// Signal worker to stop and join
	job.stop_request.store(true);
	job.cv.notify_one();
	if (worker.joinable()) worker.join();

	// Cleanup
	// TODO glDeleteTextures(1, &image1_tex_id);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}



