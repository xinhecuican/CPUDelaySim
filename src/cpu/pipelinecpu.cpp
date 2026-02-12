#include "cpu/pipelinecpu.h"
#include "cache/cachemanager.h"
#include "common/log.h"

PipelineCPU::~PipelineCPU() {
    for (int i = 0; i < id_extra_idx; i++) {
        delete id_extra_insts[i];
    }
    delete[] id_extra_insts;
    delete[] mem_end_map;
    mem_req_list.clear();
    cache_req_list.clear();
}

void PipelineCPU::afterLoad() {
    pc = Base::arch->getStartPC();
    pred_pc = pc;
    inst_count = 0;
    Base::arch->setInstret(&inst_count);
    Stats::registerStat(&inst_count, "inst_count", "total number of instructions");
    mem_end_map = new bool[retire_size];
    for (int i = 0; i < retire_size; i++) {
        CacheReqWrapper *req = initCacheReq();
        req->inst->info->exception = Base::arch->getExceptionNone();
        cache_req_list.push(req);
        CacheReq *mem_req = new CacheReq();
        mem_req->id[0] = i;
        mem_req_list.push(mem_req);
        mem_end_map[i] = false;
    }
    id_extra_insts = new Inst *[retire_size];
    for (int i = 0; i < retire_size; i++) {
        id_extra_insts[i] = new Inst();
        id_extra_insts[i]->info->exception = Base::arch->getExceptionNone();
    }

    icache = CacheManager::getInstance().getICache();
    dcache = CacheManager::getInstance().getDCache();
    icache->setCallback([this](uint16_t* id, CacheTagv* tag) {
        CacheReqWrapper *req_wrapper = this->cache_req_list.pop();
        if (!this->id_valid) {
            this->id_valid = true;
            bool finish = preId(req_wrapper->inst);
            this->id_stall_valid = !finish;
            this->id_stall_dec_more = !finish;
        } else {
            this->id_stall_valid = true;
            this->id_stall_inst = req_wrapper->inst;
            this->id_stall_inst_valid = true;
        }
    });
        
    dcache->setCallback([this](uint16_t* id, CacheTagv* tag) {
        this->mem_req_list.pop();
        this->mem_end_map[id[0]] = true;
    });
    predictor->afterLoad();
#ifdef DB_INST
    log_db = new LogDB("inst");
    log_db->addTypeName();
    log_db->addMeta("tick", 8);
    log_db->addMeta("paddr", 8);
    log_db->addMeta("type", 1);
    log_db->addMeta("result", 1);
    log_db->addMeta("id", 2);
    log_db->addMeta("exe", 2);
    log_db->addMeta("mem", 2);
    log_db->addMeta("wb", 2);
    int levels[] = {0, 5, 5, 5, 4, 3};
    log_db->addResultLevels(levels, RESULT_NUM);
    log_db->init();
#endif
}

