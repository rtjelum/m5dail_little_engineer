// toon_faces.h — "toon" faces: flat yellow cartoon faces with brown outlines, blue eyes and big
// mouths (based on yellow_face.JPG). Included from faces.h; only uses the shared `canvas`.
#pragma once

enum ToonId : uint8_t { T_GRIN, T_GRUMPY, T_SILLY, T_GOOFY, T_SMILE, T_LAUGH, T_MEH, T_EVIL, T_CHEESY,
                        T_THINK, T_DONE, T_QUESTION, T_SLEEP,   // toon versions of the moods
                        TOON_COUNT };

constexpr float TOON_NO_LID = -9.0f;

static uint16_t toonBg()       { return canvas.color565(255, 192, 0); }
static uint16_t toonLine()     { return canvas.color565(125, 75, 20); }
static uint16_t toonBrow()     { return canvas.color565(60, 34, 22); }
static uint16_t toonMouthCol() { return canvas.color565(125, 22, 62); }
static uint16_t toonTongue()   { return canvas.color565(242, 128, 158); }

// Thick quadratic Bezier (start, control, end), drawn as a row of dots.
// wEnd = width at the ends, wMid = width in the middle (brows taper, lines don't).
static void toonCurve(float x0, float y0, float x1, float y1, float x2, float y2, float wEnd, float wMid, uint16_t col) {
    float len = hypotf(x1 - x0, y1 - y0) + hypotf(x2 - x1, y2 - y1);
    int n = (int)(len / 1.5f); if (n < 6) n = 6;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, a = 1 - t;
        float x = a * a * x0 + 2 * a * t * x1 + t * t * x2, y = a * a * y0 + 2 * a * t * y1 + t * t * y2;
        canvas.fillCircle(x, y, (wEnd + (wMid - wEnd) * sinf(t * PI)) * 0.5f, col);
    }
}

// White oval eye with a brown rim, blue (or red) iris, pupil and a shine dot.
// side: -1 = left eye on screen, +1 = right. lx/ly: look direction -1..1.
// lidIn/lidOut: top-lid height as a fraction of ry at the nose side / outer side (-1 = top, 0 = middle).
// blink 0..1 closes both lids to a line just below the middle.
static void toonEye(float cx, float cy, float rx, float ry, int side, float lx, float ly,
                    float lidIn, float lidOut, float blink, bool red = false) {
    uint16_t line = toonLine(), bg = toonBg();
    canvas.fillEllipse(cx, cy, rx + 3, ry + 3, line);
    canvas.fillEllipse(cx, cy, rx, ry, TFT_WHITE);
    float m = hypotf(lx, ly); if (m > 1) { lx /= m; ly /= m; }
    float ir = fminf(rx, ry) * 0.42f;
    float ix = cx + lx * (rx - ir - 3), iy = cy + ly * (ry - ir - 3);
    canvas.fillCircle(ix, iy, ir, red ? canvas.color565(205, 45, 40) : canvas.color565(60, 145, 215));
    canvas.fillCircle(ix, iy, ir * 0.55f, canvas.color565(20, 25, 50));
    canvas.fillCircle(ix + ir * 0.35f, iy - ir * 0.35f, fmaxf(2, ir * 0.25f), TFT_WHITE);

    float lidTop = -1 + 1.1f * blink, lidBot = 1 - 0.9f * blink;
    float y0 = cy - ry - 4, y1 = cy + ry + 4;
    for (int pass = 0; pass < 2; pass++)            // pass 0: blank above/below the lids, pass 1: lid edges
        for (int x = -(int)rx - 4; x <= (int)rx + 4; x++) {
            float k = constrain(0.5f - 0.5f * side * x / rx, 0.0f, 1.0f);   // 1 at the nose side
            float lid = fmaxf(lidOut + (lidIn - lidOut) * k, lidTop);
            float yl = cy + lid * ry, yb = cy + lidBot * ry;
            float u = x / rx, half = u * u < 1 ? ry * sqrtf(1 - u * u) : -1;
            if (pass == 0) {
                if (lid > -0.98f && yl > y0) canvas.drawFastVLine(cx + x, y0, yl - y0, bg);
                if (lidBot < 0.98f && yb < y1) canvas.drawFastVLine(cx + x, yb, y1 - yb + 1, bg);
            } else if (half > 0) {
                if (lid > -0.98f && fabsf(yl - cy) <= half + 1) canvas.fillCircle(cx + x, yl, 2, line);
                if (lidBot < 0.98f && fabsf(yb - cy) <= half + 1) canvas.fillCircle(cx + x, yb, 2, line);
            }
        }
}

