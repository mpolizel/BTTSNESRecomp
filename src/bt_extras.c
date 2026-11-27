/*
 * Battletoads in Battlemaniacs -- MSU-1 packs, applied in memory.
 *
 * Chosen in the launcher's Settings, EXTRAS card: the player picks the
 * folder holding Conn's MSU-1 patch (btbm_msu.ips, or its SD2SNES variant
 * btbm_msu1_sd2snes.ips, which keeps the cartridge's pause jingle) and the
 * pack's btbm_msu1-<N>.pcm tracks (0-23). The cartridge's music then comes
 * from the pack instead, as on an emulator or an FXPak. The patch is checked
 * by its SHA-256 when the folder is chosen, and the card says at once
 * whether the patch and the tracks are recognized. The choice lives in
 * extras.ini beside the executable.
 *
 * When the game starts, after the ROM passed its own SHA-256 check, the host
 * hands it to BtReplaceRom, which applies the patch to a copy in memory --
 * the ROM file is never changed. The patch only replaces the instructions
 * where the game picks a song, starts it and pauses ($9D:80A6, $9D:81ED,
 * $80:C700/C709) and puts its code in free space at $9C:FC60. This works
 * because all of this title's code runs in the interpreter
 * (recomp/bank00.cfg forces the vectors to LLE), so patched bytes are
 * executed as written. Off: the game is the plain build.
 *
 * Save states and the rewind filmstrip do not record the MSU-1 player, so
 * after one is loaded the track the patch last chose ($7F:E000) is selected
 * again from its start.
 */
#include "bt_extras.h"

#include "common_rtl.h"
#include "snes/msu1.h"
#include "rom_patch.h"
#include "sha256.h"
#ifdef RECOMP_LAUNCHER
#include "recomp_launcher.h"
#endif

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kHeader = 0x200 };

typedef struct KnownPatch {
    const char *sha256;  /* of the .ips file */
    const char *name;
} KnownPatch;

static const KnownPatch kPatches[] = {
    { "9f1c1f661bdf1838069b7f93bf8f155703630babfc81ad2cead2288b104f2a62",
      "Conn's MSU-1 patch" },
    { "b79f74bd87d068bd23c1371e37570890769c65e79a59ce52fbcf029e65618a84",
      "Conn's MSU-1 patch (SD2SNES variant)" },
};
enum { kKnown = sizeof(kPatches) / sizeof(kPatches[0]) };

enum { kTrackWram = 0x1E000 /* $7F:E000 */, kTracks = 24 };

static struct {
    bool loaded;
    int msu_on;
    char msu_dir[1024];
} s_set;

static struct {
    int msu;                       /* kPatches index, or -1 */
    char msu_patch_path[1100];
    int msu_tracks;
    char msu_status[512];
    int msu_level;
} s_chk;

static bool s_msu_live;

static void IniPath(char *out, size_t cap)
{
    const char *env = getenv("BT_EXTRAS_INI");   /* tests */
    if (env && *env) { snprintf(out, cap, "%s", env); return; }
    const char *base = SDL_GetBasePath();
    snprintf(out, cap, "%sextras.ini", base ? base : "");
}

