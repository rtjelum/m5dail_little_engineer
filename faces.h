// faces.h — animated robot-eye expressions, drawn on the shared canvas.
// Serial: "face [bot|eyes] <name>", "face cycle", "face off", "face" (list).
// Included from m5dail_proj.ino after its globals (canvas, gState, isMoving, ...).
#pragma once

enum EyeStyle : uint8_t { EYE_PUPIL, EYE_HAPPY, EYE_RING, EYE_HEART, EYE_CLOSED_UP, EYE_CLOSED_DOWN, EYE_HEART_PUPIL, EYE_WIDE };
enum FaceId   : uint8_t { F_HAPPY, F_SAD, F_SURPRISED, F_ANGRY, F_EXCITED, F_CONFUSED, F_THINKING, F_WINKING, F_LOVE, F_SLEEPING, FACE_COUNT };

constexpr float NO_LID = -9.0f, NO_BROW = -9.0f;

// All values are fractions of the eye radius, relative to the eye centre (negative = up).
// "In" = the side toward the nose, "Out" = the side toward the screen edge.
struct EyeSpec  { EyeStyle style; float lidIn, lidOut; float browIn, browMid, browOut; float cheek; };   // cheek > 0 = smiling squint from below
struct FaceSpec { const char* name; uint8_t r, g, b; EyeSpec eye[2]; float lookX, lookY; };  // eye[0] = left on screen

#define FACE_BROW_ARCH  -1.25f, -1.55f, -1.25f
const FaceSpec FACES[FACE_COUNT] = {
  {"happy",     80, 255, 110, {{EYE_PUPIL, NO_LID, NO_LID, FACE_BROW_ARCH, 0.45f},    {EYE_PUPIL, NO_LID, NO_LID, FACE_BROW_ARCH, 0.45f}},           0.0f,  0.0f},
  {"sad",       90, 180, 255, {{EYE_PUPIL, -0.50f, -0.05f, -1.30f, -1.10f, -0.80f},   {EYE_PUPIL, -0.50f, -0.05f, -1.30f, -1.10f, -0.80f}},   0.0f,  0.35f},
  {"surprised", 255, 160, 40, {{EYE_WIDE,  NO_LID, NO_LID, -1.50f, -1.80f, -1.50f},   {EYE_WIDE,  NO_LID, NO_LID, -1.50f, -1.80f, -1.50f}},   0.0f,  0.0f},
  {"angry",     255, 50,  40, {{EYE_PUPIL, 0.05f,  -0.50f, -0.30f, -0.62f, -0.95f},   {EYE_PUPIL, 0.05f,  -0.50f, -0.30f, -0.62f, -0.95f}},   0.0f,  0.15f},
  {"excited",   255, 190, 40, {{EYE_PUPIL, NO_LID, NO_LID, -1.30f, -1.60f, -1.30f},   {EYE_PUPIL, NO_LID, NO_LID, -1.30f, -1.60f, -1.30f}},   0.0f,  0.0f},
  {"confused",  220, 80, 255, {{EYE_PUPIL, -0.40f, -0.40f, -0.75f, -0.90f, -0.85f},   {EYE_PUPIL, NO_LID, NO_LID, -1.45f, -1.75f, -1.40f}},   0.0f,  0.0f},
  {"thinking",  40, 220, 220, {{EYE_PUPIL, -0.55f, -0.50f, -1.30f, -1.50f, -1.25f},   {EYE_PUPIL, -0.55f, -0.50f, -1.15f, -1.40f, -1.60f}},   0.0f,  0.85f},
  {"winking",   210, 255, 40, {{EYE_PUPIL, NO_LID, NO_LID, -1.30f, -1.55f, -1.30f},   {EYE_PUPIL, NO_LID, NO_LID, -1.30f, -1.55f, -1.30f}},   0.0f,  0.0f},   // right eye winks (animated)
  {"love",      255, 60,  90, {{EYE_HEART_PUPIL, NO_LID, NO_LID, FACE_BROW_ARCH, 0.25f}, {EYE_HEART_PUPIL, NO_LID, NO_LID, FACE_BROW_ARCH, 0.25f}},           0.0f,  0.0f},
  {"sleeping",  60, 120, 255, {{EYE_CLOSED_DOWN, NO_LID, NO_LID, NO_BROW, 0, 0},      {EYE_CLOSED_DOWN, NO_LID, NO_LID, NO_BROW, 0, 0}},      0.0f,  0.0f},
};

