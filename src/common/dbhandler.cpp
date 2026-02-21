#include "common/dbhandler.h"
#include <zstd.h>
#include "config.h"
#include "common/common.h"
#include "common/log.h"

BS::thread_pool pool;
std::vector<LogDB*> DBHandler::dbs;

LogDB::LogDB(const std::string& dbname) {
    this->db_path = Config::getLogFilePath(dbname + ".db");
    memset(buffer_slice_info, 0, sizeof(buffer_slice_info));
    DBHandler::registerDB(this);
    meta["type_info"] = json::array();
    meta["origin_size"] = 0xffffffffffffffffULL;
#ifdef LOG_DBHANDLER
    Log::init("LogDB", "", true);
#endif

    for (int i = 0; i < BUFFER_SLICE; i++) {
        buffer_slice_info[i].zstd_ctx = ZSTD_createCCtx();
    }
}

LogDB::~LogDB() {
    for (int i = 0; i < BUFFER_SLICE; i++) {
        ZSTD_freeCCtx(buffer_slice_info[i].zstd_ctx);
    }
}



void LogDB::addMeta(const std::string& name, int size) {
    meta["type_info"].push_back(
        {{"name", name}, {"size", size}}
    );
}

void LogDB::addMeta(const std::string& name, int size, const std::string& description) {
    meta["type_info"].push_back(
        {{"name", name}, {"size", size}, {"description", description}}
    );
}

void LogDB::addMetaTypeName(const std::string& metaName, std::string* typeNames, int size) {
    if (!meta.contains("type_names")) {
        meta["type_names"] = json::object();
    }
    meta["type_names"][metaName] = json::array();
    for (int i = 0; i < size; i++) {
        meta["type_names"][metaName].push_back(typeNames[i]);
    }
}



void LogDB::setPrimaryKey(const std::string& name) {
    meta["primary_key"] = name;
}

void LogDB::addMeta(json data) {
    meta.merge_patch(data);
}

void LogDB::init() {
    file = std::fstream(db_path, std::ios::binary | std::ios::out);
    std::string dump = meta.dump();
    uint32_t size = dump.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    uint8_t placehold = 0;
    for (int i = 0; i < size; i++) {
        file.write(reinterpret_cast<const char*>(&placehold), 1);
    }
}

void LogDB::addData(uint8_t* buffer, uint32_t size) {
    std::unique_lock<std::mutex> lock(mtx);
    while (buffer_slice_info[process_slice].state != FREE) {
        cv.wait(lock);
#ifdef LOG_DBHANDLER
        Log::trace("LogDB", "Wait for process slice {} to be free", process_slice);
#endif
        checkCompress();
        checkWriteback();
    }
    lock.unlock();
    
    memcpy(&inp_buffer[process_slice][buffer_slice_info[process_slice].inp_buffer_idx], buffer, size);
    origin_size += size;
    uint32_t upd_idx = buffer_slice_info[process_slice].inp_buffer_idx + size;
    buffer_slice_info[process_slice].inp_buffer_idx = upd_idx;
    if (upd_idx >= SLICE_BUFFER_SIZE) {
        buffer_slice_info[process_slice].state = PROCESS_READY;
        process_slice = (process_slice + 1) % BUFFER_SLICE;
    }
    checkCompress();
    checkWriteback();
}

void LogDB::checkCompress() {
    if (buffer_slice_info[compress_slice].state == PROCESS_READY) {
        buffer_slice_info[compress_slice].state = PROCESSING;
#ifdef LOG_DBHANDLER
        Log::trace("LogDB", "Compress slice {} with size {}", compress_slice, buffer_slice_info[compress_slice].inp_buffer_idx);
#endif
        pool.detach_task([src=inp_buffer[compress_slice], 
                         dst=out_buffer[compress_slice], 
                         size=buffer_slice_info[compress_slice].inp_buffer_idx,
                         info=&buffer_slice_info[compress_slice],
                         &cv=this->cv,
                         &mtx=this->mtx](){
            int compressed_size = ZSTD_compressCCtx(info->zstd_ctx, dst, SLICE_BUFFER_SIZE, src, size, ZSTD_CLEVEL_DEFAULT);
            info->out_buffer_size = compressed_size;
#ifdef LOG_DBHANDLER
            Log::trace("LogDB", "Compress done with size {}", compressed_size);
#endif
            std::lock_guard<std::mutex> lock(mtx);
            info->state = PROCESS_DONE;
            cv.notify_one();
        });
        compress_slice = (compress_slice + 1) % BUFFER_SLICE;
    }
}

