/* DMCS -- Distributed Nonmonotonic Multi-Context Systems.
 * Copyright (C) 2009, 2010 Minh Dao-Tran, Thomas Krennwallner
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

    std::queue<MessagePtr> q;   /// holds data of message queue
    const std::size_t n;   /// capacity of message queue
    std::size_t enq; /// enqueuing counter
    std::size_t deq; /// dequeuing counter

    // a mutex lock and the associated condition variable
    mutable boost::mutex mtx;
    boost::condition_variable cnd;

    inline void
    notifyConsumer()
    {
      if (deq > 0) // is some consumer waiting?
	{
	  cnd.notify_one(); // notify one consuming thread
	}
    }


    inline void
    notifyProducer()
    {
      if (enq > 0) // is some producer waiting?
	{
	  cnd.notify_one(); // notify one producing thread
	}
    }

    
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


    inline void
    pushMessage (MessagePtr m)
    {
      q.push(m);
    }


    inline void 
    popMessage (MessagePtr& m)
    {
      m = q.front();
      q.pop();
    }



  public:

    /// default ctor, capacity is one
    ConcurrentMessageQueueOwning()
      : n(1), enq(0), deq(0)
    { }


    /** 
     * Initialize with capacity, if @a capacity is 0, we force it to be 1.
     * 
     * @param capacity the capacity of this message queue
     */
    ConcurrentMessageQueueOwning(std::size_t capacity)
      : n(capacity > 0 ? capacity : 1), enq(0), deq(0)
    { }


    /// copy ctor, just take capacity
    ConcurrentMessageQueueOwning(const ConcurrentMessageQueueOwning<MessageBase>& q)
      : n(q.n), enq(0), deq(0)
    { }


    virtual
    ~ConcurrentMessageQueueOwning()
    {
      flush();
    }

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

    bool
    empty () const
    {
      boost::mutex::scoped_lock lock(mtx);
      return q.empty();
    }


    bool
    size () const
    {
      return n;
    }


    void
    send (MessagePtr m, unsigned int /* prio */)
    {
      boost::mutex::scoped_lock lock(mtx);
      waitOnCapacity(lock);
      pushMessage(m);
    }


    bool
    try_send (MessagePtr m, unsigned int /* prio */)
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


    bool
    timed_send (MessagePtr m, unsigned int /* prio */, const boost::posix_time::time_duration& t)
    {
      boost::mutex::scoped_lock lock(mtx);

      if (waitOnTimedCapacity(lock, t))
	{
	  pushMessage(m);
	  return true;
	}

      return false;
    }
      

    void
    receive (MessagePtr& m, unsigned int& /* prio */)
    {
      boost::mutex::scoped_lock lock(mtx);
      waitOnEmpty(lock);
      popMessage(m);
    }


    bool
    try_receive (MessagePtr& m, unsigned int /* prio */)
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


    bool
    timed_receive (MessagePtr& m, unsigned int& /* prio */, const boost::posix_time::time_duration& t)
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

