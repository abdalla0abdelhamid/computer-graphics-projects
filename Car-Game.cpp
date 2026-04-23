#define _CRT_SECURE_NO_WARNINGS

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <glm/glm.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>

// =====================================================
// Game State
// =====================================================
enum GameState { MENU, PLAYING, GAME_OVER };
GameState gameState = MENU;

// =====================================================
// Car
// =====================================================
float carX = 0.0f;
float carZ = 0.0f;
float speed = 0.0f;
float carTilt = 0.0f;   // إمالة بصرية عند الانعطاف

// =====================================================
// Difficulty / Speed  (التدرج هنا)
// =====================================================
const float BASE_MAX_SPEED = 0.18f;   // سرعة بداية (بطيئة)
const float TOP_MAX_SPEED = 0.75f;   // أقصى سرعة ممكنة في اللعبة
const float ACCELERATION = 0.0025f; // تسارع بطيء جداً
const float FRICTION = 0.004f;  // احتكاك عند الإفلات
float currentMaxSpeed = BASE_MAX_SPEED;
int   level = 1;

// =====================================================
// Score & Input
// =====================================================
int  score = 0;
bool keyUp = false;
bool keyLeft = false;
bool keyRight = false;

// =====================================================
// Obstacles
// =====================================================
struct Obstacle {
    float x, z;
    int   type;   // 0=صندوق أحمر  1=كون برتقالي  2=جدار ضيق
    bool  passed;
};
std::vector<Obstacle> obstacles;
float nextSpawnZ = -25.0f;
float spawnInterval = 22.0f;   // المسافة بين العوائق (تقل مع المستوى)

// =====================================================
// Particles (شرار التصادم)
// =====================================================
struct Particle {
    float x, y, z, vx, vy, vz, life;
};
std::vector<Particle> particles;

// =====================================================
// إعادة اللعبة
// =====================================================
void resetGame() {
    carX = 0.0f; carZ = 0.0f;
    speed = 0.0f; carTilt = 0.0f;
    score = 0; level = 1;
    currentMaxSpeed = BASE_MAX_SPEED;
    spawnInterval = 22.0f;
    obstacles.clear();
    particles.clear();
    nextSpawnZ = -25.0f;
    keyUp = keyLeft = keyRight = false;
    gameState = PLAYING;
}

// =====================================================
// Drawing Helpers
// =====================================================
void drawCube(float x, float y, float z,
    float sx, float sy, float sz,
    float r, float g, float b, float alpha = 1.0f)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
    // Front
    glVertex3f(-0.5f, -0.5f, 0.5f); glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    // Back
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f); glVertex3f(0.5f, -0.5f, -0.5f);
    // Left
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, -0.5f);
    // Right
    glVertex3f(0.5f, -0.5f, -0.5f); glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f); glVertex3f(0.5f, -0.5f, 0.5f);
    // Top
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    // Bottom
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glEnd();
    glPopMatrix();
}

void drawText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void drawBigText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
}

// =====================================================
// Environment
// =====================================================

// جبال في الخلفية
void drawMountains() {
    float cols[][3] = { {0.35f,0.42f,0.5f},{0.28f,0.35f,0.45f},{0.42f,0.48f,0.55f} };
    float mts[][3] = { {-40.f,0,-120.f},{-20.f,0,-130.f},{0,0,-140.f},
                       {20.f,0,-130.f},{40.f,0,-120.f},{-55.f,0,-110.f},{55.f,0,-110.f} };
    float sizes[] = { 18,22,26,20,16,14,14 };
    float heights[] = { 12,16,20,15,11,10,10 };
    for (int i = 0; i < 7; i++) {
        drawCube(mts[i][0], heights[i] / 2.0f, carZ + mts[i][2],
            sizes[i], heights[i], sizes[i],
            cols[i % 3][0], cols[i % 3][1], cols[i % 3][2]);
    }
}

