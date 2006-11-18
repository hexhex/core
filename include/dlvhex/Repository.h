/* -*- C++ -*- */

/**
 * @file   Repository.h
 * @author Roman Schindlauer
 * @date   Tue Mar  7 17:06:12 CET 2006
 * 
 * @brief  Singleton class for storing all kinds of objects created from the
 * input program.
 * 
 * 
 */

#ifndef _REPOSITORY_H
#define _REPOSITORY_H

#include <vector>

#include "boost/shared_ptr.hpp"

//
// forward declaration
//
class BaseVisitor;


/**
 * @brief Abstract base class for all objects that are part of a
 * program and dynamically created.
 *
 * This class does not implement an methods. It is just used as a common base
 * class for internal storage structures.
 */
class ProgramObject
{
protected:
    ProgramObject() {}
public:
    virtual
    ~ProgramObject();

    /// The accept method is part of the visitor pattern and used to
    /// double dispatch the correct type of the child, i.e. if someone
    /// calls accept on a subclass Atom with BaseVisitor v,
    /// Atom::accept() will call v.visitAtom(this) and v can decide
    /// what to do. This is useful in situation where we have an Atom*
    /// pointer to a ExternalAtom object and want to pretty print the
    /// ExternalAtom in its different representations (say in raw,
    /// first order or higher order mode). For each representation
    /// form we implement the corresponding concrete Visitor class.
    virtual void
    accept(BaseVisitor&) const = 0;
};


typedef boost::shared_ptr<ProgramObject> ProgramObjectPtr;



/**
 * @brief Container for all elements of a program.
 */
class Repository
{
public:

    /**
     * @brief Get (unique) instance of the static repository class.
     */
    static Repository* Instance();

    /**
     * @brief Register a program element.
     *
     * By registering a program object here, it is assured that the object will
     * be destroyed at pogram termination.
     */
    void
    insert(ProgramObjectPtr);

protected:

    Repository()
    { }

    ~Repository();

private:

    std::vector<ProgramObjectPtr> objects;

    static Repository* _instance;
};




#endif /* _REPOSITORY_H */
