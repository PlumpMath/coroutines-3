
#pragma once

namespace virtual_memory
{
    namespace aux_
    {
        size_t round_up(size_t value, size_t step)
        {
            return (value + step - 1) / step * step;
        }

        size_t get_page_size()
        {
            SYSTEM_INFO si;
            ::GetSystemInfo(&si);
            return si.dwPageSize;
        }
    }

    struct ptr : boost::noncopyable
    {
        ptr();
        ptr(void * base);
        ~ptr();

        void reset(void * new_base = 0);
        void * release();
        void * get() const;

private:
        void * base_;
    };

    std::pair<void *, size_t> allocate(size_t wanted)
    {
        using namespace aux_;

        size_t real_size = round_up(wanted, get_page_size());

        void * p = ::VirtualAlloc(0, real_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (p == 0)
            throw std::runtime_error("VirtualAlloc failed");

        return std::make_pair(p, real_size);
    }

    ptr::ptr()
        : base_(0)
    {}

    ptr::ptr(void * base)
        : base_(base)
    {}

    ptr::~ptr()
    {
        if (base_ != 0)
        {
            BOOL r = ::VirtualFree(base_, 0, MEM_RELEASE);
            assert(r != FALSE);
        }
    }

    void ptr::reset(void * new_base)
    {
        ptr t(release());

        base_ = new_base;
    }

    void * ptr::release()
    {
        void * t = base_;
        base_ = 0;
        return t;
    }

    void * ptr::get() const
    {
        return base_;
    }

}
