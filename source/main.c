// SKY WIZARD DS - 3D flying wizard combo game (libnds)
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.14159265f
enum { KA, KB, KX, KY };
static const char BN[] = "ABXY";
static const int KEYS[4] = { KEY_A, KEY_B, KEY_X, KEY_Y };

typedef struct { float x, y, z, vx, vy, vz, r, g, b, dmg; int on; } Shot;
typedef struct { float x, y, z, s; int type; } Scn;
typedef struct { const char *name; int len; int seq[4]; float dmg, r, g, b; int learned, mastery; } Spell;

static Spell sp[5] = {
    { "Fireball",   3, { KA, KA, KB },     2.0f, 1.0f, 0.4f, 0.1f, 0, 0 },
    { "Frostbolt",  3, { KB, KX, KY },     2.5f, 0.3f, 0.8f, 1.0f, 0, 0 },
    { "Lightning",  3, { KX, KY, KA },     3.0f, 1.0f, 1.0f, 0.3f, 0, 0 },
    { "Gale Slash", 3, { KY, KY, KB },     3.5f, 0.5f, 1.0f, 0.6f, 0, 0 },
    { "Starfall",   4, { KA, KB, KX, KY }, 7.0f, 0.9f, 0.5f, 1.0f, 0, 0 },
};

#define NSC 10
#define NPB 8
#define NEB 10
#define NST 18
static Scn scn[NSC];
static Shot pb[NPB], eb[NEB];
static float stars[NST][3];
static float CS[8], SN[8];

static int state, fr, flight, hp, score, combo, comboT, xp, lvl, shake, hitflash, dash;
static int buf[4], blen, trick, msgT, bcd;
static float px, py, pvx, bank, tt, lr = 1, lg = 1, lb = 1;
static char msg[32];
static struct { int on, hp, maxhp; float x, y, z, t; } boss;

static float rnd(void) { return (rand() % 1000) / 1000.0f; }
static float cr = 1, cg = 1, cb = 1;
static void col(float r, float g, float b) { cr = r; cg = g; cb = b; }
static void shade(float k) { glColor3f(cr * k, cg * k, cb * k); }
#define V(x, y, z) glVertex3f(x, y, z)
static void tri(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    V(a, b, c); V(d, e, f); V(g, h, i);
}
static void say(const char *s) { snprintf(msg, 32, "%s", s); msgT = 150; }

// ---------- primitives ----------
static void cone(float r, float h) {
    int i;
    glBegin(GL_TRIANGLES);
    for (i = 0; i < 8; i++) {
        int j = (i + 1) & 7;
        shade(0.5f + 0.5f * (0.5f + 0.5f * CS[i]));
        tri(0, h, 0, r * CS[i], 0, r * SN[i], r * CS[j], 0, r * SN[j]);
        shade(0.4f);
        tri(0, 0, 0, r * CS[j], 0, r * SN[j], r * CS[i], 0, r * SN[i]);
    }
    glEnd();
}
static void octa(float s) {
    static const float EQ[4][2] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
    int i;
    glBegin(GL_TRIANGLES);
    for (i = 0; i < 4; i++) {
        int j = (i + 1) & 3;
        shade(0.65f + 0.1f * i);
        tri(0, s, 0, s * EQ[i][0], 0, s * EQ[i][1], s * EQ[j][0], 0, s * EQ[j][1]);
        shade(0.3f + 0.1f * i);
        tri(0, -s, 0, s * EQ[j][0], 0, s * EQ[j][1], s * EQ[i][0], 0, s * EQ[i][1]);
    }
    glEnd();
}
static void box(float w, float h, float d) {
    w *= .5f; h *= .5f; d *= .5f;
    glBegin(GL_QUADS);
    shade(1.0f);  V(-w, h, -d); V(-w, h, d); V(w, h, d); V(w, h, -d);
    shade(0.45f); V(-w, -h, -d); V(w, -h, -d); V(w, -h, d); V(-w, -h, d);
    shade(0.8f);  V(-w, -h, d); V(w, -h, d); V(w, h, d); V(-w, h, d);
    shade(0.65f); V(-w, -h, -d); V(-w, h, -d); V(w, h, -d); V(w, -h, -d);
    shade(0.6f);  V(-w, -h, -d); V(-w, -h, d); V(-w, h, d); V(-w, h, -d);
    shade(0.75f); V(w, -h, -d); V(w, h, -d); V(w, h, d); V(w, -h, d);
    glEnd();
}

