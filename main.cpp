#include <mbed.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm> // std::equal
#include <iterator>  // std::rbegin, std::rend
#include "EthernetInterface.h"

#define MD_MSG_ "GD0000108003\n"
#define MD_MSG "GD0000108003"


using rtos::ThisThread::sleep_for;

int decode(string code);
int split(string data,char delimiter,string *dst);
bool endsWith(const std::string& s, const std::string& suffix);
std::string tcp_transmission(TCPSocket& socket,SocketAddress& destination_address,char msg[]);

int main(){
    nsapi_error_t response;
    Thread receiveThread;
    string response_message;

    // 送信先情報
    const char *destinationIP = "192.168.3.69";
    const uint16_t destinationPort = 10940;
    SocketAddress destination_address;
    destination_address.set_ip_address(destinationIP);
    destination_address.set_port(destinationPort);

    // 自機情報
    const char *myIP = "192.168.3.1";
    const char *myNetMask = "255.255.0.0";

    // イーサネット経由でインターネットに接続するクラス
    EthernetInterface net;
    EthernetInterface destination_net;

    /* マイコンのネットワーク設定 */
    // DHCPはオフにする（静的にIPなどを設定するため）
    net.set_dhcp(false);
    // IPなど設定
    net.set_network(myIP, myNetMask, "");

    destination_net.set_dhcp(false);
    destination_net.set_network(destinationIP, "", "");

    printf("start\n");

    // マイコンをネットワークに接続
    response=net.connect();
    if(response<0)
    {
        printf("network connection Error: %d \n",response);
        return -1;
    }
    printf("network connection success: %d \n",response);
    printf("hello\n");
    // Show the network address
    SocketAddress a;
    net.get_ip_address(&a);
    printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");
    

    //tcpのやつ
    printf("next tcp socket open\n");
    TCPSocket socket;
    response=socket.open(&net);
    if(response<0){
        printf("tcp socket cant open   --failed--\n");
        socket.close();
        return -1;
    }
    printf("tcp socket opened : %d \n",response);

    response=socket.connect(destination_address);
    if(0>response){
        printf("error connect: %d\n",response);
    }
    printf("connect do :%d \n",response);
    

    //送信
    char msg_pp[]="PP\n";
    response_message=tcp_transmission(socket,destination_address,"PP\n");
    

    printf("wait 10s\n");
    sleep_for(10s); //起動を待つために 時間は減らす
    // msg="BM\n";
    response_message=tcp_transmission(socket,destination_address,"BM\n");

    sleep_for(10); 
    //while(1){
        int all_distance[1081]={0};
        printf("whileeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
       
        //response_message=tcp_transmission(socket,destination_address,MD_MSG_);//なんかこれ使うと通信が止まる　recvできないぽい？
        
        response=socket.sendto(destination_address,MD_MSG_,sizeof(MD_MSG_));
        if(0>response){
            printf("error send data: %d\n",response);
            socket.close();
            //exit();
        }
        printf("send do : %d \n",response);

        string data_string;
        //while(1){
            char rbuffer[2048]="";
            response=socket.recv(rbuffer,sizeof(rbuffer));
            //if(rbuffer[0]==NULL){break;}
            data_string+=rbuffer;
        //}
        if(0>response){
            printf("error receiving data: %d\n",response);
            socket.close();
            //exit();
        }
        printf("recv response= %d \n",response);
        //printf("recv data= \n%s \n",data_string.c_str());
        printf("recv data= \n%s \n",rbuffer);


        //static string data_string = "";
        vector<std::string> all_res;
        vector<std::string> msg_res;
        vector<std::string> scan_res;
        vector<int> mem_msg;
        //printf("data_string=%s\n",data_string.c_str());

        std::stringstream ss{data_string};
        std::string buf;
        int i=0;
        while (std::getline(ss, buf, '\n')) {
            all_res.push_back(buf);
            printf("buf=%s\n",buf.c_str());
            if(buf[0]==NULL){mem_msg.push_back(i);printf("ssssssssss%d\n",i);}
            i++;
        }

        mem_msg.push_back(all_res.size());//最終行を追加してバグ回避
        printf("size==%d\n",all_res.size());

        for(int i=0;i<all_res.size();i++){
            printf("all_msg=%d |%s\n",i,all_res[i].c_str());
        }

        for(int i=0;i<mem_msg.size();i++){
            printf("mem_msg=%d\n",mem_msg[i]);
        }
        int s=0;
        string text_msg=MD_MSG;
        //all_resをそれぞれ意味ごとに分ける　連続のやつRDのなごり
        for(int i=0;i<all_res.size();i++){
            if(all_res[i]==text_msg && all_res[i+1]=="00P"){
                msg_res.push_back(all_res[i]);
                i++;
                msg_res.push_back(all_res[i]);
                i++;
                msg_res.push_back(all_res[i]);
                i++;
                while(i<mem_msg[s]){
                    scan_res.push_back(all_res[i]);
                    i++;
                }
                printf("a\n");
                //i=mem_msg[s];
                s++;
            }

            if(all_res[i]==text_msg&&all_res[i+1]=="10Q"){//不完全状態なのでもう一回オーダーを投げる
                response_message=tcp_transmission(socket,destination_address,MD_MSG_);
            }
        }


        for(int i=0;i<msg_res.size();i++){
            printf("res_msg=%s\n",msg_res[i].c_str());
        }
        for(int i=0;i<scan_res.size();i++){
            printf("scan_msg=%s\n",scan_res[i].c_str());
        }

        printf("kk\n");

        string distance_string="";

        for(int i=0;i<scan_res.size();i++){
            scan_res[i].erase(scan_res[i].size()-1);//チェックサム削除
        }
        //1行にまとめる
        for(int i=0;i<scan_res.size();i++){
            distance_string+=scan_res[i];
        }
        //ここらへんでscanデータだけにしたかったらi=3でいけるかも

        //printf("distance_string=%s\n",distance_string.c_str());

        for(int i=0;i<distance_string.size();i++){
            if(distance_string.size()>0 && (distance_string.size()%3)==0){
                int distance_string_index=0;
                int distance_index=0;

                while(distance_string_index<distance_string.size()){
                    int distance=decode(distance_string.substr(distance_string_index,3));
                    all_distance[distance_index]=distance;
                    if(distance<0){
                        printf("distance_string_index=%s\n",distance_string.substr(distance_string_index,3).c_str());
                    }
                    distance_index++;
                    distance_string_index+=3;
                }
            }
        }
        printf("sizeof(all_distance) == %d\n",sizeof(all_distance)/sizeof(all_distance[0]));
        for(int j=0;j<sizeof(all_distance)/sizeof(all_distance[0]);j++){
            printf("%d ",all_distance[j]);
        }       

        printf("\nloooooooooooooooooooooooooooooooooooooooooooooooooooooooooop\n");
            
        data_string = "";
    //}
        /*
    */
    
    printf("fin\n");
    response_message=tcp_transmission(socket,destination_address,"QT\n");

    net.disconnect();
    
    return 0;
}

int decode(string code) {
    int value = 0;
    for (int i = 0; i < code.length(); i++) {
        value <<= 6;
        value &= ~0x3f;
        value |= code[i] - 0x30;
  }
  return value;
}


int split(string data, char delimiter, string *dst) {
    int index = 0;
    int data_length = data.length();
    dst[0] = "";
    for (int i = 0; i < data_length; i++) {
        char tmp = data[i];
        if (tmp == delimiter) {
            index++;
            dst[index] = "";
        } else
            dst[index] += tmp;
    }
    return (index + 1);
}

bool endsWith(const std::string& s, const std::string& suffix) {
   if (s.size() < suffix.size()) return false;
   return std::equal(std::rbegin(suffix), std::rend(suffix), std::rbegin(s));
}

std::string tcp_transmission(TCPSocket& socket,SocketAddress& destination_address,char msg[]){
    nsapi_error_t response;

    response=socket.sendto(destination_address,msg,sizeof(msg));
    if(0>response){
        printf("error send data: %d\n",response);
        socket.close();
        //exit();
    }
    printf("send do : %d \n",response);
    char rbuffer[1024]="";
    response=socket.recv(rbuffer,sizeof(rbuffer));
    if(0>response){
        printf("error receiving data: %d\n",response);
        socket.close();
        //exit();
        }
    //printf("recv %d |fin|\n %s |fin|\n %s |fin|\n", response, strstr(rbuffer, "\r\n") - rbuffer, rbuffer);
    printf("recv response= %d \n",response);
    printf("recv message= \n%s \n",rbuffer);
    string buffer=rbuffer;

    return buffer;
}