// أشجار على جانبي الطريق
void drawTree(float x, float z) {
    drawCube(x, 0.8f, z, 0.5f, 1.6f, 0.5f, 0.45f, 0.28f, 0.08f); // جذع
    drawCube(x, 2.4f, z, 1.8f, 1.4f, 1.8f, 0.12f, 0.55f, 0.12f); // أوراق سفلية
    drawCube(x, 3.4f, z, 1.2f, 1.0f, 1.2f, 0.15f, 0.65f, 0.15f); // أوراق وسط
    drawCube(x, 4.2f, z, 0.7f, 0.7f, 0.7f, 0.18f, 0.72f, 0.18f); // قمة
}

void drawTrees() {
    float base = floorf((carZ - 90.0f) / 12.0f) * 12.0f;
    for (float z = base; z < carZ + 25.0f; z += 12.0f) {
        // يسار
        drawTree(-7.5f, z);
        drawTree(-11.0f, z + 6.0f);
        // يمين
        drawTree(7.5f, z);
        drawTree(11.0f, z + 6.0f);
    }
}

// سواتر جانبية
void drawGuardrails() {
    float base = floorf((carZ - 90.0f) / 5.0f) * 5.0f;
    for (float z = base; z < carZ + 20.0f; z += 5.0f) {
        // عمود يسار
        drawCube(-5.3f, 0.5f, z, 0.15f, 1.0f, 0.15f, 0.85f, 0.85f, 0.85f);
        // عمود يمين
        drawCube(5.3f, 0.5f, z, 0.15f, 1.0f, 0.15f, 0.85f, 0.85f, 0.85f);
        // حاجز يسار
        drawCube(-5.3f, 0.5f, z - 2.5f, 0.12f, 0.3f, 5.0f, 0.9f, 0.9f, 0.9f);
        // حاجز يمين
        drawCube(5.3f, 0.5f, z - 2.5f, 0.12f, 0.3f, 5.0f, 0.9f, 0.9f, 0.9f);
    }
}

// أعمدة إنارة
void drawLamps() {
    float base = floorf((carZ - 90.0f) / 20.0f) * 20.0f;
    for (float z = base; z < carZ + 20.0f; z += 20.0f) {
        for (int side = -1; side <= 1; side += 2) {
            float sx = side * 6.2f;
            // عمود
            drawCube(sx, 2.5f, z, 0.2f, 5.0f, 0.2f, 0.5f, 0.5f, 0.5f);
            // ذراع
            drawCube(sx - side * 0.6f, 5.2f, z, 1.2f, 0.15f, 0.15f, 0.5f, 0.5f, 0.5f);
            // مصباح
            drawCube(sx - side * 1.2f, 5.1f, z, 0.4f, 0.25f, 0.4f, 1.0f, 1.0f, 0.7f);
        }
    }
}

void drawRoad() {
    // عشب
    glPushMatrix();
    glTranslatef(0, -0.05f, carZ);
    glScalef(70.0f, 0.1f, 220.0f);
    glColor3f(0.18f, 0.50f, 0.16f);
    glBegin(GL_QUADS);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(0.5f, 0, -0.5f);
    glVertex3f(0.5f, 0, 0.5f); glVertex3f(-0.5f, 0, 0.5f);
    glEnd();
    glPopMatrix();

    // أسفلت
    glPushMatrix();
    glTranslatef(0, 0.0f, carZ);
    glScalef(10.6f, 0.1f, 220.0f);
    glColor3f(0.22f, 0.22f, 0.22f);
    glBegin(GL_QUADS);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(0.5f, 0, -0.5f);
    glVertex3f(0.5f, 0, 0.5f); glVertex3f(-0.5f, 0, 0.5f);
    glEnd();
    glPopMatrix();

    // حافة بيضاء يمين/يسار
    drawCube(-5.0f, 0.05f, carZ, 0.15f, 0.05f, 220.0f, 0.95f, 0.95f, 0.95f);
    drawCube(5.0f, 0.05f, carZ, 0.15f, 0.05f, 220.0f, 0.95f, 0.95f, 0.95f);

    // خطوط وسط الطريق (متحركة)
    float base = floorf((carZ - 90.0f) / 8.0f) * 8.0f;
    for (float z = base; z < carZ + 20.0f; z += 8.0f) {
        drawCube(-1.75f, 0.05f, z, 0.12f, 0.05f, 4.0f, 0.95f, 0.95f, 0.95f);
        drawCube(1.75f, 0.05f, z, 0.12f, 0.05f, 4.0f, 0.95f, 0.95f, 0.95f);
    }
}