constexpr int   FACE_EYE_R = 36, FACE_EYE_Y = 126;
constexpr int   FACE_EYE_X[2] = {72, 168};
constexpr unsigned long FACE_TRANS_MS = 130, FACE_BLINK_MS = 160, FACE_FRAME_MS = 33, FACE_CYCLE_MS = 3500;

int   faceIdx = 0, facePending = -1;
bool  faceOpening = false, faceCycling = false, faceBot = false, facePendingBot = false;
unsigned long faceWinkStart = 0, faceTransStart = 0, faceBlinkStart = 0, faceNextBlink = 0, faceNextSaccade = 0, faceLastFrame = 0, faceCycleAt = 0;
float faceLookX = 0, faceLookY = 0, faceSacX = 0, faceSacY = 0;

static uint16_t faceBg() { return canvas.color565(4, 4, 12); }
static uint16_t faceCol(const FaceSpec& f, float k, float toWhite = 0) {
    auto ch = [&](uint8_t c) { float v = c * k; v += (255 - v) * toWhite; return (uint8_t)constrain(v, 0.0f, 255.0f); };
    return canvas.color565(ch(f.r), ch(f.g), ch(f.b));
}

static void faceHeart(float x, float y, float h, uint16_t col) {
    canvas.fillCircle(x - h * 0.5f, y - h * 0.25f, h * 0.55f, col);
    canvas.fillCircle(x + h * 0.5f, y - h * 0.25f, h * 0.55f, col);
    canvas.fillTriangle(x - h * 1.03f, y - h * 0.05f, x + h * 1.03f, y - h * 0.05f, x, y + h * 1.15f, col);
}

// Brow = quadratic curve through inner end, middle, outer end. side: -1 left eye, +1 right eye.
static void faceBrow(float ex, float ey, int side, const EyeSpec& e, float lift, uint16_t col, uint16_t glow) {
    const float R = FACE_EYE_R;
    float x0 = ex - side * 0.9f * R, y0 = ey + (e.browIn + lift) * R;
    float x2 = ex + side * 0.9f * R, y2 = ey + (e.browOut + lift) * R;
    float x1 = ex, y1 = 2 * (ey + (e.browMid + lift) * R) - 0.5f * (y0 + y2);
    for (int pass = 0; pass < 2; pass++) {
        float px = x0, py = y0;
        for (int i = 1; i <= 12; i++) {
            float t = i / 12.0f, u = 1 - t;
            float qx = u * u * x0 + 2 * u * t * x1 + t * t * x2, qy = u * u * y0 + 2 * u * t * y1 + t * t * y2;
            canvas.drawWideLine(px, py, qx, qy, pass ? 3.0f : 5.5f, pass ? col : glow);
            px = qx; py = qy;
        }
    }
}

// Eyelid: blank everything above a slanted line, then draw the lid edge clipped to the eye.
static void faceLid(float ex, float ey, int side, float lidIn, float lidOut, uint16_t col, float R = FACE_EYE_R, float edgeW = 2.5f) {
    if (lidIn <= NO_LID) return;
    float xi = ex - side * R, yi = ey + lidIn * R, xo = ex + side * R, yo = ey + lidOut * R;
    float m = (yo - yi) / (xo - xi);
    float xa = ex - R - 6, ya = yi + m * (xa - xi), xb = ex + R + 6, yb = yi + m * (xb - xi), top = ey - R - 6;
    canvas.fillTriangle(xa, top, xb, top, xb, yb, faceBg());
    canvas.fillTriangle(xa, top, xb, yb, xa, ya, faceBg());
    float dx = xb - xa, dy = yb - ya, fx = xa - ex, fy = ya - ey, r = R - 1.5f;
    float a = dx * dx + dy * dy, b = 2 * (fx * dx + fy * dy), c = fx * fx + fy * fy - r * r, disc = b * b - 4 * a * c;
    if (disc <= 0 || edgeW <= 0) return;
    float s = sqrtf(disc), t1 = (-b - s) / (2 * a), t2 = (-b + s) / (2 * a);
    canvas.drawWideLine(xa + dx * t1, ya + dy * t1, xa + dx * t2, ya + dy * t2, edgeW, col);
}

