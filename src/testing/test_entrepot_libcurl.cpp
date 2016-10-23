//*********************************************************************/
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
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include "libdar.hpp"
#include "no_comment.hpp"
#include "config_file.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "user_interaction.hpp"
#include "entrepot_libcurl.hpp"
#include "fichier_local.hpp"

using namespace libdar;

void f1();
void f2();

int main()
{
    U_I maj, med, min;

    try
    {
	get_version(maj, med, min);
	f1();
    }
    catch(Egeneric & e)
    {
	cout << "Execption caught: " << e.get_message() << endl;
    }
}

void f1()
{
    shell_interaction ui(&cout, &cerr, true);
    secu_string pass("themenac", 8);
    entrepot_libcurl reposito(entrepot_libcurl::proto_ftp,
			      "denis",
			      pass,
			      "localhost",
			      "");
    fichier_local readme("/etc/fstab");
    fichier_local *writetome = new fichier_local(ui,
						 "/tmp/test.tmp",
						 gf_write_only,
						 0644,
						 false,
						 true,
						 false);
    fichier_local *writetomepart = new fichier_local(ui,
						     "/tmp/test-part.tmp",
						     gf_write_only,
						     0644,
						     false,
						     true,
						     false);
    try
    {
	string tmp;

	if(writetome == nullptr || writetomepart == nullptr)
	    throw Ememory("f1");

    reposito.set_location("/tmp");
    cout << "Listing: " << reposito.get_url() << endl;
    reposito.read_dir_reset();
    while(reposito.read_dir_next(tmp))
	cout << " -> " << tmp << endl;
    cout << endl;

    try
    {
	tmp = "cuicui";
	cout << "removing file " << tmp << endl;
	reposito.unlink(tmp);
	cout << endl;
    }
    catch(Erange & e)
    {
	ui.warning(e.get_message());
    }

    cout << "Listing: " << reposito.get_url() << endl;
    reposito.read_dir_reset();
    while(reposito.read_dir_next(tmp))
	cout << " -> " << tmp << endl;
    cout << endl;

    fichier_global *remotew = reposito.open(ui,
					    "cuicui",
					    gf_write_only,
					    false,
					    0644,
					    true,
					    true,
					    hash_none);
    if(remotew == nullptr)
	throw SRC_BUG;
    try
    {
	readme.copy_to(*remotew);
    }
    catch(...)
    {
	delete remotew;
	throw;
    }
    delete remotew;

    fichier_global *fic = reposito.open(ui,
					"cuicui",
					gf_read_only,
					false,
					0,
					false,
					false,
					hash_none);
    if(fic == nullptr)
	throw SRC_BUG;

    try
    {
	infinint file_size = fic->get_size();
	ui.printf("size = %i", &file_size);

	fic->copy_to(*writetome);
    }
    catch(...)
    {
	delete fic;
	throw;
    }
    delete fic;
    fic = nullptr;
    delete writetome;
    writetome = nullptr;

    fichier_global *foc = reposito.open(ui,
					"cuicui",
					gf_read_only,
					false,
					0,
					false,
					false,
					hash_none);

    if(foc == nullptr)
	throw SRC_BUG;

    try
    {
	foc->skip(20);
	foc->copy_to(*writetomepart);
    }
    catch(...)
    {
	delete foc;
	throw;
    }
    delete foc;
    foc = nullptr;
    delete writetomepart;
    writetomepart = nullptr;

    fichier_global *fac = reposito.open(ui,
					"cuicui",
					gf_read_only,
					false,
					0,
					false,
					false,
					hash_none);
    const U_I BUFSIZE = 1000;
    char buf[BUFSIZE];
    infinint tamp;
    U_I utamp;
    U_I step = 600;

    if(fac == nullptr)
	throw SRC_BUG;

    try
    {
	fac->skip(10);

	tamp = fac->read(buf, step);
	utamp = 0;
	tamp.unstack(utamp);
	buf[utamp] = '\0';
	cout << "reading " << step << " first chars: " << buf << endl;
	tamp = fac->get_position();
	ui.printf("position = %i", &tamp);
	tamp = fac->get_size();
	ui.printf("file size = %i", &tamp);

	tamp = fac->read(buf, step);
	utamp = 0;
	tamp.unstack(utamp);
	buf[utamp] = '\0';
	cout << "reading " << step << " next chars:  " << buf << endl;
	tamp = fac->get_position();
	ui.printf("position = %i", &tamp);
    }
    catch(...)
    {
	delete foc;
	throw;
    }
    delete foc;


    fichier_global *fec = reposito.open(ui,
					"cuicui",
					gf_write_only,
					false,
					0644,
					false,
					true,
					hash_none);
    if(fec == nullptr)
	throw SRC_BUG;
    try
    {
	fec->skip_to_eof();
	readme.skip(0);
	readme.copy_to(*fec);
    }
    catch(...)
    {
	delete fec;
	throw;
    }
    delete fec;

    }
    catch(...)
    {
	if(writetome != nullptr)
	    delete writetome;
	if(writetomepart != nullptr)
	    delete writetomepart;
	throw;
    }

    if(writetome != nullptr)
	delete writetome;
    if(writetomepart != nullptr)
	delete writetomepart;
}
