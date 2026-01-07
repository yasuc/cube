/* 3D回転キューブのレンダリングプログラム */
#include <math.h>      // 数学関数（sin, cosなど）
#include <stdio.h>     // 標準入出力
#include <string.h>    // 文字列操作
#include <stdlib.h>    // メモリ管理

#ifdef _WIN32
// Windows用のinclude
#include <windows.h>   // Windows API
#include <conio.h>     // コンソール入出力
#define usleep(x) Sleep((x) / 1000)  // usleepをWindowsのSleepに置換
#define kbhit() _kbhit()               // kbhitをWindowsの_kbhitに置換
#else
// Unix/Linux用のinclude
#include <unistd.h>   // usleepなど
#include <termios.h>  // 端末制御
#include <fcntl.h>    // ファイル制御

// Unix/Linux用のkbhit関数実装
static int kbhit(void) {
    struct termios oldt, newt;  // 端末設定の保存用
    int ch;                     // 入力文字
    int oldf;                   // ファイルフラグ保存用

    // 現在の端末設定を取得
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) return 0;
    newt = oldt;
    // カノニカルモードとエコーを無効化（非ブロッキング入力のため）
    newt.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) return 0;
    
    // 非ブロッキングモードに設定
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (oldf == -1) return 0;
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();  // 文字を読み取り

    // 端末設定を元に戻す
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    // 文字があった場合にバッファに戻して1を返す
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}
#endif

// デフォルト設定値の定義
#define DEFAULT_WIDTH 160           // 画面の幅
#define DEFAULT_HEIGHT 44            // 画面の高さ
#define DEFAULT_CUBE_WIDTH 20.0f    // キューブのデフォルトサイズ
#define DEFAULT_DISTANCE 100.0f     // カメラからのデフォルト距離
#define DEFAULT_K1 40.0f            // 透視投影の定数
#define DEFAULT_INCREMENT 0.6f      // 描画の間隔
#define MIN_CUBE_WIDTH 5.0f         // キューブの最小サイズ
#define MAX_CUBE_WIDTH 50.0f        // キューブの最大サイズ

// キューブレンダラーの構造体定義
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

// 3D座標点の構造体
typedef struct {
    float x, y, z;               // X, Y, Z座標
} Point3D;

#ifdef _WIN32
// Windows用のコンソール設定
static HANDLE hConsole;      // コンソール出力ハンドル
static HANDLE hStdin;        // 標準入力ハンドル
static DWORD consoleMode;    // コンソールモード保存用
static DWORD stdinMode;      // 入力モード保存用

// コンソールの初期化（Windows用）
static void initConsole() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hConsole, &consoleMode);
    GetConsoleMode(hStdin, &stdinMode);
    // ANSIエスケープシーケンスを有効化
    SetConsoleMode(hConsole, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    // 行バッファリングとエコーを無効化
    SetConsoleMode(hStdin, stdinMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
}

// コンソール設定の復元（Windows用）
static void cleanupConsole() {
    SetConsoleMode(hConsole, consoleMode);
    SetConsoleMode(hStdin, stdinMode);
}
#else
// Unix/Linux用の端末設定
static struct termios original_termios;
static int termios_set = 0;

// 端末を初期化する関数
static void initConsole() {
    if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
        termios_set = 1;
    }
}

// 端末設定を復元する関数
static void cleanupConsole() {
    if (termios_set) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        termios_set = 0;
    }
}
#endif

// レンダラーを作成する関数
static CubeRenderer* createRenderer(int width, int height) {
    // レンダラー構造体のメモリを確保
    CubeRenderer* renderer = malloc(sizeof(CubeRenderer));
    if (!renderer) return NULL;
    
    // 基本パラメータを設定
    renderer->width = width;
    renderer->height = height;
    
    // 各バッファのメモリを確保
    renderer->zBuffer = calloc(width * height, sizeof(float));           // Zバッファ
    renderer->buffer = malloc(width * height * 10 + height + 200);        // 描画バッファ（色コード分を考慮して大きく確保）
    renderer->colorBuffer = calloc(width * height, sizeof(const char*)); // 色バッファ
    
    // メモリ確保のエラーチェック
    if (!renderer->zBuffer || !renderer->buffer || !renderer->colorBuffer) {
        free(renderer->zBuffer);
        free(renderer->buffer);
        free(renderer->colorBuffer);
        free(renderer);
        return NULL;
    }
    
    // デフォルト値を初期化
    renderer->A = 0.0f;                           // X軸回転角度
    renderer->B = 0.0f;                           // Y軸回転角度
    renderer->C = 0.0f;                           // Z軸回転角度
    renderer->cubeWidth = DEFAULT_CUBE_WIDTH;      // キューブサイズ
    renderer->backgroundASCIICode = ' ';          // 背景文字
    renderer->distanceFromCam = DEFAULT_DISTANCE;  // カメラ距離
    renderer->horizontalOffset = 0.0f;            // 水平オフセット
    renderer->verticalOffset = 0.0f;               // 垂直オフセット
    renderer->K1 = DEFAULT_K1;                     // 透視投影係数
    renderer->incrementSpeed = DEFAULT_INCREMENT;  // 描画間隔
    
    return renderer;
}

