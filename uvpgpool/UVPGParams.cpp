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
: param_count(0)
{
	
}
UVPGParams::UVPGParams(size_t starting_size)
: param_count(0)
{
	set_param_size(starting_size);
}
UVPGParams::~UVPGParams()
{
	for(size_t ix = 0; ix < param_values.size(); ++ix)
	{
		if(param_stored[ix] && param_values[ix])
			free(param_values[ix]);
	}
}

bool UVPGParams::set_param_size(const size_t size)
{
	assert(size > 0);
	if(param_values.capacity() < size && size > 0)
	{
		param_values.reserve(size);
		param_length.reserve(size);
		param_format.reserve(size);
		param_oid.reserve(size);
		param_stored.reserve(size);
		return true;
	}
	if(param_values.capacity() >= size && size > 0)
		return true;
	return false;
}

void UVPGParams::add(char *input)
{
	set_param_size(param_count + 1);
	param_values.push_back(input);
	param_length.push_back((int)strlen(input));
	param_format.push_back(FORMAT_TEXT);
	param_oid.push_back(TEXTOID);
	param_stored.push_back(false);
	++param_count;
}
void UVPGParams::add(char *input, const size_t size)
{
	set_param_size(param_count + 1);
	param_values.push_back(input);
	param_length.push_back((int)size);
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(BYTEAOID);
	param_stored.push_back(false);
	++param_count;
}
void UVPGParams::add(const int16_t input)
{
	set_param_size(param_count+1);
	char *data = (char *)malloc(sizeof(input));
	int16_t temp = htobe16(input);
	memcpy(data, &(temp), sizeof(input));
	param_values.push_back(data);
	param_length.push_back((int)sizeof(input));
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(INT2OID);
	param_stored.push_back(true);
	++param_count;
}
void UVPGParams::add(const int32_t input)
{
	set_param_size(param_count+1);
	char *data = (char *)malloc(sizeof(input));
	int32_t temp = htobe32(input);
	memcpy(data, &(temp), sizeof(input));
	param_values.push_back(data);
	param_length.push_back((int)sizeof(input));
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(INT4OID);
	param_stored.push_back(true);
	++param_count;
}
void UVPGParams::add(const int64_t input)
{
	set_param_size(param_count+1);
	char *data = (char *)malloc(sizeof(input));
	int64_t temp = htobe64(input);
	memcpy(data, &(temp), sizeof(input));
	param_values.push_back(data);
	param_length.push_back((int)sizeof(input));
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(INT8OID);
	param_stored.push_back(true);
	++param_count;
}
void UVPGParams::add(const float input)
{
	union float_wrapper
	{
		float fval;
		uint32_t ival;
	} value;
	set_param_size(param_count+1);
	char *data = (char *)malloc(sizeof(input));
	value.fval = input;
	uint32_t temp = htobe32(value.ival);
	memcpy(data, &(temp), sizeof(input));
	param_values.push_back(data);
	param_length.push_back((int)sizeof(input));
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(FLOAT4OID);
	param_stored.push_back(true);
	++param_count;
}
void UVPGParams::add(const double input)
{
	union double_wrapper
	{
		double dval;
		uint64_t ival;
	} value;
	set_param_size(param_count+1);
	char *data = (char *)malloc(sizeof(input));
	value.dval = input;
	uint64_t temp = htobe64(value.ival);
	memcpy(data, &(temp), sizeof(input));
	param_values.push_back(data);
	param_length.push_back((int)sizeof(input));
	param_format.push_back(FORMAT_BINARY);
	param_oid.push_back(FLOAT8OID);
	param_stored.push_back(true);
	++param_count;
}

