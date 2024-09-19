#ifndef HG_COMMON_HEAD_H
#include <cstdio>
#include "base/json.hpp"
#define HG_COMMON_HEAD_H

enum enumt_audioCodecs{
    enum_audioG711
};

typedef struct t_audioConfig{
    int channels=0;
    int deep=0;
    int samplesrate=0;
    int frameperbuf=0;
    int rframeperbuf=0;
    enumt_audioCodecs codec=enum_audioG711;
    void init(){
       channels=0;
       deep=0;
       samplesrate=0;
       frameperbuf=0;
       rframeperbuf=0;
       codec=enum_audioG711;
    }

} t_audioConfig;
inline void resJsoHead(uint32_t uniqid,std::string status,nlohmann::json &jss,nlohmann::json const &body){

    jss["v"]= 0;
    jss["i"]=uniqid;
    jss["b"]=status;
    jss["res"]=body;
}
#endif
