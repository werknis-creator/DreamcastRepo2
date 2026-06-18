/*
 * =====================================================
 *  Gra 3D Third-Person na Sega Dreamcast
 *  Wymagane: KallistiOS (KOS) + toolchain sh-elf
 * =====================================================
 *  Sterowanie:
 *    Analog (lewo/prawo) - obrót gracza
 *    Analog (góra/dół)  - chodzenie do przodu/tyłu
 *    Przycisk X         - skok
 *    START              - wyjście z gry
 * =====================================================
 */

#include <kos.h>
#include <math.h>
#include <stdlib.h>

#ifndef PVR_PACK_COLOR
#define PVR_PACK_COLOR(a, r, g, b) \
    ((((uint32)(a)) << 24) | (((uint32)(r)) << 16) | \
     (((uint32)(g)) << 8)  | ((uint32)(b)))
#endif

/* ========== Stałe gry ========== */
#define GRAVITY         0.012f
#define JUMP_FORCE      0.25f
#define MOVE_SPEED      0.08f
#define TURN_SPEED      0.04f
#define CAM_DISTANCE    6.0f
#define CAM_HEIGHT      3.0f
#define ANALOG_DEADZONE 20

/* ========== Struktura gracza ========== */
typedef struct {
    float x, y, z;     /* Pozycja w świecie */
    float rot_y;       /* Obrót wokół osi Y */
    float vy;          /* Prędkość pionowa (skok) */
    int   grounded;    /* Czy stoi na ziemi? */
} Player;

Player player;

/* ========== Prototypy funkcji ========== */
void update_player(cont_state_t *st);
void draw_scene(void);
void draw_cube(float cx, float cy, float cz, float sx, float sy, float sz,
               uint32 base_color, int is_last);
void draw_ground(void);

/* ======================================= */
/*              FUNKCJA MAIN               */
/* ======================================= */
int main(int argc, char **argv) {
    /* Inicjalizacja PVR (PowerVR - GPU Dreamcasta) */
    pvr_init_defaults();

    /* Stan początkowy gracza */
    player.x       = 0.0f;
    player.y       = 0.0f;
    player.z       = 0.0f;
    player.rot_y   = 0.0f;
    player.vy      = 0.0f;
    player.grounded = 1;

    /* ======= Główna pętla gry ======= */
    while (1) {
        /* Pobierz podłączony kontroler */
        maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
        if (!dev) continue;

        cont_state_t *st = (cont_state_t *)maple_dev_status(dev);
        if (!st) continue;

        /* START = wyjście */
        if (st->buttons & CONT_START) break;

        update_player(st);
        draw_scene();
    }

    pvr_shutdown();
    return 0;
}

/* ======================================= */
/*           LOGIKA GRACZA                 */
/* ======================================= */
void update_player(cont_state_t *st) {
    int ax = st->joyx;
    int ay = st->joyy;

    /* Deadzone analoga */
    if (ax > -ANALOG_DEADZONE && ax < ANALOG_DEADZONE) ax = 0;
    if (ay > -ANALOG_DEADZONE && ay < ANALOG_DEADZONE) ay = 0;

    float fx = (float)ax / 128.0f;
    float fy = (float)ay / 128.0f;

    /* Obrót (analog w lewo/prawo) */
    if (ax != 0) {
        player.rot_y -= fx * TURN_SPEED;
    }

    /* Ruch (analog góra/dół) — góra = do przodu */
    if (ay != 0) {
        float move = -fy * MOVE_SPEED;
        player.x += sinf(player.rot_y) * move;
        player.z += cosf(player.rot_y) * move;
    }

    /* Skok (przycisk X) */
    if ((st->buttons & CONT_X) && player.grounded) {
        player.vy      = JUMP_FORCE;
        player.grounded = 0;
    }

    /* Grawitacja */
    if (!player.grounded) {
        player.vy -= GRAVITY;
        player.y  += player.vy;

        if (player.y <= 0.0f) {
            player.y        = 0.0f;
            player.vy       = 0.0f;
            player.grounded = 1;
        }
    }
}

/* ======================================= */
/*              RYSOWANIE                  */
/* ======================================= */
void draw_scene(void) {
    /* --- Oblicz pozycję kamery (za i nad graczem) --- */
    float cam_x = player.x - sinf(player.rot_y) * CAM_DISTANCE;
    float cam_y = player.y + CAM_HEIGHT;
    float cam_z = player.z - cosf(player.rot_y) * CAM_DISTANCE;

    /* --- Ustaw macierz widoku (kamera) --- */
    mat_identity();
    mat_rotate_x(atan2f(CAM_HEIGHT, CAM_DISTANCE));
    mat_rotate_y(-player.rot_y);
    mat_translate(-cam_x, -cam_y, -cam_z);

    /* --- Rozpocznij renderowanie PVR --- */
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_OP_POLY);

    draw_ground();

    /* Gracz (czerwony sześcian) */
    draw_cube(player.x, player.y, player.z, 1.0f, 2.0f, 1.0f, 0x00ff4444, 0);

    /* Obiekty dekoracyjne w otoczeniu */
    draw_cube( 5.0f, 0.0f,  5.0f, 2.0f, 2.0f, 2.0f, 0x0044ff44, 0);
    draw_cube(-5.0f, 0.0f, -5.0f, 2.0f, 2.0f, 2.0f, 0x004444ff, 0);
    draw_cube( 8.0f, 0.0f, -3.0f, 1.5f, 3.0f, 1.5f, 0x00ffff44, 0);
    draw_cube(-8.0f, 0.0f,  3.0f, 1.5f, 3.0f, 1.5f, 0x00ff44ff, 0);
    draw_cube( 0.0f, 0.0f,-10.0f, 1.0f, 4.0f, 1.0f, 0x00884488, 0);
    /* Ostatni obiekt — is_last = 1 */
    draw_cube(10.0f, 0.0f,  0.0f, 3.0f, 1.0f, 3.0f, 0x00888844, 1);

    pvr_list_finish();
    pvr_scene_finish();
}

