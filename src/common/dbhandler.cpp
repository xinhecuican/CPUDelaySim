#include "common/dbhandler.h"
#include <lz4.h>
#include "config.h"

BS::thread_pool pool;
std::vector<LogDB*> DBHandler::dbs;

LogDB::LogDB(const std::string& dbname) {
    this->db_path = Config::getLogFilePath(dbname + ".db");
    memset(buffer_slice_info, 0, sizeof(buffer_slice_info) * BUFFER_SLICE);
    DBHandler::registerDB(this);
}

void LogDB::addMeta(const std::string& name, int size) {
    meta[name] = size;
}

void LogDB::addMeta(json data) {
    meta.merge_patch(data);
}

void LogDB::init() {
    file = std::fstream(db_path, std::ios::binary | std::ios::out);
    std::string dump = meta.dump();
    uint32_t size = dump.size();
    file << size << dump;
}

void LogDB::checkCompress() {
    if (buffer_slice_info[compress_slice].state == PROCESS) {
        pool.detach_task([src=inp_buffer[compress_slice], 
                         dst=out_buffer[compress_slice], 
                         size=buffer_slice_info[compress_slice].inp_buffer_idx,
                         info=&buffer_slice_info[compress_slice]](){
            int compressed_size = LZ4_compress_default(reinterpret_cast<const char*>(src), reinterpret_cast<char*>(dst), size, SLICE_BUFFER_SIZE);
            info->out_buffer_size = compressed_size;
            info->state = DONE;
        });
        compress_slice = (compress_slice + 1) % BUFFER_SLICE;
    }
}

void LogDB::checkWriteback() {
    if (buffer_slice_info[writeback_slice].state == DONE && !is_writing) {
        is_writing = true;
        pool.detach_task([dst=out_buffer[writeback_slice], 
                         size=buffer_slice_info[writeback_slice].out_buffer_size,
                         info=&buffer_slice_info[writeback_slice],
                         file=&file,
                         is_writing_ptr=&is_writing](){
            file->write(reinterpret_cast<const char*>(dst), size);
            info->state = FREE;
            info->inp_buffer_idx = 0;
            info->out_buffer_size = 0;
            *is_writing_ptr = false;
        });
        writeback_slice = (writeback_slice + 1) % BUFFER_SLICE;
    }
}

void LogDB::close() {
    if (buffer_slice_info[process_slice].inp_buffer_idx != 0) {
        buffer_slice_info[process_slice].state = PROCESS;
    }
    while (buffer_slice_info[compress_slice].state == PROCESS) {
        buffer_slice_info[compress_slice].out_buffer_size = LZ4_compress_default(
                        reinterpret_cast<const char*>(inp_buffer[compress_slice]), reinterpret_cast<char*>(out_buffer[compress_slice]), 
                        buffer_slice_info[compress_slice].inp_buffer_idx, SLICE_BUFFER_SIZE);
        buffer_slice_info[compress_slice].state = DONE;
        compress_slice = (compress_slice + 1) % BUFFER_SLICE;
    }

    while (buffer_slice_info[writeback_slice].state == DONE) {
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