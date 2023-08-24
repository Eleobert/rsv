#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>
#include <charconv>

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
        void delegate(const std::string_view& src, void* dst) = delete;

        template<>
        void delegate<std::string>(const std::string_view& src, void* dst)
        {
            auto* casted = static_cast<std::vector<std::string>*>(dst);;
            casted->emplace_back(src.begin(), src.end());
        }

        template<>
        void delegate<float>(const std::string_view& src, void* dst)
        {
            auto* casted = static_cast<std::vector<float>*>(dst);;
            casted->emplace_back(to_num<float>(src));
            //std::cout << src << ' ';
        }

        template<>
        void delegate<int32_t>(const std::string_view& src, void* dst)
        {
            auto* casted = static_cast<std::vector<int32_t>*>(dst);;
            casted->emplace_back(to_num<int32_t>(src));
        }

        struct field
        {
            std::string name;
            int64_t pos = -1;
            void* data = nullptr;
            decltype(delegate<std::string>)* del = nullptr;
        };

        // TODO: file should be open outside, so that the user can better manage opening errors
        auto open(const std::string& name)
        {
            std::ifstream file(name);

            if(not file.good())
            {
                std::perror(name.c_str());
            }
            return file;
        }

        auto find_positions(const std::string& line, const std::vector<field>& fields, char sep)
        {
            auto pos = std::vector<int64_t>();

            for (const auto& column_name: std::views::split(line, sep))
            {
                const auto cname = std::string_view(column_name.begin(), column_name.end());
                pos.push_back(-1);
                
                for(size_t i = 0; i < fields.size(); i++)
                {
                    if(fields[i].pos == (std::ssize(pos) - 1) || fields[i].name == cname)
                    {
                        pos.back() = i;
                        break;
                    }
                }
            }
            return pos;
        }

        auto next_sep(const char* beg, const char* end, char sep)
        {
            if(beg >= end)
            {
                std::cerr << "missing value!\n";
                std::exit(1);
            }

            while(*beg != sep && beg < end)
                beg++;
            
            
            return beg;
        }
    };

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

    auto schema(std::vector<internal::field> fields)
    {
        // do nothing
        return fields;
    }

    // TODO: probably it is better to receive schema as positional argument
    auto read(const std::string& filename, const std::vector<internal::field>& sch, char sep = '\t')
    {
        auto file = internal::open(filename);
        auto line = std::string();
        std::getline(file, line);
        auto pos  = find_positions(line, sch, sep);

        while(not file.eof())
        {
            auto line = std::string();
            std::getline(file, line);
            const auto* beg  = line.data();

            for(size_t i = 0; i < sch.size(); i++)
            {
                auto p    = pos[i];

                if(p < 0)
                    continue;

                auto end  = internal::next_sep(beg, line.data() + line.size(), sep);
                auto view = std::string_view(beg, end);
                sch[p].del(view, sch[p].data);
                beg = end + 1;
            }
        }
    }
};


int main()
{
    auto sequence  = std::vector<std::string>();
    auto charge    = std::vector<int>();
    auto ascesions = std::vector<std::string>();
    auto score     = std::vector<float>();

    auto schema = rsv::schema({
        rsv::f(1 , charge),
        rsv::f(2 , ascesions),
        rsv::f(0 , sequence),
        rsv::f(3 , score)}
    );

    // auto schema = rsv::schema({
    //     rsv::f(1 , charge),
    //     rsv::f(2 , ascesions),
    //     rsv::f(0 , sequence),
    //     rsv::f(3 , score)}
    // );

    rsv::read("tst.tsv", schema, '\t');

    for(size_t i = 0; i < sequence.size(); i++)
    {
        std::cout << sequence[i] << '\t' 
                  << charge[i] << '\t'
                  << ascesions[i] << '\t'
                  << score[i] << '\n';
    }
}