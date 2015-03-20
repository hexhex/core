/* DMCS -- Distributed Nonmonotonic Multi-Context Systems.
 * Copyright (C) 2006-2015 Thomas Krennwallner
 * 
 * This file is part of DMCS.
 *
 *  DMCS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DMCS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DMCS.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   ConcurrentMessageQueueOwning.h
 * @author Thomas Krennwallner <tkren@kr.tuwien.ac.at>
 * @author Peter Schueller <ps@kr.tuwien.ac.at>
 * @date   Sun Jan  9 06:57:57 2011
 * 
 * @brief  Inter-thread syncronisation using a queue that owns its messages using a shared_ptr.
 */


#ifndef _CONCURRENT_MESSAGE_QUEUE_OWNING_H
#define _CONCURRENT_MESSAGE_QUEUE_OWNING_H

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/time_duration.hpp>

#include <queue>

namespace dlvhex
{

  /** 
   * Message queue for inter-thread communication. Modelled after
   * boost::interprocess::message_queue.
   */
  template<class MessageBase>
  class ConcurrentMessageQueueOwning
  {
  private:
    typedef boost::shared_ptr<MessageBase> MessagePtr;

    /** \brief Holds data of message queue. */
    std::queue<MessagePtr> q;
    /** \brief Capacity of message queue. */
    const std::size_t n;
    /** \brief Enqueuing counter. */
    std::size_t enq;
    /** \brief Dequeuing counter. */
    std::size_t deq;

    /** \brief A mutex lock and the associated condition variable. */
    mutable boost::mutex mtx;
    /** \brief Condition for multithreading access. */
    boost::condition_variable cnd;

    /** \brief Notifier of model consumer. */
    inline void
    notifyConsumer()
    {
      if (deq > 0) // is some consumer waiting?
	{
	  cnd.notify_one(); // notify one consuming thread
	}
    }


    /** \brief Notifier of model produces. */
    inline void
    notifyProducer()
    {
      if (enq > 0) // is some producer waiting?
	{
	  cnd.notify_one(); // notify one producing thread
	}
    }

    /** \brief Wait until free space is available in the queue. */
    inline void
    waitOnCapacity(boost::mutex::scoped_lock& lock)
    {
      while (n == q.size()) // maximum capacity reached
	{
	  ++enq;
	  cnd.wait(lock);
	  --enq;
	}

      notifyConsumer();
    }

    /** \brief Wait until the queue is empty. */
    inline void
    waitOnEmpty(boost::mutex::scoped_lock& lock)
    {
      while (q.empty()) // minimum capacity reached
	{
	  ++deq;
	  cnd.wait(lock);
	  --deq;
	}

      notifyProducer();
    }

    /** \brief Wait until free space is available in the queue respecting a timeout.
      * @param lock Mutex.
      * @param t Timeout
      * @return True if free space is available, false if timeout occurred. */
    inline bool
    waitOnTimedCapacity(boost::mutex::scoped_lock& lock, const boost::posix_time::time_duration& t)
    {
      bool no_timeout = true;
      
      while (n == q.size() && no_timeout) // maximum capacity reached
	{
	  ++enq;
	  no_timeout = cnd.timed_wait(lock, t);
	  --enq;
	}
      
      notifyConsumer();
      
      return no_timeout;
    }

    /** \brief Wait until queue is empty respecting a timeout.
      * @param lock Mutex.
      * @param t Timeout
      * @return True if queue is empty, false if timeout occurred. */
    inline bool
    waitOnTimedEmpty(boost::mutex::scoped_lock& lock, const boost::posix_time::time_duration& t)
    {
      bool no_timeout = true;

      while (q.empty() && no_timeout) // minimum capacity reached
	{
	  ++deq;
	  no_timeout = cnd.timed_wait(lock, t);
	  --deq;
	}

      notifyProducer();

      return no_timeout;
    }

    /** \brief Add a message.
      * @param m Message to add. */
    inline void
    pushMessage (MessagePtr m)
    {
      q.push(m);
    }

