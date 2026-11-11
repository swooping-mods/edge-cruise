/* Cruise supplies speed; this input producer supplies bounded edge steering. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/input.h"
#include "api.h"
#include "store.h"
#include "edge_core.h"
#include "ownership.h"
#include "../vehicle-control-framework/sdk/client.h"
static const MoonControls *controls;
#include "../shop-framework/client.h"
#include "../hints-framework/sdk/client.h"
int _fltused=0;
void *memcpy(void *to,const void *from,U64 n){dcopy(to,from,n);return to;}
#define EDGE_KEY "edge-cruise"
#define EDGE_PRICE 350
static struct {
    PTR world_f,mounted_f,steer_f,presentation_f,throttle_f,brake_f,boost_f;
    PTR owner,driven,world_m,storage,cruise,fitted,set_cruise,grounded,cleaning,width,speed;
    PTR transform,position,forward,right,dust,dt,frame,focus,typing,blocked,menu,restoring,alive;
    PTR joined,host,local,player,wallet,charge,decimal_int,decimal_long,save_root;
    void (*set_field)(PTR,PTR,void *);const unsigned short *(*chars)(PTR);int (*length)(PTR);
    short (*key)(int);PTR (*foreground)(void);DWORD (*window_pid)(PTR,DWORD *);
    U64 (*clock)(void);U64 last_tick;
    const MoonShop *shop;const MoonHints *hints;
    DRoot world,walker,car;DWORD thread;int last_frame,ready,invalid,enabled,was_eligible,injected;
    int authority_set,authority,joined_initial,suspended;U64 host_initial;
    char root[SS_ROOT_MAX+1],car_id[EC_ID];EdgeSave save;EdgeCore core;
    float last_steer,sample_speed,sample_width;DVec origin,fwd,lateral;
    const char *status;
} edge;
static int edge_alive(PTR obj){int ok=1;void *a[]={obj};PTR v=obj?call(edge.alive,0,a,&ok):0;return ok&&v&&*(unsigned char *)unbox(v);}
static int edge_string(PTR obj,char *out,unsigned cap){
    if(!obj||!cap)return 0;int n=edge.length(obj);if(n<0||(unsigned)n>=cap)return 0;
    const unsigned short *s=edge.chars(obj);for(int i=0;i<n;i++){if(s[i]<32||s[i]>=127)return 0;out[i]=(char)s[i];}out[n]=0;return 1;
}
static int authority(void){
    int ok=1,joined=boolean(edge.joined,0,&ok);U64 mine=0,host=0;
    if(joined){PTR a=call(edge.local,0,0,&ok),b=call(edge.host,0,0,&ok);if(a)dcopy(&mine,unbox(a),8);if(b)dcopy(&host,unbox(b),8);}
    if(!edge.authority_set){edge.authority_set=1;edge.joined_initial=joined;edge.host_initial=host;edge.authority=ok&&(!joined||(mine&&mine==host));}
    if(!ok||joined!=edge.joined_initial||(joined&&host!=edge.host_initial))edge.suspended=1;
    return edge.authority&&!edge.suspended;
}
static int focused(void){int ok=1;DWORD pid=0;edge.window_pid(edge.foreground(),&pid);return pid==GetCurrentProcessId()&&boolean(edge.focus,0,&ok)&&!boolean(edge.typing,0,&ok)&&ok;}
static void release_steer(PTR car){
    /* Vanilla Update can already have supplied a fresh manual value. */
    if(edge.injected&&edge_alive(car)){float value=0;field_value(car,edge.steer_f,&value);if(value==edge.last_steer){float zero=0;edge.set_field(car,edge.steer_f,&zero);edge.set_field(car,edge.presentation_f,&zero);}}
    edge.injected=0;
}
static void stop(const char *reason,int cruise_off){
    PTR car=dget(edge.car);release_steer(car);if(controls&&car)controls->release("edge-cruise",car);
    if(edge.enabled&&cruise_off&&edge_alive(car)){int ok=1;unsigned char no=0;void *a[]={&no};call(edge.set_cruise,car,a,&ok);}
    edge.enabled=0;edge.last_tick=0;ec_reset(&edge.core);edge.status=reason;
}
static void drop(void){
    stop(0,1);edge.hints->clear(EDGE_KEY);ddrop(&edge.car);ddrop(&edge.world);ddrop(&edge.walker);
    dzero(&edge.save,sizeof(edge.save));edge.car_id[0]=edge.root[0]=0;edge.ready=edge.invalid=0;
    edge.authority_set=edge.authority=edge.suspended=edge.was_eligible=0;
}
static int context(PTR world,PTR walker){
    if(edge.thread&&edge.thread!=GetCurrentThreadId())return 0;edge.thread=GetCurrentThreadId();
    if(edge.world&&(world!=dget(edge.world)||walker!=dget(edge.walker)||!edge_alive(world)))drop();
    if(!edge_alive(world)||!edge_alive(walker))return 0;
    int ok=1;if(boolean(edge.restoring,world,&ok)||!ok)return 0;
    if(!edge.world){
        edge.world=dhold(world);edge.walker=dhold(walker);if(!edge.world||!edge.walker){drop();return 0;}
        PTR path=call(edge.save_root,world,0,&ok);unsigned size=0;
        if(!ok||!edge_string(path,edge.root,sizeof(edge.root)))edge.invalid=1;
        else if(ss_load(edge.root,(unsigned char *)&edge.save,sizeof(edge.save),&size))edge.invalid=size!=sizeof(edge.save)||!ec_save_valid(&edge.save);
        else if(ss.last_error!=SS_NOT_FOUND)edge.invalid=1;
        edge.ready=1;(void)authority();
    }return 1;
}
static int car_valid(PTR world,PTR walker){
    PTR car=dget(edge.car);int ok=1;char id[EC_ID];
    if(!edge_alive(car)||!ec_id_valid(edge.car_id)||!boolean(edge.fitted,car,&ok))return 0;
    PTR w=call(edge.world_m,car,0,&ok),s=call(edge.storage,car,0,&ok);
    (void)walker;return ok&&w==world&&edge_string(s,id,sizeof(id))&&ec_equal(id,edge.car_id);
}
static unsigned offer_state(void *world,void *walker){
    if(!context(world,walker)||!authority()||edge.invalid||!car_valid(world,walker))return MS_BLOCKED;
    if(ec_owned(&edge.save,edge.car_id))return MS_AVAILABLE|MS_OWNED;
    return edge.save.count<EC_CARS?MS_AVAILABLE:MS_BLOCKED;
}
static long long balance(PTR world,PTR player,int *ok){void *a[]={player};PTR money=call(edge.wallet,world,a,ok);if(!money){*ok=0;return 0;}void *b[]={unbox(money)};PTR n=call(edge.decimal_long,0,b,ok);if(!n){*ok=0;return 0;}return *(long long *)unbox(n);}
static int charge(PTR world,PTR player,int delta){int ok=1;unsigned char decimal[16]={0};void *a[]={&delta};call(edge.decimal_int,decimal,a,&ok);void *b[]={player,decimal};if(ok)call(edge.charge,world,b,&ok);return ok;}
static int purchase(void *world,void *walker){
    if(offer_state(world,walker)!=MS_AVAILABLE||!focused())return 0;
    int ok=1;PTR player=call(edge.player,0,0,&ok);long long before=balance(world,player,&ok);
    if(!ok||before<EDGE_PRICE||!charge(world,player,-EDGE_PRICE))return 0;
    long long after=balance(world,player,&ok);if(!ok||after!=before-EDGE_PRICE){edge.invalid=1;return -1;}
    EdgeSave next=edge.save;
    if(!ec_grant(&next,edge.car_id)||!ss_save(edge.root,(const unsigned char *)&next,sizeof(next))){
        int refunded=charge(world,player,EDGE_PRICE);if(!refunded||balance(world,player,&ok)!=before||!ok)edge.invalid=1;
        edge.status="Upgrade save failed";return -1;
    }
    edge.save=next;edge.status="Edge Cruise fitted";return 1;
}
static int update(int id,Context *c){
    (void)id;int ok=1,frame=integer(edge.frame,0,&ok);if(!ok||frame==edge.last_frame)return 0;edge.last_frame=frame;
    PTR walker=(PTR)c->rcx,world=0,mounted=0;field_value(walker,edge.world_f,&world);
    if(!context(world,walker)){(void)moon_input_action("edge-cruise.toggle","J",0);stop("Edge Cruise paused",1);edge.was_eligible=0;edge.hints->clear(EDGE_KEY);return 0;}
    field_value(walker,edge.mounted_f,&mounted);
    if(edge_alive(mounted)&&call(edge.owner,mounted,0,&ok)==walker&&boolean(edge.driven,mounted,&ok)&&ok){
        if(mounted!=dget(edge.car)){
            stop(0,1);ddrop(&edge.car);edge.car_id[0]=0;edge.car=dhold(mounted);
            PTR s=call(edge.storage,mounted,0,&ok);if(!ok||!edge_string(s,edge.car_id,sizeof(edge.car_id)))edge.car_id[0]=0;
        }
    }
    int eligible=ok&&mounted==dget(edge.car)&&edge_alive(mounted)&&!edge.invalid&&authority()&&ec_owned(&edge.save,edge.car_id)&&focused()&&!boolean(edge.blocked,walker,&ok)&&!boolean(edge.menu,walker,&ok)&&ok;
    MoonInputState toggle=moon_input_action("edge-cruise.toggle","J",eligible);
    if(!eligible){stop(0,1);edge.hints->clear(EDGE_KEY);edge.was_eligible=0;return 0;}
    if(toggle.pressed&&edge.was_eligible){
        if(edge.enabled)stop("Edge Cruise off",1);
        else if(boolean(edge.cruise,mounted,&ok)&&boolean(edge.cleaning,mounted,&ok)&&ok){if(controls&&!controls->acquire("edge-cruise",mounted))edge.status="Another driving assist is active";else {ec_reset(&edge.core);edge.enabled=1;edge.status="Edge Cruise acquiring";}}
        else edge.status="Enable cleaning and cruise control first";
    }
    edge.was_eligible=1;
    char label[48],hint[192];if(!moon_input_label("edge-cruise.toggle","J",label,sizeof(label)))cat(label,"J");char *p=cat(hint,label);p=cat(p,": ");cat(p,edge.enabled?"Edge Cruise off | Edge following":edge.status?edge.status:"Edge Cruise on");
    edge.hints->publish(EDGE_KEY,walker,hint);return 0;
}
static int sample(void *ctx,float ahead,float lateral,float *height){
    DVec p={edge.origin.x+edge.fwd.x*ahead+edge.lateral.x*lateral,edge.origin.y+edge.fwd.y*ahead+edge.lateral.y*lateral,edge.origin.z+edge.fwd.z*ahead+edge.lateral.z*lateral};
    int ok=1;void *a[]={&p};PTR v=call(edge.dust,ctx,a,&ok);if(!ok||!v)return 0;dcopy(height,unbox(v),4);return 1;
}
static int vehicle_before(int id,Context *c,MoonVehicleInput *input){
    (void)id;if(input)input->valid=0;PTR car=(PTR)c->rcx;if(car!=dget(edge.car)||!edge.enabled)return 0;
    U64 now=edge.clock();if(edge.last_tick&&(now<edge.last_tick||now-edge.last_tick>250)){stop("Edge Cruise paused - cruise stopped",1);return 0;}edge.last_tick=now;
    int ok=1;PTR walker=call(edge.owner,car,0,&ok),world=dget(edge.world);
    if(!ok||walker!=dget(edge.walker)||!edge_alive(world)||!focused()||!authority()||!boolean(edge.driven,car,&ok)||!boolean(edge.cruise,car,&ok)||!boolean(edge.grounded,car,&ok)||!boolean(edge.cleaning,car,&ok)||boolean(edge.blocked,walker,&ok)||boolean(edge.menu,walker,&ok)||boolean(edge.restoring,world,&ok)||!ok){stop("Edge Cruise released",1);return 0;}
    float manual=0;unsigned char brake=0;field_value(car,edge.steer_f,&manual);field_value(car,edge.brake_f,&brake);
    /* Unchanged last injection is possible between two FixedUpdates. A fresh
     * nonzero input from vanilla Update takes ownership immediately. */
    if(brake||(ec_abs(manual)>.08f&&(!edge.injected||ec_abs(manual-edge.last_steer)>.0001f))||
       (edge.key('A')&0x8000)||(edge.key('D')&0x8000)||(edge.key(0x25)&0x8000)||(edge.key(0x27)&0x8000)){
        stop("Manual steering - Edge Cruise off",1);return 0;
    }
    float dt=dnumber(edge.dt,0,&ok);
    /* Transform/speed/width queries run only when a dust scan is due. The
     * intervening physics ticks just slew toward the accepted steering goal. */
    if(edge.core.wait<=dt){
        edge.sample_speed=dnumber(edge.speed,car,&ok);edge.sample_width=dnumber(edge.width,car,&ok);
        PTR tr=call(edge.transform,car,0,&ok);edge.origin=dvector(edge.position,tr,&ok);edge.fwd=dvector(edge.forward,tr,&ok);edge.lateral=dvector(edge.right,tr,&ok);
        float values[]={edge.origin.x,edge.origin.y,edge.origin.z,edge.fwd.x,edge.fwd.y,edge.fwd.z,edge.lateral.x,edge.lateral.y,edge.lateral.z};
        for(int i=0;i<9;i++)if(!ec_finite(values[i]))ok=0;
    }
    if(!ok||ec_tick(&edge.core,dt,edge.sample_speed,edge.sample_width,sample,world)<0){stop("Edge lost or curve too sharp - cruise stopped",1);return 0;}
    edge.last_steer=edge.core.steer;edge.set_field(car,edge.steer_f,&edge.last_steer);edge.set_field(car,edge.presentation_f,&edge.last_steer);edge.injected=1;
    edge.status="Edge following";
    if(input){unsigned char b=0,h=0;input->valid=1;input->steer=edge.last_steer;field_value(car,edge.throttle_f,&input->throttle);field_value(car,edge.brake_f,&b);field_value(car,edge.boost_f,&h);input->brake=b;input->boost=h;}return 0;
}
static int before(int id,Context *c){return id==1?vehicle_before(id,c,0):update(id,c);}
static int edge_propose(void *car,const MoonControlFrame *frame,MoonVehicleInput *input){
    if(car==dget(edge.car)&&edge.enabled&&(frame->manual_active||!controls->owns("edge-cruise",car))){stop("Manual control or another assist - Edge Cruise off",1);return 0;}
    Context c={0};c.rcx=(U64)car;return vehicle_before(1,&c,input);
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(!bind_runtime(domain,game)||!dapi(game)||!ss_bind())return 0;
    edge.set_field=(void(*)(PTR,PTR,void *))GetProcAddress(game,"il2cpp_field_set_value");
    edge.chars=(const unsigned short *(*)(PTR))GetProcAddress(game,"il2cpp_string_chars");edge.length=(int(*)(PTR))GetProcAddress(game,"il2cpp_string_length");
    PTR user=LoadLibraryA("user32.dll");edge.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");edge.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");edge.window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");
    if(!edge.set_field||!edge.chars||!edge.length||!edge.key||!edge.foreground||!edge.window_pid)return 0;
    edge.clock=(U64(*)(void))GetProcAddress(GetModuleHandleA("kernel32.dll"),"GetTickCount64");if(!edge.clock)return 0;
