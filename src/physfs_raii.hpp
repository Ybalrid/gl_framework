#pragma once

struct physfs_raii
{
	bool init = false;
	explicit physfs_raii(char* argv0);

	~physfs_raii();
	physfs_raii(const physfs_raii&) = delete;
	physfs_raii& operator=(const physfs_raii&) = delete;
	physfs_raii(physfs_raii&& other) noexcept;
	physfs_raii& operator=(physfs_raii&& other) noexcept;

private:
	void steal_guts(physfs_raii& other);
};
