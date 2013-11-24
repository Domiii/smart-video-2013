#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <vjson/json.h>
#include <iostream>
#include "util.h"

#define INDENT(n) for (int i = 0; i < n; ++i) std::cout << "    "


void JSonPrint(json_value *value, int indent = 0);

inline json_value * JSonReadFile(std::string filepath)
{
    char *errorPos = 0;
    const char *errorDesc = 0;
    int errorLine = 0;


    json_value *root = json_parse(filepath.c_str(), &errorPos, &errorDesc, &errorLine);
    if (root)
    {
        return root;
    }

    printf("ERROR in JSon file %s:%d - %s\n%s\n\n", filepath.c_str(), errorLine, errorDesc, errorPos);
    return nullptr;
}

inline void JSonPrint(json_value *value, int indent)
{
        INDENT(indent);
        if (value->name.size() > 0) std::cout << ("\"%s\" = ", value->name);
        switch(value->type)
        {
        case JSON_NULL:
                std::cout << ("null\n");
                break;
        case JSON_OBJECT:
        case JSON_ARRAY:
                std::cout << (value->type == JSON_OBJECT ? "{\n" : "[\n");
                for (json_value *it = value->first_child; it; it = it->next_sibling)
                {
                        JSonPrint(it, indent + 1);
                }
                INDENT(indent);
                std::cout << (value->type == JSON_OBJECT ? "}\n" : "]\n");
                break;
        case JSON_STRING:
                std::cout << ("\"%s\"\n", value->string_value);
                break;
        case JSON_INT:
                std::cout << ("%d\n", value->int_value);
                break;
        case JSON_FLOAT:
                std::cout << ("%f\n", value->float_value);
                break;
        case JSON_BOOL:
                std::cout << (value->int_value ? "true\n" : "false\n");
                break;
        }
}

#endif // JSONUTIL_H