#define B(F,NS,T,N,S) edge.F=dmethod(NS,T,N,S);if(!edge.F)return 0
#define V(F,N) B(F,"MoonCleaners","MoonPrototypeVehicle",N,"")
    V(owner,"get_Owner");V(driven,"get_IsDriven");V(world_m,"get_World");V(storage,"get_StorageId");V(cruise,"get_CruiseOn");V(fitted,"get_CruiseFitted");V(grounded,"get_Grounded");V(cleaning,"get_Cleaning");V(width,"get_CleaningWidth");V(speed,"get_ForwardSpeed");
    B(set_cruise,"MoonCleaners","MoonPrototypeVehicle","SetCruise","System.Boolean");
    B(transform,"UnityEngine","Component","get_transform","");B(position,"UnityEngine","Transform","get_position","");B(forward,"UnityEngine","Transform","get_forward","");B(right,"UnityEngine","Transform","get_right","");
    B(dust,"MoonCleaners","MoonWorld","VisibleDustHeight","UnityEngine.Vector3");B(dt,"UnityEngine","Time","get_fixedDeltaTime","");B(frame,"UnityEngine","Time","get_frameCount","");
    B(focus,"UnityEngine","Application","get_isFocused","");B(typing,"MoonCleaners","MoonInput","get_Typing","");B(blocked,"MoonCleaners","MoonWalker","get_UIBlocked","");B(menu,"MoonCleaners","MoonWalker","get_MenuBlocked","");B(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");B(alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");
    B(joined,"MoonCleaners","MoonCrew","get_Joined","");B(host,"MoonCleaners","MoonCrew","get_HostId","");B(local,"Steamworks","SteamUser","GetSteamID","");B(player,"MoonCleaners","MoonCrew","get_LocalPlayerId","");
    B(wallet,"MoonCleaners","MoonWorld","WalletDollars","System.String");B(charge,"MoonCleaners","MoonWorld","ChargeBuild","System.String;System.Decimal");B(decimal_int,"System","Decimal",".ctor","System.Int32");B(decimal_long,"System","Decimal","ToInt64","System.Decimal");B(save_root,"MoonCleaners","MoonWorld","get_SaveDirectory","");
