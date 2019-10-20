#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketStream.h>

// That's too "simple" and written only for tests
class SimpleEchoServer {
public:
    explicit SimpleEchoServer(int port) : listener_(port) {
        auto server_socket = std::thread([this] {
            while (true) {
                auto client_conn = listener_.acceptConnection();
                auto client_thread = std::thread([this, client_conn]() mutable {
                    Poco::FIFOBuffer buf(1024);
                    while (true) {
                        int bytes = client_conn.receiveBytes(buf);
                        if (bytes == 0) {
                            return;
                        }
                        std::string data(bytes, 0);
                        buf.read(data.data(), bytes);
                        Poco::Net::SocketStream(client_conn) << data << std::flush;
                    }
                });
                client_thread.detach();
            }
        });
        server_socket.detach();
    }

private:
    Poco::Net::ServerSocket listener_;
};