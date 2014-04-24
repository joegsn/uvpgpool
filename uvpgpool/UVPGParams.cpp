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
: param_count(0), alloc_data(NULL), alloc_pos(0), alloc_size(0)
{
	set_param_size(5);
}
UVPGParams::UVPGParams(size_t starting_size)
: param_count(0), alloc_data(NULL), alloc_pos(0), alloc_size(0)
{
	set_param_size(starting_size);
}
UVPGParams::UVPGParams(const UVPGParams &rhs)
: param_count(0), alloc_data(NULL), alloc_pos(0), alloc_size(0)
{
	// copy, allocating our own space for all paramters.
	// UVPGParams is neigh-guaranteed to be populated with lenghts, formats & oids
	size_t param_size = 0;
	for(int ix = 0; ix < rhs.param_count; ++ix)
	{
		param_size += rhs.param_length[ix];
	}
	set_param_size(param_size / sizeof(int64_t));
	// get & copy all parameters
	for(int ix = 0; ix < rhs.param_count; ++ix)
	{
		add(rhs.param_values[ix], rhs.param_length[ix], rhs.param_format[ix], rhs.param_oid[ix], true);
	}
}

UVPGParams::~UVPGParams()
{
	if(alloc_data)
		free(alloc_data);
}

char *UVPGParams::alloc(size_t size)
{
	if(alloc_pos + size > alloc_size)
	{
		char *newmem = (char *)realloc(alloc_data, alloc_pos + size);
		if(newmem == NULL)
		{
			printf("Error allocating memory for UVPGParams.\n");
			throw "";
		}
		else
		{
			alloc_data = newmem;
			alloc_size = alloc_pos + size;
		}
	}
	char *mem = &(alloc_data[alloc_pos]);
	alloc_pos += size;
	return mem;
}

inline void UVPGParams::add(const char *input, int length, int format, Oid oid, bool dup)
{
	const char *data = input;
	if(dup)
	{
		// we probably only want to dup if we're using the copy constructor
		char *mem = alloc(length);
		memcpy(mem, input, length);
		data = mem;
	}
	param_values.push_back(data);
	param_length.push_back(length);
	param_format.push_back(format);
	param_oid.push_back(oid);
	++param_count;
}

bool UVPGParams::set_param_size(const size_t size)
{
	// reserve space, so that we don't need to individually do allocations
	// for each integer/float that we use.
	// reserve an additional space for allocations, since if we do it
	// all on the fly later, we'll need it.
	size_t new_alloc_data_size = (size + 1) * sizeof(int64_t);
	if(new_alloc_data_size > alloc_size)
	{
		char *newmem = (char *)realloc(alloc_data, new_alloc_data_size);
		if(newmem == NULL)
		{
			printf("Error allocating memory for UVPGParams.\n");
			throw "";
		}
		else
		{
			alloc_data = newmem;
			alloc_size = new_alloc_data_size;
		}
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
	add(input, (int)strlen(input), FORMAT_TEXT, TEXTOID);
}
void UVPGParams::add(const char *input, const size_t size)
{
	add(input, (int)size, FORMAT_BINARY, BYTEAOID);
}
void UVPGParams::add(const int16_t input)
{
	int16_t *data = (int16_t *)alloc(sizeof(input));
	*data = htobe16(input);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, INT2OID);
}
void UVPGParams::add(const int32_t input)
{
	int32_t *data = (int32_t *)alloc(sizeof(input));
	*data = htobe32(input);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, INT4OID);
}
void UVPGParams::add(const int64_t input)
{
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
	
	int64_t *data = (int64_t *)alloc(sizeof(input));
	*data = htobe64(value.ival);
	add((char *)data, (int)sizeof(input), FORMAT_BINARY, FLOAT8OID);
}

