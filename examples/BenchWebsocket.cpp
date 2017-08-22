#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>

#include <brynet/net/SocketLibFunction.h>
#include <brynet/net/http/HttpService.h>
#include <brynet/net/http/HttpFormat.h>
#include <brynet/net/http/WebSocketFormat.h>
#include <brynet/utils/packet.h>
#include <brynet/utils/systemlib.h>

using namespace brynet;
using namespace brynet::net;

std::atomic<int32_t> count;

static void sendPacket(HttpSession::PTR session, const char* data, size_t len)
{
    char buff[1024];
    BasePacketWriter bw(buff, sizeof(buff), true, true);
    bw.writeINT8('{');
    bw.writeINT32(len + 14);
    bw.writeINT32(10000);
    bw.writeINT32(0);
    bw.writeBuffer(data, len);
    bw.writeINT8('}');

    auto frame = std::make_shared<std::string>();
    WebSocketFormat::wsFrameBuild(bw.getData(), bw.getPos(), *frame, WebSocketFormat::WebSocketFrameType::TEXT_FRAME, true, true);
    session->send(frame);
}

int main(int argc, char **argv)
{
    auto server = HttpService::Create();

    server->startWorkThread(ox_getcpunum());

    for (int i = 0; i < 200; i++)
    {
        sock fd = ox_socket_connect(false, "192.168.2.78", 8008);
        server->addConnection(fd, [](const HttpSession::PTR& session){
            HttpRequest request;
            request.setMethod(HttpRequest::HTTP_METHOD::HTTP_METHOD_GET);
            request.setUrl("/ws");
            request.addHeadValue("Host", "192.168.2.78");
            request.addHeadValue("Upgrade", "websocket");
            request.addHeadValue("Connection", "Upgrade");
            request.addHeadValue("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");
            request.addHeadValue("Sec-WebSocket-Version", "13");

            std::string requestStr = request.getResult();
            session->send(requestStr.c_str(), requestStr.size());
        }, [](const HTTPParser& httpParser, const HttpSession::PTR& session){
        }, [](const HttpSession::PTR& session, WebSocketFormat::WebSocketFrameType, const std::string& payload){
            sendPacket(session, "hello world", 10);
            count += 1;
        }, [](const HttpSession::PTR& session){
        }, [](const HttpSession::PTR& session, const HTTPParser&){
            for (int i = 0; i < 200; i++)
            {
                sendPacket(session, "hello world", 10);
            }
        });
    }

    brynet::net::EventLoop mainLoop;
    while (true)
    {
        mainLoop.loop(5000);
        std::cout << (count / 5) << std::endl;
        count = 0;
    }
    std::cin.get();
}
