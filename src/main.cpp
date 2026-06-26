#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cmath>

constexpr int TILE_GRASS_TOP = 0;
constexpr int TILE_GRASS_SIDE = 1;
constexpr int TILE_DIRT = 2;
constexpr int TILE_STONE = 3;

const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aTexCoord;
layout (location = 2) in float aFaceLight;

out vec2  TexCoord;
out float FaceLight;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    TexCoord    = aTexCoord;
    FaceLight   = aFaceLight;
}
)";

const char* fragmentShaderSource = R"(
#version 460 core
in vec2  TexCoord;
in float FaceLight;
out vec4 FragColor;

uniform sampler2D uAtlas;

void main()
{
    vec4 texColor = texture(uAtlas, TexCoord);
    vec3 col = texColor.rgb * FaceLight;

    // Grass top tint — same green Minecraft uses
    if (TexCoord.x < 0.5 && TexCoord.y < 0.5)
        col *= vec3(0.47, 0.72, 0.25);

    FragColor = vec4(col, texColor.a);
}
)";

struct Camera
{
    glm::vec3 position = glm::vec3(32.0f, 20.0f, 32.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 10.0f;
    float sensitivity = 0.1f;
    float fov = 70.0f;

    void processMouseMove(float xOffset, float yOffset)
    {
        yaw += xOffset * sensitivity;
        pitch -= yOffset * sensitivity;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(dir);
    }

    void processKeyboard(const bool* keys, float dt)
    {
        float     vel = speed * dt;
        glm::vec3 flat = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
        glm::vec3 right = glm::normalize(glm::cross(front, up));

        if (keys[SDL_SCANCODE_W])     position += flat * vel;
        if (keys[SDL_SCANCODE_S])     position -= flat * vel;
        if (keys[SDL_SCANCODE_A])     position -= right * vel;
        if (keys[SDL_SCANCODE_D])     position += right * vel;
        if (keys[SDL_SCANCODE_SPACE]) position.y += vel;
        if (keys[SDL_SCANCODE_LCTRL]) position.y -= vel;
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }
    glm::mat4 getProjectionMatrix(float aspect) const
    {
        return glm::perspective(glm::radians(fov), aspect, 0.05f, 500.0f);
    }
};

namespace Perlin
{
    static int p[512];
    static const int perm[256] = {
        151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
        140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,247,
        120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,
        33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,
        71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,
        133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,
        63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,
        226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,
        59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,
        152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,
        39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,
        97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,
        145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,
        204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,
        67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };
    static void seed() { for (int i = 0;i < 256;i++) p[i] = p[i + 256] = perm[i]; }
    static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float lerp(float a, float b, float t) { return a + t * (b - a); }
    static float grad(int hash, float x, float y) {
        int h = hash & 7; float u = h < 4 ? x : y, v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
    static float noise(float x, float y) {
        int X = (int)floor(x) & 255, Y = (int)floor(y) & 255;
        x -= floor(x); y -= floor(y);
        float u = fade(x), v = fade(y);
        int A = p[X] + Y, B = p[X + 1] + Y;
        return lerp(lerp(grad(p[A], x, y), grad(p[B], x - 1, y), u),
            lerp(grad(p[A + 1], x, y - 1), grad(p[B + 1], x - 1, y - 1), u), v);
    }
    static float fbm(float x, float y, int oct = 4, float pers = 0.5f) {
        float val = 0, amp = 1, freq = 1, maxV = 0;
        for (int i = 0;i < oct;i++) {
            val += noise(x * freq, y * freq) * amp; maxV += amp; amp *= pers; freq *= 2;
        }
        return val / maxV;
    }
}

constexpr int WORLD_W = 64;
constexpr int WORLD_H = 24;
constexpr int WORLD_D = 64;
constexpr int CHUNK_W = 16;
constexpr int CHUNK_D = 16;
constexpr int NUM_CX = WORLD_W / CHUNK_W;
constexpr int NUM_CZ = WORLD_D / CHUNK_D;

constexpr uint8_t BLOCK_AIR = 0;
constexpr uint8_t BLOCK_GRASS = 1;
constexpr uint8_t BLOCK_DIRT = 2;
constexpr uint8_t BLOCK_STONE = 3;

static uint8_t world[WORLD_W][WORLD_H][WORLD_D];
static int     heightMap[WORLD_W][WORLD_D];

static void generateWorld()
{
    constexpr float SCALE = 0.04f;
    constexpr int   BASE_HEIGHT = 5;
    constexpr int   HEIGHT_RANGE = 12;

    for (int x = 0; x < WORLD_W; x++) {
        for (int z = 0; z < WORLD_D; z++) {
            float n = Perlin::fbm(x * SCALE, z * SCALE);
            float t = (n + 1.0f) * 0.5f;
            int   top = BASE_HEIGHT + (int)(t * HEIGHT_RANGE);
            if (top >= WORLD_H) top = WORLD_H - 1;

            heightMap[x][z] = top;

            for (int y = 0; y <= top; y++) {
                if (y == top)     world[x][y][z] = BLOCK_GRASS;
                else if (y >= top - 3) world[x][y][z] = BLOCK_DIRT;
                else                   world[x][y][z] = BLOCK_STONE;
            }
        }
    }
}

static uint8_t getBlock(int x, int y, int z)
{
    if (x < 0 || x >= WORLD_W || y < 0 || y >= WORLD_H || z < 0 || z >= WORLD_D) return BLOCK_AIR;
    return world[x][y][z];
}
static bool isSolid(int x, int y, int z) { return getBlock(x, y, z) != BLOCK_AIR; }


struct Vertex { float x, y, z, u, v, light; };

static constexpr float LIGHT_TOP = 1.00f;
static constexpr float LIGHT_BOTTOM = 0.45f;
static constexpr float LIGHT_SIDE_Z = 0.80f;
static constexpr float LIGHT_SIDE_X = 0.65f;

static glm::vec2 tileUV(int tile)
{
    float u = (tile % 2) * 0.5f;
    float v = (tile / 2) * 0.5f;
    return { u, v };
}

static int getTile(uint8_t block, int face)
{
    switch (block) {
    case BLOCK_GRASS:
        if (face == 0) return TILE_GRASS_TOP;
        if (face == 1) return TILE_DIRT;
        return TILE_GRASS_SIDE;
    case BLOCK_DIRT:  return TILE_DIRT;
    case BLOCK_STONE: return TILE_STONE;
    default:          return TILE_STONE;
    }
}

static void pushQuad(std::vector<Vertex>& verts,
    float x0, float y0, float z0,
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float light, int tile)
{
    glm::vec2 base = tileUV(tile);
    float s = 0.5f;

    glm::vec2 uvA = base + glm::vec2(0.0f, s);
    glm::vec2 uvB = base + glm::vec2(s, s);
    glm::vec2 uvC = base + glm::vec2(s, 0.0f);
    glm::vec2 uvD = base + glm::vec2(0.0f, 0.0f);

    verts.push_back({ x0,y0,z0, uvA.x,uvA.y, light });
    verts.push_back({ x1,y1,z1, uvB.x,uvB.y, light });
    verts.push_back({ x2,y2,z2, uvC.x,uvC.y, light });
    
    verts.push_back({ x0,y0,z0, uvA.x,uvA.y, light });
    verts.push_back({ x2,y2,z2, uvC.x,uvC.y, light });
    verts.push_back({ x3,y3,z3, uvD.x,uvD.y, light });
}

static std::vector<Vertex> buildChunkMesh(int cx, int cz)
{
    std::vector<Vertex> verts;
    verts.reserve(8192);

    int startX = cx * CHUNK_W;
    int startZ = cz * CHUNK_D;

    for (int lx = 0; lx < CHUNK_W; lx++)
        for (int lz = 0; lz < CHUNK_D; lz++)
            for (int y = 0; y < WORLD_H; y++)
            {
                int wx = startX + lx;
                int wz = startZ + lz;
                uint8_t block = getBlock(wx, y, wz);
                if (block == BLOCK_AIR) continue;

                float x = (float)wx, Y = (float)y, z = (float)wz;

                if (!isSolid(wx, y + 1, wz))
                    pushQuad(verts,
                        x, Y + 1, z, x + 1, Y + 1, z, x + 1, Y + 1, z + 1, x, Y + 1, z + 1,
                        LIGHT_TOP, getTile(block, 0));
                
                if (!isSolid(wx, y - 1, wz))
                    pushQuad(verts,
                        x + 1, Y, z, x, Y, z, x, Y, z + 1, x + 1, Y, z + 1,
                        LIGHT_BOTTOM, getTile(block, 1));
               
                if (!isSolid(wx, y, wz + 1))
                    pushQuad(verts,
                        x, Y, z + 1, x + 1, Y, z + 1, x + 1, Y + 1, z + 1, x, Y + 1, z + 1,
                        LIGHT_SIDE_Z, getTile(block, 2));
                
                if (!isSolid(wx, y, wz - 1))
                    pushQuad(verts,
                        x + 1, Y, z, x, Y, z, x, Y + 1, z, x + 1, Y + 1, z,
                        LIGHT_SIDE_Z, getTile(block, 2));
                
                if (!isSolid(wx + 1, y, wz))
                    pushQuad(verts,
                        x + 1, Y, z + 1, x + 1, Y, z, x + 1, Y + 1, z, x + 1, Y + 1, z + 1,
                        LIGHT_SIDE_X, getTile(block, 2));
                
                if (!isSolid(wx - 1, y, wz))
                    pushQuad(verts,
                        x, Y, z, x, Y, z + 1, x, Y + 1, z + 1, x, Y + 1, z,
                        LIGHT_SIDE_X, getTile(block, 2));
            }
    return verts;
}


struct ChunkMesh
{
    unsigned int VAO = 0, VBO = 0;
    int count = 0;