#undef V
#undef B
    PTR v=find_class("MoonCleaners","MoonPrototypeVehicle"),w=find_class("MoonCleaners","MoonWalker");
    int (*offset)(PTR)=(int(*)(PTR))GetProcAddress(game,"il2cpp_field_get_offset");if(!v||!w||!offset)return 0;
#define F(T,N,NAME,OFFSET) edge.N=get_field(T,NAME);if(!edge.N||offset(edge.N)!=OFFSET)return 0
    F(v,steer_f,"steer",284);F(v,presentation_f,"<Steering>k__BackingField",116);F(v,throttle_f,"throttle",280);F(v,brake_f,"brake",301);F(v,boost_f,"boost",303);
#undef F
    edge.world_f=get_field(w,"world");edge.mounted_f=get_field(w,"mountedVehicle");if(!edge.world_f||!edge.mounted_f)return 0;
    edge.shop=ms_connect(domain,game);edge.hints=mh_connect(domain,game);if(!edge.shop||!edge.hints)return 0;
    MoonShopDefinition offer={MS_ABI,sizeof(offer),EDGE_KEY,"Edge Cruise Upgrade","Fits your last driven car ($350). Board a cruise-equipped car once, then buy here. Enable cleaning and vanilla cruise, then J near a cleared dust edge. Best on straight lines and gentle curves. Manual steering or edge loss stops assist and cruise.",EDGE_PRICE,offer_state,purchase};
    if(!edge.shop->register_item(&offer))return 0;edge.last_frame=-1;
    controls=mc_connect(domain,game);if(!controls)return 0;
    MoonControlProvider provider={MC_ABI,sizeof(provider),"edge-cruise",MC_ASSIST,10,edge_propose};if(!controls->register_provider(&provider))return 0;
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;
    (void)integer;(void)num;(void)dtype;(void)darray;return install_hooks();
}
