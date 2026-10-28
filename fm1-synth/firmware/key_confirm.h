/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
#include <stdint.h>
/* An analog velocity event alone must not start a note. Confirm a new press
 * against the independently debounced contact matrix. Once accepted, retain
 * the existing analog release behaviour even if the digital contact bounces. */
static inline uint32_t fms_confirm_keys(uint32_t sensed,uint32_t contacts,uint32_t held){
 return sensed & (contacts | held);
}