// ---------- drawing ----------
static void drawWizard(void) {
    glPushMatrix();
    glTranslatef(px, py, 0);
    if (trick >= 0) { // sparkle ring while tricking
        int i;
        glPushMatrix();
        glRotateZ(tt * 720);
        col(lr, lg, lb);
        for (i = 0; i < 8; i++) {
            glPushMatrix(); glTranslatef(CS[i] * 1.4f, SN[i] * 1.4f, 0); octa(0.1f); glPopMatrix();
        }
        glPopMatrix();
    }
    glRotateZ(bank);
    if (trick == KA) glRotateZ(tt * 360);
    if (trick == KB) glRotateX(-tt * 360);
    if (trick == KX) glRotateY(tt * 360);
    if (trick == KY) { glRotateZ(tt * 720); glRotateX(-tt * 40); }
    // broom
    col(0.45f, 0.28f, 0.12f);
    glPushMatrix(); glTranslatef(0, -0.35f, 0); box(0.12f, 0.12f, 2.0f); glPopMatrix();
    col(0.85f, 0.7f, 0.3f);
    glPushMatrix(); glTranslatef(0, -0.35f, 1.5f); glRotateX(-90); cone(0.3f, 0.55f); glPopMatrix();
    // robe
    col(0.2f, 0.3f, 0.9f);
    glPushMatrix(); glTranslatef(0, -0.3f, 0); cone(0.5f, 1.1f); glPopMatrix();
    // head, beard, hat
    col(1.0f, 0.8f, 0.65f);
    glPushMatrix(); glTranslatef(0, 0.95f, 0); octa(0.22f); glPopMatrix();
    col(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glTranslatef(0, 0.7f, -0.12f); glRotateX(180); cone(0.17f, 0.4f); glPopMatrix();
    col(0.5f, 0.15f, 0.75f);
    glPushMatrix(); glTranslatef(0, 1.1f, 0); box(0.95f, 0.05f, 0.95f);
    glTranslatef(0, 0.02f, 0); glRotateX(-12); cone(0.36f, 0.85f); glPopMatrix();
    // staff with glowing orb
    col(0.5f, 0.35f, 0.2f);
    glPushMatrix(); glTranslatef(0.5f, 0.2f, -0.5f); glRotateX(-20); box(0.06f, 1.3f, 0.06f);
    glTranslatef(0, 0.7f, 0); col(lr, lg, lb); octa(0.15f + 0.03f * (fr & 7) / 7.0f); glPopMatrix();
    glPopMatrix();
}

static void drawBoss(void) {
    static const float BC[4][3] = { { 0.8f, 0.15f, 0.15f }, { 0.2f, 0.7f, 0.3f }, { 0.8f, 0.5f, 0.1f }, { 0.55f, 0.2f, 0.8f } };
    const float *c = BC[(lvl - 1) & 3];
    float flap = ((fr % 40) < 20 ? (fr % 20) : 20 - (fr % 20)) * 2.0f - 20.0f;
    glPushMatrix();
    glTranslatef(boss.x, boss.y, boss.z);
    if (hitflash > 0) col(1, 1, 1); else col(c[0], c[1], c[2]);
    glPushMatrix(); glScalef(1.0f, 0.8f, 1.3f); octa(2.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0, 2.2f); octa(1.0f); glPopMatrix(); // head
    glPushMatrix(); glTranslatef(0, 0, -2.5f); glRotateX(-90); cone(0.8f, 3.0f); glPopMatrix(); // tail
    glPushMatrix(); glTranslatef(1.5f, 0.3f, 0); glRotateZ(-90 + flap); cone(1.4f, 4.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.5f, 0.3f, 0); glRotateZ(90 - flap); cone(1.4f, 4.0f); glPopMatrix();
    col(0.95f, 0.9f, 0.7f);
    glPushMatrix(); glTranslatef(0.5f, 1.0f, 2.2f); cone(0.2f, 0.9f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.5f, 1.0f, 2.2f); cone(0.2f, 0.9f); glPopMatrix();
    col(1, 0.1f, 0.1f);
    glPushMatrix(); glTranslatef(0.4f, 0.3f, 3.0f); octa(0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.4f, 0.3f, 3.0f); octa(0.18f); glPopMatrix();
    glPopMatrix();
}

static void drawScene(void) {
    int i;
    // stars + moon
    for (i = 0; i < NST; i++) {
        float k = 0.5f + 0.5f * (((fr / 8) + i) & 1);
        glColor3f(k, k, k);
        glBegin(GL_QUADS);
        V(stars[i][0], stars[i][1], -85); V(stars[i][0] + .5f, stars[i][1], -85);
        V(stars[i][0] + .5f, stars[i][1] + .5f, -85); V(stars[i][0], stars[i][1] + .5f, -85);
        glEnd();
    }
    col(1.0f, 0.95f, 0.75f);
    glPushMatrix(); glTranslatef(-28, 22, -80); glRotateY(fr * 0.3f); octa(9); glPopMatrix();
    // clouds & floating islands
    for (i = 0; i < NSC; i++) {
        glPushMatrix();
        glTranslatef(scn[i].x, scn[i].y, scn[i].z);
        if (scn[i].type == 0) {
            float s = scn[i].s;
            col(0.8f, 0.8f, 1.0f); box(s * 2.0f, s * 0.6f, s * 1.2f);
            col(0.95f, 0.85f, 1.0f);
            glTranslatef(s * 0.5f, s * 0.4f, 0); box(s * 1.2f, s * 0.5f, s * 0.9f);
        } else {
            float s = scn[i].s;
            col(0.25f, 0.65f, 0.25f); box(s * 2.0f, s * 0.25f, s * 2.0f);
            col(0.4f, 0.28f, 0.2f);
            glTranslatef(0, -s * 0.12f, 0); glRotateX(180); cone(s * 0.95f, s * 1.8f);
        }
        glPopMatrix();
    }
}

static void drawShots(void) {
    int i;
    float p = 0.8f + 0.4f * ((fr & 7) / 7.0f);
    for (i = 0; i < NPB; i++) if (pb[i].on) {
        glPushMatrix(); glTranslatef(pb[i].x, pb[i].y, pb[i].z); glRotateY(fr * 12);
        col(pb[i].r, pb[i].g, pb[i].b); octa(0.45f * p);
        glRotateX(45); col(1, 1, 1); octa(0.25f); glPopMatrix();
    }
    for (i = 0; i < NEB; i++) if (eb[i].on) {
        glPushMatrix(); glTranslatef(eb[i].x, eb[i].y, eb[i].z); glRotateY(fr * 9); glRotateX(fr * 6);
        col(1.0f, 0.2f, 0.4f); octa(0.55f * p); col(1, 0.9f, 0.3f); octa(0.25f); glPopMatrix();
    }
}

static void render(void) {
    float sx = shake > 0 ? (rand() % 100 - 50) / 160.0f : 0;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(70, 256.0f / 192.0f, 0.2f, 100);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    gluLookAt(px * 0.6f + sx, py * 0.5f + 2.4f, 8, px * 0.3f, py * 0.3f, -12, 0, 1, 0);
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_ID(1));
    drawScene();
    if (boss.on) drawBoss();
    drawShots();
    if (state != 2 || (fr & 8)) drawWizard();
    glFlush(0);
}

