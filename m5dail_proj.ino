#include "M5Dial.h"
#include <vector>
#include <deque>

// ── PCA9685 servo driver on Port A (Wire: SDA=G13, SCL=G15) ─────────────
constexpr uint8_t  PCA_ADDR     = 0x40;      // default address, A0-A5 open
constexpr uint32_t PCA_I2C_FREQ = 400000;
constexpr int      SERVO_MIN_US = 500;       // pulse at 0 deg
constexpr int      SERVO_MAX_US = 2500;      // pulse at 180 deg
constexpr int      NUM_SERVOS   = 2;         // servo 1 = channel 0, servo 2 = channel 1
bool pcaOk = false;
int  servoPos[NUM_SERVOS] = {50, 50};        // dial position 0..100 per servo
int  activeServo = 0;

#include <Wire.h>
// Port A uses I2C port 0 (Wire). Port 1 belongs to M5Unified In_I2C (RFID, RTC).
static void pcaWrite(uint8_t reg, const uint8_t* d, size_t n) { Wire.beginTransmission(PCA_ADDR); Wire.write(reg); Wire.write(d, n); uint8_t e = Wire.endTransmission(); if (e) Serial.printf("[pca9685] write reg %02x failed: %d\n", reg, e); }
static void pcaWrite8(uint8_t reg, uint8_t v) { pcaWrite(reg, &v, 1); }
static uint8_t pcaRead8(uint8_t reg) { Wire.beginTransmission(PCA_ADDR); Wire.write(reg); if (Wire.endTransmission() != 0) return 0xEE; Wire.requestFrom((int)PCA_ADDR, 1); return Wire.available() ? Wire.read() : 0xEE; }

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
    // 50 Hz: prescale = 25MHz / (4096 * 50) - 1 = 121
    pcaWrite8(0x00, 0x10);  // MODE1: sleep
    pcaWrite8(0xFE, 121);   // PRESCALE
    pcaWrite8(0x00, 0x20);  // wake, auto-increment
    delay(1);
    pcaWrite8(0x00, 0xA0);  // restart
    pcaWrite8(0x01, 0x04);  // MODE2: totem-pole
    return true;
}

