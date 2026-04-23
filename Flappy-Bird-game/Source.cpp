#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>

// ============== الثوابت ==============
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 600;
const double GRAVITY = 0.45;
const double FLAP_FORCE = -7.5;
const double PIPE_SPEED = 3.0;
const double PIPE_WIDTH = 60.0;
const double PIPE_GAP = 160.0;
const double BIRD_SIZE = 25.0;
const double GROUND_H = 80.0;
const double SPAWN_X = WINDOW_WIDTH + 50;

// ============== نظام الثيمات ==============
struct Color { float r, g, b; };

struct Theme {
    Color skyTop, skyBottom;
    Color ground, groundStripe, groundPattern;
    Color pipeBody, pipeCap, pipeBorder;
    Color birdBody, birdWing, birdBeak, birdEye;
    Color cloud;
    Color textMain, textShadow;
    std::string name;
};

Theme themes[] = {
    // 0: Classic Day 🌞
    { {0.4f,0.75f,1.0f}, {0.6f,0.85f,1.0f},
      {0.85f,0.65f,0.35f}, {0.3f,0.8f,0.3f}, {0.75f,0.55f,0.25f},
      {0.15f,0.6f,0.15f}, {0.2f,0.7f,0.2f}, {0.1f,0.4f,0.1f},
      {1.0f,0.85f,0.0f}, {0.9f,0.75f,0.0f}, {0.9f,0.4f,0.1f}, {1.0f,1.0f,1.0f},
      {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.3f,0.3f,0.3f}, "Classic Day" },

      // 1: Night 🌙
      { {0.05f,0.1f,0.3f}, {0.1f,0.15f,0.4f},
        {0.2f,0.15f,0.35f}, {0.1f,0.6f,0.2f}, {0.15f,0.1f,0.25f},
        {0.05f,0.25f,0.05f}, {0.1f,0.35f,0.1f}, {0.0f,0.1f,0.0f},
        {0.9f,0.9f,0.2f}, {0.7f,0.7f,0.1f}, {0.8f,0.5f,0.1f}, {0.0f,0.0f,0.0f},
        {1.0f,1.0f,0.6f}, {0.0f,0.0f,0.3f}, {0.2f,0.2f,0.4f}, "Midnight" },

        // 2: Sunset 🌅
        { {0.9f,0.3f,0.2f}, {0.95f,0.6f,0.3f},
          {0.5f,0.35f,0.25f}, {0.8f,0.4f,0.2f}, {0.4f,0.25f,0.15f},
          {0.6f,0.15f,0.2f}, {0.7f,0.2f,0.25f}, {0.4f,0.05f,0.1f},
          {0.95f,0.85f,0.3f}, {0.85f,0.75f,0.2f}, {0.8f,0.3f,0.2f}, {0.0f,0.0f,0.0f},
          {1.0f,0.9f,0.4f}, {0.4f,0.1f,0.1f}, {0.2f,0.05f,0.05f}, "Sunset" },

          // 3: Cyber Neon ⚡
          { {0.0f,0.0f,0.1f}, {0.05f,0.0f,0.2f},
            {0.1f,0.05f,0.2f}, {0.0f,0.9f,0.8f}, {0.08f,0.03f,0.15f},
            {0.05f,0.05f,0.2f}, {0.1f,0.1f,0.4f}, {0.5f,0.0f,1.0f},
            {0.0f,0.9f,0.9f}, {0.0f,0.6f,0.7f}, {1.0f,0.0f,0.5f}, {0.0f,0.0f,0.0f},
            {0.0f,1.0f,1.0f}, {0.5f,0.0f,1.0f}, {0.3f,0.0f,0.5f}, "Cyber Neon" },

            // 4: Arcade Retro 🕹️ (جديد!)
            { {0.1f, 0.0f, 0.3f}, {0.2f, 0.0f, 0.5f},
              {0.15f, 0.05f, 0.25f}, {0.0f, 1.0f, 0.5f}, {0.3f, 0.1f, 0.4f},
              {0.0f, 1.0f, 0.5f}, {0.0f, 1.0f, 0.8f}, {0.0f, 0.5f, 0.3f},
              {1.0f, 0.0f, 0.8f}, {1.0f, 0.0f, 0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
              {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.8f}, "Arcade Retro" }
};

const int THEME_COUNT = 5;  // ✅ تم التحديث إلى 5
int currentThemeIdx = 0;
Theme currentTheme;

void loadTheme(int idx) {
    currentThemeIdx = (idx + THEME_COUNT) % THEME_COUNT;
    currentTheme = themes[currentThemeIdx];
}

// ============== حالة اللعبة ==============
enum GameState { MENU, PLAYING, GAME_OVER };
GameState state = MENU;

// ============== الطائر ==============
struct Bird {
    double x, y, vel;
    double rotation;
    bool alive;

    Bird() { reset(); }

    void reset() {
        x = 100;
        y = WINDOW_HEIGHT / 2.0;
        vel = 0;
        rotation = 0;
        alive = true;
    }

    void flap() {
        if (alive) vel = FLAP_FORCE;
    }

    void update() {
        if (!alive) return;
        vel += GRAVITY;
        y += vel;
        rotation = (vel < 0) ? std::max(-30.0, vel * 3.0) : std::min(70.0, vel * 3.0);

        if (y - BIRD_SIZE / 2.0 <= GROUND_H) {
            y = GROUND_H + BIRD_SIZE / 2.0;
            alive = false;
            state = GAME_OVER;
        }
        if (y + BIRD_SIZE / 2.0 >= WINDOW_HEIGHT) {
            y = WINDOW_HEIGHT - BIRD_SIZE / 2.0;
            alive = false;
            state = GAME_OVER;
        }
    }
};

// ============== الأنابيب ==============
struct Pipe {
    double x, topH, botY;
    bool scored;

    Pipe(double startX) {
        x = startX;
        topH = 80 + rand() % (int)(WINDOW_HEIGHT - GROUND_H - PIPE_GAP - 160);
        botY = topH + PIPE_GAP;
        scored = false;
    }

    void update() {
        x -= PIPE_SPEED;
    }
};

// ============== المتغيرات العامة ==============
Bird bird;
std::vector<Pipe> pipes;
int score = 0, bestScore = 0;
int groundOffset = 0;
double spawnTimer = 0;
const double SPAWN_INTERVAL = 90.0;

// ============== دوال الرسم الأساسية ==============
void drawRect(double x, double y, double w, double h, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2d(x, y);
    glVertex2d(x + w, y);
    glVertex2d(x + w, y + h);
    glVertex2d(x, y + h);
    glEnd();
}

void drawCircle(double cx, double cy, double r, float R, float G, float B) {
    glColor3f(R, G, B);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= 30; i++) {
        double angle = 2.0 * 3.14159265 * i / 30;
        glVertex2d(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

// ============== رسم الخلفية والعناصر ==============
void drawSky() {
    glBegin(GL_QUADS);
    glColor3f(currentTheme.skyTop.r, currentTheme.skyTop.g, currentTheme.skyTop.b);
    glVertex2d(0, GROUND_H);
    glVertex2d(WINDOW_WIDTH, GROUND_H);
    glColor3f(currentTheme.skyBottom.r, currentTheme.skyBottom.g, currentTheme.skyBottom.b);
    glVertex2d(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2d(0, WINDOW_HEIGHT);
    glEnd();
}

void drawCloud(double x, double y) {
    drawCircle(x, y, 20, currentTheme.cloud.r, currentTheme.cloud.g, currentTheme.cloud.b);
    drawCircle(x + 20, y - 5, 25, currentTheme.cloud.r, currentTheme.cloud.g, currentTheme.cloud.b);
    drawCircle(x + 40, y, 20, currentTheme.cloud.r, currentTheme.cloud.g, currentTheme.cloud.b);
    drawCircle(x + 18, y + 5, 18, currentTheme.cloud.r, currentTheme.cloud.g, currentTheme.cloud.b);
}

void drawGround() {
    drawRect(0, 0, WINDOW_WIDTH, GROUND_H,
        currentTheme.ground.r, currentTheme.ground.g, currentTheme.ground.b);
    drawRect(0, GROUND_H - 8, WINDOW_WIDTH, 8,
        currentTheme.groundStripe.r, currentTheme.groundStripe.g, currentTheme.groundStripe.b);

    glColor3f(currentTheme.groundPattern.r, currentTheme.groundPattern.g, currentTheme.groundPattern.b);
    for (int i = -1; i < WINDOW_WIDTH / 30 + 1; i++) {
        double xPos = i * 30 + fmod((double)groundOffset, 30.0);
        drawRect(xPos, 10, 15, GROUND_H - 15,
            currentTheme.groundPattern.r, currentTheme.groundPattern.g, currentTheme.groundPattern.b);
    }
}

void drawPipe(Pipe& p) {
    // جسم الأنبوب العلوي والسفلي
    drawRect(p.x, p.topH, PIPE_WIDTH, WINDOW_HEIGHT - p.topH,
        currentTheme.pipeBody.r, currentTheme.pipeBody.g, currentTheme.pipeBody.b);
    drawRect(p.x, 0, PIPE_WIDTH, p.botY,
        currentTheme.pipeBody.r, currentTheme.pipeBody.g, currentTheme.pipeBody.b);

    // حواف الأنابيب (Caps)
    drawRect(p.x - 4, p.topH, PIPE_WIDTH + 8, 30,
        currentTheme.pipeCap.r, currentTheme.pipeCap.g, currentTheme.pipeCap.b);
    drawRect(p.x - 4, p.botY - 30, PIPE_WIDTH + 8, 30,
        currentTheme.pipeCap.r, currentTheme.pipeCap.g, currentTheme.pipeCap.b);

    // حدود الأنابيب
    glLineWidth(2.0);
    glColor3f(currentTheme.pipeBorder.r, currentTheme.pipeBorder.g, currentTheme.pipeBorder.b);

    glBegin(GL_LINE_LOOP);
    glVertex2d(p.x - 4, p.topH);
    glVertex2d(p.x + PIPE_WIDTH + 4, p.topH);
    glVertex2d(p.x + PIPE_WIDTH + 4, p.topH + 30);
    glVertex2d(p.x - 4, p.topH + 30);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2d(p.x - 4, p.botY - 30);
    glVertex2d(p.x + PIPE_WIDTH + 4, p.botY - 30);
    glVertex2d(p.x + PIPE_WIDTH + 4, p.botY);
    glVertex2d(p.x - 4, p.botY);
    glEnd();
}

void drawBird() {
    glPushMatrix();
    glTranslatef(bird.x, bird.y, 0);
    glRotatef((float)bird.rotation, 0, 0, 1);

    // جسم الطائر
    drawCircle(0, 0, BIRD_SIZE / 2.0,
        currentTheme.birdBody.r, currentTheme.birdBody.g, currentTheme.birdBody.b);

    // الجناح
    drawCircle(-5, -3, 8,
        currentTheme.birdWing.r, currentTheme.birdWing.g, currentTheme.birdWing.b);

    // العين (بياض + بؤبؤ)
    drawCircle(5, 5, 5,
        currentTheme.birdEye.r, currentTheme.birdEye.g, currentTheme.birdEye.b);
    drawCircle(6, 5, 2.5, 0.0f, 0.0f, 0.0f);

    // المنقار
    glColor3f(currentTheme.birdBeak.r, currentTheme.birdBeak.g, currentTheme.birdBeak.b);
    glBegin(GL_TRIANGLES);
    glVertex2d(10, 0);
    glVertex2d(20, 3);
    glVertex2d(10, 6);
    glEnd();

    glPopMatrix();
}

// ============== دوال النصوص ==============
void drawText(const char* text, double x, double y, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2d(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawTextLarge(const char* text, double x, double y, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2d(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }
}

void drawBackground() {
    drawSky();
    // غيوم/نجوم في الخلفية
    drawCloud(50, 450);
    drawCloud(200, 380);
    drawCloud(320, 500);
    drawCloud(150, 520);
}

// ============== كشف التصادم ==============
bool checkCollision(Bird& b, Pipe& p) {
    double bx = b.x, by = b.y;
    double br = BIRD_SIZE / 2.0 - 3;  // هامش أمان

    // التحقق من النطاق الأفقي
    if (bx + br > p.x && bx - br < p.x + PIPE_WIDTH) {
        // التحقق من النطاق العمودي (اصطدام بالأنبوب العلوي أو السفلي)
        if (by + br > p.topH || by - br < p.botY) {
            return true;
        }
    }
    return false;
}

// ============== إعادة تعيين اللعبة ==============
void resetGame() {
    bird.reset();
    pipes.clear();
    score = 0;
    spawnTimer = 0;
    groundOffset = 0;
}

// ============== حلقة التحديث ==============
void update(int) {
    if (state == PLAYING) {
        bird.update();

        // تحديث الأنابيب والتحقق من التصادم
        for (size_t i = 0; i < pipes.size(); i++) {
            pipes[i].update();

            if (bird.alive && checkCollision(bird, pipes[i])) {
                bird.alive = false;
                state = GAME_OVER;
                if (score > bestScore) bestScore = score;
            }

            // زيادة النقاط عند تجاوز الأنبوب
            if (!pipes[i].scored && pipes[i].x + PIPE_WIDTH < bird.x) {
                pipes[i].scored = true;
                score++;
            }
        }

        // إزالة الأنابيب التي خرجت من الشاشة
        if (!pipes.empty() && pipes[0].x + PIPE_WIDTH < -10) {
            pipes.erase(pipes.begin());
        }

        // إنشاء أنابيب جديدة
        spawnTimer++;
        if (spawnTimer >= SPAWN_INTERVAL) {
            pipes.push_back(Pipe(SPAWN_X));
            spawnTimer = 0;
        }

        // تحريك نمط الأرض
        groundOffset -= (int)PIPE_SPEED;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);  // ~60 FPS
}

// ============== دالة العرض الرئيسية ==============
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawBackground();

    if (state == MENU) {
        drawGround();

        // عنوان اللعبة
        drawTextLarge("FLAPPY BIRD", WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT - 120,
            currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);

        drawText("Press SPACE to start", WINDOW_WIDTH / 2 - 75, WINDOW_HEIGHT / 2 - 20,
            currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);

        drawText("Press 'T' to change theme", WINDOW_WIDTH / 2 - 95, WINDOW_HEIGHT / 2 + 30,
            currentTheme.textShadow.r, currentTheme.textShadow.g, currentTheme.textShadow.b);

        drawBird();

    }
    else if (state == PLAYING) {
        // رسم الأنابيب
        for (size_t i = 0; i < pipes.size(); i++) {
            drawPipe(pipes[i]);
        }

        drawGround();
        drawBird();

        // عرض النقاط
        std::stringstream ss;
        ss << score;
        drawTextLarge(ss.str().c_str(), WINDOW_WIDTH / 2 - 10, WINDOW_HEIGHT - 50,
            currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);

    }
    else if (state == GAME_OVER) {
        // رسم الأنابيب
        for (size_t i = 0; i < pipes.size(); i++) {
            drawPipe(pipes[i]);
        }

        drawGround();
        drawBird();

        // صندوق Game Over
        drawRect(WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 80, 240, 160,
            currentTheme.ground.r * 0.8, currentTheme.ground.g * 0.8, currentTheme.ground.b * 0.8);

        glLineWidth(2.0);
        glColor3f(currentTheme.textShadow.r, currentTheme.textShadow.g, currentTheme.textShadow.b);
        glBegin(GL_LINE_LOOP);
        glVertex2d(WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 80);
        glVertex2d(WINDOW_WIDTH / 2 + 120, WINDOW_HEIGHT / 2 - 80);
        glVertex2d(WINDOW_WIDTH / 2 + 120, WINDOW_HEIGHT / 2 + 80);
        glVertex2d(WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 + 80);
        glEnd();

        drawTextLarge("Game Over", WINDOW_WIDTH / 2 - 70, WINDOW_HEIGHT / 2 + 50,
            currentTheme.birdBeak.r, currentTheme.birdBeak.g, currentTheme.birdBeak.b);

        std::stringstream s1, s2;
        s1 << "Score: " << score;
        s2 << "Best:  " << bestScore;

        drawText(s1.str().c_str(), WINDOW_WIDTH / 2 - 45, WINDOW_HEIGHT / 2 + 10,
            currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);
        drawText(s2.str().c_str(), WINDOW_WIDTH / 2 - 45, WINDOW_HEIGHT / 2 - 20,
            currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);

        drawText("Press SPACE to restart", WINDOW_WIDTH / 2 - 85, WINDOW_HEIGHT / 2 - 55,
            currentTheme.textShadow.r, currentTheme.textShadow.g, currentTheme.textShadow.b);
    }

    // عرض اسم الثيم الحالي في الزاوية
    drawText(currentTheme.name.c_str(), 10, WINDOW_HEIGHT - 20,
        currentTheme.textMain.r, currentTheme.textMain.g, currentTheme.textMain.b);

    glutSwapBuffers();
}

// ============== معالجة لوحة المفاتيح ==============
void keyboard(unsigned char key, int, int) {
    if (key == ' ') {  // مسافة
        if (state == MENU) {
            state = PLAYING;
            resetGame();
            bird.flap();
        }
        else if (state == PLAYING) {
            bird.flap();
        }
        else if (state == GAME_OVER) {
            resetGame();
            state = MENU;
        }
    }
    if (key == 't' || key == 'T') {
        loadTheme(currentThemeIdx + 1);
    }
    if (key == 27) {  // Escape للخروج
        exit(0);
    }
}

// ============== معالجة الماوس ==============
void mouse(int button, int pressState, int, int) {
    if (button == GLUT_LEFT_BUTTON && pressState == GLUT_DOWN) {
        if (state == PLAYING) {
            bird.flap();
        }
    }
}

// ============== تغيير حجم النافذة ==============
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

// ============== التهيئة الأولية ==============
void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    srand((unsigned int)time(NULL));
    loadTheme(0);  // بدء بالثيم الأول
}

// ============== الدالة الرئيسية ==============
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Flappy Bird - OpenGL (5 Themes)");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}