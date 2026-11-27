#ifndef OVL_OOT_CUSTOM_DEKU_BUBBLE_H
#define OVL_OOT_CUSTOM_DEKU_BUBBLE_H

#include <combo.h>

typedef struct DekuBubble {
    /* 0x000 */ Actor actor;
    /* 0x144 */ ColliderCylinder collider;
    /* 0x190 */ s16 timer;
    /* 0x192 */ u8 alpha;
} DekuBubble;

#endif