void servoWriteAngle(int ch, int deg) {
    if (!pcaOk) return;
    deg = constrain(deg, 0, 180);
    int us = map(deg, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
    uint16_t off = (uint32_t)us * 4096 / 20000;               // 20 ms period
    uint8_t d[4] = {0, 0, (uint8_t)(off & 0xFF), (uint8_t)(off >> 8)};
    pcaWrite(0x06 + 4 * ch, d, 4);
}

inline int posToDeg(int pos) { return pos * 180 / 100; }

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

enum class GState : uint8_t { IDLE, MAZE_PLAYING, MAZE_WIN };
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

static String serialBuf = "";
void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read(); if (c == '\r') continue;
        if (c == '\n') {
            serialBuf.trim(); if (serialBuf.length() == 0) { serialBuf = ""; return; }
            if (serialBuf.equalsIgnoreCase("status")) { xSemaphoreTake(dataMutex, portMAX_DELAY); long enc = currentEncoderPos; String rfi = last_rfid; xSemaphoreGive(dataMutex); Serial.printf("[status] state=%d enc=%ld rfid=%s\n", (int)gState, enc, rfi.length() ? rfi.c_str() : "(none)"); }
            else if (serialBuf.equalsIgnoreCase("help")) Serial.println("[help] status, reset, servo <1|2> <0-180>, <text>");
            else if (serialBuf.equalsIgnoreCase("scan")) { int pins[2][2] = {{13, 15}, {15, 13}}; for (int k = 0; k < 2; k++) { Wire.end(); pinMode(13, INPUT); pinMode(15, INPUT); delay(2); Serial.printf("[scan] idle G13=%d G15=%d\n", digitalRead(13), digitalRead(15)); if (k == 0 && !digitalRead(13)) { pinMode(15, OUTPUT_OPEN_DRAIN); for (int c = 0; c < 18; c++) { digitalWrite(15, c & 1); delayMicroseconds(10); } pinMode(15, INPUT); delay(1); Serial.printf("[scan] after 9 clocks G13=%d G15=%d\n", digitalRead(13), digitalRead(15)); pinMode(13, INPUT_PULLUP); delay(1); Serial.printf("[scan] G13 with pullup=%d\n", digitalRead(13)); pinMode(13, INPUT); } Wire.begin(pins[k][0], pins[k][1], 100000); Serial.printf("[scan] SDA=G%d SCL=G%d:", pins[k][0], pins[k][1]); for (int ad = 1; ad < 127; ad++) { Wire.beginTransmission(ad); if (Wire.endTransmission() == 0) Serial.printf(" 0x%02x", ad); } Serial.println(); } Wire.end(); pcaOk = pcaBegin(); Serial.printf("[scan] pcaOk=%d\n", pcaOk); }
            else if (serialBuf.equalsIgnoreCase("pca")) { Serial.printf("[pca] ok=%d MODE1=%02x MODE2=%02x PRESCALE=%d\n", pcaOk, pcaRead8(0x00), pcaRead8(0x01), pcaRead8(0xFE)); for (int ch = 0; ch < NUM_SERVOS; ch++) { uint8_t r[4]; for (int k = 0; k < 4; k++) r[k] = pcaRead8(0x06 + 4 * ch + k); Serial.printf("[pca] ch%d on=%u off=%u\n", ch, r[0] | (r[1] << 8), r[2] | (r[3] << 8)); } }
            else if (serialBuf.startsWith("servo ")) { int n = 0, deg = 0; if (sscanf(serialBuf.c_str() + 6, "%d %d", &n, &deg) == 2 && n >= 1 && n <= NUM_SERVOS) { servoPos[n - 1] = constrain(deg, 0, 180) * 100 / 180; servoWriteAngle(n - 1, deg); if (n - 1 == activeServo && gState == GState::IDLE) selectServo(activeServo); Serial.printf("[servo] %d -> %d deg\n", n, deg); } else Serial.println("[servo] usage: servo <1|2> <0-180>"); }
            else if (serialBuf.equalsIgnoreCase("reset")) { for (int i = 0; i < NUM_SERVOS; i++) { servoPos[i] = 50; servoWriteAngle(i, 90); } if (gState == GState::IDLE) selectServo(activeServo); }
            else { char buf[81]; serialBuf.substring(0, 80).toCharArray(buf, sizeof(buf)); xQueueSend(msgQueue, buf, 0); }
            serialBuf = "";
        } else if (serialBuf.length() < 80) serialBuf += c;
    }
}

void setup() {
    Serial.begin(115200); auto cfg = M5.config(); M5Dial.begin(cfg, true, true);
    pcaOk = pcaBegin(); Serial.printf("[pca9685] %s\n", pcaOk ? "found at 0x40" : "NOT found on Port A");
    for (int i = 0; i < NUM_SERVOS; i++) servoWriteAngle(i, posToDeg(servoPos[i]));
    M5Dial.Display.setTextColor(WHITE); M5Dial.Display.setTextDatum(middle_center); canvas.createSprite(240, 240);
    dataMutex = xSemaphoreCreateMutex(); msgQueue = xQueueCreate(4, 81); encoderQueue = xQueueCreate(8, sizeof(long));
    randomSeed(esp_random()); showBootInfo(); xTaskCreatePinnedToCore(encoderTask, "EncTask", 2048, NULL, 2, NULL, 0); selectServo(0);
}

void loop() {
    M5Dial.update(); handleSerial(); if (M5Dial.BtnA.wasPressed()) mazeButtonPress();
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
    } else {
        // MAZE_WIN — drain queues, hold the win banner
        char buf[81]; while (xQueueReceive(msgQueue, buf, 0) == pdTRUE) {}
        long encVal; while (xQueueReceive(encoderQueue, &encVal, 0) == pdTRUE) {}
    }
}
