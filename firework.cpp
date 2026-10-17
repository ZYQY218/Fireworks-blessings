#include<iostream>
#include<math.h>
#include<vector>
#include<cstring> 
#include<algorithm>
#include<array>
#include<fstream>
#include<string>
#include<ctime>
#include<cstdlib>
#include"app_config.h"
#include"raylib.h"

constexpr int SCENE_WIDTH = 960;
constexpr int SCENE_HEIGHT = 640;
AppConfig appConfig;

// 两份生成图集均按 4 × 4 排列，同一编号对应同一种颜色。
constexpr int ATLAS_COLUMNS = 4;
constexpr int ATLAS_ROWS = 4;
constexpr int FIREWORK_COUNT = ATLAS_COLUMNS * ATLAS_ROWS;
constexpr int FIREWORK_TILE_SIZE = 240;
Color* colorsList[FIREWORK_COUNT] = {};

Rectangle atlasCell(int width, int height, int id) {
    const int column = id % ATLAS_COLUMNS;
    const int row = id / ATLAS_COLUMNS;
    const int left = column * width / ATLAS_COLUMNS;
    const int top = row * height / ATLAS_ROWS;
    const int right = (column + 1) * width / ATLAS_COLUMNS;
    const int bottom = (row + 1) * height / ATLAS_ROWS;
    return { (float)left, (float)top, (float)(right - left), (float)(bottom - top) };
}

// 声明BGM
Music bgm;

// 声明音效
Sound shootSound1, shootSound2;   // 上升音效
Sound explodeSound1, explodeSound2; // 绽放音效

// 烟花弹结构体
struct Shoot {
    Vector2 pos;      // 坐标
    Vector2 expos;    // 绽放坐标
    int id;           // 0–15：升空与绽放共用的素材编号
    float speed;      // 速度
    bool flicker;     // 闪烁标志
    bool isActive;    // 是否未绽放
    bool soundPlayed; // 是否已播放上升音效

    void Init() {
        this->pos.x = GetRandomValue(50, SCENE_WIDTH - 50);
        this->pos.y = SCENE_HEIGHT + GetRandomValue(0, 100);
        this->id = GetRandomValue(0, FIREWORK_COUNT - 1);
        this->speed = GetRandomValue(4, 7);
        this->flicker = false;
        this->isActive = true;
        this->soundPlayed = false;  // 初始化时未播放音效
        this->expos.x = this->pos.x;
        this->expos.y = GetRandomValue(SCENE_HEIGHT / 6, SCENE_HEIGHT / 4);
    }

    void Init(Vector2 target) {
        Init();
        expos = { std::clamp(target.x, 0.0f, float(SCENE_WIDTH - 1)),
                  std::clamp(target.y, 0.0f, float(SCENE_HEIGHT - 1)) };
        pos.x = expos.x; // 从目标正下方垂直升空
    }
};

// 烟花形状轮廓类型枚举
enum FlowerType {
    FLOWER_ROUND,      // 圆形
    FLOWER_STAR,       // 星形
    FLOWER_PETAL,      // 花瓣形
    FLOWER_SPIRAL,     // 螺旋形
    FLOWER_BURST,      // 爆裂形
    FLOWER_CLUSTER,    // 簇状
    FLOWER_WAVE        // 波浪形
};

// 绽放效果结构体
struct ExplodeEffect {
    Vector2 pos;                    // 绽放位置
    bool isActive;                  // 是否在绽放
    int colorsId;                   // 烟花种类（对应colorsList索引）
    float maxRadius;                // 最大半径
    float alpha;                    // 整体透明度
    int phase;                      // 0=展开中, 1=完全展开停留, 2=从中心向外消失
    float duration;                 // 持续时间计数器
    float expandSpeed;              // 展开速度
    FlowerType flowerType;          // 花朵类型

    // 存储每个角度对应的实际半径（用于产生不规则形状）
    std::vector<float> radiusMap;   // 半径映射表
    std::vector<float> angleOffset; // 角度偏移量
    int segments;                   // 分段数（360度）

    int currentLayer;               // 当前展开层数
    int maxLayers;                  // 最大层数
    int fadeStartLayer;             // 消失开始的内层半径

    // 初始化绽放效果
    void Init(const Shoot& s) {
        this->pos = s.pos;
        this->isActive = true;
        this->colorsId = s.id;  // 保持升空、绽放颜色一一对应
        this->maxRadius = GetRandomValue(100, 180);
        this->alpha = 1.0f;
        this->phase = 0;
        this->duration = 0;
        this->expandSpeed = 1.0f + GetRandomValue(0, 20) / 10.0f;
        this->segments = 360;
        this->currentLayer = 0;
        this->maxLayers = (int)maxRadius;
        this->fadeStartLayer = 0;

        // 随机选择烟花轮廓类型
        this->flowerType = (FlowerType)GetRandomValue(0, 6);

        // 生成不规则形状的半径映射
        radiusMap.clear();
        radiusMap.reserve(segments);
        angleOffset.clear();
        angleOffset.reserve(segments);

        // 根据烟花类型生成不同的形状
        switch (flowerType) {
        case FLOWER_ROUND:
            generateRoundShape();
            break;
        case FLOWER_STAR:
            generateStarShape();
            break;
        case FLOWER_PETAL:
            generatePetalShape();
            break;
        case FLOWER_SPIRAL:
            generateSpiralShape();
            break;
        case FLOWER_BURST:
            generateBurstShape();
            break;
        case FLOWER_CLUSTER:
            generateClusterShape();
            break;
        case FLOWER_WAVE:
            generateWaveShape();
            break;
        }
    }

