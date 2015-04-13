/* dlvhex -- Answer-Set Programming with external interfaces.
 * Copyright (C) 2005-2007 Roman Schindlauer
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * Copyright (C) Peter Schüller
 * Copyright (C) 2011-2015 Christoph Redl
 *
 * This file is part of dlvhex.
 *
 * dlvhex is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * dlvhex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlvhex; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/**
 * @file   Logger.h
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 *
 * @brief  Logging facility with comfortable indentation and closures.
 */

#ifndef LOGGER_HPP_INCLUDED__17092010
#define LOGGER_HPP_INCLUDED__17092010

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dlvhex2/PlatformDefinitions.h"

#include <boost/preprocessor/cat.hpp>
#include <boost/optional.hpp>
#include <boost/cstdint.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>

#ifndef NDEBUG
# define LOG_SCOPED_LOCK(varname) boost::mutex::scoped_lock varname(Logger::Mutex());
#else
# define LOG_SCOPED_LOCK(varname) do { } while(false)
#endif

#include <iostream>
#include <iomanip>
#include <sstream>

/** \brief Singleton logger class. */
class DLVHEX_EXPORT Logger
{
    public:
        /** \brief Levels are specified and can be activated via bitmasks.
         * All 32 bits may be used. Logger itself logs on DBG. */
        typedef uint32_t Levels;
        /** \brief Debug message. */
        static const Levels DBG =     0x01;
        /** \brief Info message printed to the user. */
        static const Levels INFO =    0x02;
        /** \brief Warning message printed to the user. */
        static const Levels WARNING = 0x04;
        /** \brief Error message printed to the user. */
        static const Levels ERROR =   0x08;

        // this is now very dlvhex-specific
        /** \brief Plugin related things. */
        static const Levels PLUGIN =  0x10;
        /** \brief Program analysis. */
        static const Levels ANALYZE = 0x20;
        /** \brief Model building. */
        static const Levels MODELB =  0x40;
        /** \brief Statistic information. */
        static const Levels STATS  =  0x80;

    private:
        /** \brief Stream to print to. */
        std::ostream& out;
        /** \brief Indent to be printed at the beginning of lines. */
        boost::thread_specific_ptr<std::string> indent;
        /** \brief One or more levels to print (bitwise or). */
        Levels printlevels;
        /** \brief Width of field for level printing, if 0, level is not printed. */
        int levelwidth;

    private:
        /** \brief Constructor.
         *
         * Default output is std::cerr, change this later with stream() = ... .
         * Default output is all output levels, change this later with printlevels() = ... .
         * Default output is i hex character of level printed, change this later with levelwidth() = ... . */
        Logger():
        out(std::cerr), printlevels(~static_cast<Levels>(0)), levelwidth(1) {}

        /** \brief Destructor. */
        ~Logger() {
            stream() << std::endl;
            startline(DBG);
        #ifndef NDEBUG
            stream() << "clean exit!" << std::endl;
        #endif
        }

    public:
        /** \brief Get singleton Logger instance.
         * @return Singleton Logger instance. */
        static Logger& Instance();
        /** \brief Return Logger mutex for multithreading access.
         * @return Logger mutex for multithreading access. */
        static boost::mutex& Mutex();

        /** \brief Return stream the Logger prints to.
         * @return Stream. */
        inline std::ostream& stream()
            { return out; }

        /** \brief Sets one or more levels to print to.
         * @param levels See Logger::printlevels. */
        void setPrintLevels(Levels levels);
        /** \brief Sets width of field for level printing.
         * @param width See Logger::levelwidth. */
        void setPrintLevelWidth(int width);
        /** \brief Get current print levels.
         * @return See Logger::printlevels. */
        Levels getPrintLevels() const;

        /** \brief Starts a new output line-
         *
         * This method does not ask shallPrint!
         * @param forlevel Print level. */
        inline void startline(Levels forlevel) {
            if (!indent.get()) indent.reset(new std::string(""));
            if( levelwidth == 0 ) {
                out << *indent;
            }
            else {
                out << std::hex << std::setw(levelwidth) << forlevel << std::dec << " " << *indent;
            }
        }

        /** \brief Checks for a given level if it shall be printed according to the current settings.
         * @param forlevel Level to check.
         * @return True if \p forlevel shall be printed. */
        inline bool shallPrint(Levels forlevel)
            { return (printlevels & forlevel) != 0; }

        friend class Closure;
        /** \brief Allows for printing within a given scope using some indent. */
        class Closure
        {
            private:
                /** \brief Logger to use. */
                Logger& l;
                /** Level to print at. */
                Levels level;
                /** \brief Value to be removed from the indent after the scope was left (restore the old indent). */
                unsigned cutoff;
                /** \brief Message to print. */
                bool message;

