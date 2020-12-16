#ifndef CALLER_FUTUREINTERFACEBASEIMPL_HPP
#define CALLER_FUTUREINTERFACEBASEIMPL_HPP
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <list>
#include <caller/async/futureinterface.hpp>

CALLER_BEGIN

class FutureInterfaceBaseImpl : public std::enable_shared_from_this<FutureInterfaceBaseImpl>
{
    typedef std::list<FutureInterfaceBase::CallOut> Signal;
public:
    typedef unsigned char    StorageElement;
    typedef StorageElement*  Storage;
public:
    typedef std::mutex Mutex;
    typedef std::unique_lock<Mutex> Locker;
    typedef std::condition_variable ConditionVar;
    typedef std::atomic_int         AtomicInt;
    typedef std::error_code         StdErrorCode;
public:
    FutureInterfaceBaseImpl(size_t storeSize = 0);
    ~FutureInterfaceBaseImpl();

    bool switchStateTo(int which);

    FutureInterfaceBase::CancelFunc
         connectCallOut(const FutureInterfaceBase::CallOut &o);

    void sendCallOut(int state, Locker *locker);

    mutable Mutex                               m_mutex;
    ConditionVar                                m_waitCondition;
    AtomicInt                                   m_state;
    std::exception_ptr                          m_exceptionPtr;
    Signal                                      m_signal;
    Storage                                     m_storeData;
    StdErrorCode                                m_errorCode;
};

CALLER_END

#endif // CALLER_FUTUREINTERFACEBASEIMPL_HPP
