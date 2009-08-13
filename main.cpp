
#include <windows.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include <iostream>

#include "execution_context.h"
#include "generator.h"

void f(boost::function<void (int)> const & yield)
{
    for (int i = 0; i < 40; ++i)
        yield(i);
}

int main()
{
    generators::generator<int> g(&f);

    while (boost::optional<int> a = g())
    {
        std::cout << *a << std::endl;
    }

    return sizeof g;
}
