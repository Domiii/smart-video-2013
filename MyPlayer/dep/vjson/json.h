// Copied from: https://code.google.com/p/vjson/
// Light-weight C JSon library. Not a good choice, but lightweight nevertheless.
// Modified by Domi.

#ifndef JSON_H
#define JSON_H


#include <unordered_map>
#include <string>

enum json_type
{
        JSON_NULL,
        JSON_OBJECT,
        JSON_ARRAY,
        JSON_STRING,
        JSON_INT,
        JSON_FLOAT,
        JSON_BOOL,
};

// TODO: Proper destruction
struct json_value
{
        json_value *parent;
        json_value *next_sibling;
        json_value *first_child;
        json_value *last_child;
        std::unordered_map<std::string, json_value*> children;

        std::string name;

        union
        {
                char* string_value;
                int int_value;
                float float_value;
        };

        json_type type;



        json_value() :
            parent(nullptr),
            next_sibling(nullptr),
            first_child(nullptr),
            last_child(nullptr),
            string_value(0)
        {
        }

        std::string GetStringValue() const
        {
            if (string_value == 0) return "";
            return string_value;
        }
};

json_value *json_parse(const char *fname, char **error_pos, const char **error_desc, int *error_line);

#endif