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
// $Id: scrambler.hpp,v 1.5 2003/02/11 22:01:59 edrusb Rel $
//
/*********************************************************************/

#ifndef SCRAMBLER_HPP
#define SCRAMBLER_HPP

#include "generic_file.hpp"
#include "erreurs.hpp"
#include "infinint.hpp"

class scrambler : public generic_file
{
public:
    scrambler(const string & pass, generic_file & hidden_side);
    ~scrambler() { if(buffer != NULL) delete buffer; };
    
    bool skip(const infinint & pos) { if(ref == NULL) throw SRC_BUG; return ref->skip(pos); };
    bool skip_to_eof() { if(ref==NULL) throw SRC_BUG; return ref->skip_to_eof(); };
    bool skip_relative(S_I x) { if(ref == NULL) throw SRC_BUG; return ref->skip_relative(x); };
    infinint get_position() { if(ref == NULL) throw SRC_BUG; return ref->get_position(); };

protected:
    S_I inherited_read(char *a, size_t size);
    S_I inherited_write(char *a, size_t size);

private:
    string key;
    U_32 len;
    generic_file *ref;
    unsigned char *buffer;
    size_t buf_size;
};

#endif
