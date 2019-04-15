#pragma once
#include "node.hpp"

class input_command
{
public:
	virtual ~input_command()		  = default;
	virtual void execute(node* actor) = 0;
};