void PipelineCPU::exec() {
    if (getTick() == 558546785) {
        Log::info("debug");
    }
    if (unlikely(getTick() - wb_tick > 5000)) {
        Log::error("PipelineCPU::exec: WB stage stalled for {} ticks", getTick() - wb_tick);
        ExitHandler::exit(1);
    }
    if (wb_valid) {
        if (Base::arch->exceptionValid(wb_inst->info->exception)) {
            wb_inst->result = wb_inst->info->exception & IRQ_MASK ? InstResult::INTERRUPT : InstResult::EXCEPTION;
            excRedirect(wb_inst);
        }
#ifdef DB_INST
        DBInstData db_inst(wb_inst);
        log_db->addData<DBInstData>(&db_inst);
#endif
        wb_valid = false;
        wb_inst->info->exception = Base::arch->getExceptionNone();
        wb_inst->result = InstResult::NORMAL;
        inst_count++;
        wb_tick = getTick();
    }
    bool mem_end = false;
    if (mem_valid) {
        if (!Base::arch->exceptionValid(mem_inst->info->exception)) {
            if (mem_inst->info->type >= MEM_START && mem_inst->info->type <= MEM_END) {
                mem_end = mem_end_map[mem_inst->mem_id];
                if (mem_end) {
                    mem_end_map[mem_inst->mem_id] = false;
                }
            } else {
                mem_end = true;
            }
        } else {
            mem_end = true;
        }

        if (mem_end) {
#ifdef DB_INST
            mem_inst->delay[3] = getTick() - mem_inst->start_tick;
#endif
            mem_valid = false;
            mem_end = false;
            wb_valid = true;
            wb_inst = mem_inst;
        }
    }

    if (exe_valid) {
        if (exe_stall_cycle) {
            exe_stall_cycle = exe_stall_cycle - 1;
            if (exe_stall_cycle == 0) {
                exe_end = true;
            }
        } else if (!exe_end) {
            if (Base::arch->exceptionValid(exe_inst->info->exception)) {
                exe_end = true;
            } else {
                CacheReq *mem_req = mem_req_list.back();
                switch (exe_inst->info->type) {
                case MULT:
                    exe_stall_cycle = mult_delay;
                    break;
                case DIV:
                    exe_stall_cycle = div_delay;
                    break;
                case FADD:
                    exe_stall_cycle = fadd_delay;
                    break;
                case FMUL:
                    exe_stall_cycle = fmul_delay;
                    break;
                case FMA:
                    exe_stall_cycle = fma_delay;
                    break;
                case FDIV:
                    exe_stall_cycle = fdiv_delay;
                    break;
                case FSQRT:
                    exe_stall_cycle = fsqrt_delay;
                    break;
                case FMISC_COMPLEX:
                    exe_stall_cycle = fmisc_complex_delay;
                    break;
                case COND: {
                    if ((exe_inst->info->dst_data[1] ^ exe_inst->taken) || 
                        exe_inst->next_pc != exe_inst->real_target) {
                        exe_inst->result = InstResult::PRED_FAIL;
                        brRedirect(exe_inst);
                    }
                    exe_end = true;
                    predictor->update(exe_inst->info->dst_data[1], exe_inst->real_pc, exe_inst->real_size,
                                      exe_inst->real_target, exe_inst->info->type,
                                      exe_inst->bp_meta_idx);
                    break;
                }
                case INDIRECT:
                case IND_CALL:
                case IND_PUSH:
                case POP:
                case POP_PUSH:
                    if (exe_inst->next_pc != exe_inst->real_target) {
                        exe_inst->result = InstResult::PRED_FAIL;
                        brRedirect(exe_inst);
                    }
                case DIRECT:
                case PUSH:
                    predictor->update(true, exe_inst->real_pc, exe_inst->real_size, exe_inst->real_target,
                                      exe_inst->info->type, exe_inst->bp_meta_idx);
                    exe_end = true;
                    break;
                case LOAD:
                case LR: {
                    uint64_t mem_paddr, mem_exception;
                    Base::arch->translateAddr(exe_inst->info->exc_data, LFETCH, mem_paddr,
                                              mem_exception);
                    mem_req->addr = mem_paddr;
                    mem_req->size = exe_inst->info->dst_idx[2];
                    mem_req->req = READ_SHARED;
                    if (dcache->lookup(0, mem_req)) {
                        mem_req_list.next();
                        exe_end = true;
                    }
                    exe_inst->mem_id = mem_req->id[0];
                    break;
                }
                case SC:
                    if (exe_inst->info->dst_data[0]) {
                        exe_end = true;
                        mem_end_map[mem_req->id[0]] = true;
                        exe_inst->mem_id = mem_req->id[0];
                        mem_req_list.next();
                        break;
                    }
                case STORE:
                case AMO: {
                    uint64_t mem_paddr, mem_exception;
                    Base::arch->translateAddr(exe_inst->info->exc_data, SFETCH, mem_paddr,
                                              mem_exception);
                    mem_req->addr = mem_paddr;
                    mem_req->size = exe_inst->info->dst_idx[2];
                    mem_req->req = WRITE_BACK;
                    if (dcache->lookup(0, mem_req)) {
                        mem_req_list.next();
                        exe_end = true;
                    }
                    exe_inst->mem_id = mem_req->id[0];
                    break;
                }
                case FENCE:
                case SFENCE:
                    Base::arch->flushCache(1, 0, 0);
                    exe_end = true;
                    break;
                case IFENCE:
                    Base::arch->flushCache(0, 0, 0);
                    exe_end = true;
                    break;
                default:
                    exe_end = true;
                    break;
                }
            }
        }

        if (exe_end && !mem_valid) {
#ifdef DB_INST
            exe_inst->delay[2] = getTick() - exe_inst->start_tick;
#endif
            exe_end = false;
            exe_valid = false;
            mem_valid = true;
            mem_inst = exe_inst;
        }
    }

    if (id_valid && !exe_valid) {
        uint64_t exception = id_inst->info->exception;
        bool exc_valid = Base::arch->exceptionValid(exception);
        if (exc_valid) {
            Base::arch->handleException(exception, pc, id_inst->info);
        }
        if (!id_wait_redirect) {
            id_inst->real_target = Base::arch->updateEnv();
            pc = id_inst->real_target;

            bool target_eq = pc == id_inst->next_pc;
            bool is_cond = id_inst->info->type == COND;
            bool pred_error = (id_inst->info->dst_data[1] ^ id_inst->taken);
            bool is_direct = id_inst->info->type == DIRECT || id_inst->info->type == PUSH;
            bool is_jump = id_inst->info->type > PUSH && id_inst->info->type <= BRANCH_END;
            bool is_branch = is_cond || is_jump || is_direct;
            bool archFlush = Base::arch->needFlush(id_inst->info);
            if (Base::arch->exceptionValid(id_inst->info->exception)) {
                id_wait_redirect = true;
            } else if (is_jump) {
                id_wait_redirect = !target_eq;
            } else if (is_cond) {
                id_wait_redirect = pred_error || !target_eq;
            } else if (archFlush || // satp
                !is_branch && id_inst->taken) { // predictor error
                frontRedirect(id_inst);
                if (archFlush) id_inst->result = CSR_REDIRECT;
            } else if (!(id_inst->real_pc >= id_inst->pc && id_inst->real_pc < id_inst->pc + id_inst->size)){
                Log::error(
                    "PipelineCPU::exec: ID stage PC changed from 0x{:x} to 0x{:x} without redirect",
                    id_inst->pc, pc);
                ExitHandler::exit(1);
            }
            if (id_wait_redirect) {
                wait_redirect_tick = getTick();
                wait_redirect_inst = id_inst;
            }
        } else if (getTick() - wait_redirect_tick > 5000) {
            Log::error("id wait redirect for 5000 cycle\n");
            ExitHandler::exit(1);
        }
#ifdef DB_INST
        id_inst->delay[1] = getTick() - id_inst->start_tick;
#endif
        exe_inst = id_inst;
        id_valid = false;
        exe_valid = true;
    }

    if (!id_valid && id_stall_valid) {
        bool finish = preId(id_stall_inst);
        id_valid = true;
        if (id_stall_dec_more) {
            id_stall_dec_more = !finish;
            id_stall_valid = !(finish && !id_stall_inst_valid);
        } else if (id_stall_inst_valid) {
            id_stall_inst_valid = !finish;
            id_stall_valid = !finish;
        }
    }

    CacheReqWrapper *req_wrapper = fetch_cache_req;
    if (fetch_valid && !id_valid) {
        if (likely(!Base::arch->exceptionValid(req_wrapper->inst->info->exception))) {
            Base::arch->translateAddr(req_wrapper->inst->pc, FETCH_TYPE::IFETCH,
                                      req_wrapper->inst->paddr, req_wrapper->inst->info->exception);
        }
        if (unlikely(Base::arch->exceptionValid(req_wrapper->inst->info->exception))) {
            if (cache_req_list.one() && !id_stall_valid) {
                id_valid = true;
#ifdef DB_INST
                req_wrapper->inst->delay[0] = getTick() - req_wrapper->inst->start_tick;
#endif
                id_inst = req_wrapper->inst;
                id_remain_size += req_wrapper->inst->size;
                cache_req_list.pop();
                fetch_valid = false;
            }
        } else {
            req_wrapper->req->addr = req_wrapper->inst->paddr;
            if (icache->lookup(0, req_wrapper->req)) {
#ifdef DB_INST
                req_wrapper->inst->delay[0] = getTick() - req_wrapper->inst->start_tick;
#endif
                fetch_valid = false;
            }
        }
    }

    if (!fetch_valid) {
    req_wrapper = cache_req_list.back();
    req_wrapper->inst->pc = pred_pc;
    req_wrapper->inst->info->exception = Base::arch->getExceptionNone();
    // req_wrapper->inst->bp_meta_idx =
    //     predictor->predict(req_wrapper->inst->pc, req_wrapper->inst->next_pc,
    //                        req_wrapper->inst->size, req_wrapper->inst->taken, fetch_valid);
    // if (req_wrapper->inst->bp_meta_idx >= 0) {
        // pred_pc = req_wrapper->inst->next_pc;
        pred_pc = req_wrapper->inst->pc + 4;
        req_wrapper->inst->size = 4;
        fetch_cache_req = req_wrapper;
        cache_req_list.next();
        fetch_valid = true;
#ifdef DB_INST
        req_wrapper->inst->start_tick = getTick();
#endif
    // }
    }

    Base::upTick();
}

