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

#include "apsi/hashing/cuckoo.h"

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

unique_ptr<CSVReader::DBData> load_db(const string &db_file);

shared_ptr<SenderDB> create_sender_db(
    const CSVReader::DBData &db_data,
    unique_ptr<PSIParams> psi_params);

int main(int argc, char *argv[]){
    prepare_console();

    CLP cmd("Example of a Sender implementation", APSI_VERSION);

    if (!cmd.parse_args(argc, argv)) {
        APSI_LOG_ERROR("Failed parsing command line arguments");
        return -1;
    }

    int* elements;
    int neles;
    int nbins;
    int elebitlen;
    int outbitlen;
    int nelesinbin;
    int perm;
    int ntasks; 
    prf_state_ctx prf_state;

    hash_table = cuckoo_hashing(elements, neles, nbins, elebitlen, &outbitlen,
			nelesinbin, perm, ntasks, prf_state);

    return start_sender(cmd);
}

void sigint_handler(int param [[maybe_unused]])
{
    APSI_LOG_WARNING("Sender interrupted");
    print_timing_report(sender_stopwatch);
    exit(0);
}

shared_ptr<SenderDB> try_load_csv_db(const CLP &cmd)
{
    APSI_LOG_INFO("Reading PSI parameters...")
    unique_ptr<PSIParams> params = build_psi_params(cmd);
    if (!params) {
        // We must have valid parameters given
        APSI_LOG_ERROR("Failed to set PSI parameters");
        return nullptr;
    }

    APSI_LOG_INFO("Successfully read PSI parameters!")

    unique_ptr<CSVReader::DBData> db_data;

    if (cmd.db_file().empty() || !(db_data = load_db(cmd.db_file()))) {
        // Failed to read db file
        APSI_LOG_DEBUG("Failed to load data from a CSV file");
        return nullptr;
    }

    return create_sender_db(
        *db_data, move(params));
}

int start_sender(const CLP& cmd){
    ThreadPoolMgr::SetThreadCount(cmd.threads());
    APSI_LOG_INFO("Setting thread count to " << ThreadPoolMgr::GetThreadCount());    
    signal(SIGINT, sigint_handler);

    // Check that the database file is valid
    throw_if_file_invalid(cmd.db_file());

    // Try loading first as CSV file
    shared_ptr<SenderDB> sender_db;

    APSI_LOG_INFO("Loading sender db...");

    if (!(sender_db = try_load_csv_db(cmd))) {
        APSI_LOG_ERROR("Failed to create SenderDB: terminating");
        return -1;
    }

    // Run the dispatcher
    atomic<bool> stop = false;
    ZMQSenderDispatcher dispatcher(sender_db);

    // The dispatcher will run until stopped.
    dispatcher.run(stop, cmd.net_port());
    return 0;
}

unique_ptr<CSVReader::DBData> load_db(const string &db_file)
{
    CSVReader::DBData db_data;
    try {
        CSVReader reader(db_file);
        tie(db_data, ignore) = reader.read();
    } catch (const exception &ex) {
        APSI_LOG_WARNING("Could not open or read file `" << db_file << "`: " << ex.what());
        return nullptr;
    }

    return make_unique<CSVReader::DBData>(move(db_data));
}

shared_ptr<SenderDB> create_sender_db(
    const CSVReader::DBData &db_data,
    unique_ptr<PSIParams> psi_params
    )
{
    if (!psi_params) {
        APSI_LOG_ERROR("No PSI parameters were given");
        return nullptr;
    }

    shared_ptr<SenderDB> sender_db;
    if (holds_alternative<CSVReader::UnlabeledData>(db_data)) {
        APSI_LOG_INFO("Currently in non labeled mode")
        try {
            sender_db = make_shared<SenderDB>(*psi_params);
            sender_db->set_data(get<CSVReader::UnlabeledData>(db_data));

            APSI_LOG_INFO(
                "Created unlabeled SenderDB with " << sender_db->get_item_count() << " items");
        } catch (const exception &ex) {
            APSI_LOG_ERROR("Failed to create SenderDB: " << ex.what());
            return nullptr;
        }
    } else if (holds_alternative<CSVReader::LabeledData>(db_data)) {
        APSI_LOG_ERROR("We currently not yet support labeled PSI")
    } else {
        // Should never reach this point
        APSI_LOG_ERROR("Loaded database is in an invalid state");
        return nullptr;
    }
    return sender_db;
}