static void Trim(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = '\0';
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') ++i;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static void SaveSettings(void)
{
    char path[1100];
    IniPath(path, sizeof(path));
    char buf[2048];
    int n = snprintf(buf, sizeof(buf),
        "; Battletoads in Battlemaniacs extras, set in the launcher (Settings, EXTRAS).\n"
        "[MSU1]\nEnabled = %d\nFolder = %s\n", s_set.msu_on, s_set.msu_dir);
    if (n > 0 && (size_t)n < sizeof(buf))
        SDL_SaveFile(path, buf, (size_t)n);
}

static void CheckMsu(void);

static void LoadSettings(void)
{
    if (s_set.loaded) return;
    s_set.loaded = true;
    char path[1100];
    IniPath(path, sizeof(path));
    size_t size = 0;
    char *data = (char *)SDL_LoadFile(path, &size);
    if (data) {
        char section[32] = "";
        for (char *line = strtok(data, "\n"); line; line = strtok(NULL, "\n")) {
            Trim(line);
            if (!line[0] || line[0] == ';' || line[0] == '#') continue;
            if (line[0] == '[') { snprintf(section, sizeof(section), "%s", line); continue; }
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            char *key = line, *val = eq + 1;
            Trim(key); Trim(val);
            if (!strcmp(section, "[MSU1]")) {
                if (!strcmp(key, "Enabled")) s_set.msu_on = atoi(val) != 0;
                else if (!strcmp(key, "Folder")) snprintf(s_set.msu_dir, sizeof(s_set.msu_dir), "%s", val);
            }
        }
        SDL_free(data);
    }
    CheckMsu();
}

static int Identify(const char *path)
{
    size_t size = 0;
    void *data = SDL_LoadFile(path, &size);
    if (!data) return -2;
    uint8_t digest[32];
    char hex[65];
    sha256_compute((const uint8_t *)data, size, digest);
    SDL_free(data);
    for (int i = 0; i < 32; ++i) snprintf(hex + 2 * i, 3, "%02x", digest[i]);
    for (int k = 0; k < kKnown; ++k)
        if (strcmp(kPatches[k].sha256, hex) == 0) return k;
    return -1;
}

static bool EndsWithI(const char *s, const char *ext)
{
    size_t n = strlen(s), e = strlen(ext);
    return n >= e && SDL_strcasecmp(s + n - e, ext) == 0;
}

static bool IsAscii(const char *s)
{
    for (; *s; ++s) if ((unsigned char)*s >= 0x80) return false;
    return true;
}

static SDL_EnumerationResult SDLCALL MsuVisit(void *ud, const char *dir, const char *name)
{
    (void)ud;
    char path[1100];
    const size_t dl = strlen(dir);
    const bool sep = dl && (dir[dl - 1] == '\\' || dir[dl - 1] == '/');
    snprintf(path, sizeof(path), "%s%s%s", dir, sep ? "" : "/", name);
    if (EndsWithI(name, ".pcm")) { ++s_chk.msu_tracks; return SDL_ENUM_CONTINUE; }
    if (EndsWithI(name, ".ips")) {
        const int k = Identify(path);
        /* The main patch wins over the SD2SNES variant when both are here. */
        if (k >= 0 && (s_chk.msu < 0 || k < s_chk.msu)) {
            s_chk.msu = k;
            snprintf(s_chk.msu_patch_path, sizeof(s_chk.msu_patch_path), "%s", path);
        }
    }
    return SDL_ENUM_CONTINUE;
}

static void CheckMsu(void)
{
    s_chk.msu = -1;
    s_chk.msu_tracks = 0;
    s_chk.msu_patch_path[0] = '\0';
    s_chk.msu_level = 0;
    if (!s_set.msu_dir[0]) {
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "Choose the folder with Conn's MSU-1 patch (btbm_msu.ips) and the pack's "
                 "btbm_msu1-<N>.pcm tracks.");
        return;
    }
    s_chk.msu_level = 2;
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(s_set.msu_dir, &info) || info.type != SDL_PATHTYPE_DIRECTORY) {
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status), "This folder cannot be opened.");
        return;
    }
    if (!IsAscii(s_set.msu_dir)) {
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "The tracks cannot be played from a folder whose path has accented or "
                 "non-English letters. Move the pack to a folder such as C:\\Games\\btmsu "
                 "and choose it again.");
        return;
    }
    SDL_EnumerateDirectory(s_set.msu_dir, MsuVisit, NULL);
    if (s_chk.msu < 0 && s_chk.msu_tracks == 0)
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "No MSU-1 pack here: the folder needs Conn's btbm_msu.ips and the "
                 "btbm_msu1-<N>.pcm tracks.");
    else if (s_chk.msu < 0)
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "%d track(s) found, but not Conn's MSU-1 patch (btbm_msu.ips, checked by its "
                 "SHA-256). It comes in Conn's btbm_msu1_patch.zip: put it in this folder.",
                 s_chk.msu_tracks);
    else if (s_chk.msu_tracks == 0)
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "%s found, but no btbm_msu1-<N>.pcm tracks.", kPatches[s_chk.msu].name);
    else {
        s_chk.msu_level = 1;
        snprintf(s_chk.msu_status, sizeof(s_chk.msu_status),
                 "Recognized: %s and %d music track(s).", kPatches[s_chk.msu].name,
                 s_chk.msu_tracks);
    }
}

