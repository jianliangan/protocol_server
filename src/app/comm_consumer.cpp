#include "base/json.hpp"
#include "app/comm_consumer.h"
#include "app/app.h"
#include "app/session.h"
#include "net/rtp/common/huage_net_manage.h"
using namespace std;
CommConsumer::CommConsumer(){
    int err;

}
void CommConsumer::asyncLogin(void *ctx, t_mediaFrameChain *mfc,t_requestCtx *tRequest){
        CommConsumer *context=(CommConsumer *)ctx;
	uint32_t uniqid=tRequest->uniqId;
	HuageConnect *hgConn=tRequest->conn;
	SessionC *sessPtr=(SessionC *)hgConn->data;
	int ssrc=hgConn->ssrc;
	char resbody[200]={0};
	if(sessPtr==nullptr){
	   sessPtr=SessionC::sessGetFree();
	   hgConn->data=sessPtr;
	}
        context->Login(ssrc,sessPtr);
        sprintf(resbody,"{\"v\":0,\"i\":%u,\"b\":\"ok\"}",uniqid);
        size_t length=strlen(resbody);

        t_chainList hct;
        int mfcs=HuageNetManage::createFrameStr(hgConn->fragCache,&hct, hgConn->ssrc,(int) enum_PLTYPETEXT,false,(int)enum_CMD_DEFAULT,0,(uint8_t *)resbody, length,HG_HANDLE_MAX_FRAME_SLICE,-1);
        t_mediaFrameChain bigframetmp;
        context->FrameBufSet(&bigframetmp,0,&hct,mfcs,hgConn);
        HuageNetManage::preSendData(g_app->hgser->hnab, &bigframetmp, ssrc,
                (int) enum_PLTYPETEXT,
                true,hgConn, false, nullptr,0);
        g_app->hgser->Accept(hgConn);	
}
void CommConsumer::recvCallback(void *ctx,t_mediaFrameChain *mfc,HuageConnect *hgConn){

    CommConsumer *context=(CommConsumer *)ctx;
    int size=mfc->size;
    t_chainNode *hcn=mfc->hct.left;
    //uint32_t ssrc=pktSsrc(framedata,size);//组合起来后的大包都带rtp头
    if(size<=HG_RTP_PACKET_HEAD_SIZE)
        return;

    char jsonblock[HG_MAX_TEXT_BUF_SIZE]={0};
    t_chainNodeData *hbgt=nullptr;

    uint32_t ssrc=0;
    if(hcn!=nullptr){
        hbgt=(t_chainNodeData *)hcn->data;
        ssrc=pktSsrc((uint8_t *)hbgt->data,14);
    }
    int total=0;
    while (hcn!=nullptr) {
        hbgt=(t_chainNodeData *)hcn->data;
        memcpy((char *)jsonblock+total,(char *)hbgt->data+hbgt->start,hbgt->len);
        total+=hbgt->len;
        hcn=hcn->next;
    }

    t_chainNode *hcnR=mfc->hct.right;
    if(hcnR!=nullptr){
        t_chainNodeData *hbgt=(t_chainNodeData *)hcnR->data;
        hbgt->sfree.freehandle(hbgt->sfree.ctx,hbgt->sfree.params);
    }
    SessionC *sessPtr=(SessionC *)hgConn->data;
    uint32_t length;
    uint32_t uniqid;
    uint32_t version;
    std::string uri;
    std::string body;
    int size1=size-HG_RTP_PACKET_HEAD_SIZE;
    std::string err;
    nlohmann::json json;
    try{
        json = nlohmann::json::parse(std::string(jsonblock,size1));
    }catch(nlohmann::json::parse_error& e){

        HG_ALOGI(0,"json post recv format error %s",e.what());
        return ;
    }
    try{
        version=json.at("v");
        uniqid=json.at("i");
        uri=json.at("c");
    }catch(nlohmann::json::type_error& e){
        HG_ALOGI(0,"json post recv format error %s",e.what());
    }catch(nlohmann::json::out_of_range& e){
        HG_ALOGI(0,"json post recv format error %s",e.what());
    }
    t_requestCtx tRequest;
    tRequest.uniqId=uniqid;
    tRequest.conn=hgConn;
    uint32_t roomid=0;
    int deep=0;
    int samplesrate=0;
    int framelen=0;
    int channels=0;
    enumt_audioCodecs codec;

    const char *addbody=nullptr;
    char resbody[HG_MAX_TEXT_BUF_SIZE];
    int ret;
    if(uri=="login"){
	asyncLogin(ctx, mfc,&tRequest);
        return;

    }else{
        if(sessPtr==nullptr || sessPtr->status != enum_SESSLOGIN){
            return;
        }

        if(uri=="createenterroom"){
            try{

                roomid=json.at("req").at("roomid");

                channels=json.at("req").at("chan");
                deep=json.at("req").at("deep");
                samplesrate=json.at("req").at("samrate");
                framelen=json.at("req").at("frlen");
                codec=(enumt_audioCodecs)json.at("req").at("codec");
            }catch(nlohmann::json::type_error& e){
                HG_ALOGI(0,"json error %s",e.what());
                return;
            }catch(nlohmann::json::out_of_range& e){
                HG_ALOGI(0,"json error %s",e.what());
            }
            sessPtr->aconfigt.channels=channels;
            sessPtr->aconfigt.deep=deep;
            sessPtr->aconfigt.samplesrate=samplesrate;
            sessPtr->aconfigt.frameperbuf=framelen;
            sessPtr->aconfigt.codec=codec;
            sessPtr->roomid=roomid;
            ret=Room::CreEnterRoom(uniqid,roomid,sessPtr,true);



        }
        else if(uri=="enterroom"){
            //ret=Room::CreEnterRoom(uniqid,roomid,sessPtr,false);
        }
    }
}
//void *ctx, uint8_t *fragdata, uint32_t size, SessionC *sess
void CommConsumer::FrameBufSet(t_mediaFrameChain *mfc,uint16_t time,t_chainList *hct,int length,void *ctx){
    mfc->size = length;
    mfc->hct = *hct;
    mfc->pts=time;
    mfc->sfree.params = nullptr;
    mfc->sfree.ctx = nullptr;
    mfc->sfree.freehandle = nullptr;

    t_chainNode *hcnR=mfc->hct.right;
    t_chainNodeData *hbgtR=(t_chainNodeData *)hcnR->data;
    hbgtR->sfree.ctx=ctx;
    hbgtR->sfree.params=hct->left;
    hbgtR->sfree.freehandle=CommConsumer::writeFree;
}
//Logout
void CommConsumer::SessOut(SessionC *sess){
    SessionC::sessFree(sess);
}