// =====================================================
// Obstacles - أنواع مختلفة
// =====================================================
void drawObstacleBox(float x, float z) {
    // جسم أحمر
    drawCube(x, 0.5f, z, 1.4f, 1.0f, 1.4f, 0.85f, 0.10f, 0.10f);
    // شريط أصفر تحذيري
    drawCube(x, 1.05f, z, 1.45f, 0.18f, 1.45f, 1.0f, 0.85f, 0.0f);
    // سهم أبيض على الوجه الأمامي (محاكاة بقطعتين)
    drawCube(x, 0.55f, z + 0.72f, 0.7f, 0.1f, 0.05f, 1.f, 1.f, 1.f);
}

void drawObstacleCone(float x, float z) {
    // قاعدة
    drawCube(x, 0.1f, z, 1.0f, 0.15f, 1.0f, 0.9f, 0.9f, 0.9f);
    // جسم البرتقال (كون = مكعبات متناقصة)
    drawCube(x, 0.45f, z, 0.65f, 0.55f, 0.65f, 1.0f, 0.45f, 0.0f);
    drawCube(x, 0.85f, z, 0.35f, 0.35f, 0.35f, 1.0f, 0.50f, 0.0f);
    drawCube(x, 1.10f, z, 0.15f, 0.2f, 0.15f, 0.9f, 0.9f, 0.9f);
}

void drawObstacleBarrier(float x, float z) {
    // حاجز طريق خرساني
    drawCube(x, 0.3f, z, 2.8f, 0.6f, 0.8f, 0.75f, 0.75f, 0.75f);
    drawCube(x, 0.75f, z, 2.4f, 0.3f, 0.6f, 0.65f, 0.65f, 0.65f);
    // شرائط حمراء/بيضاء
    for (int i = -1; i <= 1; i++) {
        float cx = x + i * 0.9f;
        float col = (i + 1) % 2 == 0 ? 1.0f : 0.0f;
        drawCube(cx, 0.3f, z + 0.41f, 0.5f, 0.55f, 0.05f, 1.0f, col, col);
    }
}

void drawObstacles() {
    for (const auto& obs : obstacles) {
        switch (obs.type) {
        case 0: drawObstacleBox(obs.x, obs.z);     break;
        case 1: drawObstacleCone(obs.x, obs.z);    break;
        case 2: drawObstacleBarrier(obs.x, obs.z); break;
        }
    }
}

