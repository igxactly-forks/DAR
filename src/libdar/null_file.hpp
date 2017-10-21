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

    /// \file null_file.hpp
    /// \brief /dev/null type file implementation under the generic_file interface
    /// \ingroup Private
    ///
    /// this class is used in particular when doing dry-run execution

#ifndef NULL_FILE_HPP
#define NULL_FILE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "thread_cancellation.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the null_file class implements the /dev/null behavior

	/// this is a generic_file implementation that emulate the
	/// comportment of the /dev/null special file.
	/// all that is writen to is lost, and nothing can be read from
	/// it (empty file). This is a completed implementation all
	/// call are consistent.
	/// \ingroup Private

    class null_file : public generic_file, public thread_cancellation
    {
    public :
        null_file(gf_mode m) : generic_file(m) {};
	null_file(const null_file & ref) = default;
	null_file & operator = (const null_file & ref) = default;
	~null_file() = default;

	virtual bool skippable(skippability direction, const infinint & amount) override { return true; };
        virtual bool skip(const infinint &pos) override { return true; };
        virtual bool skip_to_eof() override { return true; };
        virtual bool skip_relative(signed int x) override { return false; };
        virtual infinint get_position() const override { return 0; };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override {};

        virtual U_I inherited_read(char *a, U_I size) override
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	    return 0;
	};

        virtual void inherited_write(const char *a, U_I siz) override
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	};

	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override {};
    };

	/// @}

} // end of namespace

#endif