// ---------- game ----------
static void reset(void) {
    int i;
    for (i = 0; i < 5; i++) { sp[i].learned = 0; sp[i].mastery = 0; }
    for (i = 0; i < NPB; i++) pb[i].on = 0;
    for (i = 0; i < NEB; i++) eb[i].on = 0;
    boss.on = 0; lvl = 0; flight = 0; hp = 5; score = 0; combo = 0; comboT = 999; xp = 0;
    blen = 0; trick = -1; px = py = pvx = bank = 0; dash = 0; shake = 0; hitflash = 0; state = 1;
    lr = lg = lb = 1;
    say("Fly! Try A/B/X/Y tricks");
}

static void fireBolt(int s) {
    int i;
    for (i = 0; i < NPB; i++) if (!pb[i].on) {
        float dx = 0, dy = 0, dz = -1.4f, d;
        Shot *b = &pb[i];
        b->x = px; b->y = py; b->z = -1;
        if (boss.on) {
            dx = boss.x - px; dy = boss.y - py; dz = boss.z - b->z;
            d = sqrtf(dx * dx + dy * dy + dz * dz);
            dx = dx / d * 1.4f; dy = dy / d * 1.4f; dz = dz / d * 1.4f;
        }
        b->vx = dx; b->vy = dy; b->vz = dz;
        b->r = sp[s].r; b->g = sp[s].g; b->b = sp[s].b;
        b->dmg = sp[s].dmg * (1.0f + 0.25f * sp[s].mastery) * (1.0f + 0.1f * combo);
        b->on = 1;
        return;
    }
}

