#ifndef HUAGE_SERVER_H
#define HUAGE_SERVER_H
class TcpServer;
class UdpServer;
class HuageConnect;
struct sockaddr_in;
#include "net/rtp/common/extern.h"
#include "stdint.h"
#include "net/rtp/common/hg_pipe.h"
#include "net/rtp/common/huage_net_manage.h"
#include "net/rtp/common/type_comm.h"
class HuageServer {
    public:
        HuageNetManage *hnab=nullptr;
        HuageServer();
        void StartRun(const char *ip,int port);
        tf_serverRecv p_recvCallback;
        void Accept(HuageConnect *hgConn);
};


#endif //HUAGE_SERVER_H
