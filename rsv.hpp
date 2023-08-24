#include <charconv>
#include <string>
#include <type_traits>
#include <vector>
#include <iostream>

namespace rsv
{
    namespace internal
    {
        template<typename T>
        auto to_num(const std::string_view& str)
        {
            T value = 0;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

            if(ec != std::errc())
            {
                std::cerr << "Error converting value '" << str << "'\n";
                std::exit(1);
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

    auto schema(std::vector<internal::field> fields) -> std::vector<internal::field>;

    // TODO: probably it is better to receive schema as positional argument
    auto read(const std::string& filename, const std::vector<internal::field>& sch, char sep = '\t') -> void;
    

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