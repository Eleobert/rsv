#include "rsv.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string>
#include <system_error>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>

auto abort_if_needs_header(const std::vector<rsv::internal::field>& fields)
{
    for(size_t j = 0; j < fields.size(); j++)
    {
        if(not fields[j].name.empty())
        {
            std::cerr << "cannot find column '" << fields[j].name << "' because header is disabled";
            std::abort();
        }
    }
}

// FIXME: this is too slow
// if header is false columns are not 'columns', but we still use it
// to know how many fields are there 
auto find_positions(const std::vector<std::string>& column_names,
                    const std::vector<rsv::internal::field>& fields, 
                    bool header)
{
    if(not header)
    {
        abort_if_needs_header(fields);
    }

    auto pos   = std::vector<int64_t>();
    auto found = std::vector<int8_t>(fields.size());

    for (size_t i = 0; i < column_names.size(); i++)
    {
        pos.push_back(-1);
        
        for(size_t j = 0; j < fields.size(); j++)
        {
            if(fields[j].pos == (std::ssize(pos) - 1) ||
              (fields[j].pos < 0 && (std::ssize(column_names) + fields[j].pos == (std::ssize(pos) - 1))) ||
               header && fields[j].name == column_names[i])
            {
                pos.back() = j;
                found[j]   = 1;
                break;
            }
        }
    }

    if(auto it = std::ranges::find(found, 0); it != found.end())
    {
        auto i = it - found.begin();
        // The programmer should not let this happen. Use rsv::columns to
        // inspect columns.
        std::cerr << "rsv: column '" << fields[i].name << "' not present\n";
        std::abort(); 
    }

    return pos;
}

auto next_sep(const char* beg, const char* end, char sep)
{
    if(beg >= end)
    {
        return beg;
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


auto read_row(std::ifstream& file, std::string& row)
{
    auto on_quote = int64_t(0);
    auto buffer   = std::string();

    do
    {
        std::getline(file, buffer);
        
        for(auto c: buffer)
        {
            on_quote += (c == '\"');
        }

        if(not row.empty())
            row += '\n';

        row += buffer;
    }
    while((on_quote % 2) and not file.eof());
}


auto skip_rows(std::ifstream& file, int64_t n)
{
    auto buffer = std::string();

    for(int64_t i = 0; i < n; i++)
    {
        read_row(file, buffer);
    }
}

auto fill_nulls(int64_t i, const rsv::schema& sch, std::vector<int64_t>& pos)
{
    for(; i < std::ssize(pos); i++)
    {
        auto p = pos[i];

        if(p < 0)
            continue;

        sch[p].del(std::string_view(), sch[p].data);
    }
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
        file.clear();
        file.seekg(0);
        //assert(file.good());
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

    auto read(std::ifstream& file, const std::vector<internal::field>& sch, char sep, options opts) -> std::vector<report_record>
    {
        file.seekg(0);
        assert(file.good());
        auto pos    = find_positions(columns(file, sep), sch, opts.header);
        auto line   = std::string();
        auto nread  = int64_t(0);
        auto report = std::vector<report_record>();

        if(not opts.header)
        {
            file.seekg(0);
        }

        skip_rows(file, opts.skip);
        // skip first line
        std::getline(file, line);

        while(not file.eof() and nread < opts.nrows)
        {
            auto line  = std::string();
            read_row(file, line);
            auto count = int64_t(0);

            if(line.empty())
                continue;

            const auto* beg  = line.data();

            for(size_t i = 0; i < pos.size(); i++)
            {
                auto p = pos[i];

                if(p < 0)
                    continue;

                const auto end  = next_sep(beg, line.data() + line.size(), sep);
                // if we reach the end of line but still have fields to process
                if(count != std::ssize(sch) and end == line.data() + line.size())
                {
                    // fill the rest of the fields with nulls
                    fill_nulls(i, sch, pos);
                }

                if(not sch[p].del({beg, end}, sch[p].data))
                {
                    report.push_back({.index = nread, .pos = p});
                }

                beg = end + 1;
                count++;
            }
            nread++;
        }
        return report;
    }
};
