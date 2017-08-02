#include "micro.h"

using namespace std;

Micro::Micro (string path)
{
        exec_path = path;
}

Micro::~Micro()
{
        if (description)
                delete description;
}


void Micro::get_device_list (void)
{
        device_map = new map <string, string>;

        // attempt to get mapping from configuration file
        if (load_xml_map())
                return;

        DIR *directory;
        struct dirent *entry;

        // prepare path to XML files...
        string xml_dir = ("xmlfiles/");
        string whole_path = this->exec_path + xml_dir;
        string path_to_file;

        gchar *path_xml_files = new char[whole_path.size() + 1];
        // !! Result is not safe, if path uses multi-byte characters (because of copy)
        copy(whole_path.begin(), whole_path.end(), path_xml_files);
        path_xml_files[whole_path.size()] = '\0';

        // check files in directory (candidate description-files) one by one
        if ((directory = opendir ((const gchar *)path_xml_files)) != NULL) {
                while ((entry = readdir(directory)) != NULL) {
                        const string filename = entry->d_name;

                        // check for appropriate filename length
                        if (filename.size() < 4)
                                continue;

                        // look for expected filename-extension
                        if ((filename.compare(filename.size() - 3, 3, "xml") != 0) &&
                            (filename.compare(filename.size() - 3, 3, "XML") != 0))
                                continue;

                        Parser_v1 xml;
                        //Parser_v2 atdf;
                        Parser *parser = &xml;
                        //Parser *parser = &atdf;

                        path_to_file = whole_path + filename;
                        if (Parser::status::success == parser->check_file (path_to_file))
                                cout << "valid description file" << endl;
                                // get device name...
                                //(*device_map)[device_name] = filename;
                        else
                                continue;
                }
                closedir (directory);
        }

        // check if device-to-xml map is still empty
        if (device_map->size() == 0) {
                delete device_map;
                return;
        }

        // save device-to-xml map in configuration file
        save_xml_map();
}


void Micro::save_xml_map (void)
{
        // open file for saving device to xml mapping
        ofstream map_file(this->exec_path + "dev2xml.lst");
        // iterate map and save each couple (device_name -> file_name) in a separate line
        string line;
        map<string, string>::iterator iter;
        for (iter = device_map->begin(); iter != device_map->end(); ++iter) {
                line = iter->first + "::" + iter->second + '\n';
                map_file << line;
                //cout << iter->first << " => " << iter->second << endl;
        }
        // close file
        map_file.close();
}


bool Micro::load_xml_map (void)
{
        // open file
        ifstream map_file(this->exec_path + "dev2xml.lst");
        // comence parsing line by line...
        int counter = 0;
        string line, device, filename;
        while (getline(map_file, line)) {
                // locate separator string
                size_t dev_end = line.find("::");
                // if not present, proceed to the next line
                if (dev_end == string::npos)
                        continue;
                // get device and filename
                device = line.substr(0, dev_end);
                filename = line.substr(dev_end + 2, string::npos);
                // add new entry in device-to-xml map
                (*device_map)[device] = filename;
                counter++;
                //cout << line << endl;
                //cout << device << endl;
                //cout << filename << endl;
        }
        // close file
        map_file.close();
        // check if any devices were found...
        if (counter == 0)
                return false;
        else
                return true;
}


// ******************************************************************************
// Parse data from specified file
// ******************************************************************************

void Micro::parse_data (string xml_file)
{
        // check if specified filename is empty
        if (xml_file == "") {
                cout << "\nXML device description file not specified!" << endl;
                return;
        }

        if (description)
                delete description;

        description = new DeviceDescription;

        // put name of description-file in specifications
        description->xml_filename = xml_file;

        // prepare path to specified description-file
        string xml_dir = ("xmlfiles/");
        string path_to_file = this->exec_path + xml_dir + xml_file;

        Parser_v1 xml;
        //Parser_v2 atdf;

        Parser *parser = &xml;
        //Parser *parser = &atdf;

        parser->process_file (path_to_file, *description);
}