static void faceEye(float ex, float ey, const FaceSpec& f, const EyeSpec& e, float lx, float ly, float pupil, float heart) {
    const float R = FACE_EYE_R;
    uint16_t col = faceCol(f, 1), glow = faceCol(f, 0.28f), dark = faceCol(f, 0.10f), light = faceCol(f, 1, 0.72f);
    switch (e.style) {
    case EYE_PUPIL: {
        canvas.fillCircle(ex, ey, R + 3, glow);
        canvas.fillCircle(ex, ey, R, light);
        canvas.fillArc(ex, ey, R, R - 3, 0, 360, col);
        float ir = R * 0.55f, ix = ex + lx * R * 0.32f, iy = ey + ly * R * 0.32f;
        canvas.fillCircle(ix, iy, ir, col);
        canvas.fillCircle(ix, iy, ir * 0.5f * pupil, dark);
        canvas.fillCircle(ix + ir * 0.38f, iy - ir * 0.40f, ir * 0.22f, TFT_WHITE);
        canvas.fillCircle(ix - ir * 0.30f, iy + ir * 0.35f, ir * 0.10f, TFT_WHITE);
    } break;
    case EYE_HEART_PUPIL: {   // normal eye whose iris is a beating heart
        canvas.fillCircle(ex, ey, R + 3, glow);
        canvas.fillCircle(ex, ey, R, light);
        canvas.fillArc(ex, ey, R, R - 3, 0, 360, col);
        float hx = ex + lx * R * 0.3f, hy = ey + ly * R * 0.3f - 2, hs = R * 0.48f * heart;
        faceHeart(hx, hy, hs, col);
        faceHeart(hx, hy + 1, hs * 0.55f, dark);
        canvas.fillCircle(hx + hs * 0.45f, hy - hs * 0.45f, hs * 0.2f, TFT_WHITE);
    } break;
    case EYE_WIDE: {   // wide-open eye: lots of white, small iris, pin-prick pupil
        canvas.fillCircle(ex, ey, R + 4, glow);
        canvas.fillCircle(ex, ey, R, faceCol(f, 1, 0.85f));
        canvas.fillArc(ex, ey, R, R - 3, 0, 360, col);
        float ir = R * 0.36f, ix = ex + lx * R * 0.4f, iy = ey + ly * R * 0.4f;
        canvas.fillCircle(ix, iy, ir, col);
        canvas.fillCircle(ix, iy, ir * 0.38f * pupil, dark);
        canvas.fillCircle(ix + ir * 0.40f, iy - ir * 0.42f, ir * 0.25f, TFT_WHITE);
    } break;
    case EYE_RING: {
        canvas.fillCircle(ex, ey, R + 3, glow);
        canvas.fillCircle(ex, ey, R, dark);
        canvas.fillArc(ex, ey, R, R - 10, 0, 360, col);
        float pr = R * 0.3f * pupil, px = ex + lx * R * 0.3f, py = ey + ly * R * 0.3f;
        canvas.fillCircle(px, py, pr, col);
        canvas.fillCircle(px, py, pr - 5, dark);
    } break;
    case EYE_HAPPY:
        canvas.fillArc(ex, ey, R + 3, R - 9, 0, 360, glow);
        canvas.fillArc(ex, ey, R, R - 6, 0, 360, col);
        canvas.fillArc(ex, ey + R * 0.35f, R * 0.5f, R * 0.5f - 6, 195, 345, col);
        break;
    case EYE_HEART:
        canvas.fillArc(ex, ey, R + 3, R - 7, 0, 360, glow);
        canvas.fillArc(ex, ey, R, R - 4, 0, 360, col);
        faceHeart(ex, ey - 2, R * 0.42f * heart * 1.15f, glow);
        faceHeart(ex, ey - 2, R * 0.42f * heart, col);
        break;
    case EYE_CLOSED_UP:
        canvas.fillArc(ex, ey + R * 0.5f, R * 0.85f + 3, R * 0.85f - 10, 200, 340, glow);
        canvas.fillArc(ex, ey + R * 0.5f, R * 0.85f, R * 0.85f - 7, 205, 335, col);
        break;
    case EYE_CLOSED_DOWN:
        canvas.fillArc(ex, ey - R * 0.55f, R * 0.9f + 3, R * 0.9f - 11, 22, 158, glow);
        canvas.fillArc(ex, ey - R * 0.55f, R * 0.9f, R * 0.9f - 8, 25, 155, col);
        break;
    }
}

// Blink / transition: close the eye from top and bottom; at nearly shut, draw a slit.
static void faceCover(float ex, float ey, float closed, uint16_t col, int R = FACE_EYE_R) {
    if (closed <= 0.02f) return;
    int h = (int)((R + 5) * closed);
    canvas.fillRect(ex - R - 6, ey - R - 6, 2 * R + 12, h + 1, faceBg());
    canvas.fillRect(ex - R - 6, ey + R + 6 - h, 2 * R + 12, h + 1, faceBg());
    if (closed > 0.85f) canvas.drawWideLine(ex - R * 0.8f, ey, ex + R * 0.8f, ey, 2.5f, col);
}

