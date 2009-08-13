
#pragma once

namespace execution_context
{
    namespace aux_
    {
        void * adv(void * p, long diff)
        {
            return (char *)p + diff;
        }

        void push(void ** p, void * value)
        {
            *p = adv(*p, - (long)sizeof value);
            *(void **)*p = value;
        }
    }

    typedef void (*function)(void *);

    struct context
    {
        context()
            : eip(0)  , esp(0)  , ebp(0)
            , ebx(0)  , esi(0)  , edi(0)
        {}

        context(void * eip, void * esp)
            : eip(eip), esp(esp), ebp(0)
            , ebx(0)  , esi(0)  , edi(0)
        {}

        void * eip;
        void * esp;
        void * ebp;
        void * ebx;
        void * esi;
        void * edi;
    };

    context create_context(function f, void * param, void * stack_base, size_t stack_size)
    {
        using namespace aux_;

        void * stack_end = adv(stack_base, (long)stack_size);
        push(&stack_end, param);                            // argument
        push(&stack_end, 0);                                // return address

        return context(f, stack_end);
    }

#pragma warning (push)
#pragma warning (disable : 4731) // frame pointer register 'ebp' modified by inline assembly code
    void __declspec(naked) __fastcall switch_context(context const * load_from, context * store_to)
    {
        __asm
        {
            // maybe xchg?
            mov eax, [ecx + 4]
            mov [edx +  4], esp
            mov esp, eax

            mov eax, [ecx + 8]
            mov [edx +  8], ebp
            mov ebp, eax

            mov eax, [ecx + 12]
            mov [edx + 12], ebx
            mov ebx, eax

            mov eax, [ecx + 16]
            mov [edx + 16], esi
            mov esi, eax

            mov eax, [ecx + 20]
            mov [edx + 20], edi
            mov edi, eax

            mov eax, [ecx]
            // f*cking ms-assembler, I can't just write "mov dword ptr [edx], cont"
            push cont
            pop dword ptr [edx]
            jmp eax

        cont:
            ret
        }
    }
#pragma warning (pop)
}