static void castSpell(int s) {
    if (!sp[s].learned) {
        sp[s].learned = 1;
        { char t[32]; snprintf(t, 32, "LEARNED %s!", sp[s].name); say(t); }
        xp += 10;
    } else {
        char t[32]; snprintf(t, 32, "%s Lv%d", sp[s].name, sp[s].mastery + 1); say(t);
    }
    if (sp[s].mastery < 9) sp[s].mastery++;
    lr = sp[s].r; lg = sp[s].g; lb = sp[s].b;
    xp += 5; score += 100 * combo;
    fireBolt(s);
    blen = 0;
}

static void doTrick(int k) {
    int s, i;
    trick = k; tt = 0;
    if (comboT > 70) combo = 0;
    combo++; comboT = 0; xp++; score += 10 * combo;
    if (blen == 4) { for (i = 0; i < 3; i++) buf[i] = buf[i + 1]; blen = 3; }
    buf[blen++] = k;
    for (s = 4; s >= 0; s--) {
        int ok = blen >= sp[s].len;
        for (i = 0; ok && i < sp[s].len; i++) ok = (buf[blen - sp[s].len + i] == sp[s].seq[i]);
        if (ok) { castSpell(s); break; }
    }
}

static void spawnOrb(float vxo) {
    int i;
    for (i = 0; i < NEB; i++) if (!eb[i].on) {
        float dx = px - boss.x + vxo, dy = py - boss.y, dz = 0 - boss.z;
        float d = sqrtf(dx * dx + dy * dy + dz * dz), sp_ = 0.45f + 0.04f * lvl;
        eb[i].x = boss.x; eb[i].y = boss.y; eb[i].z = boss.z + 2;
        eb[i].vx = dx / d * sp_; eb[i].vy = dy / d * sp_; eb[i].vz = dz / d * sp_;
        eb[i].dmg = 0; eb[i].on = 1;
        return;
    }
}

