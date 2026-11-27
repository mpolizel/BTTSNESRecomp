#ifndef BT_EXTRAS_H
#define BT_EXTRAS_H
/* MSU-1 packs, chosen in the launcher and applied in memory: see
 * bt_extras.c. */
#include <stddef.h>
#include <stdint.h>

/* SnesDesktopHostGame.replace_rom: the verified ROM with the MSU-1 patch
 * chosen in the launcher applied, or NULL when it is off. */
uint8_t *BtReplaceRom(const uint8_t *rom, size_t size, size_t *out_size);
/* SnesDesktopHostGame.on_reset: after a save state or rewind frame is
 * loaded, select the MSU-1 track the game now expects. */
void BtExtrasOnReset(void);

#ifdef RECOMP_LAUNCHER
struct RecompLauncherCGameInfo;
/* The launcher's EXTRAS card: the MSU-1 row. */
void BtExtrasConfigureLauncher(struct RecompLauncherCGameInfo *info);
#endif

#endif