    /** \brief Retrieve and remove a message.
      * @param m Retrieved message. */
    inline void 
    popMessage (MessagePtr& m)
    {
      m = q.front();
      q.pop();
    }



  public:

    /** \brief Default constructor, capacity is one. */
    ConcurrentMessageQueueOwning()
      : n(1), enq(0), deq(0)
    { }


    /** 
     * Initialize with capacity, if @a capacity is 0, we force it to be 1.
     * 
     * @param capacity the capacity of this message queue.
     */
    ConcurrentMessageQueueOwning(std::size_t capacity)
      : n(capacity > 0 ? capacity : 1), enq(0), deq(0)
    { }


    /** \brief Copy-constructor, just take capacity but not content.
      * @param q Second ConcurrentMessageQueueOwning. */
    ConcurrentMessageQueueOwning(const ConcurrentMessageQueueOwning<MessageBase>& q)
      : n(q.n), enq(0), deq(0)
    { }

    /** \brief Destructor. */
    virtual
    ~ConcurrentMessageQueueOwning()
    {
      flush();
    }

    /** \brief Pop all element from the queue. */
    void flush()
    {
      {
        boost::mutex::scoped_lock lock(mtx);
        // just pop all elements from the queue (the smart pointers automatically
        // destruct the elements and free the memory)
        while (!q.empty())
            q.pop();
      }
      notifyProducer();
    }

    /** \brief Clear queue.
      * @return True. */
    bool
    empty () const
    {
      boost::mutex::scoped_lock lock(mtx);
      return q.empty();
    }

    /** \brief Return size of the queue.
      * @return Size. */
    bool
    size () const
    {
      return n;
    }

    /** \brief Send message.
      * @param m Message.
      * @param prio Unused. */
    void
    send (MessagePtr m, unsigned int prio)
    {
      boost::mutex::scoped_lock lock(mtx);
      waitOnCapacity(lock);
      pushMessage(m);
    }

    /** \brief Try to send a message.
      * @param m Message.
      * @param prio Unused.
      * @return True if succeeded. */
    bool
    try_send (MessagePtr m, unsigned int prio)
    {
      boost::mutex::scoped_lock lock(mtx);

      if (q.size() < n)
	{
	  pushMessage(m);
	  notifyConsumer();
	  return true;
	}

      return false;      
    }

    /** \brief Try to send a message respecting a timeout.
      * @param m Message.
      * @param prio Unused.
      * @param t Timeout.
      * @return True if succeeded. */
    bool
    timed_send (MessagePtr m, unsigned int prio, const boost::posix_time::time_duration& t)
    {
      boost::mutex::scoped_lock lock(mtx);

      if (waitOnTimedCapacity(lock, t))
	{
	  pushMessage(m);
	  return true;
	}

      return false;
    }
      
    /** \brief Receive a message.
      * @param m Output message.
      * @param prio Unused. */
    void
    receive (MessagePtr& m, unsigned int& prio)
    {
      boost::mutex::scoped_lock lock(mtx);
      waitOnEmpty(lock);
      popMessage(m);
    }

    /** \brief Try to receive a message.
      * @param m Output message.
      * @param prio Unused.
      * @return True if succeeded. */
    bool
    try_receive (MessagePtr& m, unsigned int prio)
    {
      boost::mutex::scoped_lock lock(mtx);

      if (!q.empty())
	{
	  popMessage(m);
	  notifyProducer();
	  return true;
	}

      return false;      
    }

    /** \brief Try to receive a message respecting a timeout.
      * @param m Output message.
      * @param prio Unused.
      * @return t Timeout.
      * @return True if succeeded. */
    bool
    timed_receive (MessagePtr& m, unsigned int& prio, const boost::posix_time::time_duration& t)
    {
      boost::mutex::scoped_lock lock(mtx);

      if (waitOnTimedEmpty(lock, t))
	{
	  popMessage(m);
	  return true;
	}

      return false;
    }

  };

} // namespace dlvhex


#endif // _CONCURRENT_MESSAGE_QUEUE_H

// vim:ts=8:
// Local Variables:
// mode: C++
// End:

