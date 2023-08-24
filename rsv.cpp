#include "rsv.hpp"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>


auto find_positions(const std::string& line, const std::vector<rsv::internal::field>& fields, char sep)
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

    bool on_quote = false;

    while(beg < end && (*beg != sep || on_quote))
    {
        if(*beg == '\"')
            on_quote = !on_quote;
        beg++;
    }
    
    
    return beg;
}

namespace rsv
{
    /**
    * Why not simply use ifstream.open?
    * : Because I plan to experiment with mapped file, so
    *   I will have to use custom streams.
    */
    auto open(const std::string& name) -> std::tuple<std::ifstream, std::error_condition>
    {
        std::ifstream file(name);
        auto err = std::error_condition();
        if(not file.good())
        {
            err = std::system_category().default_error_condition(errno);
        }
        return {std::move(file), err};
    }

    auto columns(std::ifstream& file, char sep) -> std::vector<std::string>
    {
        file.seekg(0);
        assert(file.good());
        auto line = std::string();
        std::getline(file, line);

        auto column_names = std::vector<std::string>();

        for (const auto& column_name: std::views::split(line, sep))
        {
            column_names.emplace_back(column_name.begin(), column_name.end());
        }
        file.seekg(0);
        return column_names;
    }

    auto schema(std::vector<internal::field> fields) -> std::vector<internal::field>
    {
        // do nothing
        return fields;
    }

    // TODO: probably it is better to receive schema as positional argument
    auto read(std::ifstream& file, const std::vector<internal::field>& sch, char sep) -> void
    {
        file.seekg(0);
        assert(file.good());
        auto line = std::string();
        std::getline(file, line);
        auto pos  = find_positions(line, sch, sep);

        while(not file.eof())
        {
            auto line = std::string();
            std::getline(file, line);

            if(line.empty())
                continue;

            const auto* beg  = line.data();

            for(size_t i = 0; i < pos.size(); i++)
            {
                auto p = pos[i];

                if(p < 0)
                    continue;

                auto end  = next_sep(beg, line.data() + line.size(), sep);
                auto view = std::string_view(beg, end);
                sch[p].del(view, sch[p].data);
                beg = end + 1;
            }
        }
    }
};