PipelineCPU::CacheReqWrapper *PipelineCPU::initCacheReq() {
    CacheReqWrapper *req_wrapper = new CacheReqWrapper();
    req_wrapper->req->req = READ_SHARED;
    req_wrapper->req->size = 4;
    return req_wrapper;
}

void PipelineCPU::frontRedirect(Inst *inst) {
    id_stall_valid = false;
    id_stall_dec_more = false;
    id_stall_inst_valid = false;
    id_remain_size = 0;
    fetch_valid = false;
    predictor->redirect(inst->taken, inst->real_pc, inst->real_size, inst->real_target, inst->info->type, inst->bp_meta_idx);
    icache->redirect();
    while (!cache_req_list.empty()) {
        cache_req_list.pop();
    }
    pred_pc = inst->real_target;
}

void PipelineCPU::brRedirect(Inst *inst) {
    id_valid = false;
    id_stall_valid = false;
    id_stall_dec_more = false;
    id_stall_inst_valid = false;
    id_remain_size = 0;
    id_wait_redirect = false;
    fetch_valid = false;
    predictor->redirect(inst->info->dst_data[1], inst->real_pc, inst->real_size, inst->real_target, inst->info->type,
                        inst->bp_meta_idx);
    icache->redirect();
    while (!cache_req_list.empty()) {
        cache_req_list.pop();
    }
    pred_pc = inst->real_target;
}