// Mouth filled column by column between top(u) and bot(u), u = -1..1 across the width, with a brown rim.
// teeth = false: dark mouth with a pink tongue (ellipse tx, ty, trx, try_), clipped to the mouth.
// teeth = true:  white teeth with a grid of gaps.
template <class Top, class Bot>
static void toonMouth(float cx, float halfW, Top top, Bot bot, bool teeth = false,
                      float tx = 0, float ty = 0, float trx = 0, float try_ = 0) {
    uint16_t line = toonLine(), fill = teeth ? canvas.color565(255, 255, 250) : toonMouthCol();
    uint16_t pink = toonTongue(), gap = canvas.color565(200, 180, 150);
    int w = (int)halfW;
    canvas.fillCircle(cx - halfW, top(-1), 3, line);
    canvas.fillCircle(cx + halfW, top(1), 3, line);
    for (int x = -w; x <= w; x++) {
        float u = x / halfW, t = top(u), b = bot(u);
        canvas.drawFastVLine(cx + x, t - 3, b - t + 7, line);
    }
    for (int x = -w + 1; x <= w - 1; x++) {
        float u = x / halfW, t = top(u), b = bot(u);
        if (b - t < 1) continue;
        canvas.drawFastVLine(cx + x, t, b - t + 1, fill);
        if (teeth) {
            canvas.fillRect(cx + x, (t + b) * 0.5f - 1, 1, 2, gap);                    // gap between upper and lower row
            if (((x + w) % 14) < 2 && x > -w + 4 && x < w - 4) canvas.drawFastVLine(cx + x, t, b - t + 1, gap);
        } else if (trx > 0) {
            float du = (cx + x - tx) / trx;
            if (fabsf(du) < 1) {
                float hh = try_ * sqrtf(1 - du * du), t0 = fmaxf(t, ty - hh), t1 = fminf(b, ty + hh);
                if (t1 > t0) canvas.drawFastVLine(cx + x, t0, t1 - t0 + 1, pink);
            }
        }
    }
}

// Little sweat/tear drop with its tip pointing up.
static void toonDrop(float x, float y, float r) {
    uint16_t c = canvas.color565(110, 200, 245);
    canvas.fillCircle(x, y, r, c);
    canvas.fillTriangle(x - r * 0.9f, y - r * 0.4f, x + r * 0.9f, y - r * 0.4f, x, y - r * 2.3f, c);
}

// 4-point sparkle (cheesy grin shine).
static void toonSparkle(float x, float y, float s) {
    if (s <= 0.5f) return;
    canvas.fillTriangle(x - s, y, x + s, y, x, y - s * 2.6f, TFT_WHITE);
    canvas.fillTriangle(x - s, y, x + s, y, x, y + s * 2.6f, TFT_WHITE);
    canvas.fillTriangle(x, y - s, x, y + s, x - s * 2.6f, y, TFT_WHITE);
    canvas.fillTriangle(x, y - s, x, y + s, x + s * 2.6f, y, TFT_WHITE);
}

// Question mark drawn from dots: hook, stem and a dot. (x, y) = middle, s = scale (1 = ~44 px tall).
static void toonQuestionMark(float x, float y, float s, uint16_t col) {
    float r = 12 * s, cy = y - 14 * s, w = 4.5f * s;
    for (float a = 195; a <= 400; a += 3) canvas.fillCircle(x + r * cosf(a * DEG_TO_RAD), cy + r * sinf(a * DEG_TO_RAD), w, col);
    float ex = x + r * cosf(400 * DEG_TO_RAD), ey = cy + r * sinf(400 * DEG_TO_RAD);
    toonCurve(ex, ey, x, cy + r * 1.1f, x, y + 6 * s, 2 * w, 2 * w, col);
    canvas.fillCircle(x, y + 18 * s, w * 1.25f, col);
}

// One "Z" made of three strokes, faded from the background colour (a = 0) to brown (a = 1).
static void toonZ(float x, float y, float sz, float a) {
    uint16_t c = canvas.color565(255 + (125 - 255) * a, 192 + (75 - 192) * a, 0 + 20 * a);
    float h = sz * 0.5f, w = fmaxf(2, sz * 0.18f);
    toonCurve(x - h, y - h, x, y - h, x + h, y - h, w, w, c);
    toonCurve(x + h, y - h, x, y, x - h, y + h, w, w, c);
    toonCurve(x - h, y + h, x, y + h, x + h, y + h, w, w, c);
}