static void update(int dn, int hd) {
    int i, dirx = ((hd & KEY_RIGHT) ? 1 : 0) - ((hd & KEY_LEFT) ? 1 : 0);
    int diry = ((hd & KEY_UP) ? 1 : 0) - ((hd & KEY_DOWN) ? 1 : 0);
    int inv;
    px += dirx * 0.14f; py += diry * 0.12f;
    if (dn & KEY_L) { pvx = -0.55f; dash = 12; }
    if (dn & KEY_R) { pvx = 0.55f; dash = 12; }
    px += pvx; pvx *= 0.85f; if (dash > 0) dash--;
    if (px > 4.5f) px = 4.5f; if (px < -4.5f) px = -4.5f;
    if (py > 2.8f) py = 2.8f; if (py < -2.5f) py = -2.5f;
    bank += ((-dirx * 25 - pvx * 60) - bank) * 0.2f;
    for (i = 0; i < 4; i++) if (dn & KEYS[i]) doTrick(i);
    if (trick >= 0) { tt += 1.0f / 22; if (tt >= 1) { trick = -1; tt = 0; } }
    comboT++;
    if (comboT > 70) { combo = 0; blen = 0; }
    if (shake > 0) shake--;
    if (hitflash > 0) hitflash--;
    if (msgT > 0) msgT--;
    inv = (trick >= 0) || dash > 0;
    // scenery
    for (i = 0; i < NSC; i++) {
        scn[i].z += 0.45f;
        if (scn[i].z > 10) {
            scn[i].z = -60 - rnd() * 20;
            scn[i].x = (rnd() < 0.5f ? -1 : 1) * (7 + rnd() * 14);
            scn[i].y = -9 + rnd() * 14; scn[i].s = 1.5f + rnd() * 2.5f; scn[i].type = rand() & 1;
        }
    }
    // boss
    flight++;
    if (!boss.on && flight > 1500) {
        lvl++; boss.on = 1; boss.maxhp = boss.hp = 10 + 8 * lvl; boss.z = -24; boss.t = 0; bcd = 90;
        say("A BOSS APPROACHES!");
    }
    if (boss.on) {
        boss.t += 0.02f + 0.004f * lvl;
        boss.x = sinf(boss.t) * 3.6f; boss.y = cosf(boss.t * 1.3f) * 1.6f;
        if (--bcd <= 0) {
            bcd = 78 - 9 * lvl; if (bcd < 28) bcd = 28;
            spawnOrb(0);
            if (lvl >= 2) { spawnOrb(2.5f); spawnOrb(-2.5f); }
        }
    }
    for (i = 0; i < NEB; i++) if (eb[i].on) {
        float dx, dy, dz;
        eb[i].x += eb[i].vx; eb[i].y += eb[i].vy; eb[i].z += eb[i].vz;
        dx = eb[i].x - px; dy = eb[i].y - py; dz = eb[i].z;
        if (eb[i].z > 7) eb[i].on = 0;
        else if (dz > -1.2f && dz < 1.2f && dx * dx + dy * dy < 1.5f) {
            if (inv) {
                if (eb[i].dmg == 0) { eb[i].dmg = 1; score += 50 * (combo + 1); xp += 2; say("NEAR MISS! +bonus"); }
            } else {
                eb[i].on = 0; hp--; combo = 0; shake = 14; say("OUCH!");
                if (hp <= 0) { state = 2; say("GAME OVER - START"); }
            }
        }
    }
    for (i = 0; i < NPB; i++) if (pb[i].on) {
        float dx, dy, dz;
        pb[i].x += pb[i].vx; pb[i].y += pb[i].vy; pb[i].z += pb[i].vz;
        if (pb[i].z < -45) pb[i].on = 0;
        if (boss.on && pb[i].on) {
            dx = pb[i].x - boss.x; dy = pb[i].y - boss.y; dz = pb[i].z - boss.z;
            if (dx * dx + dy * dy + dz * dz < 9.0f) {
                pb[i].on = 0; boss.hp -= (int)pb[i].dmg + 1; hitflash = 6; score += (int)(pb[i].dmg * 20);
                if (boss.hp <= 0) {
                    int k;
                    boss.on = 0; flight = 0; xp += 40; score += 500 * lvl; if (hp < 5) hp++;
                    for (k = 0; k < NEB; k++) eb[k].on = 0;
                    say("BOSS DEFEATED! +1 HP");
                }
            }
        }
    }
}