    void upload(const std::vector<Vertex>& v)
    {
        count = (int)v.size();
        if (!count) return;
        if (!VAO) { glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); }
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, light));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
    void draw()  const { if (count) { glBindVertexArray(VAO); glDrawArrays(GL_TRIANGLES, 0, count); } }
    void destroy() { if (VAO) { glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); VAO = VBO = 0; } }
};

static unsigned int loadTextureAtlas(const char* grassTop,
    const char* grassSide,
    const char* dirt,
    const char* stone,
    int tileSize = 32)
{
    int atlasW = tileSize * 2;
    int atlasH = tileSize * 2;
    std::vector<unsigned char> atlas(atlasW * atlasH * 4, 0);

    auto blit = [&](const char* path, int offX, int offY)
        {
            int w, h, ch;
            stbi_set_flip_vertically_on_load(false);
            unsigned char* img = stbi_load(path, &w, &h, &ch, 4);
            if (!img) {
                SDL_Log("Failed to load texture: %s — %s", path, stbi_failure_reason());
                
                for (int py = 0; py < tileSize; py++)
                    for (int px = 0; px < tileSize; px++) {
                        int idx = ((offY + py) * atlasW + (offX + px)) * 4;
                        atlas[idx] = 255; atlas[idx + 1] = 0; atlas[idx + 2] = 255; atlas[idx + 3] = 255;
                    }
                return;
            }
            
            for (int py = 0; py < tileSize; py++) {
                for (int px = 0; px < tileSize; px++) {
                    int srcX = px * w / tileSize;
                    int srcY = py * h / tileSize;
                    int src = (srcY * w + srcX) * 4;
                    int dst = ((offY + py) * atlasW + (offX + px)) * 4;
                    atlas[dst + 0] = img[src + 0];
                    atlas[dst + 1] = img[src + 1];
                    atlas[dst + 2] = img[src + 2];
                    atlas[dst + 3] = img[src + 3];
                }
            }
            stbi_image_free(img);
        };

    blit(grassTop, 0, 0);
    blit(grassSide, tileSize, 0);
    blit(dirt, 0, tileSize);
    blit(stone, tileSize, tileSize);

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasW, atlasH, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return texID;
}