// レンダラーを破棄する関数
static void destroyRenderer(CubeRenderer* renderer) {
    if (renderer) {
        // 確保したメモリを解放
        free(renderer->zBuffer);      // Zバッファを解放
        free(renderer->buffer);        // 描画バッファを解放
        free(renderer->colorBuffer);   // 色バッファを解放
        renderer->zBuffer = NULL;
        renderer->buffer = NULL;
        renderer->colorBuffer = NULL;
        free(renderer);               // レンダラー構造体を解放
    }
}

// 3D座標点を計算する関数（回転行列の適用）
static inline Point3D calculatePoint(const CubeRenderer* renderer, int i, int j, int k) {
    Point3D p;
    float A = renderer->A;  // X軸回転角度
    float B = renderer->B;  // Y軸回転角度
    float C = renderer->C;  // Z軸回転角度
    
    // XYZ軸の順で回転を適用した座標変換
    // X座標の計算
    p.x = j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) +
          j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
    
    // Y座標の計算
    p.y = j * cos(A) * cos(C) + k * sin(A) * cos(C) -
          j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) -
          i * cos(B) * sin(C);
    
    // Z座標の計算
    p.z = k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
    
    return p;
}

// 3D座標を2D投影して描画する関数
static inline void projectAndDraw(CubeRenderer* renderer, float cubeX, float cubeY, float cubeZ, char ch, const char* color) {
    // 3D座標を回転させてスクリーン座標に変換
    Point3D p = calculatePoint(renderer, (int)cubeX, (int)cubeY, (int)cubeZ);
    p.z += renderer->distanceFromCam;  // カメラ距離を加算
    
    // カメラより手前にある場合は描画しない
    if (p.z <= 0.001f) return;
    
    // 透視投影の計算
    float ooz = 1.0f / p.z;  // 逆距離（遠近法用）
    // スクリーン座標への投影
    int xp = (int)(renderer->width / 2.0f + renderer->horizontalOffset + 
                   renderer->K1 * ooz * p.x * 2.0f);  // X座標
    int yp = (int)(renderer->height / 2.0f + renderer->verticalOffset + 
                   renderer->K1 * ooz * p.y);         // Y座標
    
    // バッファのインデックスを計算
    int idx = xp + yp * renderer->width;
    if (idx >= 0 && idx < renderer->width * renderer->height) {
        // Zバッファで手前の点のみを描画（デプステスト）
        if (ooz > renderer->zBuffer[idx]) {
            renderer->zBuffer[idx] = ooz;       // Zバッファ更新
            renderer->buffer[idx] = ch;         // 文字バッファ更新
            renderer->colorBuffer[idx] = color; // 色バッファ更新
        }
    }
}

// キューブの6つの面を描画する関数
static void drawCube(CubeRenderer* renderer) {
    float cubeWidth = renderer->cubeWidth;      // キューブの幅
    float increment = renderer->incrementSpeed;  // 描画間隔
    
    // 面1: 前面（赤色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, cubeX, cubeY, -cubeWidth, '#', "\x1b[91m");
        }
    }
    
    // 面2: 右面（緑色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, cubeWidth, cubeY, cubeX, '#', "\x1b[92m");
        }
    }
    
    // 面3: 背面（黄色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, -cubeWidth, cubeY, -cubeX, '#', "\x1b[93m");
        }
    }
    
    // 面4: 左面（青色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, -cubeX, cubeY, cubeWidth, '#', "\x1b[94m");
        }
    }
    
    // 面5: 下面（マゼンタ色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, cubeX, -cubeWidth, -cubeY, '#', "\x1b[95m");
        }
    }
    
    // 面6: 上面（シアン色）
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += increment) {
        for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += increment) {
            projectAndDraw(renderer, cubeX, cubeWidth, cubeY, '#', "\x1b[96m");
        }
    }
}

// 各バッファをクリアする関数
static void clearBuffers(CubeRenderer* renderer) {
    // 描画バッファを背景文字で埋める
    memset(renderer->buffer, renderer->backgroundASCIICode, 
           renderer->width * renderer->height);
    // Zバッファを0でクリア
    memset(renderer->zBuffer, 0, renderer->width * renderer->height * sizeof(float));
    // 色バッファを空文字でクリア
    for (int i = 0; i < renderer->width * renderer->height; i++) {
        renderer->colorBuffer[i] = "";
    }
}

