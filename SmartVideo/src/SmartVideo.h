#ifndef SMARTVIDEOUTIL_H
#define SMARTVIDEOUTIL_H

#include "util.h"
#include "JSonUtil.h"

namespace SmartVideo
{
    struct ClipEntry
    {
        std::string Name;
        int StartFrame;
        std::string BaseFolder;
        std::string ClipFile;
    };

    struct SmartVideoConfig
    {
        std::string CfgFolder;
        std::string CfgFile;

        std::string DataFolder;
        std::string ClipListFile;
        
        std::vector<ClipEntry> ClipEntries;
        

        void OnEntryNotFound(std::string entryName, std::string propName)
        {
            std::cerr << "ERROR: Invalid config. Property \"" << propName  << "\" missing in entry: " << entryName;
            exit(-1);           // TODO: This is a bit too drastic
        }

        json_value* GetProperty(json_value* entry, std::string propName)
        {
            if (entry->children.count(propName) == 0)
            {
                OnEntryNotFound(entry->name, propName);
                return nullptr;
            }
            return entry->children.find(propName)->second;
        }

        /// Read all config files
        bool InitializeConfig()
        {
            std::string cfgPath(CfgFolder + "/" + CfgFile);
            json_value * cfgRoot = JSonReadFile(cfgPath);
            if (!cfgRoot) return false;

            DataFolder = GetProperty(cfgRoot, "dataDir")->string_value;
            ClipListFile = GetProperty(cfgRoot, "clipFile")->string_value;

            std::string clipListPath(CfgFolder + "/" + ClipListFile);
            json_value * clipRoot = JSonReadFile(clipListPath);
            if (!clipRoot) return false;

            ClipEntries.resize(clipRoot->children.size());

            int i = 0;
            for (auto&  x : clipRoot->children)
            {
                ClipEntry& entry = ClipEntries[i++];
                json_value* entryNode = x.second;
                entry.Name = x.second->name;
                entry.BaseFolder = GetProperty(entryNode, "baseDir")->string_value;
                entry.ClipFile = GetProperty(entryNode, "frameFile")->string_value;
                entry.StartFrame = GetProperty(entryNode, "startFrame")->int_value;
            }

            return true;
        }
    };
}

#endif // SMARTVIDEOUTIL_H