void LogDB::checkWriteback() {
    if (buffer_slice_info[writeback_slice].state == PROCESS_DONE && !is_writing) {
        is_writing = true;
        buffer_slice_info[writeback_slice].state = DONE;
#ifdef LOG_DBHANDLER
        Log::trace("LogDB", "Writeback slice {} with size {}", writeback_slice, buffer_slice_info[writeback_slice].out_buffer_size);
#endif
        pool.detach_task([dst=out_buffer[writeback_slice], 
                         size=buffer_slice_info[writeback_slice].out_buffer_size,
                         info=&buffer_slice_info[writeback_slice],
                         file=&file,
                         is_writing_ptr=&is_writing,
                         &cv=this->cv,
                         &mtx=this->mtx](){
            file->write(reinterpret_cast<const char*>(dst), size);
            info->inp_buffer_idx = 0;
            info->out_buffer_size = 0;
            *is_writing_ptr = false;
#ifdef LOG_DBHANDLER
            Log::trace("LogDB", "Writeback done with size {}", size);
#endif
            std::lock_guard<std::mutex> lock(mtx);
            info->state = FREE;
            cv.notify_one();
        });
        writeback_slice = (writeback_slice + 1) % BUFFER_SLICE;
    }
}

void LogDB::close() {
#ifdef LOG_DBHANDLER
    Log::trace("LogDB", "close db {} {} {}", process_slice, compress_slice, buffer_slice_info[process_slice].inp_buffer_idx);
#endif
    meta["origin_size"] = origin_size;
    std::string dump = meta.dump();
    uint32_t size = dump.size();
    file.seekp(sizeof(uint32_t), std::ios::beg);
    file.write(dump.c_str(), size);
    file.seekp(0, std::ios::end);
    if (buffer_slice_info[process_slice].inp_buffer_idx != 0) {
        buffer_slice_info[process_slice].state = PROCESS_READY;
    }
    while (buffer_slice_info[compress_slice].state == PROCESS_READY) {
        buffer_slice_info[compress_slice].out_buffer_size = ZSTD_compressCCtx(
                        buffer_slice_info[compress_slice].zstd_ctx,
                        out_buffer[compress_slice], SLICE_BUFFER_SIZE, 
                        inp_buffer[compress_slice], buffer_slice_info[compress_slice].inp_buffer_idx, ZSTD_CLEVEL_DEFAULT);
        buffer_slice_info[compress_slice].state = PROCESS_DONE;
        compress_slice = (compress_slice + 1) % BUFFER_SLICE;
    }

    while (buffer_slice_info[writeback_slice].state == PROCESS_DONE) {
#ifdef LOG_DBHANDLER
        Log::trace("LogDB", "Writeback with size {}", buffer_slice_info[writeback_slice].out_buffer_size);
#endif
        file.write(reinterpret_cast<const char*>(out_buffer[writeback_slice]), buffer_slice_info[writeback_slice].out_buffer_size);
        buffer_slice_info[writeback_slice].state = FREE;
        writeback_slice = (writeback_slice + 1) % BUFFER_SLICE;
    }

    file.close();
}

void DBHandler::registerDB(LogDB* db) {
    dbs.push_back(db);
}

void DBHandler::closeDB() {
    pool.wait();
    for (auto db : dbs) {
        db->close();
    }
}