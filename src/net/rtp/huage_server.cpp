//
// created by ajl on 2021/11/11.
//

#include "net/rtp/huage_server.h"
#include "app/session.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/huage_tcp_manage.h"
#include "net/rtp/common/huage_udp_manage.h"
#include "net/rtp/common/rtp_packet.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hg_pipe.h"
#include "net/rtp/common/hg_channel_stru.h"
#include "net/rtp/common/hgfdevent.h"
#include "net/rtp/common/type_comm.h"
HuageServer::HuageServer() {

    HgConnectBucket *hgconnbu = new HgConnectBucket();
    hnab=new HuageNetManage();
    hnab->hgConnBucket=hgconnbu;
    int connnum=1000;
    HuageConnect::hgConnInit(connnum, &hgconnbu);
}

void HuageServer::StartRun(const char *ip, int port) {

    /***iocp***/
    HgIocp *iocp = new HgIocp();
    HgFdEvent::fdEvChainInit(&iocp->fdchainfree,100000);

    hnab->SetConfig(p_recvCallback,this);
    hnab->Listen(ip,port);
    iocp->StartRun();


}
void HuageServer::Accept(HuageConnect *hgConn){
    if(hnab->hgConnBucket!=NULL)
       hnab->hgConnBucket->Accept(hgConn);
}
