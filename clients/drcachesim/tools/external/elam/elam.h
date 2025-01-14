/* **********************************************************
 * Copyright (c) 2023 Google, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* external analysis tool example. */

#ifndef _ELAM_H_
#define _ELAM_H_ 1

#include "analysis_tool.h"
#include <unordered_map>
#include <bits/stdc++.h>


using dynamorio::drmemtrace::analysis_tool_t;
using dynamorio::drmemtrace::memref_t;
using dynamorio::drmemtrace::addr_t;

enum IoType {Load, Store};

class elam_t : public analysis_tool_t {
public:
    explicit elam_t(unsigned int verbose);
    virtual ~elam_t();
    std::string
    initialize() override;
    bool
    process_memref(const memref_t &memref) override;
    bool
    print_results() override;
    bool
    parallel_shard_supported() override;
    void *
    parallel_worker_init(int worker_index) override;
    std::string
    parallel_worker_exit(void *worker_data) override;
    void *
    parallel_shard_init(int shard_index, void *worker_data) override;
    bool
    parallel_shard_exit(void *shard_data) override;
    bool
    parallel_shard_memref(void *shard_data, const memref_t &memref) override;
    std::string
    parallel_shard_error(void *shard_data) override;
    //std::map<unsigned int, std::unordered_map<IoType, std::unordered_map<addr_t, uint64_t>>> ios;
    typedef struct access {
        unsigned int timestamp;
        addr_t addr;
        unsigned int  size;
        IoType type;
    } acccess;
    std::vector<access> ios;
    unsigned int last_timestamp = 0;
    unsigned int line_size;
    unsigned int line_size_bits_;

protected:
    const static std::string TOOL_NAME;
};

#endif /* _ELAM_H_ */
