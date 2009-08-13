
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
        typedef boost::function<bool (T                const &)> yield_function_t;
        typedef boost::function<void (yield_function_t const &)> coroutine_t;

        generator(coroutine_t const & coroutine, size_t stack_size = 4 * 1024 * 1024);
        ~generator();

        boost::optional<T> operator()();

    private:
        struct empty
        {
            empty() {} // suppress warning C4345 inside boost::variant
        };
        struct error {};

        // result from yield
        typedef boost::variant<empty, T, error> generator_return_t;

        struct terminated {};
        // "want (if != 0) or don't (if == 0) want return" or terminated
        typedef boost::variant<generator_return_t *, terminated> generator_state_t;

    private:
        void __declspec(noreturn) entry_(coroutine_t const & coroutine);
        bool yield_(T const &);

        void swap_contexts_();

    private:
        virtual_memory::ptr          stack_;
        execution_context::context   saved_execution_context_;

        generator_state_t            generator_state_;
    };

    namespace aux_
    {
        template <typename T>
        struct generator_return_visitor
        {
            typedef boost::optional<T> result_type;

            result_type operator()(typename generator<T>::empty) const
            {
                return boost::none;
            }

            result_type operator()(T const & v) const
            {
                return v;
            }

            result_type operator()(typename generator<T>::error) const
            {
                throw std::runtime_error("coroutine throws an exception");
            }
        };
    }

    template <typename T>
    generator<T>::generator(coroutine_t const & coroutine, size_t stack_size)
        : generator_state_((generator_return_t *)0)
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
    generator<T>::~generator()
    {
        if (boost::get<terminated>(&generator_state_) != 0)
            return;

        swap_contexts_();

        assert(boost::get<terminated>(&generator_state_) != 0);
    }

    template <typename T>
    boost::optional<T> generator<T>::operator()()
    {
        assert(boost::get<generator_return_t *>(generator_state_) == 0);

        generator_return_t gr;
        generator_state_ = &gr;

        swap_contexts_();

        if (boost::get<generator_return_t *>(&generator_state_) != 0)
            generator_state_ = (generator_return_t *)0;

        return boost::apply_visitor(aux_::generator_return_visitor<T>(), gr);
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

            try
            {
                c(boost::bind(&generator::yield_, this, _1));
            }
            catch (...)
            {
                generator_return_t * t = boost::get<generator_return_t *>(generator_state_);
                if (t != 0)
                    *t = error();
                else
                    assert(false);
            }
        }

        generator_state_ = terminated();

        swap_contexts_();
        assert(false);
    }

    template <typename T>
    bool generator<T>::yield_(T const & v)
    {
        // TODO: do not throw an exception
        if (boost::get<generator_return_t *>(generator_state_) != 0)
        {
            *boost::get<generator_return_t *>(generator_state_) = v;
            swap_contexts_();

            return boost::get<generator_return_t *>(generator_state_) != 0;
        }
        else
        {
            assert(false);
            return false;
        }
    }

    template <typename T>
    void generator<T>::swap_contexts_()
    {
        execution_context::switch_context(&saved_execution_context_, &saved_execution_context_);
    }
}
