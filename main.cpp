#include <tunnel/AccessPoint.h>
#include <tunnel/Connector.h>
#include <tunnel/Connector.pb.h>
#include <tunnel/config/Config.h>
#include <thread>

#include <Poco/Net/SocketStream.h>
#include <tunnel/Connector.h>
#include <tunnel/redirector/SocksToNatsRedirector.h>

int main() {
/*
    auto nats_client = nats::Connect(Poco::Net::SocketAddress{"127.0.0.1:4222"}, "foo");
    SocksToNatsRedirector redir(nats_client.get(), 1);
    Poco::Net::StreamSocket conn(Poco::Net::SocketAddress{"127.0.0.1:3425"});
    //conn.setBlocking(false);
    redir.AddSocket("a", ReadSocketDescriptor(conn, "abcd"));
    redir.Start();
    std::thread listener([nats_client = nats_client.get()] {
        auto sub = nats_client->Subscribe("abcd");
        int bytes_received = 0;
        for (int i = 0; i < 1000; ++i) {
            try {
                auto mes = sub->NextMessage(12ms);
                // std::cerr << mes->GetBody() << ' ' << mes->GetSize() << '\n';
                bytes_received += mes->GetSize() - 2;
            } catch (...) {
            }
        }
        std::cerr << bytes_received << '\n';
    });
    std::thread client([&conn] {
        for (int i = 0; i < 100; ++i) {
            std::string data(10, 0);
            Poco::Net::SocketStream(conn) << data;
        }
    });

    std::this_thread::sleep_for(10s);
    redir.Stop();
    client.join();
    listener.join();

*/
    Connector connector(Config{"config.json"},2);
    connector.Start();
    auto nats_client = nats::Connect(Poco::Net::SocketAddress("127.0.0.1:4222"), "foo");
    nats::RpcChannel chan(nats_client.get(), "rpc:C1", 100s);
    ConnectorServiceClient connector_client(&chan);

    std::thread listener([nats_client = nats_client.get()] {
        auto sub = nats_client->Subscribe("subj");
        while (!nats_client->IsClosed()) {
            try {
                auto mes = sub->NextMessage(100ms);
                std::cerr << mes->GetBody() << '\n';
            } catch (const nats::NatsException&) {
            }
        }
    });
    std::this_thread::sleep_for(50ms);

    TunnelRequest req;
    req.set_target_host("127.0.0.1");
    req.set_target_port(3425);
    req.set_connection_id("lu4u");
    req.set_data_subject("subj");
    connector_client.BuildTunnel(req);
    req.set_connection_id("tutu");
    connector_client.BuildTunnel(req);

    for (int i = 0; i < 20; ++i) {
        nats_client->PublishString(Connector::kDataPrefix + "C1", "lu4u|4mo");
        nats_client->PublishString(Connector::kDataPrefix + "C1", "tutu|hello");
    }
    std::this_thread::sleep_for(200ms);
    connector.Stop();
    nats_client->Close();
    listener.join();
    return 0;
}