// ── "Bot" style: one round face, big orange-yellow eyes, square pupils, grid-of-teeth mouth, LED-dot look ──
enum BotEyeStyle : uint8_t { BE_DOT, BE_RING, BE_ARC, BE_CHEVRON, BE_BAR, BE_HEART };
struct BotEye   { BotEyeStyle style; float size, lidIn, lidOut; };
// curve > 0 = smile; pinch > 0 = squeezed in the middle (bow-tie)
struct BotMouth { int16_t w, h; float curve, wave, tilt; uint8_t rows, cols; float pinch; };
struct BotFace  { BotEye eye[2]; BotMouth m; };

const BotFace BOT_FACES[FACE_COUNT] = {
  /* happy     */ {{{BE_ARC, 1, NO_LID, NO_LID},          {BE_ARC, 1, NO_LID, NO_LID}},          {160, 46, 14, 0, 0, 2, 7, 0}},
  /* sad       */ {{{BE_DOT, 1, -0.35f, 0.05f},           {BE_DOT, 1, -0.35f, 0.05f}},           {130, 40, -12, 0, 0, 2, 6, 0}},
  /* surprised */ {{{BE_DOT, 1.05f, NO_LID, NO_LID},      {BE_DOT, 1.05f, NO_LID, NO_LID}},      {66, 70, 0, 0, 0, 4, 4, 0}},
  /* angry     */ {{{BE_DOT, 1, 0.15f, -0.35f},           {BE_DOT, 1, 0.15f, -0.35f}},           {164, 50, -3, 0, 0, 2, 6, 0}},
  /* excited   */ {{{BE_CHEVRON, 1, NO_LID, NO_LID},      {BE_CHEVRON, 1, NO_LID, NO_LID}},      {176, 64, 16, 0, 0, 3, 9, 0}},
  /* confused  */ {{{BE_DOT, 0.95f, 0.0f, 0.0f},          {BE_RING, 1.05f, NO_LID, NO_LID}},     {150, 46, 0, 0, 0, 2, 6, 0.85f}},
  /* thinking  */ {{{BE_DOT, 1, -0.50f, -0.50f},          {BE_DOT, 1.05f, -0.60f, -0.60f}},      {116, 40, 3, 0, 0, 2, 5, 0}},
  /* winking   */ {{{BE_DOT, 1, NO_LID, NO_LID},          {BE_DOT, 1, NO_LID, NO_LID}},          {150, 44, 10, 0, 6, 2, 6, 0}},
  /* love      */ {{{BE_HEART, 1, NO_LID, NO_LID},        {BE_HEART, 1, NO_LID, NO_LID}},        {150, 40, 12, 0, 0, 2, 7, 0}},
  /* sleeping  */ {{{BE_BAR, 1, NO_LID, NO_LID},          {BE_BAR, 1, NO_LID, NO_LID}},          {140, 42, 0, 0, 0, 2, 6, 0}},
};

constexpr int BOT_EYE_R = 40, BOT_EYE_Y = 88, BOT_MOUTH_Y = 172;
constexpr int BOT_EYE_X[2] = {76, 164};

static uint16_t botYellow() { return canvas.color565(255, 205, 45); }

// Eye disc: orange rim fading to bright yellow in the middle.
static void botDisc(float ex, float ey, float R) {
    canvas.fillCircle(ex, ey, R,         canvas.color565(230, 115, 10));
    canvas.fillCircle(ex, ey, R * 0.88f, canvas.color565(245, 155, 20));
    canvas.fillCircle(ex, ey, R * 0.72f, canvas.color565(255, 190, 35));
    canvas.fillCircle(ex, ey, R * 0.50f, canvas.color565(255, 215, 70));
}

static void botPupil(float px, float py, float sz) {
    canvas.fillRoundRect(px - sz / 2, py - sz / 2, sz, sz, sz * 0.25f, canvas.color565(16, 12, 4));
}

