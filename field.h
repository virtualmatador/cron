#ifndef FIELD_H
#define FIELD_H

#include <set>
#include <string>

namespace cron
{
	class Field
	{
	public:
		Field(int min, int max);
		~Field();
		void parse(const std::string& field_text);
		bool contain(int option);

	private:
		std::set<int> options_;
		int min_;
		int max_;
	};
}

#endif // FIELD_H
