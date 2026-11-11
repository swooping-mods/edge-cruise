#ifndef MOON_EDGE_OWNERSHIP_H
#define MOON_EDGE_OWNERSHIP_H
#define EC_CARS 16
#define EC_ID 80
typedef struct {unsigned magic,version,count,check;char ids[EC_CARS][EC_ID];} EdgeSave;
static inline unsigned ec_hash(const EdgeSave *s){
    const unsigned char *p=(const unsigned char *)s;unsigned h=2166136261u;
    for(unsigned i=0;i<sizeof(*s);i++)if(i<12||i>=16){h^=p[i];h*=16777619u;}return h;
}
static inline int ec_id_valid(const char *id){
    if(!id||!id[0])return 0;
    for(unsigned i=0;i<EC_ID;i++){if(!id[i])return 1;if((unsigned char)id[i]<32||(unsigned char)id[i]>=127)return 0;}return 0;
}
static inline int ec_equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static inline int ec_owned(const EdgeSave *s,const char *id){if(!ec_id_valid(id))return 0;for(unsigned i=0;i<s->count&&i<EC_CARS;i++)if(ec_equal(s->ids[i],id))return 1;return 0;}
static inline int ec_save_valid(const EdgeSave *s){
    if(s->magic!=0x45444745||s->version!=1||s->count>EC_CARS||s->check!=ec_hash(s))return 0;
    for(unsigned i=0;i<s->count;i++){
        if(!ec_id_valid(s->ids[i]))return 0;
        for(unsigned j=0;j<i;j++)if(ec_equal(s->ids[i],s->ids[j]))return 0;
    }return 1;
}
static inline int ec_grant(EdgeSave *s,const char *id){
    if(s->count>=EC_CARS||!ec_id_valid(id)||ec_owned(s,id))return 0;
    unsigned i=0;do{s->ids[s->count][i]=id[i];}while(id[i++]);s->count++;
    s->magic=0x45444745;s->version=1;s->check=ec_hash(s);return 1;
}
#endif