    // 生成圆形形状
    void generateRoundShape() {
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float irregularity = 0.85f + 0.15f * (
                sin(angle * 2 + GetRandomValue(0, 100) / 100.0f) * 0.2f +
                cos(angle * 3 + GetRandomValue(0, 100) / 100.0f) * 0.1f
                );
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-3, 3) / 100.0f));
        }
    }

    // 生成星形形状
    void generateStarShape() {
        int points = GetRandomValue(5, 8);  // 5-8个角
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float pointAngle = angle * points / (2 * PI);
            float irregularity = 0.5f + 0.5f * pow(cos(pointAngle * PI), 2);
            irregularity = 0.3f + 0.7f * irregularity;
            irregularity *= (0.9f + 0.1f * sin(angle * 3 + GetRandomValue(0, 100) / 100.0f));
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-5, 5) / 100.0f));
        }
    }

    // 生成花瓣形状
    void generatePetalShape() {
        int petals = GetRandomValue(5, 10);
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float petalAngle = angle * petals / 2;
            float irregularity = 0.3f + 0.7f * (0.5f + 0.5f * cos(petalAngle));
            irregularity *= (0.8f + 0.2f * sin(angle * 2 + GetRandomValue(0, 100) / 100.0f));
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-3, 3) / 100.0f));
        }
    }

    // 生成螺旋形状
    void generateSpiralShape() {
        float spiralFactor = 0.5f + GetRandomValue(0, 100) / 100.0f;
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float irregularity = 0.4f + 0.6f * (0.5f + 0.5f * sin(angle * 3 + spiralFactor * angle * 2));
            irregularity *= (0.9f + 0.1f * cos(angle * 5 + GetRandomValue(0, 100) / 100.0f));
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-5, 5) / 100.0f));
        }
    }

    // 生成爆裂形状（类似真实烟花爆炸）
    void generateBurstShape() {
        // 在随机角度产生突刺
        std::vector<float> burstAngles;
        int burstCount = GetRandomValue(8, 20);
        for (int i = 0; i < burstCount; i++) {
            burstAngles.push_back(GetRandomValue(0, 360) * PI / 180.0f);
        }

        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float irregularity = 0.6f + 0.4f * sin(angle * 2 + GetRandomValue(0, 100) / 100.0f);

            // 在随机角度产生突刺
            for (float burstAngle : burstAngles) {
                float diff = fmod(angle - burstAngle + PI, 2 * PI) - PI;
                if (fabs(diff) < 0.3f) {
                    irregularity *= 1.5f + 0.5f * (1.0f - fabs(diff) / 0.3f);
                }
            }
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-3, 3) / 100.0f));
        }
    }

    // 生成簇状形状（多个小簇）
    void generateClusterShape() {
        int clusters = GetRandomValue(3, 6);
        std::vector<float> clusterAngles;
        for (int i = 0; i < clusters; i++) {
            clusterAngles.push_back(GetRandomValue(0, 360) * PI / 180.0f);
        }

        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float irregularity = 0.3f;

            // 在簇的位置产生高峰
            for (float clusterAngle : clusterAngles) {
                float diff = fmod(angle - clusterAngle + PI, 2 * PI) - PI;
                if (fabs(diff) < 0.8f) {
                    float clusterFactor = 1.0f - fabs(diff) / 0.8f;
                    irregularity += 0.7f * clusterFactor * clusterFactor;
                }
            }
            irregularity = std::min(irregularity, 1.2f);
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-5, 5) / 100.0f));
        }
    }

    // 生成波浪形状
    void generateWaveShape() {
        float waveFreq = GetRandomValue(3, 8);
        for (int i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2 * PI;
            float irregularity = 0.5f + 0.5f * (0.5f + 0.5f * sin(angle * waveFreq + GetRandomValue(0, 100) / 100.0f));
            irregularity *= (0.8f + 0.2f * cos(angle * 2 + GetRandomValue(0, 100) / 100.0f));
            radiusMap.push_back(irregularity);
            angleOffset.push_back((GetRandomValue(-3, 3) / 100.0f));
        }
    }

    // 更新绽放效果
    void Update() {
        if (!isActive) return;

        duration++;

        if (phase == 0) {
            // 阶段0：从中心向外展开
            currentLayer += 2;  // 每次增加2层
            if (currentLayer >= maxLayers) {
                currentLayer = maxLayers;
                phase = 1;      // 切换到停留阶段
                duration = 0;   // 重置计时器
            }
        }
        else if (phase == 1) {
            // 阶段1：完全展开后停留6帧（0.1秒）
            if (duration > 6) {
                phase = 2;      // 切换到消失阶段
                duration = 0;
                fadeStartLayer = 0;  // 从中心开始消失
            }
        }
        else if (phase == 2) {
            // 阶段2：从中心向四周消失
            fadeStartLayer += 3;  // 消失速度
            if (fadeStartLayer >= maxLayers) {
                isActive = false;  // 完全消失
                return;
            }
        }
    }

    // 绘制绽放效果
    void Draw() const {
        if (!isActive) return;

        if (phase == 0) {
            // 展开阶段：从中心向外逐层绘制
            drawExpanding();
        }
        else if (phase == 1) {
            // 完全展开停留阶段：绘制完整的烟花
            drawFullFlower(1.0f);
        }
        else if (phase == 2) {
            // 消失阶段：从中心到四周逐渐消失
            drawFading();
        }
    }

    // 绘制展开中的烟花
    void drawExpanding() const {
        float currentR = currentLayer;
        if (currentR < 1) return;

        // 绘制所有已展开的层
        for (int layer = 0; layer <= currentLayer; layer += 2) {
            float layerRadius = layer;
            if (layerRadius <= 0) continue;

            // 透明度随层数变化，中心更亮，边缘稍暗
            float layerAlpha = alpha * (1.0f - (float)layer / maxLayers * 0.3f);
            if (layerAlpha <= 0.01f) continue;

            // 绘制该层的所有像素
            drawLayer(layerRadius, layerAlpha);
        }

        // 在展开过程中添加随机喷射粒子（模拟爆炸碎片）
        if (currentLayer > 10 && currentLayer < maxLayers * 0.3f) {
            int particleCount = (int)(currentLayer * 0.3f);
            for (int i = 0; i < particleCount; i++) {
                float angle = GetRandomValue(0, 360) * PI / 180.0f;
                float radius = currentLayer * (1.0f + GetRandomValue(0, 60) / 100.0f);
                int imgX = GetRandomValue(0, 239);
                int imgY = GetRandomValue(0, 239);
                int dstX = (int)(cos(angle) * radius + pos.x);
                int dstY = (int)(sin(angle) * radius + pos.y);
                Color c = colorsList[colorsId][imgY * 240 + imgX];
                c.a = (unsigned char)(c.a * alpha * (150.0f / 255.0f));
                DrawPixel(dstX, dstY, c);
            }
        }
    }

    // 绘制完整的烟花
    void drawFullFlower(float alphaMultiplier) const {
        for (int layer = 0; layer <= maxLayers; layer += 2) {
            float layerRadius = layer;
            if (layerRadius <= 0) continue;

            float layerAlpha = alpha * (1.0f - (float)layer / maxLayers * 0.3f) * alphaMultiplier;
            if (layerAlpha <= 0.01f) continue;

            drawLayer(layerRadius, layerAlpha);
        }
    }

    // 绘制消失中的烟花
    void drawFading() const {
        // 绘制外层（保留的部分）
        for (int layer = fadeStartLayer; layer <= maxLayers; layer += 2) {
            float layerRadius = layer;
            if (layerRadius <= 0) continue;

            // 计算透明度：从消失边界到外层逐渐变亮
            float fadeProgress = (float)(layer - fadeStartLayer) / (maxLayers - fadeStartLayer);
            float layerAlpha = alpha * fadeProgress * 0.8f + 0.2f;
            if (layerAlpha <= 0.01f) continue;

            drawLayer(layerRadius, layerAlpha);
        }

        // 内层残影效果（已经消失的部分用微弱光晕表示）
        if (fadeStartLayer > 0 && fadeStartLayer < maxLayers * 0.5f) {
            int glowCount = 30;
            for (int i = 0; i < glowCount; i++) {
                float angle = GetRandomValue(0, 360) * PI / 180.0f;
                float radius = GetRandomValue(0, fadeStartLayer / 2);
                int dstX = (int)(cos(angle) * radius + pos.x);
                int dstY = (int)(sin(angle) * radius + pos.y);
                Color c = colorsList[colorsId][120 * 240 + 120];  // 取图片中心颜色
                c.a = (unsigned char)(c.a * (20.0f / 255.0f));  // 保留素材透明度
                DrawPixel(dstX, dstY, c);
            }
        }
    }

    // 绘制单层烟花
    void drawLayer(float layerRadius, float layerAlpha) const {
        for (int i = 0; i < segments; i += 2) {
            float angle = (float)i / segments * 2 * PI;
            angle += angleOffset[i] * 0.5f;  // 应用角度偏移

            float radiusFactor = radiusMap[i];  // 获取该角度的半径系数
            float actualRadius = layerRadius * radiusFactor;  // 计算实际半径

            // 计算在烟花图片中的位置
            // 纹理半径映射到完整图案，世界半径仍由烟花形状控制。
            float sampleRadius = layerRadius / maxRadius * (FIREWORK_TILE_SIZE / 2 - 1);
            int imgX = (int)(cos(angle) * sampleRadius + FIREWORK_TILE_SIZE / 2);
            int imgY = (int)(sin(angle) * sampleRadius + FIREWORK_TILE_SIZE / 2);

            // 确保在图片范围内
            if (imgX >= 0 && imgX < 240 && imgY >= 0 && imgY < 240) {
                // 计算在世界空间的位置
                int dstX = (int)(cos(angle) * actualRadius + pos.x);
                int dstY = (int)(sin(angle) * actualRadius + pos.y);

                // 从图片获取颜色并设置透明度
                Color c = colorsList[colorsId][imgY * 240 + imgX];
                c.a = (unsigned char)(c.a * std::clamp(layerAlpha, 0.0f, 1.0f));
                DrawPixel(dstX, dstY, c);
            }
        }
    }
};

