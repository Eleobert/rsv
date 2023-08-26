#include <charconv>
#include <cstdint>
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
            int64_t pos = std::numeric_limits<int64_t>::max();
            void* data = nullptr;
            decltype(delegate<std::string>)* del = nullptr;
        };
    }

    struct options
    {
        int64_t skip  = 0;
        int64_t nrows = std::numeric_limits<int64_t>::max();
        bool header   = true;
    };

    using schema = std::vector<internal::field>;

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

    /**
    * Reads separated values. The function assumes that all column names are present 
    * in the file, otherwise the program will be aborted. To avoid this, the user can
    * inspect for missing columns by calling rsv::columns before calling rsv::read.
    * @param file the cursor will be set to the start to the file before reading values.
    * Upon returning the cursor will be at the end.
    * @param sep the delimiter. The delimiter will be ignored if it appears witthin 
    * double quotes.
    */
    auto read(std::ifstream& file, const std::vector<internal::field>& sch, char sep = '\t', options opts = options()) -> void;
    

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