// =====================================================
// السيارة (محسّنة)
// =====================================================
void drawCar() {
    glPushMatrix();
    glTranslatef(carX, 0.0f, carZ);
    glRotatef(carTilt, 0, 0, 1); // إمالة عند الانعطاف

    // هيكل سفلي
    drawCube(0, 0.28f, 0.0f, 1.8f, 0.35f, 3.4f, 0.15f, 0.15f, 0.15f);
    // جسم أحمر
    drawCube(0, 0.60f, 0.1f, 1.75f, 0.55f, 3.0f, 0.88f, 0.08f, 0.08f);
    // سقف
    drawCube(0, 1.02f, -0.15f, 1.50f, 0.52f, 1.6f, 0.78f, 0.78f, 0.80f);
    // زجاج أمامي (لون أزرق فاتح)
    drawCube(0, 0.98f, 0.65f, 1.42f, 0.38f, 0.08f, 0.55f, 0.78f, 0.92f);
    // زجاج خلفي
    drawCube(0, 0.98f, -0.95f, 1.42f, 0.38f, 0.08f, 0.55f, 0.78f, 0.92f);

    // عجلات (4 كبيرة)
    float wy = 0.25f;
    drawCube(-1.0f, wy, 1.1f, 0.28f, 0.55f, 0.55f, 0.12f, 0.12f, 0.12f);
    drawCube(1.0f, wy, 1.1f, 0.28f, 0.55f, 0.55f, 0.12f, 0.12f, 0.12f);
    drawCube(-1.0f, wy, -1.1f, 0.28f, 0.55f, 0.55f, 0.12f, 0.12f, 0.12f);
    drawCube(1.0f, wy, -1.1f, 0.28f, 0.55f, 0.55f, 0.12f, 0.12f, 0.12f);
    // حافة فضية للعجل
    drawCube(-1.03f, wy, 1.1f, 0.08f, 0.38f, 0.38f, 0.75f, 0.75f, 0.8f);
    drawCube(1.03f, wy, 1.1f, 0.08f, 0.38f, 0.38f, 0.75f, 0.75f, 0.8f);
    drawCube(-1.03f, wy, -1.1f, 0.08f, 0.38f, 0.38f, 0.75f, 0.75f, 0.8f);
    drawCube(1.03f, wy, -1.1f, 0.08f, 0.38f, 0.38f, 0.75f, 0.75f, 0.8f);

    // أنوار أمامية صفراء
    drawCube(-0.6f, 0.62f, 1.71f, 0.38f, 0.22f, 0.08f, 1.0f, 1.0f, 0.2f);
    drawCube(0.6f, 0.62f, 1.71f, 0.38f, 0.22f, 0.08f, 1.0f, 1.0f, 0.2f);
    // أنوار خلفية حمراء
    drawCube(-0.6f, 0.62f, -1.72f, 0.38f, 0.22f, 0.08f, 1.0f, 0.1f, 0.1f);
    drawCube(0.6f, 0.62f, -1.72f, 0.38f, 0.22f, 0.08f, 1.0f, 0.1f, 0.1f);
    // شبك أمامي رمادي
    drawCube(0, 0.45f, 1.72f, 1.2f, 0.18f, 0.06f, 0.4f, 0.4f, 0.4f);

    glPopMatrix();
}

// =====================================================
// Particles
// =====================================================
void spawnParticles(float x, float y, float z) {
    for (int i = 0; i < 30; i++) {
        Particle p;
        p.x = x; p.y = y; p.z = z;
        p.vx = ((rand() % 200) - 100) / 80.0f;
        p.vy = ((rand() % 200)) / 60.0f;
        p.vz = ((rand() % 200) - 100) / 80.0f;
        p.life = 1.0f;
        particles.push_back(p);
    }
}

void updateParticles() {
    std::vector<Particle> alive;
    for (auto& p : particles) {
        p.x += p.vx; p.y += p.vy; p.z += p.vz;
        p.vy -= 0.03f;
        p.life -= 0.04f;
        if (p.life > 0) alive.push_back(p);
    }
    particles = alive;
}

