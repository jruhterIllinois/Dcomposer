/*
 *  SALSA-Lib - Control Interface
 *
 *  Copyright (c) 2007 by Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_CONTROL_H
#define __ALSA_CONTROL_H

#include "recipe.h"
#include "global.h"
#include "ctl_types.h"

int snd_card_load(int card);
int snd_card_next(int *card);
int snd_card_get_index(const char *name);
int snd_card_get_name(int card, char **name);
int snd_card_get_longname(int card, char **name);

int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode);
int snd_ctl_close(snd_ctl_t *ctl);
int snd_ctl_wait(snd_ctl_t *ctl, int timeout);

#if SALSA_HAS_TLV_SUPPORT
int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size);
int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv);
int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv);
#endif

#include "ctl_macros.h"

#define snd_ctl_elem_id_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_id_sizeof()); memset(*ptr, 0, snd_ctl_elem_id_sizeof()); } while (0)
#define snd_ctl_card_info_alloca(ptr) do { *ptr = alloca(snd_ctl_card_info_sizeof()); memset(*ptr, 0, snd_ctl_card_info_sizeof()); } while (0)
#define snd_ctl_event_alloca(ptr) do { *ptr = alloca(snd_ctl_event_sizeof()); memset(*ptr, 0, snd_ctl_event_sizeof()); } while (0)
#define snd_ctl_elem_list_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_list_sizeof()); memset(*ptr, 0, snd_ctl_elem_list_sizeof()); } while (0)
#define snd_ctl_elem_info_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_info_sizeof()); memset(*ptr, 0, snd_ctl_elem_info_sizeof()); } while (0)
#define snd_ctl_elem_value_alloca(ptr) do { *ptr = alloca(snd_ctl_elem_value_sizeof()); memset(*ptr, 0, snd_ctl_elem_value_sizeof()); } while (0)

#endif /* __ALSA_CONTROL_H */