// 画面にレンダリングする関数
static void render(CubeRenderer* renderer) {
    char* outputPtr = renderer->buffer + renderer->width * renderer->height;  // 出力バッファの先頭
    
    // カーソルを画面左上に移動
    *outputPtr++ = '\x1b';
    *outputPtr++ = '[';
    *outputPtr++ = 'H';
    
    // 各ピクセルを描画
    for (int k = 0; k < renderer->width * renderer->height; k++) {
        // 行の終わりで改行
        if (k % renderer->width == 0 && k > 0) {
            *outputPtr++ = '\n';
        }
        
        const char* color = renderer->colorBuffer[k];
        int hasColor = *color != '\0';  // 色コードがあるかチェック
        // 色コードがある場合は設定
        if (hasColor) {
            while (*color) {
                *outputPtr++ = *color++;
            }
        }
        
        // 文字を描画
        *outputPtr++ = renderer->buffer[k];
        
        // 色コードがあった場合はリセット
        if (hasColor) {
            *outputPtr++ = '\x1b';
            *outputPtr++ = '[';
            *outputPtr++ = '0';
            *outputPtr++ = 'm';
        }
    }
    
    // 色をリセット
    *outputPtr++ = '\x1b';
    *outputPtr++ = '[';
    *outputPtr++ = '0';
    *outputPtr++ = 'm';
    
    // カーソルを再び左上に移動
    *outputPtr++ = '\x1b';
    *outputPtr++ = '[';
    *outputPtr++ = 'H';
    
    // ステータス情報を表示
    int statusLen = sprintf(outputPtr, "\x1b[97mH=%.1f:V=%.1f:W=%.1f\x1b[0m", 
                           renderer->horizontalOffset,   // 水平オフセット
                           renderer->verticalOffset,     // 垂直オフセット
                           renderer->cubeWidth);         // キューブサイズ
    outputPtr += statusLen;
    *outputPtr = '\0';
    
    // バッファの内容を標準出力に書き込み
    fwrite(renderer->buffer + renderer->width * renderer->height, 
           1, outputPtr - (renderer->buffer + renderer->width * renderer->height), 
           stdout);
    fflush(stdout);  // 出力をフラッシュ
}

// キーボード入力を処理する関数
static void handleInput(CubeRenderer* renderer, int* running) {
#ifdef _WIN32
    if (!_kbhit()) return;  // Windows用のキーハイチェック
    int c = _getch();        // Windows用の文字取得
#else
    if (!kbhit()) return;    // Unix/Linux用のキーハイチェック
    int c = getchar();       // 文字を取得
#endif
    
    // 入力されたキーに応じて処理を分岐
    switch(c) {
        case 'q':           // 終了
            *running = 0;
            break;
        case 'h':           // 左移動
            renderer->horizontalOffset -= 5.0f;
            break;
        case 'j':           // 下移動
            renderer->verticalOffset += 1.0f;
            break;
        case 'k':           // 上移動
            renderer->verticalOffset -= 1.0f;
            break;
        case 'l':           // 右移動
            renderer->horizontalOffset += 5.0f;
            break;
        case '+':           // 拡大
        case '=':
            renderer->cubeWidth = fmin(renderer->cubeWidth + 1.0f, MAX_CUBE_WIDTH);
            break;
        case '-':           // 縮小
        case '_':
            renderer->cubeWidth = fmax(renderer->cubeWidth - 1.0f, MIN_CUBE_WIDTH);
            break;
    }
}

// メインのゲームループ
static void gameLoop(CubeRenderer* renderer) {
    int running = 1;  // 実行フラグ
    
    while (running) {
        clearBuffers(renderer);     // バッファをクリア
        handleInput(renderer, &running);  // 入力処理
        
        if (!running) break;        // 終了指示があれば抜ける
        
        drawCube(renderer);         // キューブを描画
        render(renderer);            // 画面にレンダリング
        
        // 回転角度を更新（アニメーション）
        renderer->A += 0.05f;  // X軸回転
        renderer->B += 0.05f;  // Y軸回転
        renderer->C += 0.01f;  // Z軸回転
        
        usleep(16000);  // 約60FPS（16ms待機）
    }
}

// メイン関数
int main() {
    initConsole();  // コンソールを初期化
    
    // 画面をクリアして前景色をシアンに設定
    printf("\x1b[2J");   // 画面クリア
    printf("\x1b[36m");  // シアン色に設定
    fflush(stdout);
    
    // レンダラーを作成
    CubeRenderer* renderer = createRenderer(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        cleanupConsole();
        return 1;
    }
    
    // 初期パラメータを設定
    renderer->cubeWidth = 10.0f;                    // キューブサイズ
    renderer->horizontalOffset = renderer->cubeWidth; // 水平位置調整
    renderer->verticalOffset = 0.0f;                 // 垂直位置調整
    
    gameLoop(renderer);  // ゲームループを実行
    
    // 後処理
    destroyRenderer(renderer);  // レンダラーを破棄
    
    // 色と画面をリセット
    printf("\x1b[0m");          // 色をリセット
    printf("\x1b[2J");          // 画面をクリア
    printf("\x1b[H");            // カーソルを左上に
    fflush(stdout);
    
    cleanupConsole();  // コンソール設定を復元
    return 0;
}