void drawParticles() {
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    for (const auto& p : particles) {
        float s = 0.15f * p.life;
        glColor4f(1.0f, 0.5f + p.life * 0.5f, 0.0f, p.life);
        glVertex3f(p.x - s, p.y - s, p.z);
        glVertex3f(p.x + s, p.y - s, p.z);
        glVertex3f(p.x + s, p.y + s, p.z);
        glVertex3f(p.x - s, p.y + s, p.z);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

// =====================================================
// Game Logic
// =====================================================
void updateDifficulty() {
    // كل 5 نقاط = مستوى جديد
    int newLevel = 1 + (score / 5);
    if (newLevel != level) {
        level = newLevel;
        // زيادة السرعة القصوى تدريجياً
        currentMaxSpeed = BASE_MAX_SPEED + (level - 1) * 0.06f;
        if (currentMaxSpeed > TOP_MAX_SPEED)
            currentMaxSpeed = TOP_MAX_SPEED;
        // تقليل مسافة الـ spawn (أكثر كثافة)
        spawnInterval = 22.0f - (level - 1) * 1.2f;
        if (spawnInterval < 12.0f) spawnInterval = 12.0f;
    }
}

void updateGame() {
    if (gameState != PLAYING) return;

    updateDifficulty();
    updateParticles();

    // فيزياء السرعة
    if (keyUp) {
        speed += ACCELERATION;
        if (speed > currentMaxSpeed) speed = currentMaxSpeed;
    }
    else {
        speed -= FRICTION;
        if (speed < 0.0f) speed = 0.0f;
    }

    // تحرك للأمام
    carZ -= speed;

    // تحرك يمين/شمال + إمالة بصرية
    float targetTilt = 0.0f;
    if (keyLeft) { carX -= 0.12f; targetTilt = 5.0f; }
    if (keyRight) { carX += 0.12f; targetTilt = -5.0f; }
    carTilt += (targetTilt - carTilt) * 0.2f;

    // حدود الطريق
    if (carX > 4.3f) carX = 4.3f;
    if (carX < -4.3f) carX = -4.3f;

    // توليد عوائق
    while (carZ < nextSpawnZ) {
        float randX = ((float)(rand() % 760) / 100.0f) - 3.8f;
        int   type = rand() % 3;
        obstacles.push_back({ randX, nextSpawnZ - 15.0f, type, false });
        nextSpawnZ -= spawnInterval;
    }

    // تصادم + نقاط + تنظيف
    std::vector<Obstacle> active;
    for (auto& obs : obstacles) {
        float hitW = (obs.type == 2) ? 1.6f : 0.9f;
        if (std::abs(carX - obs.x) < hitW &&
            std::abs(carZ - obs.z) < 1.4f)
        {
            spawnParticles(carX, 0.6f, carZ);
            gameState = GAME_OVER;
        }

        if (!obs.passed && carZ < obs.z - 2.0f) {
            obs.passed = true;
            score++;
        }

        // احتفظ بالعوائق القريبة فقط
        if (obs.z < carZ + 30.0f) {
            active.push_back(obs);
        }
    }
    obstacles = active;
}

// =====================================================
// HUD
// =====================================================
void drawHUD() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    char buf[128];

    if (gameState == MENU) {
        // خلفية شبه شفافة
        glColor4f(0, 0, 0, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 600); glVertex2f(0, 600);
        glEnd();

        glColor3f(1.0f, 0.85f, 0.0f);
        drawBigText("ARCADE CAR RACE", 240, 370);
        glColor3f(1, 1, 1);
        drawBigText("Press SPACE or UP to Start", 210, 310);
        glColor3f(0.8f, 0.8f, 0.8f);
        drawText("LEFT / RIGHT = Steer    UP = Accelerate    ESC = Quit", 140, 250);
        drawText("Dodge obstacles, earn points, survive longer!", 175, 210);
    }
    else if (gameState == PLAYING) {
        // شريط علوي شفاف
        glColor4f(0, 0, 0, 0.45f);
        glBegin(GL_QUADS);
        glVertex2f(0, 565); glVertex2f(800, 565); glVertex2f(800, 600); glVertex2f(0, 600);
        glEnd();

        float kmh = speed * 200.0f;
        glColor3f(1, 1, 1);
        sprintf(buf, "SCORE: %d", score);
        drawText(buf, 15, 576);

        sprintf(buf, "LEVEL: %d", level);
        drawText(buf, 200, 576);

        sprintf(buf, "SPEED: %.0f km/h", kmh);
        drawText(buf, 360, 576);

        // شريط سرعة
        float ratio = speed / TOP_MAX_SPEED;
        float barW = 180.0f * ratio;
        glColor4f(0, 0, 0, 0.5f);
        glBegin(GL_QUADS); glVertex2f(580, 572); glVertex2f(770, 572); glVertex2f(770, 594); glVertex2f(580, 594); glEnd();
        float rr = ratio, gg = 1.0f - ratio;
        glColor4f(rr, gg, 0.1f, 0.9f);
        glBegin(GL_QUADS); glVertex2f(580, 572); glVertex2f(580 + barW, 572); glVertex2f(580 + barW, 594); glVertex2f(580, 594); glEnd();
        glColor3f(1, 1, 1);
        drawText("SPD", 584, 576);

        // تحذير حدود الطريق
        if (std::abs(carX) > 3.8f) {
            glColor4f(1, 0.1f, 0.1f, 0.35f);
            glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 600); glVertex2f(0, 600); glEnd();
            glColor3f(1, 0.2f, 0.2f);
            drawText("! ROAD EDGE !", 340, 290);
        }
    }
    else if (gameState == GAME_OVER) {
        // تراكب داكن
        glColor4f(0, 0, 0, 0.78f);
        glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(800, 0); glVertex2f(800, 600); glVertex2f(0, 600); glEnd();

        // بطاقة نتيجة
        glColor4f(0.12f, 0.12f, 0.18f, 0.95f);
        glBegin(GL_QUADS); glVertex2f(230, 190); glVertex2f(570, 190); glVertex2f(570, 430); glVertex2f(230, 430); glEnd();
        // إطار برتقالي
        glLineWidth(3.0f);
        glColor3f(1.0f, 0.5f, 0.0f);
        glBegin(GL_LINE_LOOP); glVertex2f(230, 190); glVertex2f(570, 190); glVertex2f(570, 430); glVertex2f(230, 430); glEnd();
        glLineWidth(1.0f);

        glColor3f(1.0f, 0.15f, 0.15f);
        drawBigText("GAME  OVER", 293, 390);

        glColor3f(0.85f, 0.85f, 0.85f);
        drawText("─────────────────────────", 248, 360);

        sprintf(buf, "Score : %d", score);
        glColor3f(1.0f, 0.9f, 0.0f);
        drawBigText(buf, 310, 325);

        sprintf(buf, "Level : %d", level);
        glColor3f(0.6f, 0.9f, 1.0f);
        drawBigText(buf, 310, 285);

        sprintf(buf, "Top Speed: %.0f km/h", currentMaxSpeed * 200.0f);
        glColor3f(0.85f, 0.85f, 0.85f);
        drawText(buf, 295, 248);

        glColor3f(0.85f, 0.85f, 0.85f);
        drawText("─────────────────────────", 248, 228);

        glColor3f(0.3f, 1.0f, 0.4f);
        drawBigText("[ R ]  Restart", 295, 208);
    }

    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// =====================================================
// Input
// =====================================================
void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_UP) {
        if (gameState == MENU) resetGame();
        keyUp = true;
    }
    if (key == GLUT_KEY_LEFT)  keyLeft = true;
    if (key == GLUT_KEY_RIGHT) keyRight = true;
}

