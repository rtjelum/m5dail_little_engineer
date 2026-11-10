#include "M5Dial.h"
#include <vector>
#include <deque>

// ── PCA9685 servo driver on Port A (Wire: SDA=G13, SCL=G15) ─────────────
constexpr uint8_t  PCA_ADDR     = 0x40;      // default address, A0-A5 open
constexpr uint32_t PCA_I2C_FREQ = 400000;
constexpr int      SERVO_MIN_US = 500;       // pulse at 0 deg
constexpr int      SERVO_MAX_US = 2500;      // pulse at 180 deg
constexpr int      NUM_SERVOS   = 3;         // servo N = PCA9685 channel N-1
bool pcaOk = false;
constexpr int SERVO_DEFAULT_DEG[NUM_SERVOS] = {125, 90, 50};   // power-on / "reset" angle per servo
int  servoPos[NUM_SERVOS];                   // dial position 0..100 per servo (set from defaults in setup)
int  activeServo = 0;

#include <Wire.h>
// Port A uses I2C port 0 (Wire). Port 1 belongs to M5Unified In_I2C (RFID, RTC).
static void pcaWrite(uint8_t reg, const uint8_t* d, size_t n) { Wire.beginTransmission(PCA_ADDR); Wire.write(reg); Wire.write(d, n); uint8_t e = Wire.endTransmission(); if (e) Serial.printf("[pca9685] write reg %02x failed: %d\n", reg, e); }
static void pcaWrite8(uint8_t reg, uint8_t v) { pcaWrite(reg, &v, 1); }
static uint8_t pcaRead8(uint8_t reg) { Wire.beginTransmission(PCA_ADDR); Wire.write(reg); if (Wire.endTransmission() != 0) return 0xEE; Wire.requestFrom((int)PCA_ADDR, 1); return Wire.available() ? Wire.read() : 0xEE; }

int pcaFreq = 50;   // servo PWM frequency in Hz ("pwm <Hz>" serial command). Higher = finer pulse steps.
// PRESCALE can only be written while the chip sleeps. Internal oscillator ~25 MHz.
static void pcaWriteFreq(int hz) {
    int pre = constrain((int)lroundf(25000000.0f / (4096.0f * hz)) - 1, 3, 255);
    pcaWrite8(0x00, 0x10);  // MODE1: sleep
    pcaWrite8(0xFE, pre);   // PRESCALE
    pcaWrite8(0x00, 0x20);  // wake, auto-increment
    delay(1);
    pcaWrite8(0x00, 0xA0);  // restart
}

bool pcaBegin() {
    // Accept either wire order on Port A (SDA/SCL swapped works too).
    const int order[2][2] = {{13, 15}, {15, 13}};
    bool found = false;
    for (int k = 0; k < 2 && !found; k++) {
        Wire.end(); Wire.begin(order[k][0], order[k][1], PCA_I2C_FREQ);
        Wire.beginTransmission(PCA_ADDR); found = (Wire.endTransmission() == 0);
        if (found) Serial.printf("[pca9685] SDA=G%d SCL=G%d\n", order[k][0], order[k][1]);
    }
    if (!found) return false;
    pcaWriteFreq(pcaFreq);  // 50 Hz -> prescale 121
    pcaWrite8(0x01, 0x04);  // MODE2: totem-pole
    return true;
}

// Current angle, target angle and speed (deg/s, 0 = jump) per servo; servoUpdate() moves toward the target.
float servoCur[NUM_SERVOS], servoTarget[NUM_SERVOS], servoSpeed[NUM_SERVOS];
unsigned long servoLastUpdate = 0;
bool armsThinking = false;   // "arms thinking [deg/s]" serial command: arm typing swing on/off
float armsSpeed = 100;       // arm move speed in deg/s
float armsPauseMin = 0.1f, armsPauseMax = 1.0f;   // random pause range in seconds between arm moves
bool  armsCancel = false;    // set by "reset": drop any arm motion in progress
enum ArmsPose : uint8_t { POSE_NONE, POSE_DONE, POSE_QUESTION };
ArmsPose armsPose = POSE_NONE;   // "arms done|question" (or "done" / "question"): a pose that replaces typing (see DONE_*, QUESTION_*)

static void servoWriteDeg(int ch, float deg) {
    if (!pcaOk) return;
    deg = constrain(deg, 0.0f, 180.0f);
    float us = SERVO_MIN_US + deg * (SERVO_MAX_US - SERVO_MIN_US) / 180.0f;
    uint16_t off = (uint16_t)min(4095L, lroundf(us * 4096 * pcaFreq / 1e6f));   // fraction of the PWM period
    uint8_t d[4] = {0, 0, (uint8_t)(off & 0xFF), (uint8_t)(off >> 8)};
    pcaWrite(0x06 + 4 * ch, d, 4);
}

// Move servo `ch` to `deg` at `speed` deg/s (0 = immediately).
void servoMoveTo(int ch, float deg, float speed) {
    servoTarget[ch] = constrain(deg, 0.0f, 180.0f);
    servoSpeed[ch]  = speed;
    if (speed <= 0) { servoCur[ch] = servoTarget[ch]; servoWriteDeg(ch, servoCur[ch]); }
}

// Call every loop: steps moving servos toward their targets (every 20 ms = one servo frame).
void servoUpdate() {
    unsigned long now = millis();
    if (now - servoLastUpdate < 20) return;
    float dt = (now - servoLastUpdate) / 1000.0f;
    servoLastUpdate = now;
    if (dt > 0.1f) dt = 0.1f;
    for (int ch = 0; ch < NUM_SERVOS; ch++) {
        float diff = servoTarget[ch] - servoCur[ch];
        if (diff == 0) continue;
        float step = servoSpeed[ch] * dt;
        servoCur[ch] = fabsf(diff) <= step ? servoTarget[ch] : servoCur[ch] + (diff > 0 ? step : -step);
        servoWriteDeg(ch, servoCur[ch]);
    }
}

// Change the PWM frequency and re-send every servo's current angle at the new rate.
void pcaSetFreq(int hz) {
    pcaFreq = hz;
    if (!pcaOk) return;
    pcaWriteFreq(hz);
    for (int ch = 0; ch < NUM_SERVOS; ch++) servoWriteDeg(ch, servoCur[ch]);
}

void servoWriteAngle(int ch, int deg) {   // immediate move (dial, reset, startup)
    servoMoveTo(ch, deg, 0);
}

inline int posToDeg(int pos) { return pos * 180 / 100; }

// Move all servos to their default angles (exact degrees; the dial position is rounded for the gauge).
void servosToDefault() {
    for (int i = 0; i < NUM_SERVOS; i++) {
        servoPos[i] = (SERVO_DEFAULT_DEG[i] * 100 + 90) / 180;
        servoWriteAngle(i, SERVO_DEFAULT_DEG[i]);
    }
}

// Screen rotation: 0 = normal, 1 = 90° clockwise, 2 = 180°, 3 = 270° (also settable with "rotate <0-3>")
constexpr uint8_t DISPLAY_ROTATION = 0;

