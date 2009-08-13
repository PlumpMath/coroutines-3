
#include <windows.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>

#include <iostream>

#include "execution_context.h"
#include "generator.h"


void f(boost::function<bool (int)> const & yield)
{
    int i = 0;
    while (yield(i++));
}

int main()
{
    generators::generator<int> g(&f);

    for (int i = 0; i < 1000; ++i)
    {
        boost::optional<int> a = g();
        if (!a)
            break;
        std::cout << *a << std::endl;
    }

    return 0;
}
