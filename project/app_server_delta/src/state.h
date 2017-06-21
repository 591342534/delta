#ifndef __STATE_H__
#define __STATE_H__
#include <atomic>

namespace serverframe{;
////////////////////////////////////////////////////////////////////////////////
class state
{
protected:
    enum {
        prepare = 0,
        running,
        paused,
        stoped,
    };

public:
    state() : m_value(prepare)
    {}

    /*@ set state: */
    inline void run()
    {
        m_value = running;
    }
    inline void pause()
    {
        m_value = paused;
    }
    inline void stop()
    {
        m_value = stoped;
    }

public:
    /*@ get state: */
    inline bool is_running() const
    {
        return m_value == running;
    }
    inline bool is_paused() const
    {
        return m_value == paused;
    }
    inline bool is_stoped() const
    {
        return m_value == stoped;
    }

private:
    std::atomic<int> m_value;
};


////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