const char* BUILD_DATE = __DATE__;

constexpr uint32_t fnv1a(const char* s, uint32_t h = 2166136261u) {
    return *s ? fnv1a(s + 1, (h ^ (uint8_t)*s) * 16777619u) : h;
}
constexpr uint32_t BUILD_HASH = fnv1a(__DATE__ " " __TIME__);

SemaphoreHandle_t dataMutex;
long currentEncoderPos = 0;
volatile bool pendingEncoderReset = false;
volatile long encoderResetValue = 0;
String last_rfid = "";
QueueHandle_t msgQueue;
QueueHandle_t encoderQueue;
String uid = "";
String old_uid = "";

static M5Canvas canvas(&M5Dial.Display);

enum class GState : uint8_t { IDLE, MAZE_PLAYING, MAZE_WIN, FACE };
GState gState = GState::IDLE;

void drawMessage(const String& msg);
void drawRFID(const String& uid);
void drawEncoderGauge(long val);
void drawMaze();
void drawMazeWin();
void mazeExitToIdle();
void mazeButtonPress();
void mazeUpdateDial();
void selectServo(int idx);

void encoderTask(void* pvParameters) {
  M5Dial.Encoder.readAndReset();
  long lastHw = 0, sw = 0;
  for (;;) {
    if (pendingEncoderReset) { sw = encoderResetValue; encoderResetValue = 0; pendingEncoderReset = false; M5Dial.Encoder.readAndReset(); lastHw = 0; }
    long hw = M5Dial.Encoder.read(), delta = hw - lastHw; lastHw = hw;
    long newSw = sw + delta;
    if (gState == GState::IDLE) { if (newSw < 0) newSw = 0; if (newSw > 100) newSw = 100; }
    if (newSw != sw) { sw = newSw; xSemaphoreTake(dataMutex, portMAX_DELAY); currentEncoderPos = sw; xSemaphoreGive(dataMutex); xQueueSend(encoderQueue, &sw, 0); }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void drawMessage(const String& msg) {
    const int cx = 120, cardX = 26, cardY = 46, cardW = 188, cardH = 148, cardR = 16;
    canvas.fillScreen(canvas.color565(8, 8, 24));
    canvas.fillRoundRect(cardX, cardY, cardW, cardH, cardR, canvas.color565(28, 20, 72));
    canvas.drawRoundRect(cardX, cardY, cardW, cardH, cardR, canvas.color565(108, 92, 255));
    canvas.setTextDatum(top_center); canvas.setTextFont(&fonts::DejaVu12); canvas.setTextColor(canvas.color565(120, 100, 230));
    canvas.drawString("MESSAGE", cx, cardY + 8); canvas.setTextFont(&fonts::DejaVu18); canvas.setTextColor(TFT_WHITE);
    String lines[6]; int numLines = 0, i = 0;
    while (i <= (int)msg.length() && numLines < 6) {
        int sp = msg.indexOf(' ', i); if (sp < 0) sp = msg.length();
        String word = msg.substring(i, sp), test = lines[numLines].length() ? lines[numLines] + " " + word : word;
        if (canvas.textWidth(test.c_str()) <= cardW - 24) lines[numLines] = test; else if(numLines<5) lines[++numLines] = word;
        i = sp + 1;
    }
    numLines++; int startY = cardY + (cardH - numLines * 24) / 2 + 10;
    for (int l = 0; l < numLines; l++) canvas.drawString(lines[l], cx, startY + l * 24);
    canvas.pushSprite(0, 0);
}

void drawRFID(const String& uid) {
    int cx = 120, cy = 120; canvas.fillScreen(canvas.color565(8, 8, 24));
    canvas.fillRoundRect(18, 52, 204, 136, 18, canvas.color565(8, 38, 26));
    canvas.drawRoundRect(18, 52, 204, 136, 18, canvas.color565(52, 210, 130));
    canvas.setTextDatum(top_center); canvas.setTextFont(&fonts::DejaVu12); canvas.setTextColor(canvas.color565(52, 210, 130));
    canvas.drawString("RFID CARD", cx, 107); canvas.setTextDatum(middle_center); canvas.setTextFont(&fonts::DejaVu18); canvas.setTextColor(TFT_WHITE);
    canvas.drawString(uid, cx, cy + 22); canvas.pushSprite(0, 0);
}

void drawEncoderGauge(long val) {
    int cx = 120, cy = 120; if (val < 0) val = 0; if (val > 100) val = 100;
    float fill_deg = val * 3.6f; canvas.fillScreen(canvas.color565(8, 8, 24));
    canvas.fillArc(cx, cy, 105, 88, 0, 360, canvas.color565(30, 28, 70));
    if (fill_deg > 1.0f) canvas.fillArc(cx, cy, 105, 88, 270, 270 + fill_deg, canvas.color565(108, 92, 255));
    canvas.fillRoundRect(cx - 58, cy - 32, 116, 62, 14, canvas.color565(18, 16, 50));
    canvas.setTextDatum(middle_center); canvas.setTextColor(TFT_WHITE); canvas.setTextFont(&fonts::Orbitron_Light_32);
    canvas.drawString(String(posToDeg(val)), cx, cy - 8); canvas.setTextFont(&fonts::DejaVu12); canvas.setTextColor(canvas.color565(110, 110, 190));
    canvas.drawString(pcaOk ? ("SERVO " + String(activeServo + 1)) : "NO PCA9685", cx, cy + 22);
    canvas.setTextColor(canvas.color565(70, 70, 130)); canvas.drawString("tap to switch", cx, cy + 50); canvas.pushSprite(0, 0);
}

// ── Game: Circular Maze ──────────────────────────────────────────────────────
// Ports the Python prototype: variable sector count per ring (12 inner doubling
// to 24 outer), DFS carving with radial-bias avoidance, BFS picks farthest exit,
// dial cycles through options, button commits and auto-routes through corridors
// until the next junction or dead end.

constexpr int   MAZE_N_RINGS         = 6;
constexpr int   MAZE_R_INNER         = 6;
constexpr int   MAZE_R_STEP          = 18;
constexpr int   MAZE_MAX_SECTORS     = 24;
constexpr int   MAZE_TICKS_PER_STEP  = 6;
constexpr unsigned long MAZE_MOVE_INTERVAL_MS = 110;
constexpr uint32_t MAZE_WALL_COL = 0x3A2B;   // 565: slate ~(60,70,90)

int  sectors_per_ring[MAZE_N_RINGS];
bool mazeWallOuter[MAZE_N_RINGS][MAZE_MAX_SECTORS];
bool mazeWallCW   [MAZE_N_RINGS][MAZE_MAX_SECTORS];
int  mazeExitSector  = 0;
int  mazePlayerR     = 0, mazePlayerS     = 0;
int  mazePrevPlayerR = 0, mazePrevPlayerS = 0;
float visualAngle = 0.0f, visualRadius = 0.0f;

enum class MoveDir : int8_t { NONE, CW, CCW, IN, OUT };
struct MoveOption { MoveDir dir; int8_t r, s; };
std::vector<MoveOption> availableOptions;
int  selectedOptionIdx = 0;
bool isMoving          = false;
unsigned long lastMoveTime = 0;
long mazeDialAnchor    = 0;

static int sectorMod(int s, int n) { return (s % n + n) % n; }

static void updateAvailableOptions() {
    availableOptions.clear();
    int r = mazePlayerR, s = mazePlayerS, n_s = sectors_per_ring[r];
    if (!mazeWallCW[r][s])
        availableOptions.push_back({MoveDir::CW, (int8_t)r, (int8_t)sectorMod(s + 1, n_s)});
    if (!mazeWallCW[r][sectorMod(s - 1, n_s)])
        availableOptions.push_back({MoveDir::CCW, (int8_t)r, (int8_t)sectorMod(s - 1, n_s)});
    if (r > 0 && !mazeWallOuter[r-1][s])
        availableOptions.push_back({MoveDir::IN, (int8_t)(r-1), (int8_t)(s / (n_s / sectors_per_ring[r-1]))});
    if (r < MAZE_N_RINGS - 1) {
        int ratio = sectors_per_ring[r+1] / n_s;
        for (int i = 0; i < ratio; i++)
            if (!mazeWallOuter[r][s*ratio + i])
                availableOptions.push_back({MoveDir::OUT, (int8_t)(r+1), (int8_t)(s*ratio + i)});
    } else if (s == mazeExitSector) {
        availableOptions.push_back({MoveDir::OUT, -1, -1});  // win sentinel
    }
}

static void mazeGenerate() {
    // Sector counts: start at 12, double after every 3 rings (per Python)
    int current_s = 12;
    for (int r = 0; r < MAZE_N_RINGS; r++) {
        sectors_per_ring[r] = current_s;
        if ((r + 1) % 3 == 0 && r < MAZE_N_RINGS - 1) current_s *= 2;
    }
    for (int r = 0; r < MAZE_N_RINGS; r++)
        for (int s = 0; s < MAZE_MAX_SECTORS; s++) {
            mazeWallOuter[r][s] = true;
            mazeWallCW[r][s]    = true;
        }

    // DFS carve. Track lastType to avoid back-to-back radial jumps —
    // this is what makes solution paths longer and more winding.
    struct Cell { int8_t r, s; MoveDir lastType; };
    std::vector<Cell> stack;
    std::vector<std::vector<bool>> visited;
    for (int r = 0; r < MAZE_N_RINGS; r++)
        visited.push_back(std::vector<bool>(sectors_per_ring[r], false));

    stack.push_back({0, 0, MoveDir::NONE});
    visited[0][0] = true;

    while (!stack.empty()) {
        Cell curr = stack.back();
        int r = curr.r, s = curr.s, n_s = sectors_per_ring[r];
        std::vector<Cell> nbs;
        int sCW  = sectorMod(s + 1, n_s);
        int sCCW = sectorMod(s - 1, n_s);
        if (!visited[r][sCW])  nbs.push_back({(int8_t)r, (int8_t)sCW,  MoveDir::CW});
        if (!visited[r][sCCW]) nbs.push_back({(int8_t)r, (int8_t)sCCW, MoveDir::CCW});
        if (curr.lastType != MoveDir::OUT && curr.lastType != MoveDir::IN) {
            if (r > 0) {
                int sI = s / (n_s / sectors_per_ring[r-1]);
                if (!visited[r-1][sI])
                    nbs.push_back({(int8_t)(r-1), (int8_t)sI, MoveDir::IN});
            }
            if (r < MAZE_N_RINGS - 1) {
                int ratio = sectors_per_ring[r+1] / n_s;
                for (int i = 0; i < ratio; i++)
                    if (!visited[r+1][s*ratio + i])
                        nbs.push_back({(int8_t)(r+1), (int8_t)(s*ratio + i), MoveDir::OUT});
            }
        }
        if (!nbs.empty()) {
            Cell nxt = nbs[random(nbs.size())];
            if (nxt.r == r) {
                if (nxt.s == sCW) mazeWallCW[r][s] = false;
                else              mazeWallCW[r][nxt.s] = false;
            } else if (nxt.r > r) {
                mazeWallOuter[r][nxt.s] = false;
            } else {
                mazeWallOuter[nxt.r][s] = false;
            }
            visited[nxt.r][nxt.s] = true;
            stack.push_back(nxt);
        } else {
            stack.pop_back();
        }
    }

    // BFS from (0,0) → pick farthest outer-ring cell as exit
    std::vector<std::vector<int>> dists;
    for (int r = 0; r < MAZE_N_RINGS; r++)
        dists.push_back(std::vector<int>(sectors_per_ring[r], -1));
    std::deque<std::pair<int8_t,int8_t>> q;
    q.push_back({0, 0}); dists[0][0] = 0;
    while (!q.empty()) {
        auto c = q.front(); q.pop_front();
        int r = c.first, s = c.second, d = dists[r][s], n_s = sectors_per_ring[r];
        int sCW = sectorMod(s+1, n_s), sCCW = sectorMod(s-1, n_s);
        if (!mazeWallCW[r][s]    && dists[r][sCW]  < 0) { dists[r][sCW]  = d+1; q.push_back({(int8_t)r, (int8_t)sCW}); }
        if (!mazeWallCW[r][sCCW] && dists[r][sCCW] < 0) { dists[r][sCCW] = d+1; q.push_back({(int8_t)r, (int8_t)sCCW}); }
        if (r < MAZE_N_RINGS - 1) {
            int rt = sectors_per_ring[r+1] / n_s;
            for (int i = 0; i < rt; i++)
                if (!mazeWallOuter[r][s*rt+i] && dists[r+1][s*rt+i] < 0) {
                    dists[r+1][s*rt+i] = d+1;
                    q.push_back({(int8_t)(r+1), (int8_t)(s*rt+i)});
                }
        }
        if (r > 0) {
            int sI = s / (n_s / sectors_per_ring[r-1]);
            if (!mazeWallOuter[r-1][s] && dists[r-1][sI] < 0) {
                dists[r-1][sI] = d+1;
                q.push_back({(int8_t)(r-1), (int8_t)sI});
            }
        }
    }
    int bestS = 0, maxD = 0;
    int outerS = sectors_per_ring[MAZE_N_RINGS - 1];
    for (int s = 0; s < outerS; s++)
        if (dists[MAZE_N_RINGS - 1][s] > maxD) { maxD = dists[MAZE_N_RINGS - 1][s]; bestS = s; }
    mazeExitSector = bestS;

    mazePlayerR = mazePrevPlayerR = 0;
    mazePlayerS = mazePrevPlayerS = 0;
    visualRadius = MAZE_R_INNER + MAZE_R_STEP / 2.0f;
    visualAngle  = 0.5f * (360.0f / sectors_per_ring[0]);
    isMoving = false;
    selectedOptionIdx = 0;

    // Reset encoder + anchor so dial starts neutral
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    currentEncoderPos = 0;
    xSemaphoreGive(dataMutex);
    mazeDialAnchor = 0;
    pendingEncoderReset = true;
    long ev; while (xQueueReceive(encoderQueue, &ev, 0) == pdTRUE) {}

    Serial.printf("[maze] generated, exit sector %d, solution %d cells\n", mazeExitSector, maxD);
}

void mazeStep() {
    if (!isMoving) return;
    if (millis() - lastMoveTime < MAZE_MOVE_INTERVAL_MS) return;
    lastMoveTime = millis();

    if (availableOptions.empty()) { isMoving = false; return; }
    MoveOption opt = availableOptions[selectedOptionIdx % availableOptions.size()];
    if (opt.r == -1) {
        // Win sentinel
        gState = GState::MAZE_WIN;
        isMoving = false;
        M5Dial.Speaker.tone(2000, 400);
        return;
    }

    // Re-check that the chosen option is still valid from the current cell
    bool stillOpen = false;
    for (auto& a : availableOptions)
        if (a.r == opt.r && a.s == opt.s) { stillOpen = true; break; }
    if (!stillOpen) { isMoving = false; return; }

    mazePrevPlayerR = mazePlayerR;
    mazePrevPlayerS = mazePlayerS;
    mazePlayerR = opt.r;
    mazePlayerS = opt.s;
    M5Dial.Speaker.tone(2500, 8);

    updateAvailableOptions();
    int total = availableOptions.size();

    // Stop conditions: radial jump (ring changed), junction (>2 opts), or dead end (1 opt)
    bool ringChanged = (mazePlayerR != mazePrevPlayerR);
    if (ringChanged || total != 2) {
        isMoving = false;
        // Re-anchor dial so the new option list starts at "no rotation"
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        mazeDialAnchor = currentEncoderPos;
        xSemaphoreGive(dataMutex);
        // Default selection: the option AFTER "back" (per Python behaviour)
        if (total > 1) {
            for (int i = 0; i < total; i++)
                if (availableOptions[i].r == mazePrevPlayerR && availableOptions[i].s == mazePrevPlayerS) {
                    selectedOptionIdx = (i + 1) % total;
                    break;
                }
        } else {
            selectedOptionIdx = 0;
        }
    } else {
        // Corridor — follow forward (the non-back option)
        for (int i = 0; i < total; i++)
            if (availableOptions[i].r != mazePrevPlayerR || availableOptions[i].s != mazePrevPlayerS) {
                selectedOptionIdx = i;
                break;
            }
    }
}

void drawMaze() {
    const int cx = 120, cy = 120;
    canvas.fillScreen(canvas.color565(245, 244, 240));   // warm off-white

    // Animate dot toward target cell midpoint (per-frame smoothing)
    float tA = (mazePlayerS + 0.5f) * (360.0f / sectors_per_ring[mazePlayerR]);
    float tR = MAZE_R_INNER + mazePlayerR * MAZE_R_STEP + MAZE_R_STEP / 2.0f;
    if (gState == GState::MAZE_WIN) tR = MAZE_R_INNER + MAZE_N_RINGS * MAZE_R_STEP + 8.0f;
    if      (visualAngle - tA > 180) visualAngle -= 360;
    else if (tA - visualAngle > 180) visualAngle += 360;
    visualAngle  += (tA - visualAngle)  * 0.30f;
    visualRadius += (tR - visualRadius) * 0.30f;

    const uint32_t wallCol = MAZE_WALL_COL;

    // Radial walls (between sectors of the same ring) — thicker via 3 parallel passes
    for (int r = 0; r < MAZE_N_RINGS; r++) {
        int n_s = sectors_per_ring[r];
        float sw = 360.0f / n_s;
        int r_o = MAZE_R_INNER + (r + 1) * MAZE_R_STEP;
        int r_i = MAZE_R_INNER + r * MAZE_R_STEP;
        for (int s = 0; s < n_s; s++) {
            if (!mazeWallCW[r][s]) continue;
            float a = (s + 1) * sw * DEG_TO_RAD;
            float dx = cosf(a - HALF_PI), dy = sinf(a - HALF_PI);
            float pdx = -dy, pdy = dx;
            for (int off = -1; off <= 1; off++) {
                canvas.drawLine(
                    (int)(cx + dx*r_i + pdx*off), (int)(cy + dy*r_i + pdy*off),
                    (int)(cx + dx*r_o + pdx*off), (int)(cy + dy*r_o + pdy*off),
                    wallCol);
            }
        }
    }

    // Circumferential walls — drawn against outer-cell indexing
    for (int r = 0; r < MAZE_N_RINGS; r++) {
        int r_o = MAZE_R_INNER + (r + 1) * MAZE_R_STEP;
        if (r < MAZE_N_RINGS - 1) {
            int n_n = sectors_per_ring[r+1];
            float sw_n = 360.0f / n_n;
            for (int sn = 0; sn < n_n; sn++)
                if (mazeWallOuter[r][sn])
                    canvas.fillArc(cx, cy, r_o + 2, r_o - 1,
                                   sn * sw_n + 270.0f, (sn + 1) * sw_n + 270.0f, wallCol);
        } else {
            int n_s = sectors_per_ring[r];
            float sw_o = 360.0f / n_s;
            for (int s = 0; s < n_s; s++)
                if (s != mazeExitSector)
                    canvas.fillArc(cx, cy, r_o + 2, r_o - 1,
                                   s * sw_o + 270.0f, (s + 1) * sw_o + 270.0f, wallCol);
        }
    }

    // Inner boundary
    canvas.drawCircle(cx, cy, MAZE_R_INNER, wallCol);

    // Exit marker — tinted cell + thick outer arc + outward chevron inside the cell
    {
        int   exR     = MAZE_R_INNER + MAZE_N_RINGS * MAZE_R_STEP;
        int   exRin   = MAZE_R_INNER + (MAZE_N_RINGS - 1) * MAZE_R_STEP;
        float exSW    = 360.0f / sectors_per_ring[MAZE_N_RINGS - 1];
        float exA0    = mazeExitSector * exSW;
        float exA1    = (mazeExitSector + 1) * exSW;

        // Tint inside the exit cell so it's distinct from the rest of the ring
        canvas.fillArc(cx, cy, exR - 2, exRin + 1, exA0 + 270.0f, exA1 + 270.0f,
                       canvas.color565(200, 240, 220));   // soft mint

        // Bright emerald band just outside the outer boundary
        canvas.fillArc(cx, cy, exR + 5, exR + 1, exA0 + 270.0f, exA1 + 270.0f,
                       canvas.color565(40, 180, 130));

        // Chevron inside the exit cell, pointing outward
        float exC  = (exA0 + exA1) * 0.5f * DEG_TO_RAD - HALF_PI;
        float edx  = cosf(exC), edy = sinf(exC);
        float perX = -edy,      perY = edx;
        float tipR  = exR - 4;
        float baseR = exRin + 4;
        float baseW = 5.0f;
        canvas.fillTriangle(
            (int)(cx + edx * tipR),                     (int)(cy + edy * tipR),
            (int)(cx + edx * baseR + perX * baseW),     (int)(cy + edy * baseR + perY * baseW),
            (int)(cx + edx * baseR - perX * baseW),     (int)(cy + edy * baseR - perY * baseW),
            canvas.color565(40, 180, 130));
    }

    // Player arrow — points in the direction of the currently-selected option
    float pa = visualAngle * DEG_TO_RAD - HALF_PI;
    int   px = (int)(cx + cosf(pa) * visualRadius);
    int   py = (int)(cy + sinf(pa) * visualRadius);

    // Default direction: outward. Override based on selected option.
    float dx = cosf(pa), dy = sinf(pa);
    if (!availableOptions.empty() && gState != GState::MAZE_WIN) {
        MoveOption opt = availableOptions[selectedOptionIdx % availableOptions.size()];
        if      (opt.dir == MoveDir::CW)  { dx = -sinf(pa); dy =  cosf(pa); }
        else if (opt.dir == MoveDir::CCW) { dx =  sinf(pa); dy = -cosf(pa); }
        else if (opt.dir == MoveDir::IN)  { dx = -cosf(pa); dy = -sinf(pa); }
        // OUT and the win sentinel keep the outward default
    }
    {
        float perpX = -dy, perpY = dx;
        constexpr float tipL  = 7.0f;
        constexpr float baseL = -3.5f;
        constexpr float baseW = 4.5f;
        int tx = (int)(px + dx * tipL),                    ty = (int)(py + dy * tipL);
        int blX = (int)(px + dx * baseL + perpX * baseW),  blY = (int)(py + dy * baseL + perpY * baseW);
        int brX = (int)(px + dx * baseL - perpX * baseW),  brY = (int)(py + dy * baseL - perpY * baseW);
        canvas.fillTriangle(tx, ty, blX, blY, brX, brY, canvas.color565(255, 130, 80));   // coral
        canvas.drawTriangle(tx, ty, blX, blY, brX, brY, canvas.color565(200, 80, 40));
        canvas.fillCircle(tx, ty, 2, canvas.color565(225, 40, 40));                       // ruby tip
    }

    canvas.pushSprite(0, 0);
}

void mazeUpdateDial() {
    // While auto-routing don't poll the dial — let step() drive selection
    if (isMoving) { mazeStep(); drawMaze(); return; }

    long enc;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    enc = currentEncoderPos;
    xSemaphoreGive(dataMutex);

    long delta = enc - mazeDialAnchor;
    int n = (int)availableOptions.size();
    if (n > 1) {
        if (delta >= MAZE_TICKS_PER_STEP) {
            selectedOptionIdx = (selectedOptionIdx - 1 + n) % n;
            mazeDialAnchor += MAZE_TICKS_PER_STEP;
            M5Dial.Speaker.tone(4800, 5);
        } else if (delta <= -MAZE_TICKS_PER_STEP) {
            selectedOptionIdx = (selectedOptionIdx + 1) % n;
            mazeDialAnchor -= MAZE_TICKS_PER_STEP;
            M5Dial.Speaker.tone(4800, 5);
        }
    } else {
        mazeDialAnchor = enc;
    }
    drawMaze();
}

void mazeButtonPress() {
    if (gState == GState::IDLE) {
        gState = GState::MAZE_PLAYING;
        mazeGenerate();
        updateAvailableOptions();
        M5Dial.Speaker.tone(800, 80);
        drawMaze();
    } else if (gState == GState::MAZE_PLAYING) {
        if (!availableOptions.empty()) {
            isMoving = true;
            lastMoveTime = 0;
        }
    } else {
        mazeExitToIdle();
    }
}

void drawMazeWin() {
    const int cx = 120, cy = 120;
    canvas.fillScreen(canvas.color565(245, 244, 240));
    canvas.fillRoundRect(15, 48, 210, 144, 14, canvas.color565(220, 245, 230));
    canvas.drawRoundRect(15, 48, 210, 144, 14, canvas.color565(40, 180, 130));
    canvas.setTextDatum(middle_center);
    canvas.setTextFont(&fonts::DejaVu18);
    canvas.setTextColor(canvas.color565(40, 180, 130));
    canvas.drawString("ESCAPED!", cx, cy - 18);
    canvas.setTextFont(&fonts::DejaVu12);
    canvas.setTextColor(canvas.color565(130, 140, 160));
    canvas.drawString("press to exit", cx, cy + 28);
    canvas.pushSprite(0, 0);
}

void mazeExitToIdle() {
    gState = GState::IDLE;
    old_uid = "";
    selectServo(activeServo);
}

#include "faces.h"
#include "sounds.h"

void showBootInfo() {
    char hashStr[12]; snprintf(hashStr, sizeof(hashStr), "#%08x", BUILD_HASH); int cx = 120, cy = 120;
    M5Dial.Display.clear(); M5Dial.Display.setTextFont(&fonts::DejaVu18); M5Dial.Display.setTextColor(WHITE); M5Dial.Display.drawString("M5 Dial", cx, cy - 22);
    M5Dial.Display.setTextFont(&fonts::DejaVu12); M5Dial.Display.setTextColor(M5Dial.Display.color565(110, 110, 190)); M5Dial.Display.drawString(BUILD_DATE, cx, cy + 2); M5Dial.Display.drawString(hashStr, cx, cy + 18);
    delay(1500); M5Dial.Display.clear();
}

// Make servo `idx` the one the dial controls, syncing the encoder to its position.
void selectServo(int idx) {
    activeServo = idx;
    long pos = servoPos[idx];
    xSemaphoreTake(dataMutex, portMAX_DELAY); currentEncoderPos = pos; xSemaphoreGive(dataMutex);
    encoderResetValue = pos; pendingEncoderReset = true;
    long ev; while (xQueueReceive(encoderQueue, &ev, 0) == pdTRUE) {}
    drawEncoderGauge(pos);
}

// Start the typing arms. args: "[deg/s] [min s] [max s]" (all optional; one pause value = fixed pause).
static void armsThinkingStart(const char* args, const char* tag) {
    float sp = 0, p1 = 0, p2 = 0;
    int got = sscanf(args, "%f %f %f", &sp, &p1, &p2);
    if (got >= 1) armsSpeed = constrain(sp, 1.0f, 500.0f);
    if (got == 2) armsPauseMin = armsPauseMax = constrain(p1, 0.0f, 10.0f);
    if (got >= 3) { armsPauseMin = constrain(min(p1, p2), 0.0f, 10.0f); armsPauseMax = constrain(max(p1, p2), 0.0f, 10.0f); }
    armsThinking = true; armsPose = POSE_NONE;
    Serial.printf("[%s] thinking at %.0f deg/s, pause %.2f-%.2f s\n", tag, armsSpeed, armsPauseMin, armsPauseMax);
}

static String serialBuf = "";
void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read(); if (c == '\r') continue;
        if (c == '\n') {
            serialBuf.trim(); if (serialBuf.length() == 0 || serialBuf[0] == '[') { serialBuf = ""; return; }   // skip blanks and echoed replies
            if (serialBuf.equalsIgnoreCase("status")) { xSemaphoreTake(dataMutex, portMAX_DELAY); long enc = currentEncoderPos; String rfi = last_rfid; xSemaphoreGive(dataMutex); Serial.printf("[status] state=%d enc=%ld rfid=%s\n", (int)gState, enc, rfi.length() ? rfi.c_str() : "(none)"); }
            else if (serialBuf.equalsIgnoreCase("help")) Serial.println("[help] status, reset, servo <1-3> <0-180> [deg/s], arms thinking [deg/s]|done|question|off, thinking [deg/s] [min s] [max s], done, question, sleep, pwm <Hz>, face [bot|eyes] <name|cycle|off>, sound <name|stop|vol N>, rotate <0-3>, <text>");
            else if (serialBuf.equalsIgnoreCase("scan")) { int pins[2][2] = {{13, 15}, {15, 13}}; for (int k = 0; k < 2; k++) { Wire.end(); pinMode(13, INPUT); pinMode(15, INPUT); delay(2); Serial.printf("[scan] idle G13=%d G15=%d\n", digitalRead(13), digitalRead(15)); if (k == 0 && !digitalRead(13)) { pinMode(15, OUTPUT_OPEN_DRAIN); for (int c = 0; c < 18; c++) { digitalWrite(15, c & 1); delayMicroseconds(10); } pinMode(15, INPUT); delay(1); Serial.printf("[scan] after 9 clocks G13=%d G15=%d\n", digitalRead(13), digitalRead(15)); pinMode(13, INPUT_PULLUP); delay(1); Serial.printf("[scan] G13 with pullup=%d\n", digitalRead(13)); pinMode(13, INPUT); } Wire.begin(pins[k][0], pins[k][1], 100000); Serial.printf("[scan] SDA=G%d SCL=G%d:", pins[k][0], pins[k][1]); for (int ad = 1; ad < 127; ad++) { Wire.beginTransmission(ad); if (Wire.endTransmission() == 0) Serial.printf(" 0x%02x", ad); } Serial.println(); } Wire.end(); pcaOk = pcaBegin(); Serial.printf("[scan] pcaOk=%d\n", pcaOk); }
            else if (serialBuf.equalsIgnoreCase("pca")) { Serial.printf("[pca] ok=%d MODE1=%02x MODE2=%02x PRESCALE=%d\n", pcaOk, pcaRead8(0x00), pcaRead8(0x01), pcaRead8(0xFE)); for (int ch = 0; ch < NUM_SERVOS; ch++) { uint8_t r[4]; for (int k = 0; k < 4; k++) r[k] = pcaRead8(0x06 + 4 * ch + k); Serial.printf("[pca] ch%d on=%u off=%u\n", ch, r[0] | (r[1] << 8), r[2] | (r[3] << 8)); } }
            else if (serialBuf.startsWith("servo ")) {   // servo <n> <deg> [deg/s]
                int n = 0, deg = 0; float spd = 0;
                int got = sscanf(serialBuf.c_str() + 6, "%d %d %f", &n, &deg, &spd);
                if (got >= 2 && n >= 1 && n <= NUM_SERVOS) {
                    deg = constrain(deg, 0, 180); if (spd < 0) spd = 0;
                    servoPos[n - 1] = deg * 100 / 180;
                    servoMoveTo(n - 1, deg, spd);
                    if (n - 1 == activeServo && gState == GState::IDLE) { long p = servoPos[n - 1]; xSemaphoreTake(dataMutex, portMAX_DELAY); currentEncoderPos = p; xSemaphoreGive(dataMutex); encoderResetValue = p; pendingEncoderReset = true; drawEncoderGauge(p); }
                    if (spd > 0) Serial.printf("[servo] %d -> %d deg at %.0f deg/s\n", n, deg, spd); else Serial.printf("[servo] %d -> %d deg\n", n, deg);
                } else Serial.printf("[servo] usage: servo <1-%d> <0-180> [deg/s]\n", NUM_SERVOS);
            }
            else if (serialBuf.equalsIgnoreCase("face") || serialBuf.startsWith("face ")) handleFaceCommand(serialBuf.substring(4));
            else if (serialBuf.equalsIgnoreCase("sound") || serialBuf.startsWith("sound ")) handleSoundCommand(serialBuf.substring(5));
            else if (serialBuf.startsWith("rotate")) { int r = serialBuf.substring(6).toInt(); if (serialBuf.length() > 6) M5Dial.Display.setRotation(r & 3); Serial.printf("[rotate] %d\n", M5Dial.Display.getRotation()); }
            else if (serialBuf.startsWith("pwm")) {   // pwm <Hz>: servo PWM frequency
                String a = serialBuf.substring(3); a.trim();
                int hz = a.toInt();
                if (hz >= 40 && hz <= 330) { pcaSetFreq(hz); Serial.printf("[pwm] %d Hz (prescale %d)\n", hz, pcaRead8(0xFE)); }
                else Serial.printf("[pwm] %d Hz now; usage: pwm <40-330>\n", pcaFreq);
            }
            else if (serialBuf.startsWith("arms")) {   // arms thinking [deg/s] | arms off
                String a = serialBuf.substring(4); a.trim();
                String lower = a; lower.toLowerCase();
                if (lower == "thinking" || lower.startsWith("thinking ")) armsThinkingStart(a.c_str() + 8, "arms");
                else if (a.equalsIgnoreCase("done"))     { armsThinking = false; armsPose = POSE_DONE;     Serial.println("[arms] done"); }
                else if (a.equalsIgnoreCase("question")) { armsThinking = false; armsPose = POSE_QUESTION; Serial.println("[arms] question"); }
                else if (a.equalsIgnoreCase("off") || a.equalsIgnoreCase("stop")) { armsThinking = false; armsPose = POSE_NONE; Serial.println("[arms] off"); }
                else Serial.println("[arms] usage: arms thinking [deg/s] [min s] [max s] | arms done | arms question | arms off");
            }
            else if (serialBuf.equalsIgnoreCase("thinking") || serialBuf.startsWith("thinking ")) {   // thinking face + typing arms
                faceCycling = false; faceShow(F_THINKING, faceBot);
                armsThinkingStart(serialBuf.c_str() + 8, "thinking");
            }
            else if (serialBuf.equalsIgnoreCase("sleep")) {   // eyes-style sleeping face, arms back to rest
                armsThinking = false; armsPose = POSE_NONE; faceCycling = false; faceShow(F_SLEEPING, false);
                Serial.println("[sleep] face eyes sleeping, arms off");
            }
            else if (serialBuf.equalsIgnoreCase("done") || serialBuf.equalsIgnoreCase("question")) {   // face + sound + arm pose
                bool done = serialBuf.equalsIgnoreCase("done");
                armsThinking = false; armsPose = done ? POSE_DONE : POSE_QUESTION; faceCycling = false;
                faceShow(done ? F_DONE : F_QUESTION, faceBot); soundPlayNamed(done ? "done" : "question");
                Serial.printf("[%s] face, sound and arms (stop with \"arms off\")\n", done ? "done" : "question");
            }
            else if (serialBuf.equalsIgnoreCase("reset")) { armsThinking = false; armsPose = POSE_NONE; armsCancel = true; servosToDefault(); if (gState == GState::IDLE) selectServo(activeServo); }
            else { char buf[81]; serialBuf.substring(0, 80).toCharArray(buf, sizeof(buf)); xQueueSend(msgQueue, buf, 0); }
            serialBuf = "";
        } else if (serialBuf.length() < 80) serialBuf += c;
    }
}

