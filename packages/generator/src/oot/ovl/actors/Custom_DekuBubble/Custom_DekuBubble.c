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
    this->expansion = charge * (0.01f / 1.0f);
    /* Gentle lift and drag give the projectile the floaty MM bubble trajectory. */
    this->actor.gravity = -0.015f;
    this->actor.velocity.y = 0.2f;
    this->actor.speed = 8.0f + (charge * 0.18f);
    /* The native OoT bubble mesh is authored at unit scale. */
    Actor_SetScale(&this->actor, 1.0f);

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sColliderInit);
    /* Match the collision volume to the charged visual bubble. */
    this->collider.dim.radius = (s16)(16.0f * (1.0f + this->expansion));
    this->collider.dim.height = (s16)(32.0f * (1.0f + this->expansion));
    this->collider.dim.yShift = (s16)(-16.0f * (1.0f + this->expansion));
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
    if (this->timer <= 0)
    {
        Actor_Kill(&this->actor);
        return;
    }

    /* A shield reflection reverses the flight; ordinary target contact pops it. */
    if (this->collider.base.atFlags & AT_HIT)
    {
        if (this->collider.base.atFlags & AT_BOUNCED)
        {
            this->actor.world.rot.y += 0x8000;
            this->actor.velocity.y = -this->actor.velocity.y;
            this->actor.world.pos = this->actor.prevPos;
            this->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
            this->bounceCount++;
        }
        else
        {
            Actor_Kill(&this->actor);
            return;
        }
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
        if (this->bounceCount >= 2)
        {
            Actor_Kill(&this->actor);
            return;
        }

        /* Rebound from level geometry at reduced horizontal speed. */
        this->actor.world.pos = this->actor.prevPos;
        if (this->actor.bgCheckFlags & BGCHECKFLAG_WALL)
            this->actor.world.rot.y += 0x8000;
        if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
            this->actor.velocity.y = 3.0f;
        else if (this->actor.bgCheckFlags & BGCHECKFLAG_CEILING)
            this->actor.velocity.y = -2.0f;
        this->actor.speed *= 0.75f;
        this->bounceCount++;
    }

    Collider_UpdateCylinder(&this->actor, &this->collider);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
}

static void DekuBubble_Draw(Actor* thisx, PlayState* play)
{
    DekuBubble* this = THIS;

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Translate(
        this->actor.world.pos.x,
        this->actor.world.pos.y,
        this->actor.world.pos.z,
        MTXMODE_NEW
    );
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(1.0f + this->expansion, 1.0f + this->expansion, 1.0f, MTXMODE_APPLY);
    Matrix_RotateZ(DEG_TO_RAD((f32)(99 - this->timer) * 2.0f), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_Finalize(play->state.gfxCtx),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    /* Native OoT bubble object: object_bubble, segment 6, gBubbleDL at 0x1000. */
    gSPDisplayList(POLY_XLU_DISP++, 0x06001000);
    CLOSE_DISPS();
}

ActorProfile DekuBubble_InitVars = {
    ACTOR_CUSTOM_DEKU_BUBBLE,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_BUBBLE,
    sizeof(DekuBubble),
    DekuBubble_Init,
    DekuBubble_Destroy,
    DekuBubble_Update,
    DekuBubble_Draw,
};

OVL_INFO_ACTOR(ACTOR_CUSTOM_DEKU_BUBBLE, DekuBubble_InitVars);