void PipelineCPU::excRedirect(Inst *inst) {
    mem_req_valid = false;
    mem_valid = false;
    while (!mem_req_list.empty()) {
        mem_req_list.pop();
    }
    exe_valid = false;
    exe_stall_cycle = 0;
    exe_end = false;
    id_valid = false;
    id_stall_valid = false;
    id_stall_dec_more = false;
    id_stall_inst_valid = false;
    id_remain_size = 0;
    id_wait_redirect = false;
    fetch_valid = false;
    predictor->redirect(false, inst->real_pc, inst->real_size, inst->real_target, INT, inst->bp_meta_idx);
    icache->redirect();
    dcache->redirect();
    while (!cache_req_list.empty()) {
        cache_req_list.pop();
    }
    pred_pc = inst->real_target;
}

bool PipelineCPU::preId(Inst *inst) {
    if (id_remain_size <= 0) {
        id_inst = inst;
        id_remain_size += inst->size;
    } else {
        Inst *free_inst = id_extra_insts[id_extra_idx];
        id_extra_idx = (id_extra_idx + 1) % retire_size;
        free_inst->copy(id_inst);
        id_inst = free_inst;
    }

    uint64_t id_paddr;
    uint64_t exception = Base::arch->getExceptionNone();
    Base::arch->translateAddr(pc, FETCH_TYPE::IFETCH, id_inst->paddr, exception);
    if (unlikely(id_inst->info->exception != exception)) {
        Log::error("Exception {:x} {:x} at pc {}", exception, id_inst->info->exception, pc);
    }
    int inst_size = Base::arch->decode(pc, id_inst->paddr, id_inst->info);
    id_inst->real_size = inst_size;
    id_inst->real_pc = pc;
    uint8_t size = 0;
    id_inst->bp_meta_idx = predictor->predict(pc, id_inst->info, id_inst->next_pc, size, id_inst->taken, false);
    if (id_inst->taken) {
        id_inst->real_target = id_inst->next_pc;
        frontRedirect(id_inst);
    } else {
        id_remain_size -= inst_size;
    }

    if (id_remain_size > 0) {
        return false;
    }
    return true;
}