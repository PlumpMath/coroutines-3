
#pragma once

#include "execution_context.h"
#include "virtual_memory.h"

namespace generators
{
    namespace aux_
    {
        typedef boost::function<void ()> coroutine_entry_param_t;

        void __declspec(noreturn) coroutine_entry(void * p)
        {
            (*static_cast<coroutine_entry_param_t *>(p))();
        }
    }

    template <typename T>
    struct generator : boost::noncopyable
    {
        // TODO: unwind stack when destroyed

        typedef boost::function<void (T                const &)> yield_function_t;
        typedef boost::function<void (yield_function_t const &)> coroutine_t;

        generator(coroutine_t const & coroutine, size_t stack_size = 4 * 1024 * 1024);

        boost::optional<T> operator()();

    private:
        void __declspec(noreturn) entry_(coroutine_t const & coroutine);
        void yield_(T const &);

        void swap_contexts_();

    private:
        virtual_memory::ptr         stack_;
        execution_context::context  saved_execution_context_;

        boost::optional<T>          last_value_;
    };

    template <typename T>
    generator<T>::generator(coroutine_t const & coroutine, size_t stack_size)
    {
        std::pair<void *, size_t> base_and_size = virtual_memory::allocate(stack_size);
        stack_.reset(base_and_size.first);

        aux_::coroutine_entry_param_t ep(boost::bind(&generator::entry_, this, coroutine));
        saved_execution_context_ = execution_context::create_context(&aux_::coroutine_entry,
                                                                     &ep,
                                                                     base_and_size.first,
                                                                     base_and_size.second);

        swap_contexts_();
    }

    template <typename T>
    boost::optional<T> generator<T>::operator()()
    {
        last_value_ = boost::none;

        swap_contexts_();

        return last_value_;
    }

    template <typename T>
    void __declspec(noreturn) generator<T>::entry_(coroutine_t const & coroutine)
    {
        // scope to free memory from c
        {
            // copy 'cause coroutine is local of ctor of generator
            coroutine_t c = coroutine;
            // allow ctor to quit
            swap_contexts_();

            c(boost::bind(&generator::yield_, this, _1));

            last_value_ = boost::none;
        }

        swap_contexts_();
        assert(false);
    }

    template <typename T>
    void generator<T>::yield_(T const & v)
    {
        assert(!last_value_);

        last_value_ = v;

        swap_contexts_();
    }

    template <typename T>
    void generator<T>::swap_contexts_()
    {
        execution_context::switch_context(&saved_execution_context_, &saved_execution_context_);
    }
}