void setup() {
    Serial.begin(115200); auto cfg = M5.config(); M5Dial.begin(cfg, true, true); M5Dial.Display.setRotation(DISPLAY_ROTATION);
    pcaOk = pcaBegin(); Serial.printf("[pca9685] %s\n", pcaOk ? "found at 0x40" : "NOT found on Port A");
    servosToDefault();
    M5Dial.Display.setTextColor(WHITE); M5Dial.Display.setTextDatum(middle_center); canvas.createSprite(240, 240);
    dataMutex = xSemaphoreCreateMutex(); msgQueue = xQueueCreate(4, 81); encoderQueue = xQueueCreate(8, sizeof(long));
    randomSeed(esp_random()); showBootInfo(); xTaskCreatePinnedToCore(encoderTask, "EncTask", 2048, NULL, 2, NULL, 0); selectServo(0);
    faceShow(F_THINKING, true);   // start on the bot "thinking" face; button press goes to the servo dial
}

// Arm "typing" animation while "arms thinking" is on. Each arm rests at `home` and types a keystroke by
// pressing to `press` and coming back up. One keystroke at a time: a random arm types, then a random pause
// (armsPauseMin..Max). Moves ease in and out (cosine curve) so the servo never starts or stops hard.
// Servo 3 also lifts to 80 deg now and then (see ARM_LIFT). "arms off" lets the current keystroke finish, then both arms ease back to their default angles.
// armsSpeed = average deg/s of a move.
struct ArmSwing { int ch; float home, press; };
const ArmSwing ARM_SWINGS[] = {{0, 135, 145}, {2, 40, 30}};   // {channel, home (key up), press (key down)}
constexpr int NUM_ARM_SWINGS = sizeof(ARM_SWINGS) / sizeof(ARM_SWINGS[0]);
constexpr unsigned long ARM_PRESS_HOLD_MS = 40;   // short hold at the bottom of a keystroke
// Now and then while typing, servo 3 lifts to 80 deg at 30 deg/s, holds a moment, and goes back (other arm keeps typing).
constexpr int   ARM_LIFT = 1;                       // index into ARM_SWINGS (servo 3)
constexpr float ARM_LIFT_DEG = 80, ARM_LIFT_SPEED = 30;
constexpr long  ARM_LIFT_EVERY_MS[2] = {6000, 15000}, ARM_LIFT_HOLD_MS[2] = {500, 1500};
// thenHome: after reaching `to`, hold `holdMs`, then return home at `homeSpeed`.
struct ArmTween { bool on; float from, to; unsigned long start, dur; bool thenHome; unsigned long holdMs; float homeSpeed; };
ArmTween armTw[NUM_ARM_SWINGS];

