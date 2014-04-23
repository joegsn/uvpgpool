/*
Copyright (c) 2014, Joseph Love
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//
//  UVPGParams.h
//  UVPGPool
//
//  Created by Joseph Love on 3/21/14.
//  Copyright (c) 2014 Gameowls. All rights reserved.
//

#ifndef __UVPGParams__
#define __UVPGParams__

#include "uvpg_pgtypes.h"

#include <vector>
#include <libpq-fe.h>
#include <stdint.h>

// format options for paramFormats to PQsendQueryParams/PQexeParams
#define FORMAT_TEXT   0
#define FORMAT_BINARY 1


class UVPGParams
{
private:
	size_t param_count;
	std::vector<const char *>param_values;
	std::vector<int>param_length;
	std::vector<int>param_format;
	std::vector<Oid>param_oid;
	
	std::vector<char> alloc_data;
	size_t alloc_pos;
	char *alloc(size_t size);
	
	void add(const char *input, int length, int format, Oid oid, bool dup=false);
	
public:
	UVPGParams();
	UVPGParams(size_t starting_size);
	UVPGParams(const UVPGParams &rhs);
	~UVPGParams();
	
	bool set_param_size(const size_t size);
	
	void add(const char *input);
	void add(const char *input, const size_t size);
	void add(const int16_t input);
	void add(const int32_t input);
	void add(const int64_t input);
	void add(const float input);
	void add(const double input);
	
	const char * const *values() { return &param_values[0]; };
	const int *lengths() { return &param_length[0]; }
	const int *formats() { return &param_format[0]; }
	const Oid *oids() { return &param_oid[0]; }
	const size_t size() { return param_count; }
};

#endif /* defined(__UVPGParams__) */
