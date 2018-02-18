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

    /// \file user_interaction.hpp
    /// \brief defines the interaction interface between libdar and users.
    /// \ingroup API
    ///

#ifndef USER_INTERACTION_HPP
#define USER_INTERACTION_HPP

#include "../my_config.h"

#include <string>
#include "erreurs.hpp"
#include "integers.hpp"
#include "secu_string.hpp"
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// This is a pure virtual class that is used by libdar when interaction with the user is required.

	//! You can base your own class on it using C++ inheritance
	//! or use the predifined inherited classes user_interaction_callback which implements
	//! the interaction based on callback functions.

	//! \ingroup API
    class user_interaction
    {
    public:
	user_interaction() = default;
	user_interaction(const user_interaction & ref) = default;
	user_interaction(user_interaction && ref) noexcept = default;
	user_interaction & operator = (const user_interaction & ref) = default;
	user_interaction & operator = (user_interaction && ref) noexcept = default;
	virtual ~user_interaction() = default;

	    // the following methode are used by libdar and rely in their inherited_* versions
	    // than must be defined in the inherited classes

	bool pause(const std::string & message);
	void warning(const std::string & message);
	std::string get_string(const std::string & message, bool echo);
	secu_string get_secu_string(const std::string & message, bool echo);
	void printf(const char *format, ...);


	    /// make a newly allocated object which has the same properties as "this".

	    //! This *is* a virtual method, it *must* be overwritten in any inherited class
	    //! copy constructor and = operator may have to be overwritten too if necessary
	    //! Warning !
	    //! clone() must throw exception if necessary (Ememory), but never
	    //! return a nullptr pointer !
	virtual user_interaction *clone() const = 0;

    protected:
	    /// method used to ask a boolean question to the user.

	    //! \param[in] message The boolean question to ask to the user
	    //! \return the answer of the user (true/yes or no/false)
	virtual bool inherited_pause(const std::string & message) = 0;

	    /// method used to display a warning or a message to the user.
	    ///
	    /// \param[in] message is the message to display.
	virtual void inherited_warning(const std::string & message) = 0;

	    /// method used to ask a question that needs an arbitrary answer.

	    //! \param[in] message is the question to display to the user.
	    //! \param[in] echo is set to false is the answer must not be shown while the user answers.
	    //! \return the user's answer.
	virtual std::string inherited_get_string(const std::string & message, bool echo) = 0;

	    /// same a get_string() but uses libdar::secu_string instead of std::string

	    //! \param[in] message is the question to display to the user.
	    //! \param[in] echo is set to false is the answer must not be shown while the user answers.
	    //! \return the user's answer.
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) = 0;


    };


	/// @}

} // end of namespace

#endif