static void botEye(float ex, float ey, const BotEye& e, float lx, float ly, float pupil, float heart) {
    const float R = BOT_EYE_R * e.size;
    uint16_t yel = botYellow(), bg = faceBg();
    switch (e.style) {
    case BE_DOT:
        botDisc(ex, ey, R);
        botPupil(ex + lx * R * 0.4f, ey + ly * R * 0.4f, R * 0.40f * pupil);
        break;
    case BE_RING:   // extra-bright centre, like a startled eye
        botDisc(ex, ey, R);
        canvas.fillCircle(ex, ey, R * 0.62f, canvas.color565(255, 235, 150));
        canvas.fillCircle(ex, ey, R * 0.45f, canvas.color565(255, 250, 225));
        botPupil(ex + lx * R * 0.3f, ey + ly * R * 0.3f, R * 0.36f * pupil);
        break;
    case BE_ARC:    // smiling closed eye: dome with a dark curved crease
        botDisc(ex, ey + R * 0.2f, R);
        canvas.fillRect(ex - R - 2, ey + R * 0.35f, 2 * R + 4, R + 4, bg);
        canvas.fillArc(ex, ey + R * 0.45f, R * 0.45f, R * 0.45f - 7, 200, 340, bg);
        break;
    case BE_CHEVRON: {  // excited: thick upward arch with rounded ends
        float cy = ey + R * 0.5f, ro = R * 0.95f, w = 12, rm = ro - w / 2;
        canvas.fillArc(ex, cy, ro, ro - w, 205, 335, yel);
        for (float a : {205.0f, 335.0f})
            canvas.fillCircle(ex + rm * cosf(a * DEG_TO_RAD), cy + rm * sinf(a * DEG_TO_RAD), w / 2, yel);
    } break;
    case BE_BAR:
        canvas.fillRoundRect(ex - R * 0.75f, ey - 4, R * 1.5f, 8, 4, yel);
        break;
    case BE_HEART:
        faceHeart(ex, ey, R * 0.62f * heart, yel);
        faceHeart(ex, ey + 2, R * 0.42f * heart, canvas.color565(170, 20, 40));
        break;
    }
}

// Mouth drawn column by column: rounded ends, curved for smile/frown, teeth gaps cut out.
static void botMouth(float cx, float cy, const BotMouth& m, float hScale) {
    uint16_t tooth = canvas.color565(225, 238, 250), gap = faceBg();
    float halfW = m.w / 2.0f, cw = (float)m.w / m.cols;
    bool roundMouth = m.w < m.h * 1.3f;
    for (int x = -(int)halfW; x <= (int)halfW; x++) {
        float u = x / halfW;
        float round = roundMouth ? sqrtf(fmaxf(0, 1 - u * u)) : sqrtf(fmaxf(0, 1 - u * u * u * u));
        if (round < 0.05f) continue;
        if (m.pinch > 0) { float c = 1 - fabsf(u); round *= 1 - m.pinch * c * c * 1.6f > 0.12f ? 1 - m.pinch * c * c * 1.6f : 0.12f; }
        float bow = m.curve * (1 - u * u);
        float mid = cy + m.tilt * u + m.wave * sinf(u * PI * 3);
        float half = m.h * 0.5f * hScale * round;
        // The outer edge bends more: bottom for a smile, top for a frown.
        float topK = m.curve > 0 ? 0.4f : 1.0f, botK = m.curve > 0 ? 1.0f : 0.4f;
        int top = (int)(mid - half + bow * topK), bot = (int)(mid + half + bow * botK);
        float xi = x + halfW;
        if (xi > 3 && xi < m.w - 3 && fmodf(xi, cw) < 3.0f) continue;   // gap between teeth
        canvas.drawFastVLine(cx + x, top, bot - top, tooth);
        for (int k = 1; k < m.rows; k++) canvas.fillRect(cx + x, top + (bot - top) * k / m.rows - 1, 1, 3, gap);
    }
}

// LED-matrix look: dark lines every 3 px so everything shows as little dots.
static void botLedGrid() {
    uint16_t bg = faceBg();
    for (int v = 2; v < 240; v += 3) { canvas.drawFastHLine(0, v, 240, bg); canvas.drawFastVLine(v, 0, 240, bg); }
}

static void faceZzz(float t, uint8_t r, uint8_t g, uint8_t b, float x0, float y0) {
    canvas.setTextDatum(middle_center);
    canvas.setTextFont(&fonts::DejaVu18);
    for (int i = 0; i < 3; i++) {
        float p = fmodf(t * 0.45f + i / 3.0f, 1.0f);
        float a = p < 0.15f ? p / 0.15f : (1 - p) / 0.85f;
        auto ch = [&](uint8_t c) { float v = c * a; return (uint8_t)(v + (255 - v) * 0.3f * a); };
        canvas.setTextSize(0.8f + p * 1.2f);
        canvas.setTextColor(canvas.color565(ch(r), ch(g), ch(b)));
        canvas.drawString("Z", x0 + p * 26, y0 - p * 38);
    }
    canvas.setTextSize(1);
}

