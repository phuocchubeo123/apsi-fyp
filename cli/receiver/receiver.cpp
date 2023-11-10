// STD
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

// APSI
#include "apsi/log.h"
#include "apsi/network/zmq/zmq_channel.h"
#include "apsi/receiver.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/version.h"
#include "common/common_utils.h"
#include "common/csv_reader.h"
#include "receiver/clp.h"

using namespace std;
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace apsi;
using namespace apsi::util;
using namespace apsi::receiver;
using namespace apsi::network;

namespace {
    struct Colors {
        static const string Red;
        static const string Green;
        static const string RedBold;
        static const string GreenBold;
        static const string Reset;
    };

    const string Colors::Red = "\033[31m";
    const string Colors::Green = "\033[32m";
    const string Colors::RedBold = "\033[1;31m";
    const string Colors::GreenBold = "\033[1;32m";
    const string Colors::Reset = "\033[0m";
} // namespace

int remote_query(const CLP &cmd);

string get_conn_addr(const CLP &cmd);

pair<unique_ptr<CSVReader::DBData>, vector<string>> load_db(const string &db_file);

int main(int argc, char *argv[])
{
    CLP cmd("Example of a Receiver implementation", APSI_VERSION);
    if (!cmd.parse_args(argc, argv)) {
        APSI_LOG_ERROR("Failed parsing command line arguments");
        return -1;
    }

    return remote_query(cmd);
}

int remote_query(const CLP &cmd)
{
    // Connect to the network
    ZMQReceiverChannel channel;
    string conn_addr = get_conn_addr(cmd);
    APSI_LOG_INFO("Connecting to " << conn_addr);
    channel.connect(conn_addr);
    if (channel.is_connected()) {
        APSI_LOG_INFO("Successfully connected to " << conn_addr);
    } else {
        APSI_LOG_WARNING("Failed to connect to " << conn_addr);
        return -1;
    }
}