// 全局变量
Texture2D shootTex;      // 烟花弹纹理
Image firework;          // 烟花图片
Image backgroundImg;     // 背景图片
Texture2D backgroundTex; // 背景纹理
int frameCount = 0;      // 帧计数器
std::vector<Shoot> shoots;           // 烟花弹列表
std::vector<ExplodeEffect> effects;  // 绽放效果列表
int maxShoots = 3;       // 同时存在的烟花数量
constexpr std::size_t MAX_MANUAL_SHOOTS = 6;
constexpr std::size_t MAX_FIREWORKS = 12; // 升空和绽放合计，限制绘制开销
std::size_t automaticLaunchCount = 0;

void advanceShoot(Shoot& p) {
    if (p.pos.y > p.expos.y) p.pos.y = std::max(p.expos.y, p.pos.y - p.speed);
}

// 绘制烟花弹
void shoot_draw(const Shoot& p) {
    if (p.isActive) {
        const Rectangle src = atlasCell(shootTex.width, shootTex.height, p.id);
        const float size = 84.0f;
        // 图集内光点位于每格 18% 高度，p.pos 对齐光点而非尾焰底部。
        const Rectangle dest = { p.pos.x - size / 2, p.pos.y - size * 0.18f, size, size };
        DrawTexturePro(shootTex, src, dest, { 0.0f, 0.0f }, 0.0f,
            Fade(WHITE, p.flicker ? 1.0f : 0.65f));
    }
}

// 更新烟花弹
void shoot_update(Shoot& p) {
    if (p.isActive) {
        advanceShoot(p); // 只向上移动，最后一步精确到达目标高度

        // 播放上升音效（只在第一次开始上升时播放）
        if (!p.soundPlayed) {
            // 随机选择上升音效（sound1或sound2）
            if (GetRandomValue(0, 1) == 0) {
                PlaySound(shootSound1);
            }
            else {
                PlaySound(shootSound2);
            }
            p.soundPlayed = true;  // 标记已播放
        }

        // 每10帧切换闪烁状态
        if (frameCount % 10 == 0) {
            p.flicker = !p.flicker;
        }
    }
}

// 检查烟花弹是否到达绽放位置
void check_explosion(Shoot& p) {
    if (p.isActive && p.pos.x == p.expos.x && p.pos.y == p.expos.y) {
        p.isActive = false;  // 烟花弹失效

        // 播放绽放音效
        if (GetRandomValue(0, 1) == 0) {
            PlaySound(explodeSound1);
        }
        else {
            PlaySound(explodeSound2);
        }

        ExplodeEffect e;
        e.Init(p);  // 初始化绽放效果
        effects.push_back(e);  // 添加到绽放列表
    }
}

// 生成新的烟花弹
void spawn_new_shoot() {
    if (shoots.size() < static_cast<std::size_t>(maxShoots) && shoots.size() + effects.size() < MAX_FIREWORKS) {
        Shoot s;
        s.Init();
        // 检查是否与其他烟花弹位置重叠
        bool overlapping = false;
        for (const auto& existing : shoots) {
            if (abs(existing.pos.x - s.pos.x) < 100) {
                overlapping = true;
                break;
            }
        }
        if (!overlapping) {
            shoots.push_back(s);
            ++automaticLaunchCount;
        }
    }
}

bool spawn_manual_shoot(Vector2 target) {
    if (shoots.size() >= MAX_MANUAL_SHOOTS || shoots.size() + effects.size() >= MAX_FIREWORKS) return false;
    Shoot shoot;
    shoot.Init(target);
    shoots.push_back(shoot);
    return true;
}

// 字体变量
Font mainFont;   // 主标题字体
Font subFont;    // 副标题字体
Font helpFont;   // 操作提示字体，与装饰文字分开