/* ======================================= */
/*           RYSUJ SZEŚCIAN                */
/* ======================================= */
void draw_cube(float cx, float cy, float cz, float sx, float sy, float sz,
               uint32 base_color, int is_last)
{
    static pvr_poly_cxt_t cxt;
    static pvr_poly_hdr_t hdr;
    static pvr_vertex_t   vert = {0};

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.gen.shading = PVR_SHADE_FLAT;
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    float hx = sx * 0.5f;
    float hz = sz * 0.5f;

    uint8_t cr = (base_color >> 16) & 0xff;
    uint8_t cg = (base_color >>  8) & 0xff;
    uint8_t cb =  base_color        & 0xff;

    /* 8 wierzchołków sześcianu */
    float v[8][3] = {
        {cx - hx, cy,      cz - hz}, /* 0 */
        {cx + hx, cy,      cz - hz}, /* 1 */
        {cx - hx, cy + sy, cz - hz}, /* 2 */
        {cx + hx, cy + sy, cz - hz}, /* 3 */
        {cx - hx, cy,      cz + hz}, /* 4 */
        {cx + hx, cy,      cz + hz}, /* 5 */
        {cx - hx, cy + sy, cz + hz}, /* 6 */
        {cx + hx, cy + sy, cz + hz}  /* 7 */
    };

    /* Indeksy wierzchołków dla każdej ściany (quad jako triangle strip) */
    int faces[6][4] = {
        {0, 1, 2, 3}, /* Front  */
        {4, 5, 6, 7}, /* Back   */
        {0, 4, 2, 6}, /* Left   */
        {1, 5, 3, 7}, /* Right  */
        {2, 3, 6, 7}, /* Top    */
        {4, 5, 0, 1}  /* Bottom */
    };

    /* Wektory normalne ścian */
    float n[6][3] = {
        { 0,  0, -1}, { 0,  0,  1},
        {-1,  0,  0}, { 1,  0,  0},
        { 0,  1,  0}, { 0, -1,  0}
    };

    /* Kierunek światła (znormalizowany) */
    float lx = 0.577f, ly = 0.577f, lz = -0.577f;

    for (int f = 0; f < 6; f++) {
        float dot = n[f][0]*lx + n[f][1]*ly + n[f][2]*lz;
        float bright = fmaxf(0.3f, dot);

        uint8_t fr = (uint8_t)(cr * bright);
        uint8_t fg = (uint8_t)(cg * bright);
        uint8_t fb = (uint8_t)(cb * bright);
        uint32 fc  = PVR_PACK_COLOR(255, fr, fg, fb);

        for (int i = 0; i < 4; i++) {
            int vi = faces[f][i];
            float ox, oy, oz;

            mat_trans_single(v[vi][0], v[vi][1], v[vi][2], &ox, &oy, &oz);

            vert.flags = PVR_CMD_VERTEX;
            if (i == 3) {
                if (f == 5 && is_last)
                    vert.flags = PVR_CMD_VERTEX_EOL;
                else
                    vert.flags = PVR_CMD_VERTEX_EOS;
            }

            vert.x     = ox;
            vert.y     = oy;
            vert.z     = oz;
            vert.u     = 0.0f;
            vert.v     = 0.0f;
            vert.argb  = fc;
            vert.oargb = 0;

            pvr_prim(&vert, sizeof(vert));
        }
    }
}

/* ======================================= */
/*             RYSUJ PODŁOGĘ               */
/* ======================================= */
void draw_ground(void) {
    static pvr_poly_cxt_t cxt;
    static pvr_poly_hdr_t hdr;
    static pvr_vertex_t   vert = {0};

    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.gen.shading = PVR_SHADE_FLAT;
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    float s = 50.0f;
    float gv[4][3] = {
        {-s, 0.0f, -s},
        { s, 0.0f, -s},
        {-s, 0.0f,  s},
        { s, 0.0f,  s}
    };

    uint32 color = PVR_PACK_COLOR(255, 80, 130, 80);

    for (int i = 0; i < 4; i++) {
        float ox, oy, oz;
        mat_trans_single(gv[i][0], gv[i][1], gv[i][2], &ox, &oy, &oz);

        vert.flags = PVR_CMD_VERTEX;
        if (i == 3) vert.flags = PVR_CMD_VERTEX_EOS;

        vert.x     = ox;
        vert.y     = oy;
        vert.z     = oz;
        vert.u     = 0.0f;
        vert.v     = 0.0f;
        vert.argb  = color;
        vert.oargb = 0;

        pvr_prim(&vert, sizeof(vert));
    }
}
