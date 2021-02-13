#ifndef _RT_STACK_H_
#define _RT_STACK_H_

#include <rt/common.h>

namespace rt {

// Optimized static stack, size is determined during template initialization
// Needs to be cleared before use
// Unsafe implementation!!!
// Do not push beyond stack size!
// Only pop if stack isn't empty!
template<typename T, uint32 _Size>
class Stack
{
public:
	inline void clear();

	inline void push();
	inline void pop();

	inline T& top();

	inline bool empty() const;

	T data[_Size];
	int32 topIdx;
};

template<typename T, uint32 _Size>
void Stack<T, _Size>::clear()
{
	topIdx = -1;
}

template<typename T, uint32 _Size>
void Stack<T, _Size>::push()
{
	++topIdx;
}

template<typename T, uint32 _Size>
void Stack<T, _Size>::pop()
{
	--topIdx;
}

template<typename T, uint32 _Size>
T& Stack<T, _Size>::top()
{
	return data[topIdx];
}

template<typename T, uint32 _Size>
bool Stack<T, _Size>::empty() const
{
	return ( topIdx < 0 );
}

} // namespace rt

#endif // _RT_STACK_H_
