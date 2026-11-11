/* Fixed-work boundary tracker. Coordinates are metres in the car's ground plane. */
#ifndef MOON_EDGE_CORE_H
#define MOON_EDGE_CORE_H
typedef struct { float steer, goal, wait; int side, tracking; unsigned samples; } EdgeCore;
typedef int (*EdgeSample)(void *,float,float,float *);
static inline float ec_abs(float x){return x<0?-x:x;}
static inline int ec_finite(float x){return x==x&&x>=-1000000&&x<=1000000;}
static inline float ec_clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static inline void ec_reset(EdgeCore *e){*e=(EdgeCore){0};}
/* Reject shallow residual dust and multiple transitions rather than jumping
 * between islands or following the two sides of a narrow cleared strip. */
static inline int ec_row(EdgeCore *e,EdgeSample sample,void *ctx,float ahead,float width,float *edge,int *side){
    float span=ec_clamp(width*.5f+2,3,7),start=-span,step=span/6,previous=0,left=0,right=0;
    int previous_kind=0,count=0,found_side=0;
    if(e->tracking){start=-e->side*(width*.5f-.15f)-1.25f;step=2.5f/12;}
    for(int i=0;i<13;i++){
        float x=start+step*i,h=0;e->samples++;
        if(!sample(ctx,ahead,x,&h)||!ec_finite(h)||h<0||h>20)return 0;
        int kind=h<=.06f?-1:h>=.12f?1:0;
        if(kind&&previous_kind&&kind!=previous_kind){
            count++;left=previous;right=x;found_side=kind;
        }
        /* An ambiguous band keeps its last definite sample, bounded by the
         * whole row. A broad shallow gradient is rejected below. */
        if(kind){previous=x;previous_kind=kind;}
    }
    if(count!=1||right-left>step*2.1f||(e->tracking&&found_side!=e->side))return 0;
    for(int i=0;i<3;i++){
        float x=(left+right)*.5f,h=0;e->samples++;
        if(!sample(ctx,ahead,x,&h)||!ec_finite(h)||h<0||h>20)return 0;
        if((h>=.09f?1:-1)==found_side)right=x;else left=x;
    }
    *edge=(left+right)*.5f;*side=found_side;return 1;
}
/* Returns -1 on edge loss/unsupported geometry. No catch-up sampling after a
 * slow frame: the caller releases cruise instead of using stale steering. */
static inline int ec_tick(EdgeCore *e,float dt,float speed,float width,EdgeSample sample,void *ctx){
    if(!ec_finite(dt)||dt<=0||dt>.06f||!ec_finite(speed)||speed<0||speed>15||!ec_finite(width)||width<.5f||width>10)return -1;
    e->wait-=dt;
    if(e->wait<=0){
        float near=ec_clamp(2+speed*.25f,3,6),gap=ec_clamp(speed*.6f,3,7),a,b;int sa,sb;
        e->wait=.1f;e->samples=0;
        if(!ec_row(e,sample,ctx,near,width,&a,&sa)||!ec_row(e,sample,ctx,near+gap,width,&b,&sb)||sa!=sb)return -1;
        float desired=-sa*(width*.5f-.15f),slope=(b-a)/gap,error=a-desired;
        if(ec_abs(slope)>.55f||ec_abs(error)>1.25f)return -1;
        e->goal=ec_clamp(.38f*error+1.25f*slope,-.38f,.38f);e->side=sa;e->tracking=1;
    }
    float change=ec_clamp(e->goal-e->steer,-dt*.9f,dt*.9f);
    e->steer+=change;return 1;
}
#endif