static void armTweenTo(int i, float to, unsigned long now, float speed, bool thenHome = false, unsigned long holdMs = 0) {
    float from = servoCur[ARM_SWINGS[i].ch];
    armTw[i] = {true, from, to, now, (unsigned long)max(1.0f, fabsf(to - from) / speed * 1000), thenHome, holdMs, speed};
}

// While typing, the head (servo 2) can look around: every 2-10 s it turns to a random angle 85..95 deg at 40 deg/s.
constexpr bool  HEAD_TURNS = false;   // off: the head stays still while typing
constexpr int   HEAD_CH = 1;
constexpr float HEAD_MIN_DEG = 85, HEAD_MAX_DEG = 95, HEAD_SPEED = 40;
constexpr long  HEAD_EVERY_MS[2] = {2000, 10000};
ArmTween headTw;
static void headTweenTo(float to, unsigned long now) {
    float from = servoCur[HEAD_CH];
    headTw = {true, from, to, now, (unsigned long)max(1.0f, fabsf(to - from) / HEAD_SPEED * 1000), false, 0, HEAD_SPEED};
}

// "Job done" pose: servo 1 goes to 120 deg, servo 3 waves between 80 and 90 deg every 2 s, all at 100 deg/s.
constexpr float DONE_ARM1_DEG = 120, DONE_WAVE_DEG[2] = {80, 90}, DONE_SPEED = 100;
constexpr unsigned long DONE_WAVE_MS = 2000;
// "Question" pose: servo 1 moves to 90 deg at 100 deg/s, back to 110 deg at 30 deg/s, pauses 1 s, and repeats.
constexpr float QUESTION_DEG[2] = {90, 110}, QUESTION_SPEED[2] = {100, 30};
constexpr unsigned long QUESTION_PAUSE_MS = 1000;
ArmsPose posePlaying = POSE_NONE;   // pose the arms are in now (lags armsPose by one frame)

