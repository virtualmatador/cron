#ifndef FIELD_H
#define FIELD_H

#include <set>
#include <string>
#include <sstream>
#include <memory>
#include <array>
#include <iostream>

namespace cron
{

class FieldBase {
public:
    virtual ~FieldBase() = default;
    virtual void parse(const std::string& field_text) = 0;
    virtual bool contain(int option) const = 0;
    virtual void print() const = 0; // Método para imprimir información
};

template<std::size_t Min, std::size_t Max>
class Field : public FieldBase
{
public:
    Field() = default;
    ~Field() override = default;

    void parse(const std::string& field_text) override {
        options_.clear();
        std::istringstream field_parser(field_text);
        std::string value_text;
        while (std::getline(field_parser, value_text, ','))
        {
            int lower_bound, upper_bound;
            if (value_text == "*")
            {
                lower_bound = Min;
                upper_bound = Max;
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
                if (i >= Min && i <= Max)
                {
                    options_.insert(i);
                }
            }
        }
        if (options_.empty())
        {
            for (int i = Min; i <= Max; ++i)
            {
                options_.insert(i);
            }
        }
    }

    bool contain(int option) const override {
        return options_.find(option) != options_.end();
    }

    void print() const override {
        std::cout << "Field<" << Min << "," << Max << "> with options: ";
        for (const auto& option : options_) {
            std::cout << option << " ";
        }
        std::cout << "\n";
    }

private:
    std::set<int> options_;
};

}

#endif // FIELD_H
