#ifndef CRON_HPP
#define CRON_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <limits>
#include <memory>
#include <ranges>
#include <set>
#include <spanstream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace cronio {

template <bool second = false, bool year = false> class cron {
private:
  enum Field : std::size_t {
    f_weekday_,
    f_year_,
    f_month_,
    f_day_,
    f_hour_,
    f_minute_,
    f_second_,
  };
  std::array<std::vector<std::size_t>, f_second_ + (second ? 1 : 0)> fields_;

  void read(std::istream &is) {
    if constexpr (second) {
      fields_[Field::f_second_] = parse<0, 59, 59>(is);
    }
    fields_[Field::f_minute_] = parse<0, 59, 59>(is);
    fields_[Field::f_hour_] = parse<0, 23, 23>(is);
    fields_[Field::f_day_] = parse<1, 31, 32>(is);
    fields_[Field::f_month_] = parse<1, 12, 12>(is);
    fields_[Field::f_weekday_] = parse<0, 6, 6>(is);
    if constexpr (year) {
      fields_[Field::f_year_] = parse<1970, 2099, 2099>(is);
    } else {
      fields_[Field::f_year_] = std::ranges::to<std::vector<std::size_t>>(
          std::ranges::iota_view{1970, 2099 + 1});
    }
  }

  void write(std::ostream &os) const {
    if (second) {
      serialize<59>(fields_[f_second_]);
      os << ' ';
    }
    serialize<59>(fields_[f_minute_]);
    os << ' ';
    serialize<23>(fields_[f_hour_]);
    os << ' ';
    serialize<32 - 1>(fields_[f_day_]);
    os << ' ';
    serialize<12>(fields_[f_month_]);
    os << ' ';
    serialize<6>(fields_[f_weekday_]);
    if (year) {
      os << ' ';
      serialize<2099>(fields_[f_year_]);
    }
  }

  template <std::size_t min, std::size_t max, std::size_t last_value>
  std::vector<std::size_t> parse(std::istream &is) {
    std::set<std::size_t> options;
    enum {
      start,
      asterisk,
      last,
      slash,
      divisor,
      value,
      separator,
      range,
      comma,
    } state = start;
    char c;
    std::size_t number_left;
    std::size_t number_right;
    std::size_t number_divisor;
    while (is.read(&c, 1) || (c = ' ', is.eof())) {
      if (state == start) {
        if (c == '*') {
          state = asterisk;
        } else if (c >= '0' && c <= '9') {
          number_left = c - '0';
          state = value;
        } else if (c == 'l' || c == 'L') {
          state = last;
        } else if (c == ' ') {
          if (is.eof()) {
            throw std::runtime_error("No text to parse.");
          }
        } else {
          throw std::runtime_error("Unknown character at the beginning.");
        }
      } else if (state == asterisk) {
        if (c == ' ') {
          apply_range(options, min, max);
          break;
        } else if (c == '/') {
          number_left = min;
          number_right = max;
          state = slash;
        } else {
          throw std::runtime_error("Unknown character after '*'.");
        }
      } else if (state == slash) {
        if (c >= '0' && c <= '9') {
          number_divisor = c - '0';
          state = divisor;
        } else {
          throw std::runtime_error("Unknown character after '/'.");
        }
      } else if (state == divisor) {
        if (c >= '0' && c <= '9') {
          number_divisor *= 10;
          number_divisor += c - '0';
          if (number_divisor > max) {
            throw std::runtime_error("Divisor shouldn't be bigger than max.");
          }
        } else if (number_divisor == 0) {
          throw std::runtime_error("Divisor shouldn't be zero.");
        } else {
          if (c == ',') {
            apply_divisor(options, number_left, number_right, number_divisor);
            state = comma;
          } else if (c == ' ') {
            apply_divisor(options, number_left, number_right, number_divisor);
            break;
          } else {
            throw std::runtime_error("Unknown character after divisor.");
          }
        }
      } else if (state == value) {
        if (c >= '0' && c <= '9') {
          number_left *= 10;
          number_left += c - '0';
          if (number_left > max) {
            throw std::runtime_error("Value shouldn't be bigger than max.");
          }
        } else if (number_left < min) {
          throw std::runtime_error("Value shouldn't be less than min.");
        } else {
          if (c == '-') {
            state = separator;
          } else if (c == '/') {
            number_right = max;
            state = slash;
          } else if (c == ',') {
            apply_value(options, number_left);
            state = comma;
          } else if (c == ' ') {
            apply_value(options, number_left);
            break;
          } else {
            throw std::runtime_error("Unknown character after value.");
          }
        }
      } else if (state == last) {
        number_left = last_value;
        if (c == '-') {
          state = separator;
        } else if (c == '/') {
          number_right = max;
          state = slash;
        } else if (c == ',') {
          apply_value(options, number_left);
          state = comma;
        } else if (c == ' ') {
          apply_value(options, number_left);
          break;
        }
      } else if (state == separator) {
        if (c >= '0' && c <= '9') {
          number_right = c - '0';
          state = range;
        } else if (c == 'l' || c == 'L') {
          number_right = last_value;
          state = range;
        } else {
          throw std::runtime_error("Unknown character after '-'.");
        }
      } else if (state == range) {
        if (c >= '0' && c <= '9') {
          number_right *= 10;
          number_right += c - '0';
          if (number_right > max) {
            throw std::runtime_error("Limit shouldn't be bigger than max.");
          }
        } else if (number_right < min) {
          throw std::runtime_error("Limit shouldn't be less than min.");
        } else if (number_right < number_left) {
          throw std::runtime_error("Limit shouldn't be less than start.");
        } else {
          if (c == '/') {
            state = slash;
          } else if (c == ',') {
            apply_range(options, number_left, number_right);
            state = comma;
          } else if (c == ' ') {
            apply_range(options, number_left, number_right);
            break;
          } else {
            throw std::runtime_error("Unknown character after limit.");
          }
        }
      } else if (state == comma) {
        if (c == '*') {
          state = asterisk;
        } else if (c >= '0' && c <= '9') {
          number_left = c - '0';
          state = value;
        } else if (c == 'l' || c == 'L') {
          state = last;
        } else {
          throw std::runtime_error("Unknown character after ','.");
        }
      }
    }
    return {options.begin(), options.end()};
  }

  template <std::size_t index, std::size_t last_value>
  void serialize(std::ostream &os) const {
    for (auto it = fields_[index].begin();;) {
      if (*it > last_value) {
        os << 'l';
      } else {
        os << *it;
      }
      if (++it == fields_[index].end()) {
        break;
      }
      os << ',';
    }
  }

  void apply_divisor(std::set<std::size_t> &options, std::size_t left,
                     std::size_t right, std::size_t divisor) {
    for (std::size_t option = left; option <= right; option += divisor) {
      options.insert(option);
    }
  }

  void apply_value(std::set<std::size_t> &options, std::size_t number) {
    options.insert(number);
  }

  void apply_range(std::set<std::size_t> &options, std::size_t left,
                   std::size_t right) {
    for (std::size_t option = left; option <= right; ++option) {
      options.insert(option);
    }
  }

  std::size_t get_last_day(std::size_t y, std::size_t m) const {
    return static_cast<unsigned int>(
        std::chrono::year_month_day_last{std::chrono::last / m / y}.day());
  }

  template <bool forward, std::size_t index> auto field_range() const {
    if constexpr (forward) {
      return std::tuple{fields_[index].begin(), fields_[index].end()};
    } else {
      return std::tuple{fields_[index].rbegin(), fields_[index].rend()};
    }
  }

  template <bool forward, std::size_t index, bool track_last_day, class I,
            std::size_t S>
  bool bump(std::array<I, S> &indexes, bool *reset_last_day) const {
    auto [begin, end] = field_range<forward, index>();
    if (++indexes[index] != end) {
      if constexpr (track_last_day && index == f_month_) {
        *reset_last_day = true;
      }
      return true;
    }
    indexes[index] = begin;
    if constexpr (index > 1) {
      if (bump<forward, index - 1, track_last_day>(indexes, reset_last_day)) {
        if constexpr (track_last_day && index - 1 == f_month_) {
          *reset_last_day = true;
        }
        return true;
      }
    }
    return false;
  }

  template <bool forward, std::size_t index, class I, std::size_t S>
  bool initialize_field(std::array<I, S> &indexes, const int value,
                        bool &need_bump) const {
    auto [begin, end] = field_range<forward, index>();
    if (need_bump) {
      if constexpr (index == S - 1) {
        indexes[index] = std::upper_bound(begin, end, value);
      } else {
        indexes[index] = std::lower_bound(begin, end, value);
      }
      if (indexes[index] == end) {
        need_bump = false;
        indexes[index] = begin;
        return bump<forward, index - 1, false>(indexes, nullptr);
      } else if (*indexes[index] != value) {
        need_bump = false;
      }
    } else {
      indexes[index] = begin;
    }
    return true;
  }

  template <bool forward, bool unbumped, class I, std::size_t S>
  bool bump_day(std::array<I, S> &indexes, std::size_t &last_day) const {
    if (bool reset_last_day = false;
        bump<forward, f_day_, true>(indexes, &reset_last_day)) {
      if (reset_last_day) {
        last_day = get_last_day(*indexes[f_year_], *indexes[f_month_]);
      }
      if constexpr (unbumped) {
        if constexpr (forward) {
          indexes[f_hour_] = fields_[f_hour_].begin();
          indexes[f_minute_] = fields_[f_minute_].begin();
          if constexpr (second) {
            indexes[f_second_] = fields_[f_second_].begin();
          }
        } else {
          indexes[f_hour_] = fields_[f_hour_].rbegin();
          indexes[f_minute_] = fields_[f_minute_].rbegin();
          if constexpr (second) {
            indexes[f_second_] = fields_[f_second_].rbegin();
          }
        }
      }
      return adjust_day<forward, false>(indexes, last_day);
    }
    return false;
  }

  template <bool forward, bool unbumped, class I, std::size_t S>
  bool adjust_day(std::array<I, S> &indexes, std::size_t &last_day) const {
    std::size_t day;
    if (*indexes[f_day_] > last_day) {
      if (*indexes[f_day_] == 32) {
        day = last_day;
      } else {
        return bump_day<forward, unbumped>(indexes, last_day);
      }
    } else {
      day = *indexes[f_day_];
    }
    std::size_t weekday = std::chrono::weekday{
        std::chrono::sys_days{std::chrono::year_month_day{
            std::chrono::year{static_cast<int>(*indexes[f_year_])},
            std::chrono::month{static_cast<unsigned int>(*indexes[f_month_])},
            std::chrono::day{static_cast<unsigned int>(day)},
        }}}.c_encoding();
    auto [begin, end] = field_range<forward, f_weekday_>();
    indexes[f_weekday_] = std::find(begin, end, weekday);
    if (indexes[f_weekday_] == end) {
      return bump_day<forward, unbumped>(indexes, last_day);
    }
    return true;
  }

  template <bool forward> std::tm calculate(std::tm time) const {
    std::array<
        std::conditional_t<forward, std::vector<std::size_t>::const_iterator,
                           std::vector<std::size_t>::const_reverse_iterator>,
        std::tuple_size<decltype(fields_)>::value>
        indexes;
    std::size_t last_day;
    bool need_bump = true;
    if (initialize_field<forward, f_year_>(indexes, time.tm_year + 1900,
                                           need_bump) &&
        initialize_field<forward, f_month_>(indexes, time.tm_mon + 1,
                                            need_bump) &&
        initialize_field<forward, f_day_>(indexes, time.tm_mday, need_bump) &&
        initialize_field<forward, f_hour_>(indexes, time.tm_hour, need_bump) &&
        initialize_field<forward, f_minute_>(indexes, time.tm_min, need_bump) &&
        (!second || initialize_field<forward, f_second_>(indexes, time.tm_sec,
                                                         need_bump)) &&
        (last_day = get_last_day(*indexes[f_year_], *indexes[f_month_]),
         adjust_day<forward, true>(indexes, last_day))) {
      time.tm_wday = *indexes[f_weekday_];
      time.tm_year = *indexes[f_year_] - 1900;
      time.tm_mon = *indexes[f_month_] - 1;
      time.tm_mday = *indexes[f_day_];
      time.tm_hour = *indexes[f_hour_];
      time.tm_min = *indexes[f_minute_];
      if (second) {
        time.tm_sec = *indexes[f_second_];
      } else {
        time.tm_sec = 0;
      }
    }
    return time;
  }

public:
  cron() : fields_{} {}

  cron(std::string_view source) : cron() {
    std::ispanstream is{source};
    read(is);
  }

  bool match(const std::tm &time) const {
    return std::binary_search(fields_[f_year_].begin(), fields_[f_year_].end(),
                              time.tm_year + 1900) &&
           std::binary_search(fields_[f_month_].begin(),
                              fields_[f_month_].end(), time.tm_mon + 1) &&
           (std::binary_search(fields_[f_day_].begin(), fields_[f_day_].end(),
                               time.tm_mday) ||
            (fields_[f_day_].back() == 32 &&
             get_last_day(time.tm_year + 1900, time.tm_mon + 1) ==
                 time.tm_mday)) &&
           std::binary_search(fields_[f_hour_].begin(), fields_[f_hour_].end(),
                              time.tm_hour) &&
           std::binary_search(fields_[f_minute_].begin(),
                              fields_[f_minute_].end(), time.tm_min) &&
           (!second ||
            std::binary_search(fields_[f_second_].begin(),
                               fields_[f_second_].end(), time.tm_sec)) &&
           std::binary_search(fields_[f_weekday_].begin(),
                              fields_[f_weekday_].end(), time.tm_wday);
  }

  std::tm next(std::tm time) const { return calculate<true>(time); }

  std::tm previous(std::tm time) const { return calculate<false>(time); }

  template <bool s, bool y>
  friend std::istream &operator>>(std::istream &is, cron<s, y> &that) {
    that.read(is);
    return is;
  }
  template <bool s, bool y>
  friend std::ostream &operator<<(std::ostream &os, const cron<s, y> &that) {
    that.write(os);
    return os;
  }
};

} // namespace cronio

#endif // CRON_HPP
