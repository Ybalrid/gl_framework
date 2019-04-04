#pragma once

#include "node.hpp"
struct command
{
	virtual ~command()				  = default;
	virtual void execute(node* actor) = 0;
};
