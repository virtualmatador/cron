#include <sstream>

#include "cron.h"

cron::Cron::Cron(const std::string& expression)
	: fields_{ Field(0, 59), Field(0, 23),
		Field(1, 31), Field(1, 12), Field(0, 6) }
{
	std::istringstream parser(expression);
	for (auto& field : fields_)
	{
		std::string field_text;
		parser >> field_text;
		field.parse(field_text);
	}
}

cron::Cron::~Cron()
{
}

std::time_t cron::Cron::next(std::time_t base)
{
	base -= base % 60;
	base += 60;
	auto local_time = std::localtime(&base);
	while (local_time->tm_year < 3000 - 1900)
	{
		if (fields_[3].contain(local_time->tm_mon + 1))
		{
			if (fields_[2].contain(local_time->tm_mday) &&
				fields_[4].contain(local_time->tm_wday))
			{
				if (fields_[1].contain(local_time->tm_hour))
				{
					if (fields_[0].contain(local_time->tm_min))
					{
						break;
					}
					else
					{
						local_time->tm_sec = 0;
						base = std::mktime(local_time);
						base += 60;
						local_time = std::localtime(&base);
					}
				}
				else
				{
					local_time->tm_min = 0;
					local_time->tm_sec = 0;
					base = std::mktime(local_time);
					base += 60 * 60;
					local_time = std::localtime(&base);
				}
			}
			else
			{
				local_time->tm_hour = 0;
				local_time->tm_min = 0;
				local_time->tm_sec = 0;
				base = std::mktime(local_time);
				base += 60 * 60 * 24;
				local_time = std::localtime(&base);
			}
		}
		else
		{
			local_time->tm_mday = 1;
			local_time->tm_hour = 0;
			local_time->tm_min = 0;
			local_time->tm_sec = 0;
			if (++local_time->tm_mon > 11)
			{
				local_time->tm_mon = 0;
				++local_time->tm_year;
			}
			base = std::mktime(local_time);
			local_time = std::localtime(&base);
		}
	}
	return base;
}
