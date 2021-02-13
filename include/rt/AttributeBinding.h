#ifndef _RT_ATTRIBUTEBINDING_H_
#define _RT_ATTRIBUTEBINDING_H_

#include <rt/common.h>
#include <stack>

namespace rt {

struct AttributeBinding 
{
	AttributeBinding();
	void reset();

	RTenum normalBinding;	// default = PER_VERTEX;
	RTenum colorBinding;	// default = PER_MATERIAL;
	RTenum textureBinding;	// default = PER_MATERIAL;
};

class AttributeBindingStack
{
public:
	AttributeBindingStack();

	void pushAllAttrib();
	AttributeBinding& top();
	void popAllAttrib();
	
private:
	std::stack<AttributeBinding> _stack;
};

} // namespace rt

#endif // _RT_ATTRIBUTEBINDING_H_
