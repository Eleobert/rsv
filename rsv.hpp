#include <charconv>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>
#include <iostream>
#include <tuple>

namespace rsv
{
    namespace internal
    {
        template<typename T>
        auto to_num(const std::string_view& str)
        {
            T value = 0;
            
            if constexpr (std::is_integral<T>::value)
            {
                value = std::numeric_limits<T>::max();
            }
            else
            {
                value = std::numeric_limits<T>::quiet_NaN();
            }

            if(not str.empty()) [[likely]]
            {
                auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

                if(ec != std::errc())
                {
                    std::cerr << "Error converting value '" << str << "'\n";
                    std::exit(1);
                }
            }

            return value;
        }

        template<typename T>
        #ifdef __cpp_lib_concepts
        requires (std::integral<T> || std::floating_point<T> || std::same_as<T, std::string>) && (!std::same_as<T, bool>)
        #endif
        void delegate(const std::string_view& src, void* dst)
        {
            auto* casted = static_cast<std::vector<T>*>(dst);;
            casted->emplace_back(to_num<T>(src));
        }

        template<>
        inline void delegate<std::string>(const std::string_view& src, void* dst)
        {
            auto* casted = static_cast<std::vector<std::string>*>(dst);;
            casted->emplace_back(src.begin(), src.end());
        }

        struct field
        {
            std::string name;
            int64_t pos = -1;
            void* data = nullptr;
            decltype(delegate<std::string>)* del = nullptr;
        };
    }

    auto open(const std::string& name) -> std::tuple<std::ifstream, std::error_condition>;

    // TODO: rsv::columns should not reset the file cursor. Fixing this is not
    // straightforward since tellg/seekg does not work very well in conjunction
    // on windows when file is open in text mode.

    // TODO: consider not to pass 'sep' to rsv::columns since it is also passed
    // to rsv::read. Peharps it would be better to set it as an atrribute of file
    // (pass to rsv::open, once).

    /**
    * Return the list of column names
    * @param file input file. Resets the the cursor to the beginning of the file.
    */
    auto columns(std::ifstream& file, char sep) -> std::vector<std::string>;

    auto schema(std::vector<internal::field> fields) -> std::vector<internal::field>;

    // TODO: probably it is better to receive schema as positional argument
    auto read(std::ifstream& file, const std::vector<internal::field>& sch, char sep = '\t') -> void;
    

    template <typename T>
    auto f(const std::string& name, std::vector<T>& data)
    {
        auto this_field = internal::field();
        this_field.name = name;
        this_field.data = &data;
        this_field.del  = internal::delegate<T>;
        return this_field;
    }

    template <typename T>
    auto f(int64_t pos, std::vector<T>& data)
    {
        auto this_field = internal::field();
        this_field.pos  = pos;
        this_field.data = &data;
        this_field.del  = internal::delegate<T>;
        return this_field;
    }
}