static bool armsBusy() { for (auto& t : armTw) if (t.on) return true; return false; }

void armsUpdate(bool active) {
    enum { IDLE, TO_HOME, TYPING, FINISHING } static state = IDLE;
    static unsigned long nextKeyAt = 0, nextLiftAt = 0, nextHeadAt = 0, last = 0;
    unsigned long now = millis();
    static unsigned long nextWaveAt = 0;
    static int waveSide = 0;
    if (armsCancel) { armsCancel = false; for (auto& t : armTw) t.on = false; headTw.on = false; state = IDLE; posePlaying = POSE_NONE; }
    if (!pcaOk || now - last < 20) return;
    last = now;

    // Run the eased moves; a keystroke's press move chains into the return move.
    for (int i = 0; i < NUM_ARM_SWINGS; i++) {
        ArmTween& t = armTw[i];
        if (!t.on) continue;
        float k = min(1.0f, (now - t.start) / (float)t.dur);
        servoMoveTo(ARM_SWINGS[i].ch, t.from + (t.to - t.from) * (0.5f - 0.5f * cosf(PI * k)), 0);
        if (k < 1) continue;
        t.on = false;
        if (t.thenHome && now - (t.start + t.dur) >= t.holdMs) armTweenTo(i, ARM_SWINGS[i].home, now, t.homeSpeed);
        else if (t.thenHome) t.on = true;   // still holding at the bottom
    }

    if (headTw.on) {
        float k = min(1.0f, (now - headTw.start) / (float)headTw.dur);
        servoMoveTo(HEAD_CH, headTw.from + (headTw.to - headTw.from) * (0.5f - 0.5f * cosf(PI * k)), 0);
        if (k >= 1) headTw.on = false;
    }

    // A pose (done / question) overrides typing. When it ends, everything eases back to the default angles.
    if (armsPose != POSE_NONE) {
        if (posePlaying != armsPose) {   // start the pose
            posePlaying = armsPose; state = IDLE; waveSide = 0; nextWaveAt = now + DONE_WAVE_MS;
            headTweenTo(SERVO_DEFAULT_DEG[HEAD_CH], now);
            if (armsPose == POSE_DONE) {
                armTweenTo(0, DONE_ARM1_DEG, now, DONE_SPEED);
                armTweenTo(1, DONE_WAVE_DEG[0], now, DONE_SPEED);
            } else {
                armTweenTo(0, QUESTION_DEG[0], now, QUESTION_SPEED[0]);
                armTweenTo(1, SERVO_DEFAULT_DEG[ARM_SWINGS[1].ch], now, DONE_SPEED);
            }
        } else if (armsPose == POSE_DONE && (long)(now - nextWaveAt) >= 0) {
            waveSide ^= 1;
            armTweenTo(1, DONE_WAVE_DEG[waveSide], now, DONE_SPEED);
            nextWaveAt = now + DONE_WAVE_MS;
        } else if (armsPose == POSE_QUESTION && !armTw[0].on) {   // reached one end: head for the other
            if (waveSide == 1 && nextWaveAt == 0) { nextWaveAt = now + QUESTION_PAUSE_MS; return; }   // back at 110: pause first
            if (waveSide == 1 && (long)(now - nextWaveAt) < 0) return;
            nextWaveAt = 0;
            waveSide ^= 1;
            armTweenTo(0, QUESTION_DEG[waveSide], now, QUESTION_SPEED[waveSide]);
        }
        return;
    }
    if (posePlaying != POSE_NONE) {
        posePlaying = POSE_NONE; state = IDLE;
        for (int i = 0; i < NUM_ARM_SWINGS; i++) armTweenTo(i, SERVO_DEFAULT_DEG[ARM_SWINGS[i].ch], now, DONE_SPEED);
        return;
    }

    switch (state) {
    case IDLE:
        if (active) { for (int i = 0; i < NUM_ARM_SWINGS; i++) armTweenTo(i, ARM_SWINGS[i].home, now, armsSpeed); state = TO_HOME; }
        break;
    case TO_HOME:   // both arms move to their home (key-up) position first
        if (!armsBusy()) { state = active ? TYPING : FINISHING; nextKeyAt = now; nextLiftAt = now + random(ARM_LIFT_EVERY_MS[0], ARM_LIFT_EVERY_MS[1]); nextHeadAt = now + random(HEAD_EVERY_MS[0], HEAD_EVERY_MS[1]); }
        break;
    case TYPING:
        if (!active) { state = FINISHING; break; }
        if (HEAD_TURNS && (long)(now - nextHeadAt) >= 0 && !headTw.on) {
            headTweenTo(random((long)(HEAD_MIN_DEG * 10), (long)(HEAD_MAX_DEG * 10) + 1) / 10.0f, now);
            nextHeadAt = now + headTw.dur + random(HEAD_EVERY_MS[0], HEAD_EVERY_MS[1]);
        }
        if ((long)(now - nextLiftAt) >= 0 && !armTw[ARM_LIFT].on) {
            armTweenTo(ARM_LIFT, ARM_LIFT_DEG, now, ARM_LIFT_SPEED, true, random(ARM_LIFT_HOLD_MS[0], ARM_LIFT_HOLD_MS[1]));
            nextLiftAt = now + 2 * armTw[ARM_LIFT].dur + armTw[ARM_LIFT].holdMs + random(ARM_LIFT_EVERY_MS[0], ARM_LIFT_EVERY_MS[1]);
        }
        if ((long)(now - nextKeyAt) >= 0) {
            int arm = random(NUM_ARM_SWINGS);
            if (armTw[arm].on) arm = (arm + 1) % NUM_ARM_SWINGS;   // that arm is busy (lifted): use the other one
            if (armTw[arm].on) break;                               // both busy: try again next frame
            armTweenTo(arm, ARM_SWINGS[arm].press, now, armsSpeed, true, ARM_PRESS_HOLD_MS);
            unsigned long keyMs = 2 * armTw[arm].dur + ARM_PRESS_HOLD_MS;
            nextKeyAt = now + keyMs + random((long)(armsPauseMin * 1000), (long)(armsPauseMax * 1000) + 1);
        }
        break;
    case FINISHING:   // wait for the keystroke to complete, then ease back to rest
        if (active) { state = TYPING; break; }
        if (!armsBusy() && !headTw.on) {
            for (int i = 0; i < NUM_ARM_SWINGS; i++) armTweenTo(i, SERVO_DEFAULT_DEG[ARM_SWINGS[i].ch], now, armsSpeed);
            headTweenTo(SERVO_DEFAULT_DEG[HEAD_CH], now);
            state = IDLE;
        }
        break;
    }
}

