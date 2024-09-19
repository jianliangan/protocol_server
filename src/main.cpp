#include "common/tools.h"
#include "net/rtp/common/rtp_packet.h"
#include "net/rtp/huage_server.h"
#include <fstream>
#include <ostream>
#include <string>
#include "base/json.hpp"
#include "app/timer.h"
#include "app/app.h"
#include "app/audio_consumer.h"
#include "app/comm_consumer.h"
#include "app/video_consumer.h"
#include <getopt.h>
#include <unistd.h>
/** 
 * */
int firstbuf;
bool islittleend=false;
App *g_app=nullptr;
static void interval( void *param){
    printf("sig settimeout");
}
int daemonize()
{
  pid_t pid;
  if((pid=fork())<0){
    return -1;
  }
  else if(pid!=0){
      exit(0);
  }
  int ret=setsid();
  if ( ret < 0 )
    return -1;

  if((pid=fork())<0){
  }
  else if(pid!=0){
      exit(0);
  }

  int fd = open( "/dev/null", O_RDWR );
  if ( fd < 0 )
    return -1;

  dup2( fd, STDIN_FILENO );

  dup2( fd, STDOUT_FILENO );
  if ( fd > STDERR_FILENO )
    close( fd );
  return 0;
}
void s_recvCallback(enumt_PLTYPE pltype, t_mediaFrameChain *mfc,
        HuageConnect *hgConn) {
    if (pltype == enum_PLTYPEAUDIO) {
        AudioConsumer::recvCallback(g_app->audioctx, mfc, hgConn);
    } else if (pltype == enum_PLTYPEVIDEO) {
        VideoConsumer::recvCallback(g_app->videoctx, mfc, hgConn);
    } else if (pltype == enum_PLTYPETEXT) {
        CommConsumer::recvCallback(g_app->textctx, mfc, hgConn);
    }
}
int main(int argc,char *argv[]){

    std::string conf;
    bool isDaemon=false;
    std::string json_string;
    std::string tmps;
    islittleend=checkLitEndian();
    int pagesize=getpagesize();
    std::ifstream ifs;  
    int length;
    int thnum;
    int workSNum;
    g_app=new App();
    int ok=hg_log_init();
    init16Num();
    if(ok!=0){
        fprintf(stderr,"缺少log文件夹");
        return 0;

    }
    struct option long_options[] = {
      {"string", required_argument, 0, 'c'},
      {"string", required_argument, 0, 'd'},
      {0, 0, 0, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "c:d", long_options, NULL)) != -1) {
      switch (opt) {
	case 'c':
	  conf = optarg;
	  break;
	case 'd':
	  isDaemon = true;
	  break;
	default:
	  HG_ALOGI(0,"Usage: %s\n",argv[0]);
	  return 1;
      }
    }
    if(conf.empty()){
        HG_ALOGI(0,"config file not found\n");
        return 1;
    } 
    if(isDaemon){
      int nochdir =0;
      int noclose =0;
      if(daemonize()){
           HG_ALOGI(0,"daemon err\n");
           return 1;
      }
    }
printf("ddd");
    ifs.open(conf.c_str());
    if (ifs.fail()) {
        HG_ALOGI(0,"Failed to open the file %s\n", conf.c_str() );
        ifs.close();
        return 0;  
    }
    ht=new HgTimer();
    t_hgTimerEvent hev;
    hev.handle=interval;
    hev.param=ht;
    uintptr_t ct=hgetSysTimeMicros()/1000;
    ht->AddEvent(hev,ct+1000*10);

    while (std::getline(ifs, tmps)) {
        json_string += tmps;
    }
    ifs.close();
    std::string err;
    std::string serverip;
    nlohmann::json json;
    int serverport;
    json = nlohmann::json::parse(json_string,nullptr,false);
    if(json.is_discarded())
    {
        HG_ALOGI(0,"json format error  ");
        return 0;
    }
    //log_init(LL_TRACE, "rtp", "./log/");
    //HG_ALOGI(0,"config ", json.dump());
    serverip=json.at("ip");
    serverport=json.at("port");
    firstbuf=json.at("firstbuf");
    thnum=json.at("thnum");
    workSNum=json.at("workSessNum");
    g_app->workSessNum=workSNum;
    g_app->threadNum=thnum;
    g_app->audioctx =new AudioConsumer();
    g_app->videoctx = new VideoConsumer();
    g_app->textctx=new CommConsumer();

    Room::audioconfig.channels=HG_AUDIO__CHANNEL;
    Room::audioconfig.deep=HG_AUDIO__DEEP;
    Room::audioconfig.frameperbuf=HG_AUDIO__FRAMERPER;
    Room::audioconfig.samplesrate=HG_AUDIO__SAMPLERATE;
    Room::audioconfig.rframeperbuf=HG_AUDIO__FRAMERPER*HG_AUDIO__CHANNEL;
    Room::audioconfig.codec=enum_audioG711;
    HgWorker::init();
    SessionC::sessInit(10000);
    HuageServer *hgser=new HuageServer();
    g_app->hgser=hgser;
    hgser->p_recvCallback=s_recvCallback;
//鉴权以后要放到业务上，和server没关系，server只管传输anjianliang
    hgser->StartRun(serverip.c_str(),serverport);
    while (true) {
        g_app->AppChannel.Drive(nullptr, 1);
    }

    return 0;
}
