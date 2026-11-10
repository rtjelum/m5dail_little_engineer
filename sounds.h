// sounds.h — short notification sounds played without blocking the display.
// Serial: "sound <name>", "sound typing [seconds] [speed]", "sound stop", "sound vol <0-255>", "sound" (list).
#pragma once

struct SoundStep { uint16_t f0, f1, ms; };          // f0 = 0 → silence; f1 != f0 → slide from f0 to f1
struct SoundDef  { const char* name; const SoundStep* steps; uint8_t n; };

#define SND(name, ...) static const SoundStep SND_##name[] = { __VA_ARGS__ };
SND(done,     {1568, 1568, 90}, {0, 0, 25}, {2093, 2093, 380})                                   // ding-ding bell
SND(success,  {1047, 1047, 90}, {1319, 1319, 90}, {1568, 1568, 90}, {2093, 2093, 320})          // rising arpeggio
SND(question, {880, 880, 110}, {0, 0, 40}, {900, 1500, 240})                                     // "hm?"
SND(error,    {330, 330, 180}, {0, 0, 60}, {247, 247, 380})                                      // low double buzz
SND(alert,    {2000, 2000, 90}, {0, 0, 60}, {2000, 2000, 90}, {0, 0, 60}, {2000, 2000, 90})     // three quick beeps
SND(hello,    {1047, 1047, 80}, {1568, 1568, 80}, {1319, 1319, 80}, {2093, 2093, 170})
SND(bye,      {2093, 2093, 130}, {1568, 1568, 130}, {1047, 1047, 260})
SND(wow,      {600, 2400, 400})                                                                  // slide whistle up
SND(sad,      {700, 500, 300}, {0, 0, 50}, {600, 380, 550})                                      // wah-wah
SND(thinking, {1200, 1200, 70}, {0, 0, 60}, {1400, 1400, 70}, {0, 0, 60}, {1100, 1100, 70}, {0, 0, 60}, {1500, 1500, 110})
SND(love,     {1047, 1319, 200}, {0, 0, 30}, {1319, 1760, 320})
SND(sleep,    {800, 300, 600}, {0, 0, 120}, {600, 250, 800})                                     // yawn
SND(startup,  {523, 523, 100}, {784, 784, 100}, {1047, 1047, 100}, {1568, 1568, 260})
SND(beep,     {1500, 1500, 120})
#undef SND

#define SND_ENTRY(name) {#name, SND_##name, sizeof(SND_##name) / sizeof(SoundStep)}
static const SoundDef SOUNDS[] = {
    SND_ENTRY(done), SND_ENTRY(success), SND_ENTRY(question), SND_ENTRY(error), SND_ENTRY(alert),
    SND_ENTRY(hello), SND_ENTRY(bye), SND_ENTRY(wow), SND_ENTRY(sad), SND_ENTRY(thinking),
    SND_ENTRY(love), SND_ENTRY(sleep), SND_ENTRY(startup), SND_ENTRY(beep),
};
#undef SND_ENTRY
constexpr int SOUND_COUNT = sizeof(SOUNDS) / sizeof(SOUNDS[0]);
constexpr unsigned long SOUND_SLIDE_TICK_MS = 10;

const SoundDef* soundCur = nullptr;
int           soundStep = 0;
unsigned long soundStepStart = 0, soundLastTick = 0;

static void soundStartStep() {
    soundStepStart = soundLastTick = millis();
    const SoundStep& s = soundCur->steps[soundStep];
    if (s.f0 == 0) M5Dial.Speaker.stop();
    else if (s.f1 == s.f0) M5Dial.Speaker.tone(s.f0, s.ms);
    else M5Dial.Speaker.tone(s.f0, SOUND_SLIDE_TICK_MS * 2);
}

// Keyboard typing: the "thinking" blips (1100-1500 Hz) at random gaps, with pauses between words.
// typingSpeed scales the overall pace (1 = normal, 2 = twice as fast). On top of that the tempo
// drifts per word (some words rushed, some slow) so it sounds like a person typing.
unsigned long typingUntil = 0, typingNext = 0;
int   typingWordLeft = 0;
float typingSpeed = 1.0f, typingTempo = 1.0f;

static float typingRand(float lo, float hi) { return lo + (hi - lo) * (random(1001) / 1000.0f); }

static void typingNewWord() {
    typingWordLeft = random(2, 9);
    // Tempo drifts smoothly from word to word, with an occasional fast burst or slow stretch.
    float target = random(6) == 0 ? typingRand(1.6f, 2.2f) : random(6) == 0 ? typingRand(0.45f, 0.7f) : typingRand(0.8f, 1.3f);
    typingTempo = typingTempo * 0.4f + target * 0.6f;
}

