# AGENTS.md - Repository Guide for Agentic Coders

This file provides essential information for AI agents working with this C 3D rotating cube renderer codebase.

## Build Commands

### Compilation
```bash
# Linux/Unix
gcc -o cube cube.c -lm

# Windows (MinGW)
gcc -o cube.exe cube.c -lm

# Windows (Visual Studio)
cl cube.c
```

### Running
```bash
# Linux/Unix
./cube

# Windows
./cube.exe
# or
cube.exe
```

### Testing
This repository currently has no formal test suite. Manual testing involves:
1. Running the program
2. Verifying 3D cube renders correctly
3. Testing keyboard controls (h,j,k,l,+,-,q)
4. Checking performance (~60 FPS target)

## Code Style Guidelines

### File Structure
- Single-file project: `cube.c` contains all implementation
- Japanese README.md for documentation
- No external build system (direct gcc compilation)

### C Language Conventions

#### Preprocessor Directives
```c
// System includes first, grouped by purpose
#include <math.h>      // 数学関数（sin, cosなど）
#include <stdio.h>     // 標準入出力
#include <string.h>    // 文字列操作
#include <stdlib.h>    // メモリ管理

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif
```

#### Constants and Macros
```c
#define DEFAULT_WIDTH 160           // 画面の幅
#define DEFAULT_HEIGHT 44            // 画面の高さ
#define DEFAULT_CUBE_WIDTH 20.0f    // Use f suffix for float constants
#define MIN_CUBE_WIDTH 5.0f         // Minimum cube size
#define MAX_CUBE_WIDTH 50.0f        // Maximum cube size
```

#### Struct Definitions
```c
typedef struct {
    float A, B, C;               // X, Y, Z軸の回転角度
    float cubeWidth;             // キューブの幅
    int width, height;           // 画面の幅と高さ
    float *zBuffer;              // Zバッファ（奥行き判定用）
    char *buffer;                // 描画バッファ
    const char **colorBuffer;    // 色バッファ
    char backgroundASCIICode;    // 背景のASCII文字
    float distanceFromCam;        // カメラからの距離
    float horizontalOffset;       // 水平方向のオフセット
    float verticalOffset;         // 垂直方向のオフセット
    float K1;                     // 透視投影係数
    float incrementSpeed;         // 描画間隔
} CubeRenderer;

typedef struct {
    float x, y, z;               // X, Y, Z座標
} Point3D;
```

#### Function Naming and Style
```c
// Creator/destructor functions
static CubeRenderer* createRenderer(int width, int height);
static void destroyRenderer(CubeRenderer* renderer);

// Action functions
static void drawCube(CubeRenderer* renderer);
static void render(CubeRenderer* renderer);
static void handleInput(CubeRenderer* renderer, int* running);

// Calculation functions (use inline for performance)
static inline Point3D calculatePoint(const CubeRenderer* renderer, int i, int j, int k);
static inline void projectAndDraw(CubeRenderer* renderer, float cubeX, float cubeY, float cubeZ, char ch, const char* color);
```

#### Memory Management
```c
// Always check malloc/calloc return values
CubeRenderer* renderer = malloc(sizeof(CubeRenderer));
if (!renderer) return NULL;

// Use calloc for zero-initialized buffers
renderer->zBuffer = calloc(width * height, sizeof(float));

// Clean up in reverse order of allocation
static void destroyRenderer(CubeRenderer* renderer) {
    if (renderer) {
        free(renderer->zBuffer);
        free(renderer->buffer);
        free(renderer->colorBuffer);
        renderer->zBuffer = NULL;
        renderer->buffer = NULL;
        renderer->colorBuffer = NULL;
        free(renderer);
    }
}
```

### Platform-Specific Code

#### Windows vs Unix Handling
```c
#ifdef _WIN32
// Windows-specific implementation
#include <windows.h>
#include <conio.h>
#define usleep(x) Sleep((x) / 1000)
#define kbhit() _kbhit()
static HANDLE hConsole;
static void initConsole() { /* Windows console setup */ }
#else
// Unix/Linux implementation
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
static int kbhit(void) { /* Unix kbhit implementation */ }
static void initConsole() { /* Unix terminal setup */ }
#endif
```

### Error Handling
- Always check return values from memory allocation
- Use fprintf(stderr, ...) for error messages
- Clean up resources before returning on error
- Return appropriate error codes from main()

### Performance Considerations
- Use `static inline` for frequently called small functions
- Minimize memory allocations in hot paths
- Use memset for bulk buffer operations
- Target ~60 FPS with 16ms sleep between frames

### Code Organization
1. Includes and preprocessor directives
2. Platform-specific function implementations
3. Constants and macros
4. Struct definitions
5. Static helper functions
6. Core rendering functions
7. Main game loop
8. Entry point (main function)

### Documentation Style
- Japanese comments for user-facing descriptions
- Use // for single-line comments
- Group related constants with aligned comments
- Document struct member purposes inline

### ANSI Color Codes
```c
"\x1b[91m"  // Red
"\x1b[92m"  // Green
"\x1b[93m"  // Yellow
"\x1b[94m"  // Blue
"\x1b[95m"  // Magenta
"\x1b[96m"  // Cyan
"\x1b[0m"   // Reset
"\x1b[2J"   // Clear screen
"\x1b[H"    // Cursor home
```

## Technical Requirements
- C99 compatible compiler required
- Math library linking (-lm) on Unix/Linux
- ANSI escape sequence support in terminal
- Windows 10+ for ANSI color support

## Key Components
- 3D rotation matrices for XYZ axis rotation
- Perspective projection with 1/z depth
- Z-buffering for correct depth ordering
- Double buffering for flicker-free rendering
- Platform abstraction for input handling

## When Making Changes
1. Preserve platform compatibility (Windows/Unix)
2. Maintain ~60 FPS performance target
3. Keep memory management patterns consistent
4. Test on both platforms if possible
5. Don't break existing keyboard controls
6. Follow existing naming conventions