// Reading motion: hop left→right along a line in small jumps, sweep back to the start of the
// next line, and after the last line pause and start again at the top. rx: -1..1, ry: 0 (top)..1 (bottom).
static void faceReading(float t, float& rx, float& ry) {
    const int   STEPS = 6, LINES = 4;
    const float FIX = 0.26f, JUMP = 0.05f, RET = 0.22f, PAUSE = 0.7f;
    const float LINE_T = STEPS * FIX + RET;
    float tp = fmodf(t, LINE_T * LINES + PAUSE);
    if (tp >= LINE_T * LINES) { rx = -1; ry = 0; return; }          // end of page: rest at top-left
    int   line = (int)(tp / LINE_T);
    float lt = tp - line * LINE_T, y0 = line / (float)(LINES - 1);
    if (lt < STEPS * FIX) {                                         // reading along the line
        int   st = (int)(lt / FIX);
        float in = lt - st * FIX;
        float x  = -1 + 2.0f * st / (STEPS - 1);
        if (st > 0 && in < JUMP) { float xp = -1 + 2.0f * (st - 1) / (STEPS - 1); x = xp + (x - xp) * (in / JUMP); }
        rx = x; ry = y0;
    } else {                                                        // quick sweep back, one line down
        float k = (lt - STEPS * FIX) / RET; k = k * k * (3 - 2 * k);
        float y1 = line + 1 < LINES ? (line + 1) / (float)(LINES - 1) : 0;
        rx = 1 - 2 * k; ry = y0 + (y1 - y0) * k;
    }
}

static float facePulse(float x) { return x >= 0 ? expf(-x * 12.0f) : 0.0f; }

