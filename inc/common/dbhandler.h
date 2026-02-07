#ifndef COMMON_DB_HANDLER_H
#define COMMON_DB_HANDLER_H
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include "BS_thread_pool.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <zstd.h>
using json = nlohmann::json;

class LogDB {
public:
    static constexpr uint64_t BUFFER_SIZE = 1024 * 1024 * 32;
    static constexpr uint64_t BUFFER_SLICE = 4;
    static constexpr uint64_t SLICE_SIZE = BUFFER_SIZE / BUFFER_SLICE;
    static constexpr uint64_t SLICE_BUFFER_SIZE = SLICE_SIZE + 1024;
    LogDB(const std::string& dbname);
    ~LogDB();
    void addMeta(const std::string& name, int size);
    void addMeta(const std::string& name, int size, const std::string& description);
    void setPrimaryKey(const std::string& name);
    void addMeta(json data);
    void addTypeName();
    void init();
    void addData(uint8_t* buffer, uint32_t size);
    template <typename T>
    void addData(T& data) {
        constexpr uint32_t size = sizeof(T);
        addData((uint8_t*)&data, size);
    }
    template <typename T>
    void addData(T* data) {
        constexpr uint32_t size = sizeof(T);
        addData(reinterpret_cast<uint8_t*>(data), size);
    }
    void close();

private:
    enum BufferState {
        FREE,
        PROCESS_READY,
        PROCESSING,
        PROCESS_DONE,
        DONE
    };
    struct BufferSlice {
        BufferState state;
        int inp_buffer_idx;
        int out_buffer_size;
        ZSTD_CCtx* zstd_ctx;
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
    std::mutex mtx;
    std::condition_variable cv;
    uint64_t origin_size = 0;
};

class DBHandler {
public:
    static void registerDB(LogDB* db);
    static void closeDB();

private:
    static std::vector<LogDB*> dbs;
};

#endif