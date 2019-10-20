#include <gtest/gtest.h>
#include <nats/rpc/Rpc.h>
#include <tests/rpc_test.pb.h>
#include <thread>

class TestServiceDummy : public TestServiceHandler {
public:
    void Echo(const TestRequest &request, TestResponse *response) override {
        response->set_results(request.query());
    }

    void Ping(const PingRequest &request, PingResponse *response) override {
        throw std::runtime_error("pong");
    }
};

TEST(RPC, Correctness) {
    std::unique_ptr<nats::Client> nats_client;
    try {
        nats_client = nats::Connect(Poco::Net::SocketAddress{"127.0.0.1", 4222}, "foo");
    } catch (const nats::NatsException &) {
        std::cout << "Test isn't run. To run tests that use nats you need to start a nats-server "
                     "on port 4222 with auth token \"foo\"\n";
        return;
    }
    TestServiceDummy dummy;
    nats::RpcServer server(nats_client.get(), "subj");
    server.Register(&dummy);
    nats::RpcChannel channel(nats_client.get(), "subj", 5s);
    TestServiceClient client(&channel);
    std::thread server_thread([&server] { server.Serve(); });
    std::this_thread::sleep_for(50ms);
    TestRequest echo_request;
    echo_request.set_query("hello");
    ASSERT_EQ(client.Echo(echo_request)->results(), "hello");
    ASSERT_THROW(client.Ping(PingRequest{}), nats::RpcCallError);
    nats_client->Close();
    server_thread.join();
}