#ifndef CRON_HPP
#define CRON_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace cronio {

template <bool second = false, bool year = false>
class cron {
 private:
  inline static constexpr std::size_t size_{5 + (second ? 1 : 0)};

  std::array<std::vector<std::size_t>, size_> fields_;
  std::vector<std::size_t> week_days_;

  void read(std::istream& is) {
    is >> std::noskipws;
    if constexpr (second) {
      fields_[5] = parse<0, 59>(is);
    }
    fields_[4] = parse<0, 59>(is);
    fields_[3] = parse<0, 23>(is);
    fields_[2] = parse<1, 31>(is);
    fields_[1] = parse<1, 12>(is);
    week_days_ = parse<0, 6>(is);
    std::conditional_t<year, std::istream*, std::unique_ptr<std::istream>>
        year_is;
    if constexpr (year) {
      year_is = &is;
    } else {
      year_is = std::make_unique<std::istringstream>("*");
    }
    fields_[0] = parse<1970, 2099>(*year_is);
  }

  void write(std::ostream& os) {}

  template <std::size_t min, std::size_t max>
  std::vector<std::size_t> parse(std::istream& is) {
    std::set<std::size_t> options;
    enum {
      start,
      asterisk,
      slash,
      divisor,
      value,
      separator,
      range,
      comma,
      error,
    } state = start;
    char c;
    std::size_t number_left;
    std::size_t number_right;
    while (is >> c || (c = ' ', is.eof())) {
      if (state == start) {
        if (c == '*') {
          state = asterisk;
        } else if (c >= '0' && c <= '9') {
          number_left = c - '0';
          state = value;
        } else if (c == ' ') {
          if (is.eof()) {
            is.setstate(std::ios::badbit);
            break;
          }
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      } else if (state == asterisk) {
        if (c == ' ') {
          for (std::size_t option = min; option <= max; ++option) {
            options.insert(option);
          }
          break;
        } else if (c == '/') {
          state = slash;
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      } else if (state == slash) {
        if (c >= '0' && c <= '9') {
          number_left = c - '0';
          state = divisor;
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      } else if (state == divisor) {
        if (c >= '0' && c <= '9') {
          number_left *= 10;
          number_left += c - '0';
        } else if (c == ',') {
          apply_divisor<min, max>(options, number_left);
          state = comma;
        } else if (c == ' ') {
          apply_divisor<min, max>(options, number_left);
          break;
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      } else if (state == value) {
        if (c >= '0' && c <= '9') {
          number_left *= 10;
          number_left += c - '0';
        } else if (c == '-') {
          state = separator;
        } else if (c == ',') {
          apply_value<min, max>(options, number_left);
          state = comma;
        } else if (c == ' ') {
          apply_value<min, max>(options, number_left);
          break;
        }
      } else if (state == separator) {
        if (c >= '0' && c <= '9') {
          number_right = c - '0';
          state = range;
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      } else if (state == range) {
        if (c >= '0' && c <= '9') {
          number_right *= 10;
          number_right += c - '0';
        } else if (c == ',') {
          apply_range<min, max>(options, number_left, number_right);
          state = comma;
        } else if (c == ' ') {
          apply_range<min, max>(options, number_left, number_right);
          break;
        }
      } else if (state == comma) {
        if (c == '*') {
          state = asterisk;
        } else if (c >= '0' && c <= '9') {
          number_left = c - '0';
          state = value;
        } else {
          is.setstate(std::ios::badbit);
          break;
        }
      }
    }
    if (is.bad()) {
      options.clear();
    }
    return {options.begin(), options.end()};
  }

  template <std::size_t min, std::size_t max>
  void apply_divisor(std::set<std::size_t>& options, std::size_t number) {
    for (std::size_t option = min; option <= max; option += number) {
      options.insert(option);
    }
  }

  template <std::size_t min, std::size_t max>
  void apply_value(std::set<std::size_t>& options, std::size_t number) {
    if (number >= min && number <= max) {
      options.insert(number);
    }
  }

  template <std::size_t min, std::size_t max>
  void apply_range(std::set<std::size_t>& options, std::size_t number_left,
                   std::size_t number_right) {
    for (std::size_t option = std::max(min, number_left);
         option < std::min(max, number_right); ++option) {
      options.insert(option);
    }
  }

  template <bool forward>
  auto field_range(const std::size_t index) const {
    if constexpr (forward) {
      return std::tuple{fields_[index].begin(), fields_[index].end()};
    } else {
      return std::tuple{fields_[index].rbegin(), fields_[index].rend()};
    }
  }

  template <bool forward, class I>
  bool bump(std::array<I, size_>& indexes, const std::size_t index) const {
    auto [begin, end] = field_range<forward>(index);
    if (++indexes[index] != end) {
      return true;
    }
    if (index > 0) {
      indexes[index] = begin;
      return bump<forward>(indexes, index - 1);
    }
    return false;
  }

  template <bool forward, class I>
  bool initialize_iterator(bool& match_previous, std::array<I, size_>& indexes,
                           const std::size_t index, const int value) const {
    auto [begin, end] = field_range<forward>(index);
    if (match_previous) {
      if (index < size_ - 1) {
        indexes[index] = std::lower_bound(begin, end, value);
      } else {
        indexes[index] = std::upper_bound(begin, end, value);
      }
      if (indexes[index] == end) {
        match_previous = false;
        if (index == 0) {
          return false;
        }
        indexes[index] = begin;
        return bump<forward>(indexes, index - 1);
      } else if (*indexes[index] != value) {
        match_previous = false;
      }
      return true;
    }
    indexes[index] = begin;
    return true;
  }

  template <bool forward, class I>
  bool validate(std::array<I, size_>& indexes) const {
    std::chrono::year_month ym{std::chrono::year{*indexes[0]},
                               std::chrono::month{*indexes[1]}};
    std::chrono::year_month_day_last ymdl{ym / std::chrono::last};
    if (ymdl.day().ok()) {
      std::chrono::year_month_day ymd{ym.year(), ym.month(),
                                      std::chrono::day{*indexes[2]}};
      if (std::binary_search(week_days_.begin(), week_days_.end(),
                             std::chrono::weekday{ymd}.c_encoding())) {
        return true;
      }
    }
    if (bump<forward>(indexes, 2)) {
      return validate<forward>(indexes);
    }
    return false;
  }

  template <bool forward>
  std::tm calculate(std::tm time) const {
    std::array<
        std::conditional_t<forward, std::vector<std::size_t>::const_iterator,
                           std::vector<std::size_t>::const_reverse_iterator>,
        size_>
        indexes;
    bool match_previous = true;
    if (initialize_iterator<forward>(match_previous, indexes, 0,
                                     time.tm_year + 1900) &&
        initialize_iterator<forward>(match_previous, indexes, 1,
                                     time.tm_mon + 1) &&
        initialize_iterator<forward>(match_previous, indexes, 2,
                                     time.tm_mday) &&
        initialize_iterator<forward>(match_previous, indexes, 3,
                                     time.tm_hour) &&
        initialize_iterator<forward>(match_previous, indexes, 4, time.tm_min) &&
        (!second || initialize_iterator<forward>(match_previous, indexes, 5,
                                                 time.tm_sec))) {
      if (validate<forward>(indexes)) {
        if (second) {
          time.tm_sec = *indexes[5];
        } else {
          time.tm_sec = 0;
        }
        time.tm_min = *indexes[4];
        time.tm_hour = *indexes[3];
        time.tm_mday = *indexes[2];
        time.tm_mon = *indexes[1] - 1;
        time.tm_year = *indexes[0] - 1900;
      }
    }
    return time;
  }

 public:
  cron() {}

  cron(std::string_view source) { read(std::spanstream(source)); }

  bool match(const std::tm& time) const {
    return std::binary_search(fields_[0].begin(), fields_[0].end(),
                              time.tm_year) &&
           std::binary_search(fields_[1].begin(), fields_[1].end(),
                              time.tm_mon) &&
           std::binary_search(fields_[2].begin(), fields_[2].end(),
                              time.tm_mday) &&
           std::binary_search(fields_[3].begin(), fields_[3].end(),
                              time.tm_hour) &&
           std::binary_search(fields_[4].begin(), fields_[4].end(),
                              time.tm_min) &&
           (!second || std::binary_search(fields_[5].begin(), fields_[5].end(),
                                          time.tm_sec)) &&
           std::binary_search(week_days_.begin(), week_days_.end(),
                              time.tm_wday);
  }

  std::tm next(std::tm time) const { return calculate<true>(time); }

  std::tm previous(std::tm time) const { return calculate<false>(time); }

  template <bool s, bool y>
  friend std::istream& std::operator>>(std::istream& is, cron<s, y>& that);
  template <bool s, bool y>
  friend std::ostream& std::operator<<(std::ostream& os,
                                       const cron<s, y>& that);
};

}  // namespace cronio

namespace std {

template <bool second, bool year>
std::istream& operator>>(std::istream& is, cronio::cron<second, year>& that) {
  that.read(is);
  return is;
}

template <bool second, bool year>
std::ostream& operator<<(std::ostream& os,
                         const cronio::cron<second, year>& that) {
  that.write(os);
  return os;
}

}  // namespace std

#endif  // CRON_HPP
