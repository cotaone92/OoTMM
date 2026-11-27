#include "Custom_DekuBubble.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)
#define THIS ((DekuBubble*)thisx)

static ColliderCylinderInit sColliderInit = {
    {
        COL_MATERIAL_HIT0,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK2,
        { 0x00000008, 0x00, 0x00 },
        { 0x00000000, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 12, 12, 0, { 0, 0, 0 } },
};

static void DekuBubble_Init(Actor* thisx, PlayState* play)
{
    DekuBubble* this = THIS;

    u8 charge = (u8)this->actor.params;

    if (charge > 30)
        charge = 30;
    this->timer = 99;
    this->alpha = 220;
    this->actor.gravity = 0.0f;
    this->actor.speed = 8.0f + (charge * 0.18f);
    Actor_SetScale(&this->actor, 0.0045f + (charge * 0.00005f));

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sColliderInit);
}

static void DekuBubble_Destroy(Actor* thisx, PlayState* play)
{
    DekuBubble* this = THIS;
    Collider_DestroyCylinder(play, &this->collider);
}

static void DekuBubble_Update(Actor* thisx, PlayState* play)
{
    DekuBubble* this = THIS;

    if (this->timer > 0)
        this->timer--;
    if (this->timer <= 0 || (this->collider.base.atFlags & AT_HIT))
    {
        Actor_Kill(&this->actor);
        return;
    }

    Actor_MoveXZGravity(&this->actor);
    Actor_UpdateBgCheckInfo(
        play,
        &this->actor,
        8.0f,
        8.0f,
        8.0f,
        UPDBGCHECKINFO_FLAG_WALL | UPDBGCHECKINFO_FLAG_FLOOR | UPDBGCHECKINFO_FLAG_CEILING
    );

    if (this->actor.bgCheckFlags & (BGCHECKFLAG_WALL | BGCHECKFLAG_GROUND | BGCHECKFLAG_CEILING))
    {
        Actor_Kill(&this->actor);
        return;
    }

    Collider_UpdateCylinder(&this->actor, &this->collider);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
}

static void DekuBubble_Draw(Actor* thisx, PlayState* play)
{
    DekuBubble* this = THIS;

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 145, 235, 255, this->alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, 30, 125, 255, 0);
    Matrix_Translate(
        this->actor.world.pos.x,
        this->actor.world.pos.y,
        this->actor.world.pos.z,
        MTXMODE_NEW
    );
    Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(play->state.gfxCtx),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    /* Temporary OoT translucent effect geometry; replace with imported MM bubble mesh. */
    gSPDisplayList(POLY_XLU_DISP++, 0x04052a10);
    CLOSE_DISPS();
}

ActorProfile DekuBubble_InitVars = {
    ACTOR_CUSTOM_DEKU_BUBBLE,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(DekuBubble),
    DekuBubble_Init,
    DekuBubble_Destroy,
    DekuBubble_Update,
    DekuBubble_Draw,
};

OVL_INFO_ACTOR(ACTOR_CUSTOM_DEKU_BUBBLE, DekuBubble_InitVars);
