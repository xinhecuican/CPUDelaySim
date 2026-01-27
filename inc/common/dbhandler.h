#ifndef COMMON_DB_HANDLER_H
#define COMMON_DB_HANDLER_H
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "BS_thread_pool.hpp"
#include <vector>
using json = nlohmann::json;

class LogDB {
public:
    static constexpr uint64_t BUFFER_SIZE = 1024 * 1024 * 16;
    static constexpr uint64_t BUFFER_SLICE = 4;
    static constexpr uint64_t SLICE_SIZE = BUFFER_SIZE / BUFFER_SLICE;
    static constexpr uint64_t SLICE_BUFFER_SIZE = SLICE_SIZE + 1024;
    LogDB(const std::string& dbname);
    void addMeta(const std::string& name, int size);
    void addMeta(json data);
    void init();
    template <typename T>
    void addData(T& data) {
        constexpr uint32_t size = sizeof(data);
        memcpy(&inp_buffer[process_slice][buffer_slice_info[process_slice].inp_buffer_idx], &data, size);
        uint32_t upd_idx = buffer_slice_info[process_slice].inp_buffer_idx + size;
        if (upd_idx >= SLICE_BUFFER_SIZE) {
            buffer_slice_info[process_slice].state = PROCESS;
        }
        buffer_slice_info[process_slice].inp_buffer_idx = upd_idx;
        checkCompress();
        checkWriteback();
    }
    void close();

private:
    enum BufferState {
        FREE,
        PROCESS,
        DONE
    };
    struct BufferSlice {
        BufferState state;
        int inp_buffer_idx;
        int out_buffer_size;
    };
    void checkCompress();
    void checkWriteback();

private:
    std::string db_path;
    std::fstream file;
    json meta;
    uint8_t inp_buffer[BUFFER_SLICE][SLICE_BUFFER_SIZE];
    uint8_t out_buffer[BUFFER_SLICE][SLICE_BUFFER_SIZE];
    BufferSlice buffer_slice_info[BUFFER_SLICE];
    int process_slice = 0;
    int compress_slice = 0;
    int writeback_slice = 0;
    bool is_writing = false;
};

class DBHandler {
public:
    static void registerDB(LogDB* db);
    static void closeDB();

private:
    static std::vector<LogDB*> dbs;
};

#endif