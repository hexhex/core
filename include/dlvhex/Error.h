/* -*- C++ -*- */

/**
 * @file   Error.h
 * @author Roman Schindlauer
 * @date   Thu Nov 24 23:59:33 CET 2005
 * 
 * @brief  Exception classes.
 * 
 * 
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <string>
#include <iostream>
#include <stdexcept>


/**
 * @brief General exception class.
 *
 */
class GeneralError : public std::runtime_error
{
public:

    /**
     * @brief initialize exception with error string.
     */
    GeneralError(const std::string msg);

    /**
     * @brief Returns error string.
     *
     * In derived classes, this function returns an error message extended with
     * context information of the error. what() just returns the message itself.
     * In this base class, getErrorMsg() is equal to what().
     */
    virtual std::string
    getErrorMsg() const
    {
        return this->what();
    }
};



/**
 * Error caused by malformed input program.
 */
class SyntaxError : public GeneralError
{
public:
    
    /// Ctor.
    SyntaxError(const std::string msg,
                const unsigned line = 0,
                const std::string file = "");

    /**
     * @brief Destructor.
     *
     * A custom destructor definition is needed here, because we have additional
     * members in the class, which make it necessary to redefine the destructor
     * with throw ().
     */
    virtual ~SyntaxError() throw() {};

    /**
     * @brief Returns a formatted error message, indicating the origin of the
     * syntax error, if available.
     */
    virtual std::string
    getErrorMsg() const;

    /**
     * @brief Specifies the line that should be included in the error message.
     */
    void
    setLine(unsigned);

    /**
     * @brief Specifies the filename that should be specified in the error
     * message.
     */
    void
    setFile(const std::string&);

private:

    unsigned line;

    std::string file;
};


/**
 * Severe Error, supposed to be followed by program termination.
 */
class FatalError : public GeneralError
{
public:

    /**
     * @brief Constructs a formatted error message, indicating that this error is
     * fatal.
     *
     * A FatalError has no additional context, so we don't need a getErrorMsg()
     * function for building a special string after construction.
     */
    FatalError(const std::string msg);

};


/**
 * A plugin error is thrown by plugins and caught inside dlvhex.
 */
class PluginError : public GeneralError
{
public:

    /// Ctor.
    PluginError(std::string msg);

    /**
     * @brief See SyntaxError::~SyntaxError().
     */
    virtual ~PluginError() throw() {};

    /**
     * @brief Sets the context of the error.
     *
     * The context is usually the Atom, where this error occurred, and possibly
     * the line number, if available.
     */
    void
    setContext(const std::string&);

    /**
     * @brief Returns a formatted error message.
     *
     * The returned. message is built from the context and the actual error
     * message.
     */
    virtual std::string
    getErrorMsg() const;

private:

    std::string context;
};


#endif /* _ERROR_H */
