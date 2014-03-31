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
//  UVPGParams.cpp
//  UVPGPool
//
//  Created by Joseph Love on 3/21/14.
//  Copyright (c) 2014 Gameowls. All rights reserved.
//

#include "UVPGParams.h"
#include "byteorder_endian.h"

#include <assert.h>

UVPGParams::UVPGParams()
: param_count(0), alloc_pos(0)
{
	
}
UVPGParams::UVPGParams(size_t starting_size)
: param_count(0), alloc_pos(0)
{
	set_param_size(starting_size);
}
UVPGParams::~UVPGParams()
{
	
}

char *UVPGParams::alloc(size_t size)
{
	assert(alloc_pos + size < alloc_data.size());
	char *mem = &(alloc_data[alloc_pos]);
	alloc_pos += size;
	return mem;
}

inline void UVPGParams::add(const char *input, int length, int format, Oid oid)
{
	param_values[param_count] = input;
	param_length[param_count] = length;
	param_format[param_count] = format;
	param_oid[param_count]    = oid;
	++param_count;
}

bool UVPGParams::set_param_size(const size_t size)
{
	// reserve space, so that we don't need to individually do allocations
	// for each integer/float that we use.
	size_t new_alloc_data_size = size * sizeof(int64_t);
	if(new_alloc_data_size > alloc_data.capacity())
	{
		alloc_data.reserve(new_alloc_data_size);
	}
	
	if(param_values.capacity() < size && size > 0)
	{
		param_values.reserve(size);
		param_length.reserve(size);
		param_format.reserve(size);
		param_oid.reserve(size);
		return true;
	}
	if(param_values.capacity() >= size && size > 0)
		return true;
	return false;
}

void UVPGParams::add(const char *input)
{
	set_param_size(param_count + 1);
	add(input, (int)strlen(input), FORMAT_TEXT, TEXTOID);
}
void UVPGParams::add(const char *input, const size_t size)
{
	set_param_size(param_count + 1);
	add(input, (int)size, FORMAT_BINARY, BYTEAOID);
}
void UVPGParams::add(const int16_t input)
{
	set_param_size(param_count+1);
	int16_t *data = (int16_t *)alloc(sizeof(input));
	*data = htobe16(input);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, INT2OID);
}
void UVPGParams::add(const int32_t input)
{
	set_param_size(param_count+1);
	int32_t *data = (int32_t *)alloc(sizeof(input));
	*data = htobe32(input);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, INT4OID);
}
void UVPGParams::add(const int64_t input)
{
	set_param_size(param_count+1);
	int64_t *data = (int64_t *)alloc(sizeof(input));
	*data = htobe64(input);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, INT8OID);
}
void UVPGParams::add(const float input)
{
	union float_wrapper
	{
		float fval;
		uint32_t ival;
	} value;
	value.fval = input;
	set_param_size(param_count+1);
	int32_t *data = (int32_t *)alloc(sizeof(input));
	*data = htobe32(value.ival);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, FLOAT4OID);
}
void UVPGParams::add(const double input)
{
	union double_wrapper
	{
		double dval;
		uint64_t ival;
	} value;
	value.dval = input;
	set_param_size(param_count+1);
	int64_t *data = (int64_t *)alloc(sizeof(input));
	*data = htobe64(value.ival);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, FLOAT8OID);
}