#ifdef RECOMP_LAUNCHER
static int MsuGet(void) { LoadSettings(); return s_set.msu_on; }
static void MsuSet(int on) { LoadSettings(); s_set.msu_on = on != 0; SaveSettings(); }
static const char *MsuPath(void) { LoadSettings(); return s_set.msu_dir; }
static void MsuPick(const char *path)
{
    LoadSettings();
    snprintf(s_set.msu_dir, sizeof(s_set.msu_dir), "%s", path);
    CheckMsu();
    s_set.msu_on = s_chk.msu_level == 1;
    SaveSettings();
}
static const char *MsuStatus(int *level) { LoadSettings(); *level = s_chk.msu_level; return s_chk.msu_status; }

static const RecompLauncherHostExtra kExtras[] = {
    {
        .label = "MSU-1 music",
        .help = "Plays an MSU-1 pack's CD-quality soundtrack instead of the cartridge's music. "
                "Choose the folder holding Conn's MSU-1 patch (btbm_msu.ips, in his "
                "btbm_msu1_patch.zip) and the pack's btbm_msu1-<N>.pcm tracks; packs are "
                "shared on the Zeldix forum. Sound effects still come from the console's sound "
                "chip. A change made while playing applies the next time the game starts.",
        .pick_folder = 1,
        .pick_title = "Choose the MSU-1 pack folder",
        .get_enabled = MsuGet, .set_enabled = MsuSet,
        .get_path = MsuPath, .set_path = MsuPick,
        .status = MsuStatus,
    },
};

void BtExtrasConfigureLauncher(RecompLauncherCGameInfo *info)
{
    LoadSettings();
    info->host_extras_title = "EXTRAS";
    info->host_extras = kExtras;
    info->num_host_extras = (int)(sizeof(kExtras) / sizeof(kExtras[0]));
}
#endif

uint8_t *BtReplaceRom(const uint8_t *rom, size_t size, size_t *out_size)
{
    LoadSettings();
    CheckMsu();   /* files may have moved since they were chosen */
    if (s_set.msu_on && s_chk.msu_level != 1)
        fprintf(stderr, "[extras] MSU-1 is on but not used: %s\n", s_chk.msu_status);
    if (!rom || !s_set.msu_on || s_chk.msu_level != 1)
        return NULL;
    size_t patch_size = 0;
    void *patch = SDL_LoadFile(s_chk.msu_patch_path, &patch_size);
    if (!patch) {
        fprintf(stderr, "[extras] cannot read '%s': MSU-1 not used\n", s_chk.msu_patch_path);
        return NULL;
    }
    /* Work on the bare ROM: a copier header is dropped. */
    const size_t head = (size & 0x7FFF) == kHeader ? kHeader : 0;
    uint8_t *out = NULL;
    size_t n = 0;
    RomPatchStatus st = snes_rom_patch_apply(rom + head, size - head, (const uint8_t *)patch,
                                             patch_size, (size_t)16 << 20, &out, &n);
    SDL_free(patch);
    if (st != kRomPatch_Ok) {
        fprintf(stderr, "[extras] the MSU-1 patch could not be applied: %s\n",
                snes_rom_patch_status_text(st));
        free(out);
        return NULL;
    }
    s_msu_live = true;
    /* RtlRegisterGame (right after this) arms the chip from the environment;
     * a folder resolves to the pack's "<name>-<N>.pcm" base. */
#ifdef _WIN32
    _putenv_s("SNESRECOMP_MSU1", s_set.msu_dir);
#else
    setenv("SNESRECOMP_MSU1", s_set.msu_dir, 1);
#endif
    fprintf(stderr, "[extras] applied %s ('%s')\n[MSU-1] pack on: %d track(s) in '%s'\n",
            kPatches[s_chk.msu].name, s_chk.msu_patch_path, s_chk.msu_tracks, s_set.msu_dir);
    *out_size = n;
    return out;
}

/* A save state or rewind frame was loaded: play what the game now expects. */
void BtExtrasOnReset(void)
{
    if (!s_msu_live || !msu1_enabled())
        return;
    const uint8_t track = g_ram[kTrackWram];
    if (track >= kTracks) {
        msu1_write(0x2007, 0);
        return;
    }
    msu1_write(0x2004, track);
    msu1_write(0x2005, 0);
    msu1_write(0x2006, 0xFF);
    msu1_write(0x2007, 3);   /* every track of this pack loops */
}
