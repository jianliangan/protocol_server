//
// Created by ajl on 2021/9/14.
//

#include "app/room.h"
#include "app/app.h"
#include "net/rtp/common/hg_channel_stru.h"
#include "net/rtp/common/huage_net_manage.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hg_netcommon.h"
#include "app/session.h"
#include "net/rtp/huage_server.h"
#include <unordered_map>
t_audioConfig Room::audioconfig;
Room::Room(uint32_t roomid){



    audioCaNum=audioconfig.samplesrate/audioconfig.frameperbuf;//1秒分成几段
    rooBufinfo.size=(char *)malloc(sizeof(char)*audioCaNum);//用来存储加了多少个
    fragCache=memoryCreate(4096);
    //rooBufinfo.rawcache=(uint8_t *)malloc(audioconfig.channels*audioconfig.samplesrate*audioconfig.deep/8);
    //1秒内非编码原始音频容量
    rooBufinfo.samptota=(int *)malloc(audioconfig.channels*audioconfig.samplesrate*4);//用来存储加以后的和


    id=roomid;
    audioFramRawByte=audioconfig.frameperbuf*audioconfig.deep*audioconfig.channels >> 3;//一帧的原始字节数
    audioFrameOutByte=audioconfig.channels*audioconfig.frameperbuf*32/8;//一帧编码后字节数
    for(int i=0;i<audioCaNum;i++){
        *(rooBufinfo.size+i)=0;
    }
    userList=new std::unordered_map<uint32_t, RoomUserInfo>;
}
int Room::subIndex(int newd,int oldd){

    if(newd<oldd){
        int ret;
        ret= (newd-0)+1+(audioCaNum-1)-oldd;
        return ret;
    }else{
        return newd-oldd;
    }
}
void Room::HandleCreEnterRoo(void *pth,void *ctx,void *params,int psize){

    t_textRoomEnter *ctre=(t_textRoomEnter *)params;
    SessionC *sess=ctre->sess;
    bool create=ctre->create;
    uint32_t roomid=ctre->roomid;
    uint32_t uniqid=ctre->uniqid;
    Room *roomptr=CreateRoom(uniqid,roomid,sess,create);
    roomptr->EnterRoom(sess);
    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    bsl=roomptr->userList;
    /*这里代表用户打算看那些人，以后做成命令触发，这里是临时的写法*/
    sess->views[0]=sess;
    sess->viewsn=1;
    /**/
    nlohmann::json jss=nlohmann::json{
        { "ssrc", (double)sess->ssrc },
            { "room", (double)roomid }};

    nlohmann::json resobj;
    resJsoHead(uniqid,"ok",resobj,jss);
    std::string resbody=resobj.dump();
    int length=resbody.length();
    t_chainList hct;
    int mfcs=HuageNetManage::createFrameStr(roomptr->fragCache,&hct, sess->ssrc,(int) enum_PLTYPETEXT,false,(int)enum_CMD_DEFAULT,0,(uint8_t *)resbody.c_str(), length,HG_HANDLE_MAX_FRAME_SLICE,-1);

    t_mediaFrameChain bigframetmp;
    bigframetmp.hct=hct;
    bigframetmp.size=mfcs;
    bigframetmp.pts=0;
    t_chainNodeData *headhbgtL=(t_chainNodeData *)hct.left->data;
    t_chainNode *hcnR=bigframetmp.hct.right;
    t_chainNodeData *hbgtR=(t_chainNodeData *)hcnR->data;
    hbgtR->sfree.ctx=roomptr;
    hbgtR->sfree.params=bigframetmp.hct.left;
    hbgtR->sfree.freehandle=Room::writeFree;



    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        SessionC *itsess=(SessionC *)bslit->second.sess;
        if(itsess->flags&0x01==1){
            HuageNetManage::preSendData(g_app->hgser->hnab, &roomptr->mfcULst, itsess->ssrc,
                    (int) enum_PLTYPETEXT,
                    false, itsess->data,  false, nullptr,0);

        }else{
            headhbgtL->freenum++;
            HuageNetManage::preSendData(g_app->hgser->hnab, &bigframetmp, itsess->ssrc,
                    (int) enum_PLTYPETEXT,
                    false, itsess->data, false, nullptr,0);
            itsess->flags=itsess->flags|0x01;

        }
    }


}
int Room::CreEnterRoom(unsigned int uniqid,unsigned int roomid,SessionC *sess,bool create) {
    //this thread scope set sess
    if (sess->status != enum_SESSLOGIN) {
        return -1;
    }
    //this thread scope set room



    int lens = sizeof(t_huageEvent) + sizeof(t_textRoomEnter);
    char hgteventptr[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;
    t_textRoomEnter *ctre = (t_textRoomEnter *) (hgteventptr + sizeof(t_huageEvent));
    ctre->roomid=roomid;
    ctre->sess=sess;
    ctre->create=create;
    ctre->uniqid=uniqid;
    hgtevent->handle = Room::HandleCreEnterRoo;
    hgtevent->psize = lens-sizeof(t_huageEvent);
    hgtevent->ctx = nullptr;
    hgtevent->i=0;
    HgWorker *hgworker = HgWorker::getWorker(roomid);
    hgworker->WriteChanWor(hgteventptr, lens);
    return 0;
}
Room *Room::CreateRoom(uint32_t uniqid,int roomid,SessionC *sess,bool create) {
    if (sess->status != enum_SESSLOGIN) {
        return nullptr;
    }
    std::unordered_map<std::string, Room *>::iterator roomit;
    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    roomit = g_app->rooms->find(std::to_string(roomid));
    Room *rm=nullptr;
    if (roomit != g_app->rooms->end()) {
        rm= roomit->second;

        if(create==false){
            return rm;
        }

    }else{
        if(create==false){
            return nullptr;
        }


        rm = new Room(roomid);
        g_app->rooms->insert(std::make_pair(std::to_string(roomid), rm));

    }
    //uint32_t roomid,t_audioConfig *aconft,uint8_t *acache,int acachelen


    bsl=rm->userList;
    std::map <std::string, nlohmann::json> roomvs;
    SessionC *sesstmp=nullptr;
    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit){
        sesstmp= bslit->second.sess;
        nlohmann::json jss=nlohmann::json{
            { "ssrc", (double)sesstmp->ssrc },
                { "key2", "ajl" }};
        roomvs.insert(std::make_pair(std::to_string(sesstmp->ssrc), jss));

    }
    nlohmann::json jss=nlohmann::json(roomvs);

    nlohmann::json resobj;
    resJsoHead(uniqid,"ok",resobj,jss);


    std::string resbody=resobj.dump();


    Hg_ChainDelChain(rm->fragCache,rm->mfcULst.hct.left);
    rm->mfcULst.hct.init();
    int mfcs=HuageNetManage::createFrameStr(rm->fragCache,&rm->mfcULst.hct, sess->ssrc,(int) enum_PLTYPETEXT,false,(int)enum_CMD_DEFAULT,0,(uint8_t *)resbody.c_str(), resbody.length(),HG_HANDLE_MAX_FRAME_SLICE,-1);
    rm->mfcULst.size=mfcs;

    return rm;
}
int Room::EnterRoom(SessionC *sess){
    RoomUserInfo rui;
    rui.flag=0;
    sess->rroom = this;
    sess->cpts=0;
    sess->spts=0;
    rui.sess=sess;
    userList->insert(std::make_pair(id,rui));
    return 0;
}
int Room::addIndex(int add1,int add2){
    return (add1+add2)%(audioCaNum);
}
int Room::MergeAudioData(int index,t_mediaFrameChain *mfc,SessionC *sess){
    //uint32_t pts,int8_t *data,int size
    //t_serverPacketRoomMergeParams *sprp=(t_serverPacketRoomMergeParams *)params.data;

    t_chainNode *hcn=mfc->hct.left;
    int size=mfc->size-HG_RTP_PACKET_HEAD_SIZE;
    if(2*size!=audioFramRawByte){//编码率为0.5
        //    if(3*size!=audioFramRawByte)//24deep
        return -1;//-1
    }

    int offset=index*audioFramRawByte;

    int frameoff=index*(audioconfig.rframeperbuf);

    int *curptrtotal=rooBufinfo.samptota+frameoff;
    int16_t *curptrfrca=sess->framecache+frameoff;
    int16_t val=0;
    int oldval=0;
    //int8_t *valptr=(int8_t *)&val;
    t_chainNodeData *hbgt;

    char *rsize=rooBufinfo.size+index;
    char *ffsize=sess->framesize+index;
    int rrsize=*rsize;
    int8_t *data=nullptr;
    while(hcn!=nullptr){
        hbgt=(t_chainNodeData *)hcn->data;
        data=(int8_t *)hbgt->data+hbgt->start;
        for(int j=0;j<hbgt->len;j++)
        {

            val=MuLaw_Decode(data[j]);
            if(rrsize==0){
                oldval=0;
            }else{
                oldval=*curptrtotal;//16deep
            }
            *curptrfrca=val;
            *curptrtotal =oldval+val;
            //val=(*curptrtotal)/(*rbinsize);
            //*curptr=*(valptr);
            //*(curptr+1)=*(valptr+1);//16deep
            //curptr+=2;//16deep
            curptrtotal++;
            curptrfrca++;
        }

        hcn=hcn->next;
    }
    *rsize=1;
    *ffsize=1;

    ///////////////////
    /*
       t_chainNode *hcnttt=mfc->hct.left;
       t_chainNodeData *bddfdf=(t_chainNodeData *)hcnttt->data;
       uint8_t *dddd=(uint8_t *)bddfdf->data;
       uint16_t time=pktTimestamp(dddd,50);
       int *tmp=rooBufinfo.samptota+frameoff;

       int16_t tmpbody[1024]={0};
       char contents[512]={0};
       FILE *f2 = fopen(HG_APP_ROOT"send-1.data", "a+");
       fwrite(contents, 1, strlen(contents), f2);
       for(int i=0;i<size;i++){
       tmpbody[i]=*(tmp+i);

       }
       fwrite(tmpbody, 1, size*2, f2);
       fclose(f2);
       */
    /////////////////////

    return 0;
}
FreeFrameChain *Room::GetFreeFrame(){
    FreeFrameChain *re=nullptr;
    if(framefreea!=nullptr){
        re=framefreea;
        framefreea=framefreea->next;


    }else{
        re=FreeFrameChain::instan(audioconfig.rframeperbuf*2+HG_RTP_PACKET_HEAD_SIZE);
        t_chainNode *hcn=re->mfc.hct.left;
        t_chainNodeData *hbgtR=(t_chainNodeData *)hcn->data;
        hbgtR->sfree.ctx=this;
        hbgtR->sfree.params=re;
        hbgtR->sfree.freehandle=Room::writeFree2;
        hbgtR->start=HG_RTP_PACKET_HEAD_SIZE;
        hbgtR->len=audioconfig.rframeperbuf*2;
        hbgtR->end=hbgtR->start+hbgtR->len;

    }
    frameusea++;
    return re;
}
void Room::WriteHeader(uint8_t *data,uint32_t ssrc,int pltype,int padding,bool isfirst,int channel,uint32_t time,int cmdtype,int comptype){
    t_rtpHeader rh;
    rh.ssrc = ssrc;
    rh.payloadType = pltype;

    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }



    rh.timestamp = time;
    rh.seq = 0;//到这里了
    rh.tail = 0;
    rh.cmd = cmdtype;
    rh.payloadType = pltype;
    rh.comp=comptype;
    rtpPacketHead(data, &rh);

}
void Room::OutConsumeQueue(char rsize,uint32_t pts,int fromi,SessionC *sess){

    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    uint32_t ssrc=0;
    uint32_t index=0;
    bool sendmsg0=false;
    SessionC *itsess=nullptr;
    uint64_t curtime=hgetSysTimeMicros();
    if(curtime-lasttime>1000000){
        lasttime=curtime;
        sendmsg0=true;
    }
    bsl=userList;

    int frameoff=fromi*(audioconfig.rframeperbuf);

    int *curptrtotal=rooBufinfo.samptota+frameoff;

    int framebodylen=audioconfig.rframeperbuf*2;

    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {

        itsess=(SessionC *)bslit->second.sess;
        int16_t *curptrfrca=itsess->framecache+frameoff;
        char ssize=*(itsess->framesize+fromi);
        if(itsess->lastvptsmsg){
            FreeFrameChain *ffca=GetFreeFrame();

            t_mediaFrameChain * mfctmp=&ffca->mfc;
            t_chainNode *hcntmp=mfctmp->hct.left;
            t_chainNodeData *hbgttmp=(t_chainNodeData *)hcntmp->data;

            HG_ALOGI(0,"i4444444444444444444444444 %d ",itsess->lastvptsmsg);

            WriteHeader((uint8_t *)hbgttmp->data,itsess->ssrc,enum_PLTYPEAUDIO,0,true,0,pts,enum_CMD_DEFAULT,1);

            uint8_t *datatmp=(uint8_t *)((char *)hbgttmp->data+hbgttmp->start);
            int offset=0;
            int datamove=0;
            datatmp[0]=0;
            datatmp[1]=itsess->viewsn;
            datamove=2;
            datatmp+=datamove;
            for(int i=0;i<itsess->viewsn;i++){
                SessionC *tmpsess=itsess->views[i];
                memcpy(datatmp+offset,tmpsess->vptsmsgca,sizeof(tmpsess->vptsmsgca));
                offset+=sizeof(tmpsess->vptsmsgca);
            }
            mfctmp->pts=pts;
            mfctmp->size=offset+datamove+HG_RTP_PACKET_HEAD_SIZE;
            hbgttmp->len=offset+datamove;
            hbgttmp->end=hbgttmp->start+offset+datamove;
            HG_ALOGI(0,"i4444444444444444444444444 %d",mfctmp->size);

            HuageNetManage::preSendData(g_app->hgser->hnab, mfctmp, itsess->ssrc,
                    (int) enum_PLTYPEAUDIO,
                    false, itsess->data,  false, nullptr,1);


        }
        FreeFrameChain *ffca=GetFreeFrame();

        t_mediaFrameChain * mfctmp=&ffca->mfc;
        t_chainNode *hcntmp=mfctmp->hct.left;
        t_chainNodeData *hbgttmp=(t_chainNodeData *)hcntmp->data;
        mfctmp->pts=pts;
        HG_ALOGI(0,"i4444444444444444444444444 %d ",itsess->lastvptsmsg);

        WriteHeader((uint8_t *)hbgttmp->data,itsess->ssrc,enum_PLTYPEAUDIO,0,true,0,pts,enum_CMD_DEFAULT,0);

        int16_t *datatmp=(int16_t *)((char *)hbgttmp->data+hbgttmp->start);

        for(int i=0;i<audioconfig.rframeperbuf;i++){
            int total=0;
            if(rsize!=0)
            {
                total=*(curptrtotal+i);
            }
            int curfrca=0;
            if(ssize!=0){
                curfrca=*(curptrfrca+i);
            }
            int cc= total-curfrca;

            if(cc>65535){
                cc=65535;
            }
            *datatmp=total;
            datatmp++;

        }
        *(itsess->framesize+fromi)=0;
        itsess->lastvptsmsg=false;

        mfctmp->size=framebodylen+HG_RTP_PACKET_HEAD_SIZE;
        hbgttmp->len=framebodylen;
        hbgttmp->end=hbgttmp->start+framebodylen;

        HuageNetManage::preSendData(g_app->hgser->hnab, mfctmp, itsess->ssrc,
                (int) enum_PLTYPEAUDIO,
                false, itsess->data,  false, nullptr,0);
    }


}
void Room::RoomMergeData(void *pth,void *ctx,void *params,int psize){
    t_serverPacketRoomMergeParams *sprp=(t_serverPacketRoomMergeParams *)params;
    SessionC *sess=sprp->sess;
    uint32_t framepts=sprp->pts;
    uint32_t ssrc=sprp->ssrc;
    int comp=sprp->comp;
    t_mediaFrameChain *mfc=&sprp->mfc;

    t_chainNode *hcnR=mfc->hct.right;
    t_chainNodeData *hbgtR=(t_chainNodeData *)hcnR->data;
    tf_paramHandle freeHandle=hbgtR->sfree.freehandle;

    Room *room=(Room *)sprp->room;

    t_chainNode *hcntmp=mfc->hct.left;
    t_chainNodeData *hbgttmp= nullptr;
    /*
       char tttt[1000];
       FILE *f2 = fopen(HG_APP_ROOT"recv0-.data", "a+");
       fwrite(tttt, 1, strlen(tttt), f2);

       while(hcntmp!= nullptr){
       hbgttmp=(t_chainNodeData *)hcntmp->data;
       fwrite((char *)hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
       hcntmp=hcntmp->next;
       }
       fclose(f2);
       */

    if(comp==1){
        if(mfc->size-HG_RTP_PACKET_HEAD_SIZE==HG_SINGLE_MSG0_LEN+sizeof(MsgObjHead)){
            MsgObj0 msgarr;
            t_chainNode *hcntmp=mfc->hct.left;
            t_chainNodeData *hbgttmp= nullptr;

            int offset=0;
            while(hcntmp!= nullptr){
                hbgttmp=(t_chainNodeData *)hcntmp->data;
                int datamove=0;
                if(hbgttmp->start!=0){
                    datamove=sizeof(MsgObjHead);
                }
                memcpy(sess->vptsmsgca + offset, (char *)hbgttmp->data+hbgttmp->start+datamove, hbgttmp->len-datamove);
                hcntmp=hcntmp->next;
                offset+=hbgttmp->len;
            }
            offset-=sizeof(MsgObjHead);
            sess->lastvptsmsg=true;
            MsgObj0::decodeone(sess->vptsmsgca,offset,&msgarr);

            uint16_t apts=msgarr.apts;
            uint16_t vpts=msgarr.vpts;

        }


gore:
        freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
        return;
    }
    if(framepts!=0&&uint16Sub(framepts,sess->cpts)<=0){

        freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
        return ; //-1
    }
    uint32_t realpts=0;
    uint64_t curtime=hgetSysTimeMicros();
    bool first=false;
    if(room->startplay==0){
        room->startplay=curtime;
        first=true;
    }
    uint64_t diffptsmic=curtime-room->startplay;
    uint32_t diffpts=(diffptsmic*room->audioCaNum)/1000000;

    if(first){

        room->nextptsplay=0;
        sess->cpts=framepts;
        sess->spts=diffpts;
    }
    uint32_t sess_spts=0;
    uint32_t sess_cpts=0;

    sess_cpts=sess->cpts;
    sess_spts=sess->spts;

    if(uint16Sub(sess_spts,diffpts)>=0){
        realpts=uint16Add(sess_spts,uint32Sub(framepts,sess_cpts));//相当于当音频流产生缓冲时，保存在缓冲后面
    }else{
        realpts=diffpts;//没有缓冲时直接播放，整个效果类似播放器的缓冲器原理
    }
    if(first){
        room->nextptsplay=realpts;
    }
    if(uint16Sub(realpts,room->nextptsplay)>=room->audioCaNum){//大于缓冲就扔掉了
        HG_ALOGI(0,"ignored pts too big realpts real %d next %d",realpts,room->nextptsplay);
    }else{
        //这块得加一个写保护，不能覆盖缓冲中的数据，必要时加cache
        int index=realpts%room->audioCaNum;
        room->MergeAudioData(index,mfc,sess);                

    }
    //2 base on spts merge packet in server
    //for(int i=0;i<1;i++){
    sess->spts=realpts;
    sess->cpts=framepts;
    // }
    //clear data

    freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
    //end clear

    //3 find need to send frames ,first,tag self lock

    if(uint16Sub(diffpts,room->nextptsplay)>0){
        int ret;
        uint32_t nextpts=room->nextptsplay;
        int nexti;


        do{

            nexti=nextpts%room->audioCaNum;
            char rbinsize=*(room->rooBufinfo.size+nexti);

            if(rbinsize!=0){
                room->OutConsumeQueue(rbinsize,nextpts,nexti,sess);
            }

            nextpts=uint16Add(nextpts,1);
            *(room->rooBufinfo.size+nexti)=0;

            if(nextpts==diffpts){
                room->nextptsplay=diffpts;
                break;
            }


        }while(true);
    }
}
void Room::writeFree2(void *ctx, void *data) {
    Room *room = (Room *) ctx;

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char tmp[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    hgtevent->handle = Room::freeByChan2;
    hgtevent->i=12;
    hgtevent->ctx = ctx;
    hgtevent->psize = lens-sizeof(t_huageEvent);
    *((void **) ((char *) tmp + sizeof(t_huageEvent))) = data;

    HgWorker *hgworker = HgWorker::getWorker(room->id);
    hgworker->WriteChanWor(tmp, lens);
}

void Room::writeFree(void *ctx, void *data) {
    Room *room = (Room *) ctx;

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char tmp[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    hgtevent->handle = Room::freeByChan;
    hgtevent->i=11;
    hgtevent->ctx = ctx;
    hgtevent->psize = lens-sizeof(t_huageEvent);
    *((void **) ((char *) tmp + sizeof(t_huageEvent))) = data;

    HgWorker *hgworker = HgWorker::getWorker(room->id);
    hgworker->WriteChanWor(tmp, lens);
}
void Room::freeByChan(void *pth, void *ctx, void *params, int psize) {
    t_chainNode **ptmp = (t_chainNode **) params;
    t_chainNode *hcn=*ptmp;

    t_chainNodeData *hbgttmp = (t_chainNodeData *) hcn->data;
    hbgttmp->freenum--;

    if(hbgttmp->freenum<=0){
        Room *hcl = (Room *) ctx;
        while (hcn != nullptr) {
            memoryFree(hcl->fragCache, (uint8_t *)hcn);
            hcn = hcn->next;
        }
    }
}
void Room::freeByChan2(void *pth, void *ctx, void *params, int psize) {
    Room *room=(Room *)ctx;
    FreeFrameChain **ptmp = (FreeFrameChain **) params;
    FreeFrameChain *ffca=*ptmp;
    ffca->next=room->framefreea;
    room->framefreea=ffca;
    room->frameusea--;

}

FreeFrameChain::FreeFrameChain(){
    next=nullptr;
    mfc.init();
}
void FreeFrameChain::init(){
    mfc.init();
    next=nullptr;
}
int FreeFrameChain::getlen(){
    return sizeof(FreeFrameChain)+sizeof(t_chainNode)+sizeof(t_chainNodeData);
}
FreeFrameChain *FreeFrameChain::instan(int relays){
    FreeFrameChain *re= (FreeFrameChain *)malloc(getlen()+relays);
    re->init();
    re->mfc.size=relays;
    t_chainNode *hcn=(t_chainNode *)((char *)re+sizeof(FreeFrameChain));
    hcn->init();
    re->mfc.hct.left=hcn;
    re->mfc.hct.right=hcn;

    t_chainNodeData *hbgt=(t_chainNodeData *)((char *)re+sizeof(FreeFrameChain)+sizeof(t_chainNode));
    hbgt->init();
    hbgt->data=(char *)re+sizeof(FreeFrameChain)+sizeof(t_chainNode)+sizeof(t_chainNodeData);
    hbgt->cap=relays;
    hcn->data=hbgt;
    return re;
}
void FreeFrameChain::reset(){
    //mfc.reset();
}
