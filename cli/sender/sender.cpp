// STD
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

// APSI
#include "apsi/log.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/version.h"
#include "apsi/zmq/sender_dispatcher.h"
#include "common/common_utils.h"
#include "common/csv_reader.h"
#include "sender/clp.h"
#include "sender/sender_utils.h"

using namespace std;
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace apsi;
using namespace apsi::util;
using namespace apsi::sender;
using namespace apsi::oprf;

int start_sender(const CLP &cmd);

int main(int argc, char *argv[]){
    prepare_console();

    CLP cmd("Example of a Sender implementation", APSI_VERSION);

    if (!cmd.parse_args(argc, argv)) {
        APSI_LOG_ERROR("Failed parsing command line arguments");
        return -1;
    }

    return start_sender(cmd);
}

void sigint_handler(int param [[maybe_unused]])
{
    APSI_LOG_WARNING("Sender interrupted");
    print_timing_report(sender_stopwatch);
    exit(0);
}

shared_ptr<SenderDB> try_load_sender_db(const CLP &cmd, OPRFKey &oprf_key)
{
    shared_ptr<SenderDB> result = nullptr;

    ifstream fs(cmd.db_file(), ios::binary);
    fs.exceptions(ios_base::badbit | ios_base::failbit);

    return 0;
}

shared_ptr<SenderDB> try_load_csv_db(const CLP &cmd, OPRFKey &oprf_key)
{
    unique_ptr<PSIParams> params = build_psi_params(cmd);
    if (!params) {
        // We must have valid parameters given
        APSI_LOG_ERROR("Failed to set PSI parameters");
        return nullptr;
    }

    unique_ptr<CSVReader::DBData> db_data;

    return param
}

int start_sender(const CLP& cmd){
    ThreadPoolMgr::SetThreadCount(cmd.threads());
    APSI_LOG_INFO("Setting thread count to " << ThreadPoolMgr::GetThreadCount());    
    signal(SIGINT, sigint_handler);

    // Check that the database file is valid
    throw_if_file_invalid(cmd.db_file());

    // Try loading first as a SenderDB, then as a CSV file
    shared_ptr<SenderDB> sender_db;
    OPRFKey oprf_key;

    if (!(sender_db = try_load_sender_db(cmd, oprf_key)) &&
        !(sender_db = try_load_csv_db(cmd, oprf_key))) {
        APSI_LOG_ERROR("Failed to create SenderDB: terminating");
        return -1;
    }

    return 0;
}