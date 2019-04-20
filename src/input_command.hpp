#pragma once

class input_command
{
public:
	virtual ~input_command() = default;
	virtual void execute()   = 0;
};