# YbtVoxel

A Minecraft-inspired voxel world built with OpenGL 4.6 and SDL3 as a Computer Graphics course final project. Features procedurally generated terrain, texture mapping, Phong lighting, AABB collision detection, and interactive block placement and destruction.

---

## Libraries

| Library | Purpose |
|---|---|
| [SDL3](https://github.com/libsdl-org/SDL) | Window creation, input handling, OpenGL context |
| [GLAD](https://glad.dav1d.de/) | OpenGL function loader |
| [GLM](https://github.com/g-truc/glm) | Math library (vectors, matrices) |
| [stb_image](https://github.com/nothings/stb) | PNG texture loading |

---

## Installation & Running

### Requirements
- Visual Studio 2026
- Windows x64

### Steps

1. Clone the repository:
   ```
   git clone https://github.com/Hieratic2/YbtVoxel.git
   ```

2. Open `YbtVoxel.sln` in Visual Studio.

3. Make sure the build configuration is set to **x64 / Debug** or **x64 / Release**.

4. Set the working directory:
   - Right-click the project → **Properties**
   - **Configuration Properties → Debugging → Working Directory**
   - Set to `$(ProjectDir)`

5. Build and run with **F5**.

> The `assets/` folder must be present in the project root directory and contain the following textures:
> `grass_block_top.png`, `grass_block_side.png`, `dirt.png`, `stone.png`

---

## Controls

| Key / Input | Action |
|---|---|
| `W` `A` `S` `D` | Move forward / left / backward / right |
| `Space` | Jump |
| `Mouse` | Look around |
| `Left Click` | Break block |
| `Right Click` | Place block |
| `ESC` | Quit |
