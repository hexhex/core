/* -*- C++ -*- */

/**
 * @file Program.h
 * @author Roman Schindlauer
 * @date Tue Mar  7 16:48:47 CET 2006
 *
 * @brief Program class.
 *
 */


#ifndef _PROGRAM_H
#define _PROGRAM_H

#include "dlvhex/Rule.h"


/**
 * @brief Program class.
 *
 * The Program class encapsulates rules and external atoms to represent a subprogram
 */
class Program
{
public:

    /// @todo: we should use a set here!
    typedef std::vector<const Rule*> ruleset_t;

    class const_iterator
    {
        ruleset_t::const_iterator it;

    public:

        const_iterator()
        {
            //assert(0);
        }

        const_iterator(const ruleset_t::const_iterator &it1)
            : it(it1)
        { }

        const Rule*
        operator *() const
        {
            return (*it);
        }

        void
        operator ++()
        {
            it++;
        }

        bool
        operator== (const const_iterator& i2) const
        {
            return it == i2.it;
        }

        bool
        operator != (const const_iterator& i2) const
        {
            return (it != i2.it);
        }
    };

    const_iterator
    begin() const
    {
        return const_iterator(rules.begin());
    }

    const_iterator
    end() const
    {
        return const_iterator(rules.end());
    }

    Program();

    void
    addRule(const Rule*);

    bool
    exists(const Rule*);

    const std::vector<ExternalAtom*>&
    getExternalAtoms() const;

    /**
     * Only for debugging purposes. The real output functions are implemented
     * by the ProgramBuilder class!
     */
    void
    dump(std::ostream&) const;

private:

    ruleset_t rules;

    std::vector<ExternalAtom*> externalAtoms;
};



#endif /* _PROGRAM_H */
