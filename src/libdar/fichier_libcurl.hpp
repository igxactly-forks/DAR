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

    /// \file fichier_libcurl.hpp
    /// \brief class fichier_libcurl definition. This is a full implementation/inherited class of class fichier_global
    /// this type of object are generated by entrepot_libcurl.
    /// \ingroup Private

#ifndef FICHIER_LIBCURL_HPP
#define FICHIER_LIBCURL_HPP


#include "../my_config.h"

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
} // end extern "C"

#include <string>
#ifdef LIBTHREADAR_AVAILABLE
#include <libthreadar/libthreadar.hpp>
#endif
#include "integers.hpp"
#include "thread_cancellation.hpp"
#include "label.hpp"
#include "user_interaction.hpp"
#include "fichier_global.hpp"
#include "mycurl_easyhandle_sharing.hpp"
#include "mycurl_protocol.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

	/// libcurl remote files

    class fichier_libcurl : public fichier_global, protected libthreadar::thread
    {
    public:

	    /// constructor
	fichier_libcurl(user_interaction & dialog,      //< for user interaction requested by fichier_global
			const std::string & chemin,     //< full path of the file to open
			mycurl_protocol proto,          //< to workaround some libcurl strange behavior for some protocols
			mycurl_shared_handle && handle, //< the easy handle wrapper object
			gf_mode m,                      //< open mode
			U_I waiting,                    //< retry timeout in case of network error
			bool force_permission,          //< whether file permission should be modified
			U_I permission,                 //< file permission to enforce if force_permission is set
			bool erase);                    //< whether to erase the file before writing to it

	    /// no copy constructor available

	    ///\note because we inherit from libthreadar::thread that has not copy constructor
	fichier_libcurl(const fichier_libcurl & ref) = delete;

	    /// no assignment operator
	    ///\note because we inherit from libthreadar::thread that has not copy constructor
	fichier_libcurl & operator = (const fichier_libcurl & ref) = delete;

	    /// destructor
	~fichier_libcurl() noexcept { detruit(); };

	    /// change the permission of the file
	virtual void change_permission(U_I perm) override;

	    /// set the ownership of the file
	virtual void change_ownership(const std::string & user, const std::string & group) override
	{ throw Efeature(gettext("user/group ownership not supported for this repository")); }; // not supported

	    /// return the size of the file
        virtual infinint get_size() const override;

	    /// set posix_fadvise for the whole file
	virtual void fadvise(advise adv) const override {}; // not supported and ignored

            // inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override;
        virtual bool skip(const infinint & pos) override;
        virtual bool skip_to_eof() override;
        virtual bool skip_relative(S_I x) override;
        virtual infinint get_position() const override { return current_offset; };

    protected:
	    // inherited from generic_file grand-parent class
	virtual void inherited_read_ahead(const infinint & amount) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override;
	virtual void inherited_terminate() override;

	    // inherited from fichier_global parent class
	virtual U_I fichier_global_inherited_write(const char *a, U_I size) override;
        virtual bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message);

	    // inherited from thread
	virtual void inherited_run() override;

    private:
	static const U_I tampon_size = CURL_MAX_WRITE_SIZE;

	    //////////////////////////////
	    //
	    // implementation internals
	    //
	    //////////////////////////////
	    // the object has two modes:
	    // - meta data mode (skip, get_position() and other non read/write operations)
	    // - data mode (read or write operations)
	    //
	    // in metadata mode each method is a simple code execution (no subthread, no callback)
	    //
	    // in data mode, a subthread is used to interact with libcurl. It sends or receives
	    // data through the interthread pipe. A callback is occasionally run bu libcurl in this
	    // subthread.
	    // in read mode, the subthread is run only if the interthread is empty. the subthread may
	    // survive the inherited_read call and may suspend on writing data to interthread being full
	    // - "network_offset" is updated by the callback and read by the subthread when libcurl has returned
	    //   it keeps trace of the amount of data sent to interthread.
	    // - "network_block" is set by the main thread to define the amount of data to be fetched it
	    //   it used to setup libcurl and is read by the subthread for control/validation purposes

	bool end_data_mode;               //< true if subthread has been requested to end
	bool sub_is_dying;                //< is set by subthread when about to end
	mycurl_shared_handle ehandle;     //< easy handle (wrapped in C++ object) that we modify when necessary
	bool metadatamode;                //< wether we are acting on metadata rather than file's data
	infinint current_offset;          //< current offset we are reading / writing at
	bool has_maxpos;                  //< true if maxpos is set
	infinint maxpos;                  //< in read mode this is the filesize, in write mode this the offset where to append data (not ovewriting)
	bool append_write;                //< whether we should append to data (and not replace) when uploading
	char meta_tampon[tampon_size];    //< trash in transit data used to carry metadata
	U_I meta_inbuf;                   //< amount of byte available in "meta_tampon"
	U_I wait_delay;                   //< time in second to wait before retrying in case of network error
	infinint network_block;           //< maximum amount of data read at once from the network (only read by subthread)
	infinint subthread_net_offset;    //< updated by sub thread in network block mode to give amount of bytes pushed to interthread
	infinint subthread_cur_offset;    //< subthread copy of current_offset
	libthreadar::fast_tampon<char> interthread; //< data channel for reading or writing with subthread
	libthreadar::barrier synchronize; //< used to be sure subthread has been launched
	mycurl_protocol x_proto;          //< used to workaround some libcurl strange behavoir for some protocols

	void set_range(const infinint & begin, const infinint & range_size); //< set range in easyhandle
	void unset_range();  //< unset range in easyhandler
	void switch_to_metadata(bool mode);//< set to true to get or set file's metadata, false to read/write file's data
	void detruit();     //< get ready for object destruction
	void run_thread();  //< run subthread with the previously defined parameters
	void stop_thread(); //< ask subthread to stop and wait for its end
	void relaunch_thread(const infinint & block_size); //< re-run the subthread if not running
	void initialize_subthread(); //< subthread routine to init itself
	void finalize_subthread();   //< subthread routine to end itself
	void set_subthread(U_I & needed_bytes); //< set parameters and run subthtread if necessary

	static size_t write_data_callback(char *buffer, size_t size, size_t nmemb, void *userp);
	static size_t read_data_callback(char *bufptr, size_t size, size_t nitems, void *userp);
       	static size_t write_meta_callback(char *buffer, size_t size, size_t nmemb, void *userp);
	static size_t read_meta_callback(char *bufptr, size_t size, size_t nitems, void *userp);
    };

#endif

#ifdef LIBTHREADAR_AVAILABLE
	/// helper function to handle libcurl error code
	/// wait or throw an exception depending on error condition
	///
	/// \param[in] dialog used to report the reason we are waiting for and how much time we wait
	/// \param[in] err is the curl easy code to examin
	/// \param[in] wait_seconds is the time to wait for recoverable error
	/// \param[in] err_context is the error context message use to prepend waiting message or exception throw
    extern void fichier_libcurl_check_wait_or_throw(user_interaction & dialog,
						    CURLcode err,
						    U_I wait_seconds,
						    const std::string & err_context);
#else
#if LIBCURL_AVAILABLE
    inline void fichier_libcurl_check_wait_or_throw(user_interaction & dialog,
						    CURLcode err,
						    U_I wait_seconds,
						    const std::string & err_context)
    { throw SRC_BUG; }
#endif
#endif
	/// @}

} // end of namespace

#endif
