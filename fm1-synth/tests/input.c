/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../firmware/key_confirm.h"
int main(void){
 uint32_t held=0,a=1u<<19,b=1u<<7;
 /* The observed analog-only key 19 event cannot start a note. */
 held=fms_confirm_keys(a,0,held);assert(!held);
 held=fms_confirm_keys(0,0,held);assert(!held);
 /* A real press can arrive through either sensor first. */
 held=fms_confirm_keys(0,a,held);assert(!held);
 held=fms_confirm_keys(a,a,held);assert(held==a);
 /* A contact bounce must not cut an already accepted note. */
 held=fms_confirm_keys(a,0,held);assert(held==a);
 /* A second unconfirmed key stays silent while the first sustains. */
 held=fms_confirm_keys(a|b,a,held);assert(held==a);
 held=fms_confirm_keys(a|b,a|b,held);assert(held==(a|b));
 /* Analog release, and a later false retrigger while contacts remain open. */
 held=fms_confirm_keys(b,a|b,held);assert(held==b);
 held=fms_confirm_keys(a|b,b,held);assert(held==b);
 held=fms_confirm_keys(0,0,held);assert(!held);
 puts("Input: false press, sensor ordering, chord, bounce, release and retrigger checks passed");
}
