#include <glad/glad.h> 
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <stb_image.h>

int main()
{

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE);

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Failed to initialize SDL");
		return -1;
	}

	int windowWidth{ 1280 };
	int windowHeight{ 720 };

	SDL_Window* window = SDL_CreateWindow("YbtVoxel", windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		SDL_Log("Failed to create the window.");	
		SDL_Quit();
		return -1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		SDL_Log("Failed to initialize GLAD.");
		SDL_GL_DestroyContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return -1;
	}

	glViewport(0, 0, windowWidth, windowHeight);


	bool quit{ false };
	SDL_Event event{};

	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				quit = true;
				break;
			}

			if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				int width = event.window.data1;
				int height = event.window.data2;

				glViewport(0, 0, width, height);
			}
		}
	}

	SDL_Quit();
	return 0;
}

