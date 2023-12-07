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

/* External analysis tool example. */

#include "dr_api.h"
#include "memsyscall.h"
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include "analysis_tool.h"
#include <analyzer.h>
#include "trace_entry.h"

const std::string memsyscall_t::TOOL_NAME = "Elam's tool";
const int memsyscall_t::SYSCALL_BRK = 12;
const int memsyscall_t::SYSCALL_MMAP = 9;
const int memsyscall_t::SYSCALL_MREMAP = 25;

using dynamorio::drmemtrace::analyzer_t;

analysis_tool_t *
memsyscall_tool_create(unsigned int verbose)
{
    return new memsyscall_t(verbose);
}

memsyscall_t::memsyscall_t(unsigned int verbose)
{
    line_size = 64; // bytes
    line_size_bits_ = dynamorio::drmemtrace::compute_log2((int)line_size);
}

std::string
memsyscall_t::initialize()
{
    return std::string("");
}

memsyscall_t::~memsyscall_t()
{
}

bool
memsyscall_t::parallel_shard_supported()
{
    return false;
    
}

void *
memsyscall_t::parallel_worker_init(int worker_index)
{
    return NULL;
}

std::string
memsyscall_t::parallel_worker_exit(void *worker_data)
{
    return std::string("");
}

void *
memsyscall_t::parallel_shard_init(int shard_index, void *worker_data)
{
    return NULL;
}

bool
memsyscall_t::parallel_shard_exit(void *shard_data)
{
    return true;
}

static inline addr_t
back_align(addr_t addr, addr_t align)
{
    return addr & ~(align - 1);
}


//void update_map_from_access(std::unordered_map<addr_t, uint64_t> * m, addr_t start_addr, uint64_t size, uint64_t line_size, uint64_t line_size_bits_) {
//        for (dynamorio::drmemtrace::addr_t addr = back_align(start_addr, line_size);
//            addr < start_addr + size && addr < addr + line_size /* overflow */;
//            addr += line_size) {
//            ++(*m)[addr >> line_size_bits_];
//        }
//}


bool
memsyscall_t::parallel_shard_memref(void *shard_data, const memref_t &memref)
{
    return false;
}

std::string
memsyscall_t::parallel_shard_error(void *shard_data)
{
    return std::string("");
}


bool
memsyscall_t::process_memref(const memref_t &memref) // TODO make this work with parallel / multithreaded applications if we actually care about delineating
{

	// memref data has type read/write at data load/store
	if (memref.data.type == dynamorio::drmemtrace::TRACE_TYPE_READ || memref.data.type == dynamorio::drmemtrace::TRACE_TYPE_WRITE) {
		++ios[last_timestamp][memref.data.addr][memref.data.size];

	// brk/mmap/mremap syscall does not involve data load/store
	} else if (memref.marker.type == dynamorio::drmemtrace::TRACE_TYPE_MARKER && memref.marker.marker_type == dynamorio::drmemtrace::TRACE_MARKER_TYPE_SYSCALL) {
		int syscall_num = static_cast<int>(memref.marker.marker_value);
#ifdef X64
		assert(static_cast<uintptr_t>(syscall_num) == memref.marker.marker_value);
#endif
		switch (syscall_num) {
			case memsyscall_t::SYSCALL_BRK: case memsyscall_t::SYSCALL_MMAP: case memsyscall_t::SYSCALL_MREMAP:
				//++syscall_count[syscall_num];
				++sysstats[last_timestamp][syscall_num];
				break;
			default: break;
		}

	// memref data has type timestamp at some anchor points
    } else if (memref.marker.type == dynamorio::drmemtrace::TRACE_TYPE_MARKER && memref.marker.marker_type == dynamorio::drmemtrace::TRACE_MARKER_TYPE_TIMESTAMP) {
        uint64_t timestamp = memref.marker.marker_value;
        last_timestamp = timestamp;
	}

    return true;
}

bool
memsyscall_t::print_results()
{
    std::fprintf(stderr, "timestamp,address,size,count,brk,mmap,mremap\n");
	//for (const auto & io : ios) {
	auto pio = ios.begin();
	auto pss = sysstats.begin();
	for (; pio != ios.end() && pss != sysstats.end(); ++pio, ++pss) {
		const auto & io = *pio;
		const auto & ss = *pss;

		unsigned int timestamp = io.first;
		syscall_count_t syscall_count = ss.second;
		uint64_t brk_count = syscall_count[memsyscall_t::SYSCALL_BRK];
		uint64_t mmap_count = syscall_count[memsyscall_t::SYSCALL_MMAP];
		uint64_t mremap_count = syscall_count[memsyscall_t::SYSCALL_MREMAP];
		for (const auto & addr_map : io.second) {
			for (const auto & size_map : addr_map.second) {
				addr_t addr = addr_map.first;
				size_t size = size_map.first;
				uint64_t count = size_map.second;
				std::fprintf(stderr, "%u,%lu,%zu,%lu,%lu,%lu,%lu\n", timestamp, addr, size, count, brk_count, mmap_count, mremap_count);
			}
		}
	}
    return true;
}
