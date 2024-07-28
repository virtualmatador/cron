#include <iomanip>
#include <sstream>

#include "field.h"

cron::Field::Field(int min, int max)
	: min_{ min }
	, max_{ max }
{
}

cron::Field::~Field()
{
}

void cron::Field::parse(const std::string& expression)
{
	options_.clear();
	std::istringstream field_parser(expression);
	std::string value_text;
	while (std::getline(field_parser, value_text, ','))
	{
		int lower_bound, upper_bound;
		if (value_text == "*")
		{
			lower_bound = min_;
			upper_bound = max_;
		}
		else
		{
			std::istringstream range_parser(value_text);
			lower_bound = -1;
			range_parser >> lower_bound;
			char fill = ' ';
			range_parser >> fill;
			if (fill == '-')
			{
				upper_bound = -1;
				range_parser >> upper_bound;
			}
			else
			{
				upper_bound = lower_bound;
			}
		}
		for (int i = lower_bound; i <= upper_bound; ++i)
		{
			if (i >= min_ && i <= max_)
			{
				options_.insert(i);
			}
		}
	}
	if (options_.empty())
	{
		for (int i = min_; i <= max_; ++i)
		{
			options_.insert(i);
		}
	}
}

bool cron::Field::contain(int option)
{
	return options_.find(option) != options_.end();
}
