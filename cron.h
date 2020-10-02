#ifndef CRON_H
#define CRON_H

#include <array>
#include <ctime>
#include <string>

#include "field.h"

namespace cron
{
	class Cron
	{
	public:
		Cron(const std::string& expression);
		~Cron();
		std::time_t next(std::time_t base);

	private:
		std::array<Field, 5> fields_;
	};
}
#endif // CRON_H
