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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: database_header.cpp,v 1.4.2.1 2003/04/15 21:51:52 edrusb Rel $
//
/*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "cygwin_adapt.hpp"
#include "database_header.hpp"
#include "compressor.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"

static unsigned char database_version = 1;

#define HEADER_OPTION_NONE 0x00

generic_file *database_header_create(const string & filename, bool overwrite)
{
    char *ptr = tools_str2charptr(filename);
    generic_file *ret = NULL;
    
    try
    {
	struct stat buf;
	S_I fd;
	database_header h;
	compressor *comp;
	
	if(stat(ptr, &buf) >= 0 && !overwrite)
	    throw Erange("database_header_create", "Cannot create database, file exists");
	fd = open(ptr, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
	if(fd < 0)
	    throw Erange("database_header_create", string("Cannot create database: ") + strerror(errno));
	ret = new fichier(fd);
	if(ret == NULL)
	{
	    close(fd);
	    throw Ememory("database_header_create");
	}
	
	h.version = database_version;
	h.options = HEADER_OPTION_NONE;
	h.write(*ret);

	comp = new compressor(gzip, ret); // upon success, ret is owned by compr
	if(comp == NULL)
	    throw Ememory("database_header_create");
	else
	    ret = comp; 
    }
    catch(...)
    {
	delete ptr;
	if(ret != NULL)
	    delete ret;
	throw;
    }
    delete ptr;

    return ret;
}

generic_file *database_header_open(const string & filename)
{
    char *ptr = tools_str2charptr(filename);
    generic_file *ret = NULL;

    try
    {
	database_header h;
	compressor *comp;

	ret = new fichier(ptr, gf_read_only);
	if(ret == NULL)
	    throw Ememory("database_header_open");
	h.read(*ret);
	if(h.version != database_version)
	    user_interaction_pause("The format version of the database is too high for that software version, try reading anyway ? ");
	if(h.options != HEADER_OPTION_NONE)
	    throw Erange("database_header_open", "Unknown header option in database, aborting\n");
	
	comp = new compressor(gzip, ret);
	if(comp == NULL)
	    throw Ememory("database_header_open");
	else
	    ret = comp;
    }
    catch(...)
    {
	delete ptr;
	if(ret != NULL)
	    delete ret;
	throw;
    }
    
    delete ptr;
    return ret;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: database_header.cpp,v 1.4.2.1 2003/04/15 21:51:52 edrusb Rel $";
    dummy_call(id);
}
