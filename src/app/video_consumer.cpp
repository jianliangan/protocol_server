#include "app/video_consumer.h"
#include "app/app.h"
#include "h264parse/h264_stream.h"
using namespace std;
VideoConsumer::VideoConsumer(){
    int err;


}

//目前没法先需要动态大小的udp包，暂时用源自己的seq，未来如果需要可以放到sess->bySubscList下的SessSubInfo里边
void VideoConsumer::OutConsumeQueue(t_mediaFrameChain *mfc,SessionC *sess){
    std::unordered_map<uint32_t,SessSubInfo> * bsl;
    std::unordered_map<uint32_t,SessSubInfo>::iterator bslit;
    uint32_t ssrc=0;
    uint32_t index=0;
    uint64_t currentsec=0;
    SessionC *itsess=nullptr;
    VideoConsumer *vc=nullptr;
    currentsec=hgetSysTimeMicros()/1000000;
    bsl=sess->bySubscList;

    t_chainNodeData *hbgt=nullptr;
    t_chainNode *lefthcn=mfc->hct.left;
    hbgt=(t_chainNodeData *)lefthcn->data;

    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        if(bslit->second.flag&HG_SESS_SUB_VIDEO!=HG_SESS_SUB_VIDEO){
            continue;
        }

        itsess=bslit->second.sess;
        ssrc=itsess->ssrc;
        //
        hbgt->freenum++;
        HuageNetManage::preSendData(g_app->hgser->hnab, mfc, ssrc,
                (int) enum_PLTYPEVIDEO,
                false, itsess->data, false, nullptr,0);


    }
}

void VideoConsumer::recvCallback(void *ctx,t_mediaFrameChain *mfc,HuageConnect *hgConn) {
    VideoConsumer * context=(VideoConsumer *)ctx;
    uint8_t *nalstart=nullptr;
    SessionC *sess=(SessionC *)hgConn->data;
    int rbspsize;
    if(sess==nullptr || sess->status != enum_SESSLOGIN){
       return;
    }
    /*int naltype=h264_get_type(framedata+HG_RTP_PACKET_HEAD_SIZE,size-HG_RTP_PACKET_HEAD_SIZE,&nalstart,&rbspsize);
      if(naltype==7||naltype==8)
      {    
      sps_t *sps=new sps_t;
      int ret=h264_get_attr_sps(nalstart,rbspsize,sps);
      if(ret<0){
      return;
      }
      }*/
    context->OutConsumeQueue(mfc,sess);
}

