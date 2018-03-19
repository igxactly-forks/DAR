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

    /// \file cat_status.hpp
    /// \brief the different status of data and EA
    /// \ingroup Private

#ifndef CAT_STATUS_HPP
#define CAT_STATUS_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"


namespace libdar
{
	/// \addtogroup Private
	/// @{

    enum class saved_status
    {
	saved,      //< inode is saved in the archive
	inode_only, //< data is not saved but inode meta data has changed since the archive of reference
	fake,       //< inode is not saved in the archive but is in the archive of reference (isolation context) s_fake is no more used in archive format "08" and above: isolated catalogue do keep the data pointers and s_saved stays a valid status in isolated catalogues.
	not_saved,  //< inode is not saved in the archive
	delta       //< inode is saved but as delta binary from the content (delta signature) of what was found in the archive of reference
    };

    enum class ea_saved_status
    {
	none,    //< no EA present for this inode in filesystem
	partial, //< EA present in filesystem but not stored (ctime used to check changes)
	fake,    //< EA present in filesystem but not attached to this inode (isolation context) no more used in archive version "08" and above, ea_partial or ea_full stays a valid status in isolated catalogue because pointers to EA and data are no more removed during isolation process.
	full,    //< EA present in filesystem and attached to this inode
	removed  //< EA were present in the reference version, but not present anymore

    };
	/// @}

} // end of namespace

#endif