void loadHelpFont() {
    // 使用 Windows 自带的界面字体，无需新增或分发字体素材。
    std::string fontsDirectory = "C:/Windows/Fonts/";
#ifdef _MSC_VER
    char* windowsDirectory = nullptr;
    std::size_t directorySize = 0;
    if (_dupenv_s(&windowsDirectory, &directorySize, "WINDIR") == 0 && windowsDirectory) {
        fontsDirectory = std::string(windowsDirectory) + "/Fonts/";
    }
    std::free(windowsDirectory);
#endif
    for (const char* filename : { "segoeui.ttf", "arial.ttf" }) {
        const std::string path = fontsDirectory + filename;
        if (!FileExists(path.c_str())) continue;
        helpFont = LoadFontEx(path.c_str(), 40, nullptr, 0);
        if (helpFont.texture.id && helpFont.texture.id != GetFontDefault().texture.id) {
            SetTextureFilter(helpFont.texture, TEXTURE_FILTER_BILINEAR);
            return;
        }
    }
    helpFont = GetFontDefault(); // 系统字体不可用时，使用整数倍字号保持清晰
}

std::vector<int> configuredCodepoints() {
    std::vector<int> glyphs;
    for (int c = 32; c < 127; ++c) glyphs.push_back(c);
    for (const auto& text : { appConfig.title, appConfig.subtitle, appConfig.name }) {
        int count = 0;
        int* codepoints = LoadCodepoints(text.c_str(), &count);
        if (codepoints) {
            glyphs.insert(glyphs.end(), codepoints, codepoints + count);
            UnloadCodepoints(codepoints);
        }
    }
    std::sort(glyphs.begin(), glyphs.end());
    glyphs.erase(std::unique(glyphs.begin(), glyphs.end()), glyphs.end());
    return glyphs;
}

float fitText(Font font, const char* text, float size, float width) {
    // Spacing is applied per glyph by MeasureTextEx, so measure after each reduction.
    for (int i = 0; i < 8 && MeasureTextEx(font, text, size, 1.0f).x > width; ++i) {
        size *= width / MeasureTextEx(font, text, size, 1.0f).x;
    }
    return size;
}

struct PlaybackState {
    bool paused = false;
    bool muted = false;
    bool autoFireworks = true;
    bool showHelp = true;
    float volume = 0.5f;
    std::array<bool, 4> pausedSounds{};
    bool pausedMusic = false;
};

void togglePause(PlaybackState& state) {
    state.paused = !state.paused;
    const std::array<Sound, 4> sounds = { shootSound1, shootSound2, explodeSound1, explodeSound2 };
    if (state.paused) {
        state.pausedMusic = IsMusicStreamPlaying(bgm);
        if (state.pausedMusic) PauseMusicStream(bgm);
        for (std::size_t i = 0; i < sounds.size(); ++i) {
            state.pausedSounds[i] = IsSoundPlaying(sounds[i]);
            if (state.pausedSounds[i]) PauseSound(sounds[i]);
        }
    }
    else {
        if (state.pausedMusic) ResumeMusicStream(bgm);
        for (std::size_t i = 0; i < sounds.size(); ++i) {
            if (state.pausedSounds[i]) ResumeSound(sounds[i]);
        }
    }
}

Rectangle sceneViewport(int width, int height) {
    const float scale = std::min(float(width) / SCENE_WIDTH, float(height) / SCENE_HEIGHT);
    return { (width - SCENE_WIDTH * scale) / 2, (height - SCENE_HEIGHT * scale) / 2,
             SCENE_WIDTH * scale, SCENE_HEIGHT * scale };
}

Vector2 sceneMouse(Vector2 mouse, Rectangle viewport) {
    return { (mouse.x - viewport.x) * SCENE_WIDTH / viewport.width,
             (mouse.y - viewport.y) * SCENE_HEIGHT / viewport.height };
}

void drawPlaybackHelp(const PlaybackState& state, Rectangle viewport) {
    const float scale = std::clamp(viewport.width / SCENE_WIDTH, 1.0f, 1.5f);
    const float fontSize = helpFont.texture.id == GetFontDefault().texture.id
        ? 20.0f : std::round(20.0f * scale);
    const float x = std::round(viewport.x + 14);
    const float y = std::round(viewport.y + 14);
    if (!state.showHelp) {
        if (state.paused) DrawTextEx(helpFont, "PAUSED  |  Space: resume", { x, y }, fontSize, 0, WHITE);
        return;
    }
    const std::string status = TextFormat("%s  |  %s  |  %s %d%%",
        state.paused ? "PAUSED" : "PLAYING",
        state.autoFireworks ? "AUTO + CLICK" : "CLICK ONLY",
        state.muted ? "MUTED / Volume" : "Volume", int(std::round(state.volume * 100)));
    const std::array<const char*, 3> lines = {
        status.c_str(),
        "Click: launch  |  Space: pause  |  A: auto  |  M: mute",
        "Up/Down: volume  |  F11: fullscreen  |  F12: screenshot  |  H: hide  |  Esc: exit"
    };
    float longestLine = 0;
    for (const char* line : lines) longestLine = std::max(longestLine, MeasureTextEx(helpFont, line, fontSize, 0).x);
    const float panelWidth = std::min(std::ceil(longestLine) + 28, GetScreenWidth() - x - 14);
    const float textSize = fontSize * std::min(1.0f, (panelWidth - 28) / std::max(longestLine, 1.0f));
    const float lineHeight = std::ceil(textSize + 9);
    const Rectangle panel = { x, y, panelWidth, lineHeight * 3 + 16 };
    DrawRectangleRec(panel, Color{ 10, 16, 26, 235 });
    DrawRectangleLinesEx(panel, 1, Color{ 96, 123, 150, 200 });
    for (std::size_t i = 0; i < lines.size(); ++i) {
        DrawTextEx(helpFont, lines[i], { x + 14, std::round(y + 8 + i * lineHeight) }, textSize, 0,
            i == 0 ? WHITE : Color{ 225, 233, 242, 255 });
    }
}

bool saveScreenshot() {
    if (MakeDirectory("screenshots") != 0 && !DirectoryExists("screenshots")) {
        std::cerr << "Cannot create screenshots directory" << std::endl;
        return false;
    }
    static unsigned int sequence = 0;
    std::string path;
    do {
        path = "screenshots/fireworks-" + std::to_string(std::time(nullptr)) + "-" + std::to_string(sequence++) + ".png";
    } while (FileExists(path.c_str()));
    TakeScreenshot(path.c_str());
    std::cout << "Screenshot: " << path << std::endl;
    return FileExists(path.c_str());
}

