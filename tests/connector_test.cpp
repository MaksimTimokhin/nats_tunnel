#include <Poco/Net/SocketStream.h>
#include <Poco/Net/StreamSocket.h>
#include <gtest/gtest.h>
#include <thread>
#include "SimpleEchoServer.h"

TEST(Connector, Tunnels) {
    SimpleEchoServer echo(3425);
    Poco::Net::StreamSocket serv_conn(Poco::Net::SocketAddress("127.0.0.1:3425"));
    Poco::Net::SocketStream str(serv_conn);
    str << "hello" << std::flush;
    std::string data(5, 0);
    serv_conn.receiveBytes(data.data(), data.size());
    ASSERT_EQ(data, "hello");
}