static unsigned int compileShader(GLenum type, const char* src)
{
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); SDL_Log("Shader: %s", log); }
    return s;
}
static unsigned int createProgram(const char* vs, const char* fs)
{
    unsigned int v = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(p, 512, nullptr, log); SDL_Log("Link: %s", log); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}


int main()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL init failed"); return -1; }

    int windowWidth = 1280, windowHeight = 720;
    SDL_Window* window = SDL_CreateWindow("YbtVoxel", windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window failed");
        SDL_Quit();
        return -1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        SDL_Log("GLAD failed");
        SDL_Quit();
        return -1;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowRelativeMouseMode(window, true);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    unsigned int shader = createProgram(vertexShaderSource, fragmentShaderSource);
    int uView = glGetUniformLocation(shader, "view");
    int uProj = glGetUniformLocation(shader, "projection");
    int uAtlas = glGetUniformLocation(shader, "uAtlas");

    unsigned int atlasID = loadTextureAtlas(
        "./assets/grass_block_top.png",
        "./assets/grass_block_side.png",
        "./assets/dirt.png",
        "./assets/stone.png"
    );

    Perlin::seed();
    generateWorld();

    ChunkMesh chunks[NUM_CX][NUM_CZ];
    for (int cx = 0;cx < NUM_CX;cx++)
        for (int cz = 0;cz < NUM_CZ;cz++)
            chunks[cx][cz].upload(buildChunkMesh(cx, cz));

    Camera camera;
    int mx = WORLD_W / 2, mz = WORLD_D / 2;
    camera.position = glm::vec3(mx, heightMap[mx][mz] + 3.0f, mz);

    Uint64 lastTime = SDL_GetTicks();
    bool quit = false;
    SDL_Event event;

    while (!quit)
    {
        Uint64 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT) quit = true;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) quit = true;
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                windowWidth = event.window.data1; windowHeight = event.window.data2;
                glViewport(0, 0, windowWidth, windowHeight);
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION)
                camera.processMouseMove((float)event.motion.xrel, (float)event.motion.yrel);
        }

        camera.processKeyboard(SDL_GetKeyboardState(nullptr), dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasID);
        glUniform1i(uAtlas, 0);

        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
        glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix((float)windowWidth / windowHeight)));

        for (int cx = 0;cx < NUM_CX;cx++)
            for (int cz = 0;cz < NUM_CZ;cz++)
                chunks[cx][cz].draw();

        SDL_GL_SwapWindow(window);
    }

    for (int cx = 0;cx < NUM_CX;cx++)
        for (int cz = 0;cz < NUM_CZ;cz++)
            chunks[cx][cz].destroy();

    glDeleteTextures(1, &atlasID);
    glDeleteProgram(shader);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}