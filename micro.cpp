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
        // initialize map (device to xmlfile)
        device_map = new map <string, string>;

        // attempt to get mapping from file
        if (load_xml_map())
                return;

        // create C compatible string with path to XML Files
        const string subdir = ("devices/");
        const string path = this->exec_path + subdir;
        const char *cpath = path.c_str();

        // attempt to get directory listing
        DIR *directory;
        struct dirent *entry;
        if ((directory = opendir (cpath)) != NULL) {

                // get candidate description files one by one...
                Parser xml;
                string file_uri, device, file;
                while ((entry = readdir(directory)) != NULL) {

                        // get a filename
                        file = entry->d_name;

                        // check for appropriate length
                        if (file.size() < 4)
                                continue;

                        // look for expected extension
                        if ((file.compare(file.size() - 3, 3, "xml") != 0) &&
                            (file.compare(file.size() - 3, 3, "XML") != 0))
                                continue;

                        cout << "Checking: " << file;
                        file_uri = path + file;
                        if (xml.is_description (file_uri, device)) {
                                cout << ": It is!" << endl;
                                (*device_map)[device] = file;
                        } else {
                                cout << ": NOT!" << endl;
                                continue;
                        }
                }
                closedir (directory);
        }

        // check if mapping still empty
        // there will be a new attempt to populate it at next run
        if (device_map->size() == 0) {
                delete device_map;
                return;
        }

        // save mapping to file
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
        string xml_dir = ("devices/");
        string path_to_file = this->exec_path + xml_dir + xml_file;

        Parser xml;
        xml.process_file (path_to_file, *description);
}
