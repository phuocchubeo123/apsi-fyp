// STD
#include <cstddef>
#include <iterator>
#include <sstream>
#include <stdexcept>

// APSI
#include "apsi/fourq/random.h"
#include "apsi/log.h"
#include "apsi/network/result_package_generated.h"
#include "apsi/network/sop_generated.h"
#include "apsi/network/sop_header_generated.h"
#include "apsi/network/zmq/zmq_channel.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/randomgen.h"
#include "seal/util/streambuf.h"

// ZeroMQ
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "zmq.hpp"
#include "zmq_addon.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace zmq;

namespace apsi {
    using namespace util;

    namespace network {
        namespace {
            template <typename T>
            size_t load_from_string(string data, T &obj)
            {
                ArrayGetBuffer agbuf(
                    reinterpret_cast<const char *>(data.data()),
                    static_cast<streamsize>(data.size()));
                istream stream(&agbuf);
                return obj.load(stream);
            }

            template <typename T>
            size_t load_from_string(string data, shared_ptr<SEALContext> context, T &obj)
            {
                ArrayGetBuffer agbuf(
                    reinterpret_cast<const char *>(data.data()),
                    static_cast<streamsize>(data.size()));
                istream stream(&agbuf);
                return obj.load(stream, move(context));
            }

            template <typename T>
            size_t save_to_message(const T &obj, multipart_t &msg)
            {
                stringstream ss;
                size_t size = obj.save(ss);
                msg.addstr(ss.str());
                return size;
            }

            template <>
            size_t save_to_message(const vector<unsigned char> &obj, multipart_t &msg)
            {
                msg.addmem(obj.data(), obj.size());
                return obj.size();
            }

            vector<unsigned char> get_client_id(const multipart_t &msg)
            {
                size_t client_id_size = msg[0].size();
                vector<unsigned char> client_id(client_id_size);
                copy_bytes(msg[0].data(), client_id_size, client_id.data());
                return client_id;
            }
        } //namespace

        ZMQChannel::ZMQChannel() : end_point_(""), context_(make_unique<context_t>())
        {}

        ZMQChannel::~ZMQChannel()
        {
            if (is_connected()) {
                disconnect();
            }
        }

        void ZMQChannel::bind(const string &end_point)
        {
            throw_if_connected();

            try {
                end_point_ = end_point;
                get_socket()->bind(end_point);
            } catch (const zmq::error_t &) {
                APSI_LOG_ERROR("ZeroMQ failed to bind socket to endpoint " << end_point);
                throw;
            }
        }

        void ZMQChannel::connect(const string &end_point)
        {
            throw_if_connected();

            try {
                end_point_ = end_point;
                get_socket()->connect(end_point);
            } catch (const zmq::error_t &) {
                APSI_LOG_ERROR("ZeroMQ failed to connect socket to endpoint " << end_point);
                throw;
            }
        }

        void ZMQChannel::disconnect()
        {
            throw_if_not_connected();

            // Cannot use get_socket() in disconnect(): this function is called by the destructor
            // and get_socket() is virtual. Instead just do this.
            if (nullptr != socket_) {
                socket_->close();
            }
            if (context_) {
                context_->shutdown();
                context_->close();
            }

            end_point_ = "";
            socket_.reset();
            context_.reset();
        }

        void ZMQChannel::throw_if_not_connected() const
        {
            if (!is_connected()) {
                APSI_LOG_ERROR("Socket is not connected");
                throw runtime_error("socket is not connected");
            }
        }

        void ZMQChannel::throw_if_connected() const
        {
            if (is_connected()) {
                APSI_LOG_ERROR("Socket is already connected");
                throw runtime_error("socket is already connected");
            }
        }

        unique_ptr<socket_t> &ZMQChannel::get_socket()
        {
            if (nullptr == socket_) {
                socket_ = make_unique<socket_t>(*context_.get(), get_socket_type());
                set_socket_options(socket_.get());
            }

            return socket_;
        }

        zmq::socket_type ZMQReceiverChannel::get_socket_type()
        {
            return zmq::socket_type::dealer;
        }

        void ZMQReceiverChannel::set_socket_options(socket_t *socket)
        {
            // Ensure messages are not dropped
            socket->set(sockopt::rcvhwm, 70000);

            string buf;
            buf.resize(32);
            random_bytes(
                reinterpret_cast<unsigned char *>(&buf[0]), static_cast<unsigned int>(buf.size()));
            // make sure first byte is _not_ zero, as that has a special meaning for ZeroMQ
            buf[0] = 'A';
            socket->set(sockopt::routing_id, buf);
        }

        zmq::socket_type ZMQSenderChannel::get_socket_type()
        {
            return zmq::socket_type::router;
        }

        void ZMQSenderChannel::set_socket_options(socket_t *socket)
        {
            // Ensure messages are not dropped
            socket->set(sockopt::sndhwm, 70000);
        }
    }
}