void loop() {
    M5Dial.update(); handleSerial(); soundUpdate(); armsUpdate(armsThinking); servoUpdate(); if (gState == GState::FACE) { if (M5Dial.BtnA.wasHold()) faceExit(); } else if (M5Dial.BtnA.wasPressed()) mazeButtonPress();
    if (gState == GState::IDLE) {
        char buf[81]; if (xQueueReceive(msgQueue, buf, 0) == pdTRUE) { drawMessage(String(buf)); M5Dial.Speaker.tone(2000, 100); }
        long encVal; while (xQueueReceive(encoderQueue, &encVal, 0) == pdTRUE) { servoPos[activeServo] = encVal; servoWriteAngle(activeServo, posToDeg(encVal)); drawEncoderGauge(encVal); }
        if (M5Dial.Touch.getDetail().wasPressed()) { M5Dial.Speaker.tone(3000, 30); selectServo((activeServo + 1) % NUM_SERVOS); }
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
            uid = ""; for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) uid += String(M5Dial.Rfid.uid.uidByte[i], HEX);
            if (old_uid != uid) { old_uid = uid; xSemaphoreTake(dataMutex, portMAX_DELAY); last_rfid = uid; xSemaphoreGive(dataMutex); drawRFID(uid); }
        }
    } else if (gState == GState::MAZE_PLAYING) {
        long encVal; while (xQueueReceive(encoderQueue, &encVal, 0) == pdTRUE) {}
        char buf[81]; while (xQueueReceive(msgQueue, buf, 0) == pdTRUE) {}
        mazeUpdateDial();
        // After win sentinel fired inside step(), state flips to MAZE_WIN
        if (gState == GState::MAZE_WIN) drawMazeWin();
    } else if (gState == GState::FACE) {
        long encVal; while (xQueueReceive(encoderQueue, &encVal, 0) == pdTRUE) {}
        static unsigned long faceMsgUntil = 0;
        char buf[81];
        if (xQueueReceive(msgQueue, buf, 0) == pdTRUE) { drawMessage(String(buf)); faceMsgUntil = millis() + 3000; }   // message shows briefly, face stays active
        if (M5Dial.Touch.getDetail().wasPressed()) { faceCycling = false; faceMsgUntil = 0; faceShow((faceIdx + 1) % FACE_COUNT, faceBot); }
        if ((long)(millis() - faceMsgUntil) >= 0) faceUpdate();
    } else {
        // MAZE_WIN — drain queues, hold the win banner
        char buf[81]; while (xQueueReceive(msgQueue, buf, 0) == pdTRUE) {}
        long encVal; while (xQueueReceive(encoderQueue, &encVal, 0) == pdTRUE) {}
    }
}
