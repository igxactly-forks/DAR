/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_LIBRSYNC_H
#include <stdio.h>
#include <librsync.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "generic_rsync.hpp"
#include "memory_file.hpp"
#include "null_file.hpp"

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

#define SMALL_BUF 10

using namespace std;

namespace libdar
{

	// signature creation

    generic_rsync::generic_rsync(generic_file *signature_storage,
				 generic_file *below): generic_file(gf_write_only)
    {
#if LIBRSYNC_AVAILABLE

	    // sanity checks

	if(signature_storage == NULL)
	    throw SRC_BUG;
	if(below == NULL)
	    throw SRC_BUG;

	    // setting up the object

	meta_new(working_buffer, BUFFER_SIZE);
	if(working_buffer == NULL)
	    throw Ememory("generic_rsync::generic_rsync (sign)");
	try
	{
	    working_size = 0;
	    status = sign;
	    x_below = below;
	    x_output = signature_storage;
	    x_input = NULL;
	    orig_crc = NULL;
	    sumset = NULL;
	    initial = true;
	    patching_completed = false; // not used in sign mode
	    job = rs_sig_begin(RS_DEFAULT_BLOCK_LEN, RS_DEFAULT_STRONG_LEN);
	}
	catch(...)
	{
	    meta_delete(working_buffer);
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }


	// delta creation

    generic_rsync::generic_rsync(generic_file *base_signature,
				 generic_file *signature_storage,
				 generic_file *below): generic_file(gf_write_only)
    {
	char *inbuf = NULL;
	char *outbuf = NULL;
	U_I lu = 0;
	U_I out;
	bool eof = false;

	    // sanity checks

	if(base_signature == NULL)
	    throw SRC_BUG;
	if(signature_storage == NULL)
	    throw SRC_BUG;
	if(below == NULL)
	    throw SRC_BUG;

	    // setting up the object

	working_size = 0;
	status = delta;
	initial = true;
	patching_completed = false; // not used in delta mode
	meta_new(working_buffer,BUFFER_SIZE);
	if(working_buffer == NULL)
	    throw Ememory("generic_rsync::generic_rsync (sign)");

	try
	{
		// loading signature into memory

#if LIBRSYNC_AVAILABLE
	    job = rs_loadsig_begin(&sumset);
	    try
	    {
		meta_new(inbuf, BUFFER_SIZE);
		meta_new(outbuf, SMALL_BUF); // nothing should be output
		if(inbuf == NULL || outbuf == NULL)
		    throw Ememory("generic_rsync::generic_rsync (delta)");

		base_signature->skip(0);

		do
		{
		    lu += base_signature->read(inbuf + lu, BUFFER_SIZE - lu);
		    if(lu == 0)
			eof = true;
		    out = SMALL_BUF;
		    if(!step_forward(inbuf, lu,
				     outbuf, out)
		       && eof)
			throw SRC_BUG;
		    if(out != 0)
			throw SRC_BUG; // loading signature into memory
			// should never produce data on output
		}
		while(!eof);

		meta_delete(inbuf);
		inbuf = NULL;
		meta_delete(outbuf);
		outbuf = NULL;
		free_job();
	    }
	    catch(...)
	    {
		if(inbuf != NULL)
		    meta_delete(inbuf);
		if(outbuf != NULL)
		    meta_delete(outbuf);
		free_job();
		rs_free_sumset(sumset);
		throw;
	    }

		// creating the delta job

	    job = rs_delta_begin(sumset);
	    x_below = below;
	    x_input = NULL;
	    x_output = signature_storage;
	    orig_crc = NULL;
	}
	catch(...)
	{
	    meta_delete(working_buffer);
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }

	// patch creation

    generic_rsync::generic_rsync(generic_file *current_data,
				 generic_file *delta,
				 const crc *original_crc): generic_file(gf_read_only)
    {
#if LIBRSYNC_AVAILABLE

	    // sanity checks

	if(current_data == NULL)
	    throw SRC_BUG;
	if(delta == NULL)
	    throw SRC_BUG;
	    // original_crc may be NULL

	    // calculating the CRC of the file to be patched

	if(original_crc != NULL)
	{
	    null_file tmp = null_file(gf_write_only);
	    crc *real_crc = NULL;

	    try
	    {
		current_data->skip(0);
		current_data->reset_crc(original_crc->get_size());
		current_data->copy_to(tmp);
		real_crc = current_data->get_crc();
		if(*real_crc != *original_crc)
		    throw Edata(gettext("CRC of file to apply binary diff to does not match the expected CRC value"));
		current_data->skip(0);
	    }
	    catch(...)
	    {
		if(real_crc != NULL)
		    delete real_crc;
		throw;
	    }
	    if(real_crc != NULL)
		delete real_crc;
	}

	    // setting up the object

	status = patch;
	patching_completed = false;
	x_input = current_data;
	x_output = NULL;
	x_below = delta;
	sumset = NULL;
	orig_crc = original_crc;
	if(orig_crc == NULL)
	    throw SRC_BUG;
	initial = true;
	working_size = 0;
	meta_new(working_buffer,BUFFER_SIZE);
	if(working_buffer == NULL)
	    throw Ememory("generic_rsync::generic_rsync (sign)");
	try
	{
	    x_input->reset_crc(original_crc->get_size());
	    job = rs_patch_begin(generic_rsync::patch_callback, this);
	}
	catch(...)
	{
	    meta_delete(working_buffer);
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }


    generic_rsync::~generic_rsync()
    {
	terminate();
	meta_delete(working_buffer);
    }

    U_I generic_rsync::inherited_read(char *a, U_I size)
    {
	initial = false;
	U_I lu = 0;
	U_I remain;
	bool eof = false;

	if(patching_completed)
	    return 0;

	switch(status)
	{
	case sign:
	case delta:
	    throw SRC_BUG;
	case patch:
	    do
	    {
		working_size += x_below->read(working_buffer + working_size,
					      BUFFER_SIZE - working_size);
		if(working_size == 0)
		    eof = true;
		remain = size - lu;
		if(step_forward(working_buffer,
				 working_size,
				 a + lu,
				remain))
		{
		    if(working_size > 0)
			throw Edata("While patching file, librsync tells it has finished processing data while we still have pending data to send to it");
		    patching_completed = true;
		}
		else
		{
		    if(eof && remain == 0)
			throw Edata("While patching file, librsync tells it has not finished processing data while we have no more to feed to it and librsync did not made any progression in the last cycle (it did not produce new data)");
		}
		lu += remain;
	    }
	    while(lu < size && (remain > 0 || !eof) && !patching_completed);
	    break;
	default:
	    throw SRC_BUG;
	}

	return lu;
    }

    void generic_rsync::inherited_write(const char *a, U_I size)
    {
	initial = false;

	switch(status)
	{
	case sign:
	    x_below->write(a, size);
		/* no break ! */
	case delta:
	    do
	    {
		working_size = BUFFER_SIZE;
		if(step_forward(a, size,
				working_buffer, working_size))
		    throw SRC_BUG;
		if(working_size > 0)
		    x_output->write(working_buffer, working_size);
	    }
	    while(size > 0);
	    break;
	case patch:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void generic_rsync::inherited_terminate()
    {
	switch(status)
	{
	case sign:
	case delta:
	    send_eof();
	    break;
      	case patch:
	    break;
	default:
	    throw SRC_BUG;
	}

	if(sumset != NULL)
	{
	    rs_free_sumset(sumset);
	    sumset = NULL;
	}
	free_job();
    }

    rs_result generic_rsync::patch_callback(void *opaque,
					    rs_long_t pos,
					    size_t *len,
					    void **buf)
    {
	rs_result ret;
	generic_rsync *me = (generic_rsync *)(opaque);
	U_I lu;

	if(me == NULL)
	    throw SRC_BUG;
	if(me->x_input == NULL)
	    throw SRC_BUG;

	try
	{
	    me->x_input->skip(pos);
	    lu = me->x_input->read((char *)*buf, *len);
	    *len = lu;
	    ret = RS_DONE;
	}
	catch(...)
	{
	    *len = 0;
	    ret = RS_IO_ERROR;
	}

	return ret;
    }


    bool generic_rsync::step_forward(const char *buffer_in,
				     U_I & avail_in,
				     char *buffer_out,
				     U_I  & avail_out)
    {
#if LIBRSYNC_AVAILABLE
	bool ret;
	rs_buffers_t buf;
	rs_result res;

	buf.next_in = const_cast<char *>(buffer_in);
	buf.avail_in = avail_in;
	buf.next_out = buffer_out;
	buf.avail_out = avail_out;
	if(avail_in == 0) // EOF reached when called with no input bytes
	    buf.eof_in = 1;
	else
	    buf.eof_in = 0;

	res = rs_job_iter(job, &buf);
	switch(res)
	{
	case RS_DONE:
	    ret = true;
	    break;
	case RS_BLOCKED:
	    ret = false;
	    break;
	default:
	    throw Erange("generic_rsync::step_forward", string(gettext("Error met while feeding data to librsync: ")) + rs_strerror(res));
	}

	if(buf.avail_in > 0)
	    (void)memmove(const_cast<char *>(buffer_in), buf.next_in, buf.avail_in);
	avail_in = buf.avail_in;
	avail_out = buf.next_out - buffer_out;

	return ret;
#endif
    }

    void generic_rsync::free_job()
    {
#if LIBRSYNC_AVAILABLE
    	if(job != NULL)
	{
	    rs_result err = rs_job_free(job);
	    job = NULL;
	    if(err != RS_DONE)
		throw Erange("generic_rsync::inherited_terminate", string(gettext("Error releasing librsync job: ")) + string(rs_strerror(err)));
	}
#endif
    }

    void generic_rsync::send_eof()
    {
	U_I tmp;
	bool finished;

	do
	{
	    tmp = 0;
	    working_size = BUFFER_SIZE;

	    finished = step_forward(working_buffer, tmp, // as tmp is set to zero we can use working_buffer in input too
				    working_buffer, working_size);
	    if(working_size > 0)
		x_output->write(working_buffer, working_size);
	    if(tmp > 0)
		throw SRC_BUG;
	}
	while(working_size > 0 && !finished);
    }

} // end of generic_thread::namespace

