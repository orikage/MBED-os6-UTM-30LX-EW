#include <mbed.h>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm> // std::equal
#include <iterator>  // std::rbegin, std::rend
#include "EthernetInterface.h"

//GDコマンドは打つ前にBMコマンドが必要 
//|GD|start(4桁)|END(4桁)|GROUPING(2桁)|RT(\nのこと)|
#define MD_MSG_ "GD0000108002\n"
//MBEDは制御文字をstringに含めれない？ので別定義
#define MD_MSG "GD0000108002"


using rtos::ThisThread::sleep_for;

int decode(string code);
int split(string data,char delimiter,string *dst);
bool endsWith(const std::string& s, const std::string& suffix);
std::string tcp_transmission(TCPSocket& socket,SocketAddress& destination_address,char msg[]);//送受信をまとめた関数

int main(){
    string md_msg=MD_MSG;
    nsapi_error_t response;
    Thread receiveThread;
    string response_message;

    // 送信先情報
    const char *destinationIP = "192.168.3.69";//utm30のアドレス　一回pcのUrgBenriPlusにつなげて確認する
    const uint16_t destinationPort = 10940;//固定?
    SocketAddress destination_address;
    destination_address.set_ip_address(destinationIP);
    destination_address.set_port(destinationPort);

    // 自機情報
    const char *myIP = "192.168.3.1";//destinationIPの3組目(今回の場合は192.168.3.までのこと)までは同じ数値にすること
    const char *myNetMask = "255.255.0.0";//255.255.255.0でもいいはず　試してない　これで同じネットワーク内に指定してることになる

    // イーサネット経由でインターネットに接続するクラス
    EthernetInterface net;
    EthernetInterface destination_net;

    /* マイコンのネットワーク設定 */
    // DHCPはオフにする（静的にIPなどを設定するため）
    net.set_dhcp(false);
    // IPなど設定
    net.set_network(myIP, myNetMask, "0.0.0.0");

    destination_net.set_dhcp(false);
    destination_net.set_network(destinationIP, "", "");//送信先情報

    printf("start\n");

    // マイコンをネットワークに接続
    response=net.connect();//responseがマイナスなときがエラーを指す
    if(response<0)
    {
        printf("network connection Error: %d \n",response);
        return -1;
    }
    printf("network connection success: %d \n",response);
    printf("hello\n");

    // Show the network address
    //SocketAddress a;
    //net.get_ip_address(&a);
    //printf("IP address: %s\n", a.get_ip_address() ? a.get_ip_address() : "None");
    

    //tcpのやつ
    printf("next tcp socket open\n");
    TCPSocket socket;
    response=socket.open(&net);
    if(response<0){
        printf("tcp socket cant open : %d  --failed--\n",response);
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
    //char msg_pp[]="PP\n";
    response_message=tcp_transmission(socket,destination_address,"PP\n");
    
    printf("wait 10s\n");
    sleep_for(10s); //UTM30の起動を待つために待つ(赤い点滅をしているときは測定不可) 時間は減らす

    // msg="BM\n";
    response_message=tcp_transmission(socket,destination_address,"BM\n");

    while(1){
        
        int all_distance[541]={0};//配列サイズは1081/md_msg_grouping 繰り上げ
        

        printf("whileeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
       
        //response_message=tcp_transmission(socket,destination_address,MD_MSG_);//なんかこれ使うと通信が止まる　recvできないぽい？
        
        response=socket.sendto(destination_address,MD_MSG_,sizeof(MD_MSG_));
        if(0>response){
            printf("error send data: %d\n",response);
            socket.close();
        }
        printf("send do : %d \n",response);

        string data_string="";
        char rbuffer[4096]="";
        response=socket.recv(rbuffer,sizeof(rbuffer));
        data_string+=rbuffer;

        if(0>response){
            printf("error receiving data: %d\n",response);
            socket.close();
            //exit();
        }
        printf("recv response= %d \n",response);
        printf("recv data= \n%s \n",data_string.c_str());
        //printf("recv data= \n%s \n",rbuffer);

        if(response!=1699){
            printf("low response\n");
            int tmpp=response;
            response=socket.recv(rbuffer,sizeof(rbuffer));
            printf("%d\n",response);
            data_string+=rbuffer;
            if(tmpp+response!=1699){
                response=socket.recv(rbuffer,sizeof(rbuffer));
                printf("%d\n",response);
                continue;
            }
        }//grouping=3のとき

        vector<std::string> all_res;//すべてのレスポンス
        vector<std::string> msg_res;//メッセージレスポンス
        vector<std::string> scan_res;//スキャンデータレスポンス
        vector<int> mem_msg;//なにも書いてない行の行数

        //printf("data_string=%s\n",data_string.c_str());

        //\nの制御文字があるところで行を分けて格納する ついでに虚無の行を見つける
        std::stringstream ss{data_string};
        std::string buf;
        int i=0;
        while (std::getline(ss, buf, '\n')) {
            all_res.push_back(buf);
            //printf("buf=%s\n",buf.c_str());
            if(buf[0]==NULL){mem_msg.push_back(i);printf("ssssssssss%d\n",i);}
            i++;
        }

        mem_msg.push_back(all_res.size());//最終行を追加してバグ回避
        printf("size==%d\n",all_res.size());

        /*
        for(int i=0;i<all_res.size();i++){
            printf("all_msg=%d |%s\n",i,all_res[i].c_str());
        }

        for(int i=0;i<mem_msg.size();i++){
            printf("mem_msg=%d\n",mem_msg[i]);
        }
        */
        int s=0;
        string text_msg=MD_MSG;
        //all_resをそれぞれ意味ごとに分ける
        for(int i=0;i<all_res.size();i++){
            if(all_res[i]==text_msg && all_res[i+1]=="00P"){
                //ここの実装ひどすぎるのでなんとかして
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

        /*
        for(int i=0;i<msg_res.size();i++){
            printf("res_msg=%s\n",msg_res[i].c_str());
        }
        for(int i=0;i<scan_res.size();i++){
            printf("scan_msg=%s\n",scan_res[i].c_str());
        }
        */

        printf("kk\n");

        string distance_string="";

        for(int i=0;i<scan_res.size();i++){
            scan_res[i].erase(scan_res[i].size()-1);//チェックサム削除
        }
        //1行にまとめる
        for(int i=0;i<scan_res.size();i++){
            distance_string+=scan_res[i];
        }

        //printf("distance_string=%s\n",distance_string.c_str());

        //デコード
        for(int i=0;i<distance_string.size();i++){
            if(distance_string.size()>0 && (distance_string.size()%3)==0){
                int distance_string_index=0;
                int distance_index=0;

                while(distance_string_index<distance_string.size()){
                    int distance=decode(distance_string.substr(distance_string_index,3));
                    all_distance[distance_index]=distance;
                    distance_index++;
                    distance_string_index+=3;
                }
            }
        }

        printf("sizeof(all_distance) == %d\n",sizeof(all_distance)/sizeof(all_distance[0]));//データ数表示
        
        for(int j=0;j<sizeof(all_distance)/sizeof(all_distance[0]);j++){
            printf("%d ",all_distance[j]);
        }
         
        //printf("%d",all_distance[10]);      

        //printf("\nloooooooooooooooooooooooooooooooooooooooooooooooooooooooooop\n");
    }
    
    response_message=tcp_transmission(socket,destination_address,"QT\n");//計測終了

    printf("fin\n");

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