//Login
void CommConsumer::Login(uint32_t ssrc,SessionC *sess){//
    //Login
    SessionC::sessSetStatus(sess,enum_SESSLOGIN);
    //test sub 
    sess->ssrc=ssrc;
    uint32_t byssrc=ssrc;
    SessionC *bysess=nullptr;
    bysess=sess;
    /****
      SessionC::sessAddSubApi(sess,bysess,HG_SESS_SUB_AUDIO|HG_SESS_SUB_VIDEO);
      SessionC::sessAddBySubApi(sess,bysess,HG_SESS_SUB_AUDIO|HG_SESS_SUB_VIDEO);
     ****/
    SessionC::sessAddSubApi(sess,bysess,bysess->ssrc,HG_SESS_SUB_AUDIO|HG_SESS_SUB_VIDEO);

    //async  other user sess
    int lens = sizeof(t_huageEvent) + sizeof(t_serverPacketLoginParams);
    char hgteventptr[lens];
    t_serverPacketLoginParams *uprp = (t_serverPacketLoginParams *)(hgteventptr+sizeof(t_huageEvent));
    uprp->ssrc=sess->ssrc;
    uprp->sess=sess;
    uprp->bysess=bysess;


    t_huageEvent *hgtevent=(t_huageEvent *)hgteventptr;

    hgtevent->handle=CommConsumer::workHanLogin;
    hgtevent->psize=lens-sizeof(t_huageEvent);
    hgtevent->ctx =this;
    hgtevent->i=9;
    HgWorker *hgworker=HgWorker::getWorker(bysess->ssrc);
    hgworker->WriteChanWor(hgteventptr,lens);
    //async end

}
void CommConsumer::workHanLogin(void *pth,void *ctx,void *params,int psize){
    t_serverPacketLoginParams *uprp=(t_serverPacketLoginParams *)params;
    uint32_t ssrc=uprp->ssrc;

    SessionC::sessAddBySubApi((SessionC *)uprp->sess,uprp->ssrc,(SessionC *)uprp->bysess,HG_SESS_SUB_AUDIO|HG_SESS_SUB_VIDEO);

}
void CommConsumer::Logout(SessionC *sess){
    std::unordered_map<uint32_t,SessSubInfo> *msl;
    std::unordered_map<uint32_t,SessSubInfo> *bsl;

    std::unordered_map<uint32_t,SessSubInfo>::iterator mslit;
    std::unordered_map<uint32_t,SessSubInfo>::iterator bslit;
    msl=new std::unordered_map<uint32_t,SessSubInfo>;
    bsl=new std::unordered_map<uint32_t,SessSubInfo>;
    msl->insert(sess->meSubList->begin(),sess->meSubList->end());
    bsl->insert(sess->bySubscList->begin(),sess->bySubscList->end());
    SessionC *tmpsess=nullptr;
    //1、删除我订阅的人的被订阅列表，删除别人的被我订阅的被订阅列表
    for ( mslit = msl->begin() ; mslit != msl->end(); ++mslit)
    {
        SessSubInfo *ssif=&mslit->second;
        tmpsess=ssif->sess;
        SessionC::sessDelBySubApi(sess,tmpsess);
    }
    //2、删除我被订阅列表的订阅列表,删除别人的订阅我的订阅列表
    for ( bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        SessSubInfo *ssif=&bslit->second;
        tmpsess=ssif->sess;
        SessionC::sessDelSubApi(tmpsess,sess);
    }

    SessOut(sess);
}
void CommConsumer::writeFree(void *ctx, void *data) {
    HuageConnect *hgConn = (HuageConnect *) ctx;

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char tmp[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    hgtevent->handle = CommConsumer::freeByChan;

    hgtevent->ctx = ctx;
    hgtevent->i=10;
    hgtevent->psize = lens-sizeof(t_huageEvent);
    *((void **) ((char *) tmp + sizeof(t_huageEvent))) = data;

    HgWorker *hgworker = HgWorker::getWorker(hgConn->ssrc);
    hgworker->WriteChanWor(tmp, lens);
}

void CommConsumer::freeByChan(void *pth, void *ctx, void *params, int psize) {
    t_chainNode **ptmp = (t_chainNode **) params;
    t_chainNode *hcn=*ptmp;


    HuageConnect *hcl = (HuageConnect *) ctx;
    while (hcn != nullptr) {
        memoryFree(hcl->fragCache, (uint8_t *)hcn);
        hcn = hcn->next;
    }
}



