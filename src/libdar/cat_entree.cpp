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
} // end extern "C"

#include "cat_entree.hpp"
#include "cat_all_entrees.hpp"
#include "cat_tools.hpp"

using namespace std;

namespace libdar
{

    unsigned char mk_signature(unsigned char base, saved_status state)
    {
        if(! islower(base))
            throw SRC_BUG;
        switch(state)
        {
        case s_saved:
            return base;
        case s_fake:
            return base | SAVED_FAKE_BIT;
        case s_not_saved:
            return toupper(base);
        default:
            throw SRC_BUG;
        }
    }


    const U_I entree::ENTREE_CRC_SIZE = 2;

    void entree_stats::add(const entree *ref)
    {
        if(dynamic_cast<const eod *>(ref) == NULL // we ignore eod
           && dynamic_cast<const ignored *>(ref) == NULL // as well we ignore "ignored"
           && dynamic_cast<const ignored_dir *>(ref) == NULL) // and "ignored_dir"
        {
            const inode *ino = dynamic_cast<const inode *>(ref);
            const mirage *h = dynamic_cast<const mirage *>(ref);
            const detruit *x = dynamic_cast<const detruit *>(ref);


            if(h != NULL) // won't count twice the same inode if it is referenced with hard_link
            {
                ++num_hard_link_entries;
                if(!h->is_inode_counted())
                {
                    ++num_hard_linked_inodes;
                    h->set_inode_counted(true);
                    ino = h->get_inode();
                }
            }

            if(ino != NULL)
            {
                ++total;
                if(ino->get_saved_status() == s_saved)
                    ++saved;
            }

            if(x != NULL)
                ++num_x;
            else
            {
                const directory *d = dynamic_cast<const directory*>(ino);
                if(d != NULL)
                    ++num_d;
                else
                {
                    const chardev *c = dynamic_cast<const chardev *>(ino);
                    if(c != NULL)
                        ++num_c;
                    else
                    {
                        const blockdev *b = dynamic_cast<const blockdev *>(ino);
                        if(b != NULL)
                            ++num_b;
                        else
                        {
                            const cat_tube *p = dynamic_cast<const cat_tube *>(ino);
                            if(p != NULL)
                                ++num_p;
                            else
                            {
                                const prise *s = dynamic_cast<const prise *>(ino);
                                if(s != NULL)
                                    ++num_s;
                                else
                                {
                                    const lien *l = dynamic_cast<const lien *>(ino);
                                    if(l != NULL)
                                        ++num_l;
                                    else
                                    {
					const door *D = dynamic_cast<const door *>(ino);
					if(D != NULL)
					    ++num_D;
					else
					{
					    const file *f = dynamic_cast<const file *>(ino);
					    if(f != NULL)
						++num_f;
					    else
						if(h == NULL)
						    throw SRC_BUG; // unknown entry
					}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void entree_stats::listing(user_interaction & dialog) const
    {
        dialog.printf(gettext("\nCATALOGUE CONTENTS :\n\n"));
        dialog.printf(gettext("total number of inode : %i\n"), &total);
        dialog.printf(gettext("saved inode           : %i\n"), &saved);
        dialog.printf(gettext("distribution of inode(s)\n"));
        dialog.printf(gettext(" - directories        : %i\n"), &num_d);
        dialog.printf(gettext(" - plain files        : %i\n"), &num_f);
        dialog.printf(gettext(" - symbolic links     : %i\n"), &num_l);
        dialog.printf(gettext(" - named pipes        : %i\n"), &num_p);
        dialog.printf(gettext(" - unix sockets       : %i\n"), &num_s);
        dialog.printf(gettext(" - character devices  : %i\n"), &num_c);
        dialog.printf(gettext(" - block devices      : %i\n"), &num_b);
	dialog.printf(gettext(" - Door entries       : %i\n"), &num_D);
        dialog.printf(gettext("hard links information\n"));
        dialog.printf(gettext(" - number of inode with hard link           : %i\n"), &num_hard_linked_inodes);
        dialog.printf(gettext(" - number of reference to hard linked inodes: %i\n"), &num_hard_link_entries);
        dialog.printf(gettext("destroyed entries information\n"));
        dialog.printf(gettext("   %i file(s) have been record as destroyed since backup of reference\n\n"), &num_x);
    }

    entree *entree::read(user_interaction & dialog,
			 memory_pool *pool,
                         generic_file & f,
                         const archive_version & reading_ver,
                         entree_stats & stats,
                         std::map <infinint, etoile *> & corres,
                         compression default_algo,
                         generic_file *data_loc,
                         compressor *efsa_loc,
                         bool lax,
                         bool only_detruit,
                         escape *ptr)
    {
        char type;
        saved_status saved;
        entree *ret = NULL;
        map <infinint, etoile *>::iterator it;
        infinint tmp;
        bool read_crc = (ptr != NULL) && !f.crc_status();

        if(read_crc)
            f.reset_crc(ENTREE_CRC_SIZE);

        try
        {
            S_I lu = f.read(&type, 1);

            if(lu == 0)
                return ret;

            if(!extract_base_and_status((unsigned char)type, (unsigned char &)type, saved))
            {
                if(!lax)
                    throw Erange("entree::read", gettext("corrupted file"));
                else
                    return ret;
            }

            switch(type)
            {
            case 'f':
                ret = new (pool) file(dialog, f, reading_ver, saved, default_algo, data_loc, efsa_loc, ptr);
                break;
            case 'l':
                ret = new (pool) lien(dialog, f, reading_ver, saved, efsa_loc, ptr);
                break;
            case 'c':
                ret = new (pool) chardev(dialog, f, reading_ver, saved, efsa_loc, ptr);
                break;
            case 'b':
                ret = new (pool) blockdev(dialog, f, reading_ver, saved, efsa_loc, ptr);
                break;
            case 'p':
                ret = new (pool) cat_tube(dialog, f, reading_ver, saved, efsa_loc, ptr);
                break;
            case 's':
                ret = new (pool) prise(dialog, f, reading_ver, saved, efsa_loc, ptr);
                break;
            case 'd':
                ret = new (pool) directory(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, efsa_loc, lax, only_detruit, ptr);
                break;
            case 'm':
                ret = new (pool) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, efsa_loc, mirage::fmt_mirage, lax, ptr);
                break;
            case 'h': // old hard-link object
                ret = new (pool) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, efsa_loc, mirage::fmt_hard_link, lax, ptr);
                break;
            case 'e': // old etiquette object
                ret = new (pool) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, efsa_loc, lax, ptr);
                break;
            case 'z':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("entree::read", gettext("corrupted file"));
                    else
                        dialog.warning(gettext("LAX MODE: Unexpected saved status for end of directory entry, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (pool) eod(f);
                break;
            case 'x':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("entree::read", gettext("corrupted file"));
                    else
                        dialog.warning(gettext("LAX MODE: Unexpected saved status for class \"detruit\" object, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (pool) detruit(f, reading_ver);
                break;
	    case 'o':
		ret = new (pool) door(dialog, f, reading_ver, saved, default_algo, data_loc, efsa_loc, ptr);
		break;
            default :
                if(!lax)
                    throw Erange("entree::read", gettext("unknown type of data in catalogue"));
                else
                {
                    dialog.warning(gettext("LAX MODE: found unknown catalogue entry, assuming data corruption occurred, cannot read further the catalogue as I do not know the length of this type of entry"));
                    return ret;  // NULL
                }
            }
	    if(ret == NULL)
		throw Ememory("entree::read");
        }
        catch(...)
        {
            if(read_crc)
            {
		crc * tmp = f.get_crc(); // keep f in a coherent status
		if(tmp != NULL)
		    delete tmp;
            }
            throw;
        }

        if(read_crc)
        {
            crc *crc_calc = f.get_crc();

	    if(crc_calc == NULL)
		throw SRC_BUG;

	    try
	    {
		crc *crc_read = create_crc_from_file(f, pool);
		if(crc_read == NULL)
		    throw SRC_BUG;

		try
		{
		    if(*crc_read != *crc_calc)
		    {
			nomme * ret_nom = dynamic_cast<nomme *>(ret);
			string nom = ret_nom != NULL ? ret_nom->get_name() : "";

			try
			{
			    if(!lax)
				throw Erange("", "temporary exception");
			    else
			    {
				if(nom == "")
				    nom = gettext("unknown entry");
				dialog.pause(tools_printf(gettext("Entry information CRC failure for %S. Ignore the failure?"), &nom));
			    }
			}
			catch(Egeneric & e) // we catch here the temporary exception and the Euser_abort thrown by dialog.pause()
			{
			    if(nom != "")
				throw Erange("entree::read", tools_printf(gettext("Entry information CRC failure for %S"), &nom));
			    else
				throw Erange("entree::read", gettext(gettext("Entry information CRC failure")));
			}
		    }
		    ret->post_constructor(*ptr);
		}
		catch(...)
		{
		    if(crc_read != NULL)
			delete crc_read;
		    throw;
		}
		if(crc_read != NULL)
		    delete crc_read;
	    }
	    catch(...)
	    {
		if(crc_calc != NULL)
		    delete crc_calc;
		throw;
	    }
	    if(crc_calc != NULL)
		delete crc_calc;
	}

        stats.add(ret);
        return ret;
    }

    void entree::dump(generic_file & f, bool small) const
    {
        if(small)
        {
	    crc *tmp = NULL;

	    try
	    {
		f.reset_crc(ENTREE_CRC_SIZE);
		try
		{
		    inherited_dump(f, small);
		}
		catch(...)
		{
		    tmp = f.get_crc(); // keep f in a coherent status
		    throw;
		}

		tmp = f.get_crc();
		if(tmp == NULL)
		    throw SRC_BUG;

		tmp->dump(f);
	    }
	    catch(...)
	    {
		if(tmp != NULL)
		    delete tmp;
		throw;
	    }
	    if(tmp != NULL)
		delete tmp;
        }
        else
            inherited_dump(f, small);
    }

    void entree::inherited_dump(generic_file & f, bool small) const
    {
        char s = signature();
        f.write(&s, 1);
    }

    bool compatible_signature(unsigned char a, unsigned char b)
    {
        a = tolower(a & ~SAVED_FAKE_BIT);
        b = tolower(b & ~SAVED_FAKE_BIT);

        switch(a)
        {
        case 'e':
        case 'f':
            return b == 'e' || b == 'f';
        default:
            return b == a;
        }
    }

    unsigned char get_base_signature(unsigned char a)
    {
	unsigned char ret = tolower(a & ~SAVED_FAKE_BIT);
	if(ret == 'e')
	    ret = 'f';
	return ret;
    }


} // end of namespace