// t = seconds, blink 0..1 (blinks and face switches), sx/sy = small random glance (-0.35..0.35).
static void drawToonFace(int id, float t, float blink, float sx, float sy) {
    uint16_t brow = toonBrow(), line = toonLine();
    const float N = TOON_NO_LID;
    canvas.fillScreen(toonBg());
    switch (id) {
    case T_GRIN: {      // crazy happy: wonky eyes, huge open grin, bouncing
        float b = -fabsf(sinf(t * 5)) * 6, o = 0.88f + 0.12f * fabsf(sinf(t * 10));
        float w = sinf(t * 3) * 0.25f;
        toonEye(88, 90 + b, 26, 25, -1, 0.45f + w, 0.35f, N, N, blink);
        toonEye(146, 86 + b, 22, 25, 1, -0.40f + w, 0.30f, N, N, blink);
        toonCurve(68, 56 + b, 80, 38 + b, 100, 48 + b, 4, 10, brow);
        toonCurve(134, 44 + b, 148, 28 + b, 164, 40 + b, 4, 10, brow);
        auto top = [&](float u) { return 128 + b + 10 * (1 - u * u) + 4 * u; };
        auto bot = [&](float u) { return top(u) + 62 * o * sqrtf(fmaxf(0, 1 - u * u)); };
        toonMouth(116, 50, top, bot, false, 116, top(0) + 62 * o - 12, 26, 13);
        toonCurve(52, 112 + b, 56, 124 + b, 66, 124 + b, 4, 5, line);
        toonCurve(166, 132 + b, 180, 134 + b, 186, 120 + b, 4, 5, line);
    } break;

    case T_GRUMPY: {    // angry: heavy slanted brows, red eyes under lids, frown, trembling
        float sh = sinf(t * 37) * 1.2f, d = sinf(t * 3) * 2;
        toonEye(86 + sh, 104, 27, 22, -1, sx * 0.3f, 0.45f, 0.10f, -0.55f, blink, true);
        toonEye(154 + sh, 104, 27, 22, 1, sx * 0.3f, 0.45f, 0.10f, -0.55f, blink, true);
        toonCurve(52 + sh, 62, 80 + sh, 62, 108 + sh, 82 + d, 6, 12, brow);
        toonCurve(188 + sh, 62, 160 + sh, 62, 132 + sh, 82 + d, 6, 12, brow);
        toonCurve(115 + sh, 70, 117 + sh, 78, 116 + sh, 86, 3, 3, line);
        toonCurve(125 + sh, 70, 123 + sh, 78, 124 + sh, 86, 3, 3, line);
        toonCurve(86 + sh, 182, 120 + sh, 150, 158 + sh, 172, 5, 6, line);
        toonCurve(86 + sh, 182, 80 + sh, 184, 78 + sh, 192, 4, 4, line);
    } break;

    case T_SILLY: {     // cross-eyed, tongue sticking out and wagging
        float sw = sinf(t * 2) * 3, cr = 0.15f * sinf(t * 2.5f);
        toonEye(96 + sw, 86, 22, 23, -1, 0.55f + cr, 0.20f, N, N, blink);
        toonEye(148 + sw, 84, 24, 26, 1, -0.55f - cr, 0.10f, N, N, blink);
        toonCurve(80 + sw, 50, 88 + sw, 40, 102 + sw, 44, 4, 9, brow);
        toonCurve(136 + sw, 44, 150 + sw, 30, 166 + sw, 42, 4, 10, brow);
        float tw = sinf(t * 7) * 4, tx = 146 + sw + tw, ty = 172;   // tongue hangs out of the right corner (drawn first, mouth covers its root)
        canvas.fillEllipse(tx, ty, 20, 26, canvas.color565(200, 80, 110));
        canvas.fillEllipse(tx, ty, 17, 23, toonTongue());
        toonCurve(tx - 2, 156, tx, 168, tx - 1, 182, 3, 3, canvas.color565(215, 95, 125));
        auto top = [&](float u) { return 128 + 10 * (1 - u * u) - 4 * u; };
        auto bot = [&](float u) { return top(u) + 44 * sqrtf(fmaxf(0, 1 - u * u)); };
        toonMouth(108 + sw, 44, top, bot);
        toonCurve(54 + sw, 116, 56 + sw, 128, 64 + sw, 132, 4, 5, line);
    } break;

    case T_GOOFY: {     // buck teeth, tall open mouth flapping
        float o = 0.82f + 0.18f * sinf(t * 7), bob = sinf(t * 3.5f) * 2;
        toonEye(92, 86 + bob, 22, 24, -1, -0.45f, 0.40f + sy * 0.3f, N, N, blink);
        toonEye(142, 82 + bob, 26, 28, 1, -0.25f, 0.20f + sy * 0.3f, N, N, blink);
        toonCurve(76, 52 + bob, 86, 40 + bob, 100, 48 + bob, 4, 9, brow);
        toonCurve(128, 44 + bob, 142, 28 + bob, 158, 40 + bob, 4, 10, brow);
        toonCurve(46, 118, 118, 168, 192, 116, 5, 5, line);
        auto top = [&](float u) { return 132 + 4 * u; };
        auto bot = [&](float u) { return top(u) + 72 * o * powf(fmaxf(0, 1 - u * u * u * u), 0.5f); };
        toonMouth(104, 30, top, bot, false, 96, top(0) + 72 * o - 6, 18, 10);
        uint16_t wht = TFT_WHITE;
        canvas.fillRoundRect(94, 130, 18, 16, 3, line); canvas.fillRoundRect(96, 130, 14, 13, 2, wht);      // buck tooth
        float by = top(0) + 72 * o;
        canvas.fillRoundRect(98, by - 20, 16, 15, 3, line); canvas.fillRoundRect(100, by - 18, 12, 12, 2, wht);  // bottom tooth
        canvas.fillRoundRect(56, 128, 14, 14, 3, line); canvas.fillRoundRect(58, 130, 10, 10, 2, wht);
        canvas.fillRoundRect(170, 126, 14, 14, 3, line); canvas.fillRoundRect(172, 128, 10, 10, 2, wht);
    } break;

    case T_SMILE: {     // calm happy: big U smile, eyes wander
        float dy = sinf(t * 1.6f) * 2;
        toonEye(94, 96 + dy, 26, 28, -1, 0.30f + sx, 0.10f + sy, N, N, blink);
        toonEye(148, 96 + dy, 24, 26, 1, -0.10f + sx, 0.10f + sy, N, N, blink);
        toonCurve(76, 58 + dy, 88, 44 + dy, 102, 52 + dy, 4, 10, brow);
        toonCurve(132, 52 + dy, 146, 40 + dy, 162, 50 + dy, 4, 10, brow);
        toonCurve(54, 128 + dy, 120, 226 + dy, 188, 124 + dy, 5, 6, line);
        toonCurve(46, 120 + dy, 50, 130 + dy, 58, 132 + dy, 4, 4, line);
        toonCurve(184, 128 + dy, 192, 126 + dy, 194, 116 + dy, 4, 4, line);
        toonCurve(112, 198 + dy, 120, 204 + dy, 128, 198 + dy, 3, 4, line);
    } break;

    case T_LAUGH: {     // eyes squeezed shut, tears flying, shaking with laughter
        float sh = sinf(t * 18) * 2, o = 0.85f + 0.15f * fabsf(sinf(t * 9));
        toonCurve(70, 102 + sh, 92, 74 + sh, 114, 102 + sh, 5, 8, line);
        toonCurve(126, 102 + sh, 148, 74 + sh, 170, 102 + sh, 5, 8, line);
        toonCurve(76, 66 + sh, 90, 54 + sh, 106, 60 + sh, 4, 9, brow);
        toonCurve(134, 60 + sh, 150, 54 + sh, 164, 66 + sh, 4, 9, brow);
        auto top = [&](float u) { return 128 + sh - 8 * u * u; };
        auto bot = [&](float u) { return top(u) + 58 * o * sqrtf(fmaxf(0, 1 - u * u)); };
        toonMouth(120, 58, top, bot, false, 120, top(0) + 58 * o - 12, 28, 13);
        for (int s = -1; s <= 1; s += 2)
            for (int k = 0; k < 2; k++) {
                float p = fmodf(t * 1.3f + k * 0.5f + (s > 0 ? 0.25f : 0), 1.0f);
                toonDrop(120 + s * (62 + 50 * p), 88 + sh - 26 * p + 70 * p * p, 6 * (1 - 0.3f * p));
            }
    } break;

    case T_MEH: {       // bored: heavy lids, eyes rolling slowly, small sideways frown
        float dy = sinf(t * 0.5f) * 3, lx = sinf(t * 0.6f) * 0.7f;
        float lid = -0.05f + 0.15f * (0.5f + 0.5f * sinf(t * 0.4f));
        toonEye(86, 108 + dy, 30, 20, -1, lx, 0.5f, lid, lid, blink);
        toonEye(154, 108 + dy, 30, 20, 1, lx, 0.5f, lid, lid, blink);
        toonCurve(70, 80 + dy, 84, 72 + dy, 100, 78 + dy, 4, 9, brow);
        toonCurve(138, 74 + dy, 152, 64 + dy, 170, 70 + dy, 4, 9, brow);
        toonCurve(96, 182 + dy, 124, 166 + dy, 152, 178 + dy, 4, 5, line);
        toonCurve(96, 182 + dy, 92, 184 + dy, 90, 190 + dy, 4, 4, line);
    } break;

    case T_EVIL: {      // scheming grin: angry brows, red eyes darting side to side, chuckling
        float ch = -fabsf(sinf(t * 8)) * 2.5f * (fmodf(t, 3) < 1.5f ? 1 : 0.3f);
        float g = fmodf(t, 3.2f), lx = g < 1.4f ? -0.55f : g < 1.6f ? -0.55f + 1.1f * (g - 1.4f) / 0.2f : g < 3.0f ? 0.55f : 0.55f - 1.1f * (g - 3.0f) / 0.2f;
        toonEye(88, 100 + ch, 25, 21, -1, lx, 0.40f, 0.05f, -0.50f, blink, true);
        toonEye(152, 100 + ch, 25, 21, 1, lx, 0.40f, 0.05f, -0.50f, blink, true);
        toonCurve(56, 60 + ch, 82, 60 + ch, 108, 82 + ch, 6, 12, brow);
        toonCurve(184, 60 + ch, 158, 60 + ch, 132, 82 + ch, 6, 12, brow);
        auto top = [&](float u) { return 140 + ch - 12 * u * u; };
        auto bot = [&](float u) { return top(u) + 50 * sqrtf(fmaxf(0, 1 - u * u)); };
        toonMouth(122, 58, top, bot, false, 122, top(0) + 40, 26, 11);
        toonCurve(54, 118 + ch, 56, 128 + ch, 64, 130 + ch, 4, 5, line);
        toonCurve(180, 130 + ch, 188, 128 + ch, 190, 118 + ch, 4, 5, line);
    } break;

    case T_CHEESY: {    // big toothy grin with a shine running across the teeth
        float dy = sinf(t * 2) * 2;
        toonEye(96, 90 + dy, 24, 26, -1, 0.10f + sx, 0.15f + sy * 0.5f, N, N, blink);
        toonEye(146, 86 + dy, 26, 28, 1, -0.10f + sx, 0.15f + sy * 0.5f, N, N, blink);
        toonCurve(76, 54 + dy, 90, 40 + dy, 106, 50 + dy, 4, 10, brow);
        toonCurve(132, 46 + dy, 146, 30 + dy, 162, 44 + dy, 4, 10, brow);
        auto top = [&](float u) { return 140 + dy + 8 * (1 - u * u); };
        auto bot = [&](float u) { return 140 + dy + 44 * powf(fmaxf(0, 1 - u * u), 0.8f); };
        toonMouth(124, 54, top, bot, true);
        toonCurve(58, 128 + dy, 62, 140 + dy, 70, 142 + dy, 4, 5, line);
        toonCurve(178, 142 + dy, 186, 140 + dy, 190, 128 + dy, 4, 5, line);
        float p = fmodf(t * 0.6f, 1.0f);
        toonSparkle(90 + 80 * p, 152 + dy - 10 * sinf(p * PI), 5 * sinf(p * PI));
    } break;

    case T_THINK: {     // reading the laptop: eyes hop along the lines, one brow up, "hmm" mouth, "..." dots
        float rx, ry; faceReading(t, rx, ry);
        float lx = rx * 0.6f, ly = 0.45f + ry * 0.4f, dy = sinf(t * 1.2f) * 1.5f;
        toonEye(92, 102 + dy, 26, 26, -1, lx, ly, -0.35f, -0.45f, blink);
        toonEye(148, 98 + dy, 24, 27, 1, lx, ly, -0.45f, -0.35f, blink);
        toonCurve(70, 68 + dy, 88, 62 + dy, 106, 70 + dy, 4, 10, brow);
        toonCurve(132, 56 + dy, 148, 40 + dy, 166, 52 + dy, 4, 10, brow);
        float tilt = 3 * sinf(t * 0.9f);
        toonCurve(122, 180 + dy - tilt, 142, 174 + dy, 162, 178 + dy + tilt, 4, 5, line);
        for (int d = 0; d < 3; d++) {
            bool on = fmodf(t * 1.4f - d * 0.22f, 1.0f) < 0.35f;
            canvas.fillCircle(106 + d * 14, 26 + dy * 0.5f, on ? 5 : 4, on ? line : canvas.color565(225, 160, 0));
        }
    } break;

    case T_DONE: {      // job done: eyes squeezed happy, huge grin, bouncing, sparkles around the head
        float b = -fabsf(sinf(t * 4)) * 5, o = 0.88f + 0.12f * fabsf(sinf(t * 8));
        toonCurve(70, 100 + b, 92, 74 + b, 114, 100 + b, 5, 8, line);
        toonCurve(126, 100 + b, 148, 74 + b, 170, 100 + b, 5, 8, line);
        toonCurve(74, 60 + b, 90, 46 + b, 106, 54 + b, 4, 9, brow);
        toonCurve(134, 54 + b, 150, 46 + b, 166, 60 + b, 4, 9, brow);
        auto top = [&](float u) { return 126 + b - 8 * u * u; };
        auto bot = [&](float u) { return top(u) + 58 * o * sqrtf(fmaxf(0, 1 - u * u)); };
        toonMouth(120, 58, top, bot, false, 120, top(0) + 58 * o - 12, 28, 13);
        const float SX[4] = {40, 202, 206, 34}, SY[4] = {74, 66, 152, 150};
        for (int i = 0; i < 4; i++) toonSparkle(SX[i], SY[i] + b, 6 * fmaxf(0, sinf(t * 3 + i * 1.7f)));
    } break;

    case T_QUESTION: {  // puzzled: eyes rolled up to the "?", one brow raised and wiggling, small "o" mouth
        float dy = sinf(t * 1.6f) * 2;
        float g = fmodf(t, 4.0f), glance = g < 2.6f ? 0 : g < 2.8f ? (g - 2.6f) / 0.2f : g < 3.6f ? 1 : 1 - (g - 3.6f) / 0.4f;
        float lx = 0.50f - 0.40f * glance, ly = -0.60f + 0.10f * glance;
        float raise = 4 * (0.5f + 0.5f * sinf(t * 2.2f)), m = 1 + 0.1f * sinf(t * 3);
        toonEye(80, 108 + dy, 24, 25, -1, lx, ly, N, N, blink);
        toonEye(136, 104 + dy, 26, 28, 1, lx, ly, N, N, blink);
        toonCurve(58, 72 + dy, 80, 68 + dy, 102, 74 + dy, 4, 9, brow);
        toonCurve(114, 60 + dy - raise, 134, 40 + dy - raise, 158, 56 + dy - raise, 4, 10, brow);
        canvas.fillEllipse(106, 178 + dy, 15 * m, 14 * m, line);
        canvas.fillEllipse(106, 178 + dy, 12 * m, 11 * m, toonMouthCol());
        canvas.fillEllipse(103, 184 + dy, 7 * m, 4 * m, toonTongue());
        toonQuestionMark(196, 100 + sinf(t * 2.5f) * 5, 1.5f, line);
    } break;

    case T_SLEEP: {     // asleep: closed eyes, slow breathing "o" mouth, Zzz floating up
        float br = sinf(t * 0.8f), dy = br * 3, m = 0.85f + 0.2f * (0.5f + 0.5f * br);
        toonCurve(70, 106 + dy, 92, 122 + dy, 114, 106 + dy, 5, 7, line);
        toonCurve(126, 106 + dy, 148, 122 + dy, 170, 106 + dy, 5, 7, line);
        toonCurve(74, 80 + dy, 90, 74 + dy, 106, 80 + dy, 4, 8, brow);
        toonCurve(134, 80 + dy, 150, 74 + dy, 166, 80 + dy, 4, 8, brow);
        canvas.fillEllipse(120, 172 + dy, 13 * m, 11 * m, line);
        canvas.fillEllipse(120, 172 + dy, 10 * m, 8 * m, toonMouthCol());
        for (int i = 0; i < 3; i++) {
            float p = fmodf(t * 0.45f + i / 3.0f, 1.0f), a = p < 0.15f ? p / 0.15f : (1 - p) / 0.85f;
            toonZ(170 + p * 26, 82 - p * 40, 10 + p * 14, a);
        }
    } break;
    }
}
