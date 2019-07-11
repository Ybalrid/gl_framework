#pragma once

#include "node.hpp"

///Command interface
struct command
{
	///The usual virtual dtor
	virtual ~command() = default;

	///Execute callback. Will provide pointer to a node in the scene if relevant
	virtual void execute(node* actor) = 0;
};