void drawFace() {
    unsigned long now = millis();
    float closed = 0;

    // Face change = quick close with the old face, then open with the new one.
    if (facePending >= 0) {
        float p = (now - faceTransStart) / (float)FACE_TRANS_MS;
        if (p >= 1) { faceIdx = facePending; faceBot = facePendingBot; facePending = -1; faceOpening = true; faceTransStart = now; closed = 1; }
        else closed = p;
    } else if (faceOpening) {
        float p = (now - faceTransStart) / (float)FACE_TRANS_MS;
        if (p >= 1) faceOpening = false; else closed = 1 - p;
    }

    const FaceSpec& f = FACES[faceIdx];
    float t = now / 1000.0f;

    if (faceIdx != F_SLEEPING && now >= faceNextBlink) { faceBlinkStart = now; faceNextBlink = now + random(2200, 5500); }
    float bp = (now - faceBlinkStart) / (float)FACE_BLINK_MS;
    if (bp < 1 && faceIdx != F_SLEEPING) closed = max(closed, 1 - fabsf(2 * bp - 1));

    if (now >= faceNextSaccade) { faceSacX = random(-35, 36) / 100.0f; faceSacY = random(-20, 21) / 100.0f; faceNextSaccade = now + random(800, 2800); }

    // Per-expression motion
    float wink = 0, offX = 0, offY = sinf(t * 1.6f) * 2.0f, pupil = 1, heart = 1, sacc = 1, lift[2] = {0, 0};
    float lx = f.lookX, ly = f.lookY;
    switch (faceIdx) {
    case F_HAPPY:     offY = -fabsf(sinf(t * 5)) * 5; lift[0] = lift[1] = -fabsf(sinf(t * 5)) * 0.06f; break;
    case F_SAD:       offY = 3 + sinf(t * 0.9f) * 1.5f; sacc = 0.3f; break;
    case F_SURPRISED: pupil = 0.8f + 0.25f * sinf(t * 9); lift[0] = lift[1] = -0.08f * (0.5f + 0.5f * sinf(t * 3)); sacc = 0; break;
    case F_ANGRY:     offX = random(-15, 16) / 10.0f; lift[0] = lift[1] = sinf(t * 14) * 0.03f; pupil = 0.8f; sacc = 0.2f; break;
    case F_EXCITED:   offY = -fabsf(sinf(t * 7)) * 6; pupil = 1.1f + 0.2f * sinf(t * 12); lift[0] = lift[1] = -fabsf(sinf(t * 7)) * 0.12f; break;
    case F_CONFUSED:  lx = fmodf(t, 2.4f) < 1.2f ? -0.5f : 0.4f; lift[1] = -0.1f * (0.5f + 0.5f * sinf(t * 4)); sacc = 0; break;
    case F_THINKING:  { float rx, ry; faceReading(t, rx, ry); lx = rx * 0.50f; ly = 0.75f + ry * 0.25f; sacc = 0; } break;
    case F_WINKING: {
        unsigned long ph = (now - faceWinkStart) % 2600;
        wink = ph < 90 ? ph / 90.0f : ph < 370 ? 1 : ph < 480 ? 1 - (ph - 370) / 110.0f : 0;
        lift[0] = -0.12f * wink;      // other brow goes up
        lift[1] =  0.18f * wink;      // winking brow comes down
    } break;
    case F_LOVE:      { float hb = fmodf(t, 1.0f); heart = 0.9f + 0.22f * (facePulse(hb) + facePulse(hb - 0.22f)); } break;
    case F_SLEEPING:  offY = sinf(t * 0.8f) * 3; break;
    }
    lx += faceSacX * sacc; ly += faceSacY * sacc;
    faceLookX += (lx - faceLookX) * 0.25f;
    faceLookY += (ly - faceLookY) * 0.25f;

    canvas.fillScreen(faceBg());
    if (faceBot) {
        const BotFace& bf = BOT_FACES[faceIdx];
        uint16_t yel = botYellow();
        float hScale = 1;
        if (faceIdx == F_SURPRISED) hScale = 0.9f + 0.12f * sinf(t * 9);
        if (faceIdx == F_SLEEPING)  hScale = 0.85f + 0.15f * sinf(t * 0.8f);
        float blx = faceLookX, bly = faceLookY;
        BotMouth mouth = bf.m;
        float mouthX = 120;
        if (faceIdx == F_THINKING) {
            // Pupils stay looking down, reading a laptop screen line by line.
            float rx, ry; faceReading(t, rx, ry);
            blx = 0.10f + rx * 0.50f;
            bly = 0.60f + ry * 0.25f;
            // "Hmm" mouth pushed to one side, slowly tilting.
            mouthX = 132;
            mouth.tilt = 3 * sinf(t * 0.9f);
        }
        for (int i = 0; i < 2; i++) {
            const BotEye& e = bf.eye[i];
            float ex = BOT_EYE_X[i] + offX, ey = BOT_EYE_Y + offY + lift[i] * 20;
            if (faceIdx == F_THINKING && i == 1) ey -= 5;   // one eye raised, like a lifted eyebrow
            float cl = i == 1 ? max(closed, wink) : closed;
            if (i == 1 && wink > 0.85f) { BotEye shut = e; shut.style = BE_BAR; botEye(ex, ey, shut, 0, 0, 1, 1); continue; }
            botEye(ex, ey, e, blx, bly, pupil, heart);
            faceLid(ex, ey, i == 0 ? -1 : 1, e.lidIn, e.lidOut, yel, BOT_EYE_R * e.size, 0);
            if (e.style != BE_BAR) faceCover(ex, ey, cl, yel, (int)(BOT_EYE_R * e.size));
        }
        botMouth(mouthX + offX, BOT_MOUTH_Y + offY, mouth, hScale);
        if (faceIdx == F_THINKING) {   // "..." thinking dots above the eyes, lighting up one after another
            for (int d = 0; d < 3; d++) {
                float ph = fmodf(t * 1.4f - d * 0.22f, 1.0f);
                float k = ph < 0.35f ? 1.0f : 0.25f;
                canvas.fillCircle(106 + d * 14, 28 + offY * 0.5f, ph < 0.35f ? 5 : 4,
                                  canvas.color565(255 * k, 205 * k, 45 * k));
            }
        }
        if (faceIdx == F_SLEEPING) faceZzz(t, 200, 240, 255, 150, 50);
        botLedGrid();
        canvas.pushSprite(0, 0);
        return;
    }
    uint16_t col = faceCol(f, 1), glow = faceCol(f, 0.28f);
    for (int i = 0; i < 2; i++) {
        const EyeSpec& e = f.eye[i];
        int side = i == 0 ? -1 : 1;
        float ex = FACE_EYE_X[i] + offX, ey = FACE_EYE_Y + offY;
        float cl = i == 1 ? max(closed, wink) : closed;
        if (i == 1 && wink > 0.85f) {   // fully winked: happy closed-eye arc instead of a slit
            EyeSpec shut = e; shut.style = EYE_CLOSED_UP;
            faceEye(ex, ey, f, shut, 0, 0, 1, 1);
            faceBrow(ex, ey, side, e, lift[i], col, glow);
            continue;
        }
        faceEye(ex, ey, f, e, faceLookX, faceLookY, pupil, heart);
        if (e.cheek > 0) {   // smiling squint: a round cheek pushes up from below; edge drawn only inside the eye
            const float R = FACE_EYE_R;
            float cy = ey + R * (1.85f - e.cheek * 1.2f), cr = R * 1.15f, d = cy - ey;
            float th = acosf(constrain((d * d + cr * cr - R * R) / (2 * d * cr), -1.0f, 1.0f)) * RAD_TO_DEG;
            canvas.fillCircle(ex, cy, cr, faceBg());
            canvas.fillArc(ex, cy, cr + 3, cr, 270 - th, 270 + th, col);
        }
        faceLid(ex, ey, side, e.lidIn, e.lidOut, col);
        faceCover(ex, ey, cl, col);
        if (e.browIn > NO_BROW) faceBrow(ex, ey, side, e, lift[i], col, glow);
    }

    if (faceIdx == F_SLEEPING) faceZzz(t, f.r, f.g, f.b, 170, 92);
    canvas.pushSprite(0, 0);
}