// ---------- HUD on bottom screen ----------
static void line(int row, const char *s) { iprintf("\x1b[%d;0H%-32s", row, s); }

static void hud(void) {
    char t[48]; int i;
    if (state == 0) {
        line(2, "         E L E W I Z");
        line(4, "  Fly, flip and cast combos!");
        line(6, " A/B/X/Y  air tricks");
        line(7, " D-Pad    fly      L/R dash");
        line(8, " Chain tricks to find spells.");
        line(9, " Dodge bosses, learn & master");
        line(10, " spells to defeat them.");
        line(13, "      Press START");
        return;
    }
    snprintf(t, 48, "SCORE %-7d  MEMORY %d", score, xp); line(0, t);
    for (i = 0; i < 5; i++) t[i] = i < hp ? '*' : '-';
    t[5] = 0; { char u[48]; snprintf(u, 48, "HP [%s]  COMBO x%-3d", t, combo); line(1, u); }
    if (boss.on) {
        char bar[17]; int n = boss.hp * 16 / boss.maxhp, k;
        if (n < 0) n = 0;
        for (k = 0; k < 16; k++) bar[k] = k < n ? '#' : '.';
        bar[16] = 0; snprintf(t, 48, "BOSS %d [%s]", lvl, bar); line(2, t);
    } else {
        snprintf(t, 48, "Next boss in %d s", (1500 - flight) / 60 + 1); line(2, t);
    }
    line(4, "---------- SPELLBOOK ----------");
    for (i = 0; i < 5; i++) {
        char sq[12]; int k, shown = sp[i].learned ? sp[i].len : xp / (15 + 10 * i);
        if (shown > sp[i].len) shown = sp[i].len;
        for (k = 0; k < sp[i].len; k++) { sq[k * 2] = k < shown ? BN[sp[i].seq[k]] : '?'; sq[k * 2 + 1] = ' '; }
        sq[sp[i].len * 2] = 0;
        if (sp[i].learned) snprintf(t, 48, "%-11s %-8s Lv%d", sp[i].name, sq, sp[i].mastery);
        else snprintf(t, 48, "%-11s %-8s", "???", sq);
        line(5 + i, t);
    }
    { // current input buffer
        char b[8]; int k;
        for (k = 0; k < blen; k++) b[k] = BN[buf[k]];
        b[blen] = 0; snprintf(t, 48, "Combo chain: %s", b); line(11, t);
    }
    line(14, msgT > 0 ? msg : "");
    line(20, "A B X Y: tricks  L/R: dash");
    if (state == 2) line(16, "   You fell... press START");
}

int main(void) {
    int i;
    videoSetMode(MODE_0_3D);
    consoleDemoInit();
    glInit();
    glEnable(GL_ANTIALIAS);
    glClearColor(3, 5, 14, 31);
    glClearPolyID(63);
    glClearDepth(0x7FFF);
    glViewport(0, 0, 255, 191);
    for (i = 0; i < 8; i++) { CS[i] = cosf(i * PI / 4); SN[i] = sinf(i * PI / 4); }
    for (i = 0; i < NST; i++) { stars[i][0] = (rnd() - 0.5f) * 140; stars[i][1] = (rnd() - 0.2f) * 60; }
    for (i = 0; i < NSC; i++) {
        scn[i].z = -70 + i * 7; scn[i].x = (i & 1 ? -1 : 1) * (7 + rnd() * 14);
        scn[i].y = -9 + rnd() * 14; scn[i].s = 1.5f + rnd() * 2.5f; scn[i].type = i & 1;
    }
    state = 0; trick = -1; hp = 5;
    while (1) {
        int dn, hd;
        scanKeys(); dn = keysDown(); hd = keysHeld(); fr++;
        if (state == 0 || state == 2) { if (dn & KEY_START) reset(); }
        else update(dn, hd);
        render();
        if ((fr & 3) == 0) hud();
        swiWaitForVBlank();
    }
    return 0;
}
