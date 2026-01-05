#include "cache/cache.h"
#include "cache/replace/lru.h"

void Cache::setParent(Cache* parent) {
    this->parent = parent;
}

Cache::~Cache() {
    for (int i = 0; i < set_size; i++) {
        for (int j = 0; j < way; j++) {
            delete tagvs[i][j];
        }
        delete[] tagvs[i];
    }
    delete[] tagvs;
}

void Cache::splitAddr(uint64_t addr, uint64_t& tag, uint32_t& set, uint32_t& offset) {
    set = (addr >> index_offset) & set_mask;
    offset = addr & line_mask;
    tag = addr >> tag_offset;
}

uint32_t Cache::getOffset(uint64_t addr) {
    return addr & line_mask;
}

CacheTagv* Cache::match(uint64_t tag, uint32_t set) {
    for (int i = 0; i < way; i++) {
        if (tagvs[set][i]->valid && tagvs[set][i]->tag == tag) {
            return tagvs[set][i];
        }
    }
    return nullptr;
}

void Cache::setCallback(cache_callback_t const & callback) {
    callbacks.push_back(callback);
}

void Cache::afterLoad() {
    if (replace == nullptr) {
        spdlog::error("Replace policy not set");
        exit(1);
    }
    if (replace_method == "lru") {
        replace = new LRUReplace();
        replace->setParams(2, set_size, way);
    }
    line_byte = line_size / 8;
    tag_offset = log2(line_byte) + log2(set_size);
    index_offset = log2(line_byte);
    set_mask = set_size - 1;
    line_mask = line_byte - 1;

    tagvs = new CacheTagv**[set_size];
    for (int i = 0; i < set_size; i++) {
        tagvs[i] = new CacheTagv*[way];
        for (int j = 0; j < way; j++) {
            tagvs[i][j] = new CacheTagv();
        }
    }
}