void unloadRes() {
    if (mainFont.texture.id && mainFont.texture.id != GetFontDefault().texture.id) UnloadFont(mainFont);
    if (subFont.texture.id && subFont.texture.id != GetFontDefault().texture.id) UnloadFont(subFont);
    if (helpFont.texture.id && helpFont.texture.id != GetFontDefault().texture.id) UnloadFont(helpFont);
    if (IsMusicValid(bgm)) UnloadMusicStream(bgm);
    if (IsSoundValid(shootSound1)) UnloadSound(shootSound1);
    if (IsSoundValid(shootSound2)) UnloadSound(shootSound2);
    if (IsSoundValid(explodeSound1)) UnloadSound(explodeSound1);
    if (IsSoundValid(explodeSound2)) UnloadSound(explodeSound2);
    if (IsAudioDeviceReady()) CloseAudioDevice();
    for (int i = 0; i < FIREWORK_COUNT; i++) {
        if (colorsList[i]) UnloadImageColors(colorsList[i]);
    }
    if (shootTex.id) UnloadTexture(shootTex);
    if (firework.data) UnloadImage(firework);
    if (backgroundImg.data) UnloadImage(backgroundImg);
    if (backgroundTex.id) UnloadTexture(backgroundTex);
}

// 加载资源
bool loadRes(bool silent = false) {
    // ========== 初始化音频设备 ==========
    InitAudioDevice();
    SetMasterVolume(silent ? 0.0f : appConfig.volume);

    // ========== 加载BGM ==========
    bgm = LoadMusicStream("sound/bgm1.wav");
    SetMusicVolume(bgm, 0.5f);
    PlayMusicStream(bgm);

    // ========== 加载音效 ==========
    shootSound1 = LoadSound("sound/sound1.mp3");
    shootSound2 = LoadSound("sound/sound2.mp3");
    explodeSound1 = LoadSound("sound/sound3.mp3");
    explodeSound2 = LoadSound("sound/sound4.mp3");

    // ========== 加载自定义字体 ==========
    std::vector<int> glyphs = configuredCodepoints();
    mainFont = LoadFontEx("font/font1.ttf", 64, glyphs.data(), static_cast<int>(glyphs.size()));
    if (mainFont.texture.id == 0) {
        mainFont = GetFontDefault();
        std::cout << "Main font not found, using default font" << std::endl;
    }
    else {
        std::cout << "Main font loaded successfully" << std::endl;
    }

    subFont = LoadFontEx("font/font3.TTF", 48, glyphs.data(), static_cast<int>(glyphs.size()));
    if (subFont.texture.id == 0) {
        subFont = GetFontDefault();
        std::cout << "Sub font not found, using default font" << std::endl;
    }
    else {
        std::cout << "Sub font loaded successfully" << std::endl;
    }
    loadHelpFont();

    // 加载烟花弹图片
    shootTex = LoadTexture("picture/launch-generated-v1.png");
    // 加载烟花绽放图片
    firework = LoadImage("picture/fireworks-generated-v1.png");
    // 加载背景图片
    backgroundImg = LoadImage("picture/bg.png");

    // 检查图片是否加载成功
    if (shootTex.id == 0 || firework.data == NULL || backgroundImg.data == NULL) {
        std::cout << "Failed to load images!" << std::endl;
        return false;
    }

    // 将背景图片转换为纹理（用于高效绘制）
    backgroundTex = LoadTextureFromImage(backgroundImg);
    if (!backgroundTex.id || firework.width < ATLAS_COLUMNS || firework.height < ATLAS_ROWS ||
        shootTex.width < ATLAS_COLUMNS || shootTex.height < ATLAS_ROWS) return false;
    SetTextureFilter(shootTex, TEXTURE_FILTER_BILINEAR);

    // 按实际尺寸四等分，兼容无法被 4 整除的图集。
    for (int i = 0; i < FIREWORK_COUNT; i++) {
        Rectangle src = atlasCell(firework.width, firework.height, i);
        Image tile = ImageFromImage(firework, src);
        if (!tile.data) return false;
        ImageResize(&tile, FIREWORK_TILE_SIZE, FIREWORK_TILE_SIZE);
        colorsList[i] = LoadImageColors(tile);
        UnloadImage(tile);
        if (!colorsList[i]) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    const bool interactionTest = argc > 1 && std::strcmp(argv[1], "--interaction-test") == 0;
    const bool smokeTest = interactionTest || (argc > 1 && std::strcmp(argv[1], "--smoke-test") == 0);
    const char* requiredAssets[] = {
        "picture/launch-generated-v1.png", "picture/fireworks-generated-v1.png", "picture/bg.png",
        "font/font1.ttf", "font/font3.TTF", "sound/bgm1.wav", "sound/sound1.mp3",
        "sound/sound2.mp3", "sound/sound3.mp3", "sound/sound4.mp3"
    };
    // 支持从项目根目录调试，以及在输出目录外双击/命令行启动。
    if (!FileExists(requiredAssets[0])) ChangeDirectory(GetApplicationDirectory());
    for (const char* path : requiredAssets) {
        if (!FileExists(path)) {
            std::cerr << "Missing required asset: " << path << std::endl;
            return 1;
        }
    }
    if (smokeTest) {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        SetRandomSeed(20261013);
    }

    // 工作目录（VS/项目根目录）优先，其次读取可执行文件旁的个人配置。
    std::ifstream settings("config.ini");
    if (!settings) settings.open(std::string(GetApplicationDirectory()) + "config.ini");
    if (settings) appConfig = readConfig(settings);
    if (smokeTest) appConfig = AppConfig{}; // 自动检查不受个人配置影响

    // 初始化窗口
    InitWindow(SCENE_WIDTH, SCENE_HEIGHT, "Fireworks Blessings");
    SetTargetFPS(60);

    // 加载所有资源（包括BGM和字体）
    if (!loadRes(smokeTest)) {
        unloadRes();
        CloseWindow();
        return 1;
    }
    if (smokeTest) {
        SetMasterVolume(0.0f);
        // 检查全部 16 个编号都在升空 -> 绽放时保持不变。
        for (int id = 0; id < FIREWORK_COUNT; id++) {
            Shoot sample{};
            sample.id = id;
            ExplodeEffect effect;
            effect.Init(sample);
            if (effect.colorsId != id) {
                std::cerr << "Pairing check failed: " << id << std::endl;
                unloadRes();
                CloseWindow();
                return 1;
            }
        }
        SetRandomSeed(20261013);
    }

    PlaybackState state;
    state.volume = appConfig.volume;
    state.autoFireworks = appConfig.autoFireworks;
    SetMasterVolume(smokeTest ? 0.0f : state.volume);
    RenderTexture2D scene = LoadRenderTexture(SCENE_WIDTH, SCENE_HEIGHT);
    if (!IsRenderTextureValid(scene)) {
        std::cerr << "Cannot create scene framebuffer" << std::endl;
        unloadRes();
        CloseWindow();
        return 1;
    }
    SetTextureFilter(scene.texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(scene);
    ClearBackground(BLACK);
    DrawTexture(backgroundTex, 0, 0, WHITE);
    EndTextureMode();
    bool checksPassed = true;
    const auto verify = [&](bool passed, const char* message) {
        if (!passed) {
            checksPassed = false;
            std::cerr << "CHECK FAILED: " << message << std::endl;
        }
    };
    if (smokeTest) {
        for (float targetX : { 0.0f, 480.0f, 959.0f }) {
            for (float targetY : { 0.0f, 320.0f, 639.0f }) {
                Shoot vertical;
                vertical.Init({ targetX, targetY });
                verify(vertical.pos.x == targetX && vertical.pos.y > targetY,
                    "launch starts directly below target");
                for (int i = 0; i < 200; ++i) {
                    const float previousY = vertical.pos.y;
                    advanceShoot(vertical);
                    verify(vertical.pos.x == targetX && vertical.pos.y <= previousY && vertical.pos.y >= targetY,
                        "launch travels upward only, without horizontal drift or overshoot");
                }
                verify(vertical.pos.x == targetX && vertical.pos.y == targetY, "exact vertical arrival");
            }
        }
        Shoot edge;
        edge.Init({ -20, 900 });
        verify(edge.expos.x == 0 && edge.expos.y == SCENE_HEIGHT - 1, "clamp click targets");
        for (std::size_t i = 0; i < MAX_MANUAL_SHOOTS; ++i) {
            verify(spawn_manual_shoot({ 480, 200 }), "manual spawn within limit");
        }
        verify(!spawn_manual_shoot({ 480, 200 }), "manual launch limit");
        shoots.clear();
        effects.resize(MAX_FIREWORKS);
        verify(!spawn_manual_shoot({ 480, 200 }), "combined effect limit");
        effects.clear();
        const Rectangle wide = sceneViewport(1920, 1080);
        const Vector2 center = sceneMouse({ 960, 540 }, wide);
        verify(std::fabs(center.x - 480) < 0.01f && std::fabs(center.y - 320) < 0.01f,
            "letterboxed mouse coordinates");
        SetRandomSeed(20261013);
    }

    int nameCount = 0;
    int* nameCodepoints = LoadCodepoints(appConfig.name.c_str(), &nameCount);

    // 仅自动模式预置烟花；手动模式启动时保持安静。
    for (int i = 0; state.autoFireworks && i < maxShoots; i++) {
        Shoot s;
        s.Init();
        s.pos.x = 150 + i * 300;
        s.expos.x = s.pos.x;
        s.expos.y = GetRandomValue(SCENE_HEIGHT / 4, SCENE_HEIGHT / 2);
        shoots.push_back(s);
    }

    // ========== 动画变量 ==========
    float animTime = 0.0f;

    // 星星位置预生成
    std::vector<Vector2> starPositions;
    std::vector<float> starSpeeds;
    std::vector<float> starSizes;
    for (int i = 0; i < 30; i++) {
        Vector2 pos;
        pos.x = (float)GetRandomValue(0, SCENE_WIDTH);
        pos.y = (float)GetRandomValue(0, SCENE_HEIGHT);
        starPositions.push_back(pos);
        starSpeeds.push_back(0.5f + GetRandomValue(0, 100) / 100.0f);
        starSizes.push_back(1.0f + GetRandomValue(0, 30) / 10.0f);
    }

    // 主循环
    int loopCount = 0;
    int pausedFrame = -1;
    std::size_t automaticAtDisable = 0;
    int pausedImageSize = 0;
    std::vector<unsigned char> pausedPixels;
    float nextManualLaunch = 0.0f;
    while (!WindowShouldClose() && (!smokeTest || loopCount < 240)) {
        const bool scriptedPause = interactionTest &&
            (loopCount == 20 || loopCount == 35 || loopCount == 95 || loopCount == 105);
        if (IsKeyPressed(KEY_SPACE) || scriptedPause) togglePause(state);
        if (IsKeyPressed(KEY_M) || (interactionTest && loopCount == 40)) state.muted = !state.muted;
        if (IsKeyPressed(KEY_A) || (interactionTest && (loopCount == 50 || loopCount == 130))) {
            state.autoFireworks = !state.autoFireworks;
            if (interactionTest && !state.autoFireworks) automaticAtDisable = automaticLaunchCount;
        }
        if (IsKeyPressed(KEY_H)) state.showHelp = !state.showHelp;
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || (interactionTest && loopCount == 45)) {
            state.volume = std::clamp(state.volume + 0.05f, 0.0f, 1.0f);
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            state.volume = std::clamp(state.volume - 0.05f, 0.0f, 1.0f);
        }
        SetMasterVolume(smokeTest || state.muted ? 0.0f : state.volume);
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        const Rectangle viewport = sceneViewport(GetScreenWidth(), GetScreenHeight());
        const Vector2 mouse = GetMousePosition();
        if (!state.paused && animTime >= nextManualLaunch && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(mouse, viewport)) {
            if (spawn_manual_shoot(sceneMouse(mouse, viewport))) nextManualLaunch = animTime + 0.15f;
        }
        if (interactionTest && loopCount == 70) {
            verify(!state.autoFireworks, "manual-only mode");
            verify(spawn_manual_shoot({ 700, 450 }), "manual spawn with auto disabled");
        }
        if (interactionTest && loopCount == 20) {
            pausedFrame = frameCount;
            Image image = LoadImageFromTexture(scene.texture);
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            pausedImageSize = image.width * image.height * 4;
            if (image.data) pausedPixels.assign(static_cast<unsigned char*>(image.data),
                static_cast<unsigned char*>(image.data) + pausedImageSize);
            UnloadImage(image);
        }
        if (interactionTest && loopCount == 34) {
            verify(state.paused && frameCount == pausedFrame, "pause stops simulation");
            Image image = LoadImageFromTexture(scene.texture);
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            verify(image.data && !pausedPixels.empty() &&
                std::memcmp(image.data, pausedPixels.data(), pausedImageSize) == 0,
                "pause freezes every scene pixel");
            UnloadImage(image);
            verify(!IsMusicStreamPlaying(bgm), "pause stops music");
        }
        if (interactionTest && loopCount == 46) {
            verify(state.muted && std::fabs(state.volume - 0.55f) < 0.001f, "mute retains adjusted volume");
            verify(GetMasterVolume() == 0.0f, "muted audio stays silent");
        }
        if (interactionTest && loopCount == 36) {
            verify(!state.paused && IsMusicStreamPlaying(bgm), "resume restores music");
        }
        if (interactionTest && loopCount == 129) {
            verify(automaticLaunchCount == automaticAtDisable, "manual mode never auto-refills");
        }
        if (!state.paused) UpdateMusicStream(bgm);

        // 暂停时直接复用上一帧，连随机闪光也保持不变。
        if (!state.paused) {
            // ========== 更新动画参数 ==========
            animTime += 0.02f;
            float titleScale = 1.0f + 0.12f * sin(animTime * 1.2f);
            float bounceY = 8.0f * sin(animTime * 0.8f);

            BeginTextureMode(scene);
            ClearBackground(BLACK);

            // 绘制背景图片
            DrawTexture(backgroundTex, 0, 0, WHITE);

            // ========== 1. 闪烁的小星星装饰 ==========
            for (int i = 0; i < (int)starPositions.size(); i++) {
                float twinkle = 0.3f + 0.7f * (0.5f + 0.5f * sin(animTime * starSpeeds[i] * 2.0f + i));
                unsigned char alpha = (unsigned char)(twinkle * 200);
                Color starColor = { 255, 255, 200, alpha };

                float size = starSizes[i] * twinkle;
                Vector2 pos = starPositions[i];
                pos.x += sin(animTime * 0.3f + i) * 0.3f;
                pos.y += cos(animTime * 0.4f + i * 0.7f) * 0.3f;

                DrawCircleV(pos, size, starColor);
                DrawCircleV(pos, size * 0.5f, Color{ 255, 255, 255, (unsigned char)(alpha * 0.5f) });
            }

            // ========== 2. 彩色的飘动气球装饰（靠近窗口下部） ==========
            for (int i = 0; i < 6; i++) {
                float x = 80 + i * 160 + 20 * sin(animTime * 0.5f + i * 1.2f);
                float y = SCENE_HEIGHT - 80 + 30 * sin(animTime * 0.7f + i * 0.9f);
                float balloonSize = 15.0f + 5 * sin(animTime * 0.3f + i);

                Color balloonColor = {
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.3f + i * 0.5f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.3f + i * 0.5f + 1.0f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.3f + i * 0.5f + 2.0f))),
                    200
                };

                DrawCircle(x, y, balloonSize, balloonColor);
                DrawCircle(x - balloonSize * 0.3f, y - balloonSize * 0.3f, balloonSize * 0.3f,
                    Color{ 255, 255, 255, 100 });
                DrawLine(x, y + balloonSize, x + 5 * sin(animTime * 0.5f + i), y + balloonSize + 20,
                    Color{ 200, 200, 200, 150 });

                Vector2 p1 = { x - 3, y + balloonSize };
                Vector2 p2 = { x + 3, y + balloonSize };
                Vector2 p3 = { x, y + balloonSize + 6 };
                DrawTriangle(p1, p2, p3, Color{ 200, 200, 200, 150 });
            }

            // ========== 3. 飘落的彩色花瓣 ==========
            for (int i = 0; i < 15; i++) {
                float x = fmodf((i * 137.0f + animTime * 20.0f * (0.5f + (i % 3) * 0.3f)), (float)SCENE_WIDTH);
                float y = fmodf((i * 251.0f + animTime * 30.0f * (0.3f + (i % 2) * 0.5f)), (float)SCENE_HEIGHT);
                float petalSize = 3.0f + 2 * sin(animTime * 0.5f + i);

                Color petalColor = {
                    (unsigned char)(200 + 55 * sin(animTime * 0.2f + i * 0.7f)),
                    (unsigned char)(100 + 155 * fabs(sin(animTime * 0.3f + i * 0.5f))),
                    (unsigned char)(200 + 55 * sin(animTime * 0.4f + i * 0.3f)),
                    180
                };

                DrawEllipse(x, y, petalSize, petalSize * 0.6f, petalColor);
                DrawEllipse(x - petalSize * 0.3f, y - petalSize * 0.2f,
                    petalSize * 0.4f, petalSize * 0.3f,
                    Color{ 255, 255, 255, 60 });
            }

            // ========== 4. 主标题（使用 font1.ttf） ==========
            const char* title = appConfig.title.c_str();
            float fontSize = fitText(mainFont, title, 75 * titleScale, SCENE_WIDTH - 100.0f);
            Vector2 textSize = MeasureTextEx(mainFont, title, fontSize, 1.0f);
            int textX = SCENE_WIDTH / 2 - (int)textSize.x / 2;
            int textY = SCENE_HEIGHT / 2 - static_cast<int>(fontSize) / 2 + (int)bounceY - 30;

            // 五彩纸屑效果
            for (int i = 0; i < 30; i++) {
                float angle = animTime * 0.5f + i * 0.5f;
                float radius = fontSize * 1.2f + 20 * sin(animTime * 0.3f + i);
                float confettiX = textX + textSize.x / 2 + cos(angle) * radius;
                float confettiY = textY + fontSize * 0.5f + sin(angle * 0.7f) * radius * 0.5f;

                Color confettiColor = {
                    (unsigned char)(150 + 105 * fabs(sin(animTime + i * 0.3f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime + i * 0.3f + 1.0f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime + i * 0.3f + 2.0f))),
                    (unsigned char)(100 + 155 * fabs(sin(animTime * 0.5f + i)))
                };
                DrawCircle(confettiX, confettiY, 2.0f + 2 * sin(animTime + i), confettiColor);
            }

            // 光晕效果
            for (int i = 5; i > 0; i--) {
                float hue = animTime + (float)i / 10;
                Color glowColor = {
                    (unsigned char)(100 + 155 * fabs(sin(hue))),
                    (unsigned char)(100 + 155 * fabs(sin(hue + 1.0f))),
                    (unsigned char)(100 + 155 * fabs(sin(hue + 2.0f))),
                    (unsigned char)(40 / i)
                };
                DrawTextEx(mainFont, title,
                    Vector2{ (float)(textX + i * sin(animTime + i)),
                            (float)(textY + i * cos(animTime + i)) },
                    fontSize, 1.0f, glowColor);
            }

            // 主文字 - 金色渐变
            Color mainColor = {
                (unsigned char)(200 + 55 * sin(animTime)),
                (unsigned char)(200 + 55 * sin(animTime + 1.0f)),
                (unsigned char)(50 + 50 * sin(animTime + 2.0f)),
                255
            };
            DrawTextEx(mainFont, title, Vector2{ (float)textX, (float)textY }, fontSize, 1.0f, mainColor);

            // 白色描边
            DrawTextEx(mainFont, title, Vector2{ (float)(textX - 1), (float)textY }, fontSize, 1.0f, Color{ 255, 255, 255, 150 });
            DrawTextEx(mainFont, title, Vector2{ (float)(textX + 1), (float)textY }, fontSize, 1.0f, Color{ 255, 255, 255, 150 });
            DrawTextEx(mainFont, title, Vector2{ (float)textX, (float)(textY - 1) }, fontSize, 1.0f, Color{ 255, 255, 255, 150 });
            DrawTextEx(mainFont, title, Vector2{ (float)textX, (float)(textY + 1) }, fontSize, 1.0f, Color{ 255, 255, 255, 150 });

            // ========== 5. 副标题（使用 font3.TTF） ==========
            const char* subTitle = appConfig.subtitle.c_str();
            float subFontSize = fitText(subFont, subTitle, 48, SCENE_WIDTH - 140.0f);
            Vector2 subTextSize = MeasureTextEx(subFont, subTitle, subFontSize, 1.0f);
            int subTextX = SCENE_WIDTH / 2 - (int)subTextSize.x / 2;
            int subTextY = textY + static_cast<int>(fontSize) + 25 + (int)(5 * sin(animTime * 0.5f));

            // 副标题使用渐变颜色（水墨风格）
            Color subColor = {
                (unsigned char)(180 + 75 * fabs(sin(animTime * 0.3f))),
                (unsigned char)(200 + 55 * fabs(sin(animTime * 0.3f + 0.5f))),
                (unsigned char)(220 + 35 * fabs(sin(animTime * 0.3f + 1.0f))),
                255
            };

            // 副标题阴影（更淡的墨色）
            DrawTextEx(subFont, subTitle, Vector2{ (float)(subTextX + 2), (float)(subTextY + 2) },
                subFontSize, 1.0f, Color{ 0, 0, 0, 80 });

            // 主副标题
            DrawTextEx(subFont, subTitle, Vector2{ (float)subTextX, (float)subTextY },
                subFontSize, 1.0f, subColor);

            // 副标题两侧的装饰~
            for (int i = 0; !appConfig.subtitle.empty() && i < 2; i++) {
                float starX = subTextX - 30 - i * 8 + 5 * sin(animTime * 0.7f + i);
                float starY = subTextY + subFontSize / 2 + 5 * sin(animTime * 0.5f + i * 1.2f);
                DrawTextEx(subFont, "~", Vector2{ starX, starY - subFontSize / 2 },
                    subFontSize * 0.6f, 1.0f, Color{ 255, 215, 0, 180 });

                float starX2 = subTextX + subTextSize.x + 30 + i * 8 + 5 * sin(animTime * 0.7f + i + 1);
                float starY2 = subTextY + subFontSize / 2 + 5 * sin(animTime * 0.5f + i * 1.2f + 1);
                DrawTextEx(subFont, "~", Vector2{ starX2, starY2 - subFontSize / 2 },
                    subFontSize * 0.6f, 1.0f, Color{ 255, 215, 0, 180 });
            }

            // ========== 6. 姓名：按 UTF-8 字符数自动居中排布 ==========
            const float cellWidth = SCENE_WIDTH / float(std::max(nameCount, 1));
            const float nameSize = std::min(30.0f, cellWidth * 0.55f);
            for (int i = 0; nameCodepoints && i < nameCount; i++) {
                float emojiX = (i + 0.5f) * cellWidth + std::min(12.0f, cellWidth * 0.1f) * sin(animTime * 0.6f + i * 1.1f);
                float emojiY = SCENE_HEIGHT - 45 + 10 * sin(animTime * 0.8f + i * 0.7f);
                float emojiSize = nameSize * (1.0f + 0.12f * sin(animTime * 0.3f + i));

                Color emojiColor = {
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.2f + i * 0.5f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.3f + i * 0.7f + 0.5f))),
                    (unsigned char)(150 + 105 * fabs(sin(animTime * 0.4f + i * 0.3f + 1.0f))),
                    255
                };
                int characterBytes = 0;
                const std::string character = CodepointToUTF8(nameCodepoints[i], &characterBytes);
                const float characterWidth = MeasureTextEx(mainFont, character.c_str(), emojiSize, 0).x;
                DrawTextCodepoint(mainFont, nameCodepoints[i],
                    { emojiX - characterWidth / 2, emojiY }, emojiSize, emojiColor);
            }

            // 定期补充烟花弹
            if (state.autoFireworks && frameCount % 40 == 0) {
                int activeShoots = 0;
                for (const auto& s : shoots) {
                    if (s.isActive) activeShoots++;
                }
                if (activeShoots < maxShoots) {
                    spawn_new_shoot();
                }
            }

            // 更新和绘制所有烟花弹
            for (auto& s : shoots) {
                shoot_update(s);
                check_explosion(s);
                shoot_draw(s);
            }

            // 清理已完成的烟花弹（已绽放的）
            shoots.erase(
                std::remove_if(shoots.begin(), shoots.end(),
                    [](const Shoot& s) { return !s.isActive; }),
                shoots.end()
            );

            // 更新和绘制所有绽放效果
            for (auto& e : effects) {
                e.Update();
                e.Draw();
            }

            // 清理已完成的绽放效果（已完全消失的）
            effects.erase(
                std::remove_if(effects.begin(), effects.end(),
                    [](const ExplodeEffect& e) { return !e.isActive; }),
                effects.end()
            );

            // 如果烟花太少，立即补充
            if (state.autoFireworks && (int)shoots.size() < 2 && (int)effects.size() < 2) {
                for (int i = 0; i < 2; i++) {
                    spawn_new_shoot();
                }
            }

            EndTextureMode();
            frameCount++;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(scene.texture, { 0, 0, float(SCENE_WIDTH), -float(SCENE_HEIGHT) },
            viewport, { 0, 0 }, 0, WHITE);
        drawPlaybackHelp(state, viewport);
        EndDrawing();
        if (IsKeyPressed(KEY_F12) || (interactionTest && loopCount == 110)) {
            const bool saved = saveScreenshot();
            if (interactionTest) verify(saved, "screenshot file saved");
        }
        if (smokeTest && loopCount == 120) {
            TakeScreenshot(interactionTest ? "interaction-test.png" : "smoke-test.png");
        }
        loopCount++;
    }

    // ========== 释放资源 ==========
    if (nameCodepoints) UnloadCodepoints(nameCodepoints);
    UnloadRenderTexture(scene);
    unloadRes();

    CloseWindow();
    if (smokeTest && checksPassed) {
        std::cout << (interactionTest ? "INTERACTION TEST OK" : "SMOKE TEST OK")
            << ": 16 paired assets, targeting, limits, viewport, 240 frames" << std::endl;
    }
    return checksPassed ? 0 : 1;
}