int faceFind(const String& name) {
    for (int i = 0; i < FACE_COUNT; i++) if (name.equalsIgnoreCase(FACES[i].name)) return i;
    return -1;
}

void faceShow(int idx, bool bot) {
    unsigned long now = millis();
    if (gState != GState::FACE) {
        gState = GState::FACE; isMoving = false;
        faceIdx = idx; faceBot = bot; facePending = -1; faceOpening = true; faceTransStart = now;
    } else if (idx != faceIdx || bot != faceBot || facePending >= 0) {
        facePending = idx; facePendingBot = bot; faceOpening = false; faceTransStart = now;
    }
    faceNextBlink = now + random(2000, 4000);
    if (idx == F_WINKING) faceWinkStart = now + FACE_TRANS_MS * 2 + 500 - 2600;   // first wink ~0.75 s in
}

void faceExit() { faceCycling = false; mazeExitToIdle(); }

void faceUpdate() {
    unsigned long now = millis();
    if (now - faceLastFrame < FACE_FRAME_MS) return;
    faceLastFrame = now;
    if (faceCycling && now >= faceCycleAt) { faceCycleAt = now + FACE_CYCLE_MS; faceShow((faceIdx + 1) % FACE_COUNT, faceBot); }
    drawFace();
}

static void faceListNames() {
    Serial.print("[face] names:");
    for (int i = 0; i < FACE_COUNT; i++) Serial.printf(" %s", FACES[i].name);
    Serial.println("  (also: cycle, off; prefix \"bot\" for the teeth style, \"eyes\" for the big-eye style)");
}

void handleFaceCommand(String arg) {
    arg.trim();
    // Optional style prefix: "bot" (teeth face) or "eyes" (big-eye face). Without it the current style is kept.
    bool bot = (gState == GState::FACE) ? faceBot : false;
    String lower = arg; lower.toLowerCase();
    if (lower == "bot" || lower.startsWith("bot "))   { bot = true;  arg = arg.substring(3); arg.trim(); if (!arg.length()) arg = FACES[gState == GState::FACE ? faceIdx : 0].name; }
    else if (lower == "eyes" || lower.startsWith("eyes ")) { bot = false; arg = arg.substring(4); arg.trim(); if (!arg.length()) arg = FACES[gState == GState::FACE ? faceIdx : 0].name; }
    if (arg.length() == 0) { faceListNames(); return; }
    if (arg.equalsIgnoreCase("off")) { if (gState == GState::FACE) faceExit(); Serial.println("[face] off"); return; }
    if (arg.equalsIgnoreCase("cycle")) { faceCycling = true; faceCycleAt = millis() + FACE_CYCLE_MS; faceShow(gState == GState::FACE ? (faceIdx + 1) % FACE_COUNT : 0, bot); Serial.println("[face] cycling"); return; }
    int idx = faceFind(arg);
    if (idx < 0) { Serial.printf("[face] unknown '%s'\n", arg.c_str()); faceListNames(); return; }
    faceCycling = false;
    faceShow(idx, bot);
    Serial.printf("[face] %s%s\n", bot ? "bot " : "", FACES[idx].name);
}