                /** \brief Prints an ENTRY message. */
                inline void sayHello() {
                    // hello message
                    if( message ) {
                        l.startline(level);
                        l.stream() << "ENTRY" << std::endl;
                    }
                }

                /** \brief Prints an EXIT message. */
                inline void sayGoodbye() {
                    // goodbye message
                    if( message ) {
                        l.startline(level);
                        l.stream() << "EXIT" << std::endl;
                    }
                }

            public:
                /** \brief Generic constructor.
                 *
                 * With value (converted/reinterpret-casted to const void* const).
                 * @param l See Logger::Closure::l.
                 * @param level See Logger::Closure::level.
                 * @param str String to print.
                 * @param message See Logger::Closure::message. */
                Closure(Logger& l, Levels level, const std::string& str, bool message):
                l(l), level(level), message(message) {
                    if (!l.indent.get()) l.indent.reset(new std::string(""));
                    cutoff = l.indent->size();

                    if( l.shallPrint(level) ) {
                        LOG_SCOPED_LOCK(lock);
                        *l.indent += str + " ";
                        sayHello();
                    }
                }

                /** \brief Constructor.
                 *
                 * With value (converted/reinterpret-casted to const void* const).
                 * @param l See Logger::Closure::l.
                 * @param level See Logger::Closure::level.
                 * @param str String to print.
                 * @param val Value to print.
                 * @param message See Logger::Closure::message. */
                Closure(Logger& l, Levels level, const std::string& str, const void* const val, bool message):
                l(l), level(level), message(message) {
                    if (!l.indent.get()) l.indent.reset(new std::string(""));
                    cutoff = l.indent->size();

                    if( l.shallPrint(level) ) {
                        LOG_SCOPED_LOCK(lock);
                        std::stringstream ss;
                        ss << str << "/" << val << " ";
                        *l.indent += ss.str();
                        sayHello();
                    }
                }

                /** \brief Destructor. */
                ~Closure() {
                    if (!l.indent.get()) l.indent.reset(new std::string(""));
                    if( l.shallPrint(level) ) {
                        LOG_SCOPED_LOCK(lock);
                        sayGoodbye();
                        // restore indentation level
                        l.indent->erase(cutoff);
                    }
                }
        };

        /** \brief Logger initializer. */
        class Init
        {
            public:
                /** \brief Initializes the logger for a given set of levels to print.
                 * @param levels Levels to print. */
                Init(Levels levels) {
                    Logger::Instance().setPrintLevels(levels);
                }
        };
};

// the following will always be realized
//#ifndef NDEBUG
#  define LOG(level,streamout) do { LOG_SCOPED_LOCK(lock); \
        if( Logger::Instance().shallPrint(Logger:: level) ) \
        { \
            Logger::Instance().startline(Logger:: level); \
            Logger::Instance().stream() << streamout << std::endl; \
        } \
    } while(false);
#    define LOG_CLOSURE_ID BOOST_PP_CAT(log_closure_,__LINE__)
#  define LOG_INDENT(level)              Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, "  ", false)
#  define LOG_SCOPE(level,name,msg)      Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, name, msg)
#  define LOG_VSCOPE(level,name,val,msg) Logger::Closure LOG_CLOSURE_ID (Logger::Instance(), Logger:: level, name, reinterpret_cast<const void* const>(val), msg)
    /*
    #else
    #  define LOG(level,streamout) 			do { } while(false)
    #  define LOG_CLOSURE_ID BOOST_PP_CAT(log_closure_,__LINE__) do { } while(false)
    #  define LOG_INDENT(level)              	do { } while(false)
    #  define LOG_SCOPE(level,name,msg)      	do { } while(false)
    #  define LOG_VSCOPE(level,name,val,msg) 	do { } while(false)
    #endif
    */

#  define LOG_INIT(setlevel)             namespace { Logger::Init LOG_CLOSURE_ID (setlevel); }

    // the following are debug-flag dependant
#ifndef NDEBUG
#  define DBGLOG(level,streamout)           LOG(level,streamout)
#  define DBGLOG_INDENT(level)              LOG_INDENT(level)
#  define DBGLOG_SCOPE(level,name,msg)      LOG_SCOPE(level,name,msg)
#  define DBGLOG_VSCOPE(level,name,val,msg) LOG_VSCOPE(level,name,val,msg)
#else
#  define DBGLOG(level,streamout)           do { } while(false)
#  define DBGLOG_INDENT(level)              do { } while(false)
#  define DBGLOG_SCOPE(level,name,msg)      do { } while(false)
#  define DBGLOG_VSCOPE(level,name,val,msg) do { } while(false)
#endif
#endif                           // LOGGER_HPP_INCLUDED__17092010
