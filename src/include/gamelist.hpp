/*
 * HyperCore - Dynamic Game List & Foreground Game Detector Header
 * Author: @itswill00
 */

#ifndef HYPERCORE_GAMELIST_HPP
#define HYPERCORE_GAMELIST_HPP

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void load_gamelist(void);
int is_game_in_foreground(char *out_game_name, size_t max_len, profile_t *out_profile);

#ifdef __cplusplus
}
#endif

#endif /* HYPERCORE_GAMELIST_HPP */
