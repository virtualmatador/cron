#ifndef CRON_H
#define CRON_H

#include <array>
#include <ctime>
#include <string>

#include "field.hpp"

namespace cron
{
	class Cron
	{
	public:
		Cron(const std::string& expression);
		~Cron();
		std::time_t next(std::time_t base);

	private:
	    std::array<std::unique_ptr<cron::FieldBase>, 5> fields_;
	};
}
#endif // CRON_H
