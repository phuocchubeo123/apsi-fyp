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
    }
}