void typingStart(float seconds, float speed) {
    soundCur = nullptr;
    typingSpeed = constrain(speed, 0.25f, 4.0f);
    typingTempo = 1.0f;
    typingUntil = millis() + (unsigned long)(constrain(seconds, 0.5f, 120.0f) * 1000);
    typingNext = millis();
    typingNewWord();
}

static void typingUpdate() {
    if (!typingUntil) return;
    unsigned long now = millis();
    if ((long)(now - typingUntil) >= 0) { typingUntil = 0; return; }
    if ((long)(now - typingNext) < 0) return;
    float k = 1.0f / (typingSpeed * typingTempo);              // >1 = slower
    if (typingWordLeft > 0) {                                  // a key in the current word
        int blip = constrain((int)(70 * k), 25, 90);
        M5Dial.Speaker.tone(random(1100, 1501), blip);
        typingWordLeft--;
        typingNext = now + blip + (unsigned long)(random(40, 110) * k);
        if (random(25) == 0) typingNext += random(150, 400) * k;   // occasional hesitation
    } else {                                                   // gap between words
        typingNewWord();
        typingNext = now + (unsigned long)(random(150, 300) * k);
        if (random(8) == 0) typingNext += random(400, 900) * k;    // pause between sentences
    }
}

void soundPlay(int idx) { typingUntil = 0; soundCur = &SOUNDS[idx]; soundStep = 0; soundStartStep(); }
void soundPlayNamed(const char* name) { for (int i = 0; i < SOUND_COUNT; i++) if (!strcmp(SOUNDS[i].name, name)) { soundPlay(i); return; } }
void soundStop() { soundCur = nullptr; typingUntil = 0; M5Dial.Speaker.stop(); }

// Call every loop: advances steps and updates slide pitch.
void soundUpdate() {
    typingUpdate();
    if (!soundCur) return;
    unsigned long now = millis();
    const SoundStep& s = soundCur->steps[soundStep];
    unsigned long el = now - soundStepStart;
    if (el >= s.ms) {
        if (++soundStep >= soundCur->n) { soundCur = nullptr; return; }
        soundStartStep();
        return;
    }
    if (s.f0 && s.f1 != s.f0 && now - soundLastTick >= SOUND_SLIDE_TICK_MS) {
        soundLastTick = now;
        float k = el / (float)s.ms;
        M5Dial.Speaker.tone(s.f0 + (s.f1 - s.f0) * k, SOUND_SLIDE_TICK_MS * 2);
    }
}

static void soundListNames() {
    Serial.print("[sound] names:");
    for (int i = 0; i < SOUND_COUNT; i++) Serial.printf(" %s", SOUNDS[i].name);
    Serial.println(" typing [seconds] [speed|slow|fast]  (also: stop, vol <0-255>)");
}

void handleSoundCommand(String arg) {
    arg.trim();
    if (arg.length() == 0) { soundListNames(); return; }
    if (arg.equalsIgnoreCase("stop")) { soundStop(); Serial.println("[sound] stop"); return; }
    if (arg.startsWith("vol")) {
        int v = arg.substring(3).toInt();
        if (arg.substring(3).length()) M5Dial.Speaker.setVolume(constrain(v, 0, 255));
        Serial.printf("[sound] volume %d\n", M5Dial.Speaker.getVolume());
        return;
    }
    if (arg.equalsIgnoreCase("typing") || arg.startsWith("typing ")) {
        // "typing [seconds] [speed]" — speed is a number (0.25-4, 1 = normal) or slow / normal / fast / turbo.
        // Either part may be left out, and the order doesn't matter for the words: "typing fast" works.
        float sec = 6.0f, speed = 1.0f;
        String rest = arg.substring(6); rest.trim();
        bool haveSec = false;
        while (rest.length()) {
            int sp = rest.indexOf(' ');
            String tok = sp < 0 ? rest : rest.substring(0, sp);
            rest = sp < 0 ? "" : rest.substring(sp + 1); rest.trim();
            if      (tok.equalsIgnoreCase("slow"))   speed = 0.5f;
            else if (tok.equalsIgnoreCase("normal")) speed = 1.0f;
            else if (tok.equalsIgnoreCase("fast"))   speed = 1.8f;
            else if (tok.equalsIgnoreCase("turbo"))  speed = 3.0f;
            else if (tok.toFloat() > 0) { if (!haveSec) { sec = tok.toFloat(); haveSec = true; } else speed = tok.toFloat(); }
        }
        typingStart(sec, speed);
        Serial.printf("[sound] typing %.1fs speed %.2f\n", constrain(sec, 0.5f, 120.0f), typingSpeed);
        return;
    }
    for (int i = 0; i < SOUND_COUNT; i++)
        if (arg.equalsIgnoreCase(SOUNDS[i].name)) { soundPlay(i); Serial.printf("[sound] %s\n", SOUNDS[i].name); return; }
    Serial.printf("[sound] unknown '%s'\n", arg.c_str());
    soundListNames();
}
