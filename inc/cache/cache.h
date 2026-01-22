#ifndef CACHE_H
#define CACHE_H
#include "common/base.h"
#include "cache/replace/replace.h"

enum packed snoop_req_t {
    SNOOP_NONE = 0,
    READ_ONCE = 1,
    READ_SHARED = 2,
    READ_CLEAN = 3,
    READ_UNIQUE = 4,
    MAKE_UNIQUE = 5,
    CLEAN_SHARED = 6,
    CLEAN_INVALID = 7,
    CLEAN_UNIQUE = 8,
    MAKE_INVALID = 9,
    READ_END = 9,
    WRITE_UNIQUE = 10,
    WRITE_CLEAN = 11,
    WRITE_BACK = 12,
    WRITE_EVICT = 13
};

struct CacheTagv {
    uint64_t tag;
    bool valid;
    bool shared;
    bool dirty;
};

typedef std::function<void(uint16_t*, CacheTagv*)> cache_callback_t;

struct CacheReq {
    snoop_req_t req;
    uint32_t size;
    uint16_t id[4];
    uint64_t addr;
};


class Cache : public Base {
public:
    virtual ~Cache();
    /**
     * @brief cache lookup, return true if cache receive reqeuest
     * 
     * @param addr address to lookup
     * @param req snoop request type
     * @param size size to lookup
     * @return whether the lookup is successful
     */
    virtual bool lookup(int callback_id, CacheReq* req) = 0;
    virtual void tick() {}
    virtual void load() override;
    virtual void afterLoad();
    /**
     * @brief flush cache
     * 
     * @param addr address to flush
     * @param asid cache id
     */
    virtual void flush(uint64_t addr, uint32_t asid){}
    /**
     * @brief remove current cache access, reset state
     */
    virtual void redirect() {}
    void setParent(Cache* parent);
    void splitAddr(uint64_t addr, uint64_t& tag, uint32_t& set, uint32_t& offset);
    uint32_t getOffset(uint64_t addr);
    uint8_t setCallback(cache_callback_t const & callback);
    int getLineSize() { return line_size; }
    int getLevel() { return level; }

protected:
    CacheTagv* match(uint64_t tag, uint32_t set);

protected:
    /**
     * @ingroup config
     * @brief cache line size
     */
    int line_size = 64;
    /**
     * @ingroup config
     * @brief cache set size
     */
    int set_size = 64;
    /**
     * @ingroup config
     * @brief cache way num
     */
    int way = 8;
    /**
     * @ingroup config
     * @brief cache access delay
     */
    int delay = 1;
    /**
     * @ingroup config
     * @brief cache level, 1 is the L1 cache
     */
    int level = 0;
    /**
     * @ingroup config
     * @brief cache replace method
     */
    std::string replace_method = "lru";

    int line_byte = line_size / 8;
    Cache* parent = nullptr;
    CacheTagv*** tagvs;
    uint32_t tag_offset;
    uint32_t index_offset;
    uint32_t set_mask;
    uint32_t line_mask;
    Replace* replace = nullptr;
    uint8_t callback_id = 0;
    std::vector<cache_callback_t> callbacks;
};

#endif