void specialUpKeys(int key, int x, int y) {
    if (key == GLUT_KEY_UP)    keyUp = false;
    if (key == GLUT_KEY_LEFT)  keyLeft = false;
    if (key == GLUT_KEY_RIGHT) keyRight = false;
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    if (key == ' ' && gameState == MENU)     resetGame();
    if ((key == 'r' || key == 'R') && gameState == GAME_OVER) resetGame();
}

// =====================================================
// Display
// =====================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // كاميرا تتبع السيارة — تنخفض مع السرعة للإحساس بالتسارع
    float camY = 4.5f + speed * 1.5f;
    float camDist = 9.0f + speed * 2.0f;
    gluLookAt(carX * 0.85f, camY, carZ + camDist,
        carX * 0.5f, 0.6f, carZ - 6.0f,
        0.0f, 1.0f, 0.0f);

    drawMountains();
    drawTrees();
    drawRoad();
    drawGuardrails();
    drawLamps();
    drawObstacles();
    drawCar();
    drawParticles();
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(62.0, (double)w / h, 0.1, 400.0);
    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    updateGame();
    glutPostRedisplay();
}

// =====================================================
// Main
// =====================================================
int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_ALPHA);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Arcade Car Race");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.42f, 0.68f, 0.90f, 1.0f); // سماء

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialUpKeys);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
