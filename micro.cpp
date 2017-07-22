#include "micro.h"

using namespace std;

DeviceFuseSettings::DeviceFuseSettings()
{
        this->fuse_settings = new list<FuseSetting>;
        this->option_lists = new map <string, list<OptionEntry>*>;
}

DeviceFuseSettings::~DeviceFuseSettings()
{
        // iterate through map and delete embedded lists
        map<string, list<OptionEntry>*>::iterator iter = option_lists->begin();
        for (; iter!=option_lists->end(); ++iter)
                delete iter->second;

        // delete the whole map and the extra list
        delete option_lists;
        delete fuse_settings;
}

Micro::Micro (string path)
{
        exec_path = path;
}

Micro::~Micro()
{
        if (specifications)
                delete specifications;
        if (warnings)
                delete warnings;
        if (settings)
                delete settings;
}

// ******************************************************************************
// Create a map with device names and the corresponding files
// ******************************************************************************

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
        gchar *path_xml_files = new char[whole_path.size() + 1];
        // !! Result is not safe, if path uses multi-byte characters (because of copy)
        copy(whole_path.begin(), whole_path.end(), path_xml_files);
        path_xml_files[whole_path.size()] = '\0';

        // parse xml files one by one
        if ((directory = opendir ((const gchar *)path_xml_files)) != NULL) {
                // check every XML file in given directory
                while ((entry = readdir(directory)) != NULL) {
                        const string filename = entry->d_name;
                        // check for appropriate filename-length
                        if (filename.size() < 4)
                                continue;
                        // look for expected filename-extension
                        if ((filename.compare(filename.size() - 3, 3, "xml") != 0) &&
                            (filename.compare(filename.size() - 3, 3, "XML") != 0))
                                continue;
                        // process specific XML file
                        try {
                                xmlpp::DomParser parser;
                                parser.parse_file((const gchar *)path_xml_files + filename);
                                if (parser) {
                                        xmlpp::Node *xml_node = parser.get_document()->get_root_node();
                                        const string node_name = xml_node->get_name();
                                        // check if valid part-description file
                                        if (node_name.compare("AVRPART") != 0)
                                                continue;
                                        // go to ADMIN node
                                        xml_node = xml_node->get_first_child("ADMIN");
                                        // go to PART_NAME node
                                        xml_node = xml_node->get_first_child("PART_NAME");
                                        // get node content
                                        string device_name = this->get_txt_value(xml_node);
                                        // insert new entry in device map
                                        (*device_map)[device_name] = filename;

                                }
                        } catch(const exception& ex) {
                                cerr << "Exception caught: " << ex.what() << endl;
                                continue;
                        }
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

gboolean Micro::load_xml_map (void)
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
        // save device description filename
        this->device_xml = xml_file;
        // prepare data structures
        if (specifications)
                delete specifications;
        if (warnings)
                delete warnings;
        if (settings)
                delete settings;
        specifications = new DeviceSpecifications;
        settings = new DeviceFuseSettings;
        warnings = new list<FuseWarning>;

        // prepare path to specified XML file...
        string xml_dir = ("xmlfiles/");
        string path_to_file = this->exec_path + xml_dir + device_xml;

        // process specific XML file
        try {
                xmlpp::DomParser parser;
                parser.parse_file(path_to_file);
                if (parser) {
                        // go to root node
                        xmlpp::Node *root_node = parser.get_document()->get_root_node();
                        // check if a valid description file
                        const string node_name = root_node->get_name();
                        if (node_name.compare("AVRPART") != 0) {
                                cout << "Not a valid description file!" << endl;
                                return;
                        }

                        // --- should check return status! ----------------
                        parse_specifications(root_node);
                        parse_settings (root_node);
                        parse_warnings (root_node);
                        parse_default (root_node);
                        // ------------------------------------------------
                }
        } catch(const exception& ex) {
                cout << "Exception caught, trying to parse device description file: " << ex.what() << endl;
                return;
        }

        // copy default fuse values over user fuse values
        for (int i = 0; i < 3; i++)
                usr_fusebytes[i] = def_fusebytes[i];


        return;
}

// ============================= PROTECTED METHODS =============================



// ******************************************************************************
//   Parse fuse settings
// ******************************************************************************

int Micro::parse_settings (xmlpp::Node *root_node)
{
        //cout << "PARSING FUSES..." << endl;

        settings = new DeviceFuseSettings;

        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;

        // go to V2 node
        xml_node = xml_node->get_first_child("V2");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;
        // go to templates node
        xml_node = xml_node->get_first_child("templates");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;
        // go to node with attribute: class = "FUSE"
        tmp_node = this->get_child_with_attr(xml_node,  "class", "FUSE");

        // if no such node, check if it's a device with NVM...
        if (!tmp_node) {
                // go to node with attribute: class = "NVM"
                tmp_node = this->get_child_with_attr(xml_node,  "class", "NVM");
                // if no such node, it must be a device without fuses...
                if (!tmp_node)
                        return 0;
                // go to node with attribute: name = "NVM_FUSES"
                tmp_node = this->get_child_with_attr(tmp_node,  "name", "NVM_FUSES");
                // if no such node, there must be a problem...
                if (!tmp_node)
                        return -1;
                // use default node pointer
                xml_node = tmp_node;
        } else {
                // go to registers node
                tmp_node = tmp_node->get_first_child("registers");
                // use default node pointer
                xml_node = tmp_node;
        }

        int fuse_byte_counter = 0;
        // loop through children (reg nodes)
        xmlpp::Node::NodeList children = xml_node->get_children();
        xmlpp::Node::NodeList::iterator node_iterator;
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // get offset value for this reg node
                string regoffset = this->get_att_value((*node_iterator), "offset");
                // if no offset attribute was found, proceed to next iteration
                if (regoffset.size() < 1)
                        continue;
                // increase fuse-byte counter
                fuse_byte_counter++;
                uint offset = ::atoi((regoffset.substr(2, 2)).c_str());
                // get name for this reg node
                string regname = this->get_att_value((*node_iterator), "name");
                // prepare pseudo-setting with fuse-byte name
                FuseSetting* entry = new FuseSetting;
                entry->single_option = true;
                entry->fdesc = "FUSE REGISTER:  " + regname;
                entry->offset = 512; // special value that denotes a pseudo-entry
                // prepare list of settings from this node
                list<FuseSetting> *fuse_listing = new list<FuseSetting>;
                fuse_listing = this->get_fuse_list ((*node_iterator), offset);
                // add pseudo-setting in this list of settings
                fuse_listing->push_front(*entry);
                // add this settings list to the rest
                list<FuseSetting>::iterator pos = (settings->fuse_settings)->begin();
                (settings->fuse_settings)->splice(pos, *fuse_listing);
                delete entry;
        }
        //cout << "FUSE BYTES: " << fuse_byte_counter << endl;
        settings->fusebytes_count = fuse_byte_counter;

        // debug print-out...
        //print_fuse_settings();

        // go up to parent node
        xml_node = xml_node->get_parent();
        // loop through children (registers and enumerator nodes)
        children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // skip irrelevant nodes (namely registers)
                if (((*node_iterator)->get_name()).compare("enumerator") != 0)
                        continue;
                // prepare list with this enumerator's entries
                list<OptionEntry> *enum_listing = new list<OptionEntry>;
                enum_listing = this->get_enum_list ((*node_iterator));
                // get enumerator name
                string enum_name = this->get_att_value ((*node_iterator), "name");
                // add new entry in map
                (*settings->option_lists)[enum_name] = enum_listing;
        }

        // debug print-out...
        //print_option_lists();

        return 1;
}

// ******************************************************************************
// Parse fuse warnings
// ******************************************************************************

int Micro::parse_warnings (xmlpp::Node *root_node)
{
        //cout << "PARSING WARNINGS..." << endl;

        // Fuse Warnings in XML file...
        // <FuseWarning>[fuse-byte-no],[AND-mask],[result],[warning text]</FuseWarning>

        string raw_warning;
        FuseWarning *warning_entry;
        warnings = new list<FuseWarning>;

        xmlpp::Node* xml_node = root_node;

        // go to PROGRAMMING node
        xml_node = xml_node->get_first_child("PROGRAMMING");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;

        // go to ISPInterface node
        xml_node = xml_node->get_first_child("ISPInterface");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;

        // loop through children and get warnings
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                // get name of current sibling...
                const string node_name = (*node_iterator)->get_name();
                // if not a FuseWarning, proceed to next sibling...
                if (node_name.compare("FuseWarning") != 0)
                        continue;
                // get string (raw) representation of warning...
                raw_warning = get_txt_value ((*node_iterator));
                // assemble fuse warning structure
                warning_entry = new FuseWarning;
                stringstream(raw_warning.substr(0,1)) >> hex >> warning_entry->fbyte;
                stringstream(raw_warning.substr(2,4)) >> hex >> warning_entry->fmask;
                stringstream(raw_warning.substr(7,4)) >> hex >> warning_entry->fresult;
                warning_entry->warning = raw_warning.substr(21,1000);
                // inset warning in list
                warnings->push_back(*warning_entry);
                // delete temporary wanring entry
                delete warning_entry;
        }

        // debug print-out...
        //print_fuse_warnings();

        return 1;
}

// ******************************************************************************
// Parse default fuse settings
// ******************************************************************************

int Micro::parse_default (xmlpp::Node *root_node)
{
        // apply default-default values
        def_fusebytes[0] = 255;
        def_fusebytes[1] = 255;
        def_fusebytes[2] = 255;

        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = root_node;

        // go to FUSE node
        xml_node = xml_node->get_first_child("FUSE");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;

        // go to LIST node
        xml_node = xml_node->get_first_child("LIST");
        // if no such node, it must be a device without fuses...
        if (!xml_node)
                return 0;

        // get list of fuse-byte names
        string name_list = get_txt_value (xml_node);
        name_list = name_list.substr(1, name_list.length() - 2);

        // extract individual byte names
        int byte_count = 0;
        string byte_names[3];
        while (true) {
                // locate separator string
                size_t name_end = name_list.find(":");
                // extract name
                byte_names[byte_count] = name_list.substr(0, name_end);
                // check if it was the last name in the list
                if (name_end == string::npos)
                        break;
                // delete name and separator from list
                name_list.erase(0, name_end + 1);

                byte_count++;
        }

        xmlpp::Node::NodeList children;
        xmlpp::Node::NodeList::iterator node_iterator;
        string node_name, bit_val;
        int bit_order;

        // get value for each fuse-byte
        for (int i = 0; i < (byte_count + 1); i++) {
                // go up to FUSE node
                // (fuse-byte description nodes are children of FUSE node)
                xml_node = xml_node->get_parent();
                // go to <FUSE-BYTE-NAME> node
                tmp_node = xml_node->get_first_child(byte_names[i]);
                // if no such node, proceed to next fuse-byte...
                if (!tmp_node)
                        continue;
                else
                        xml_node = tmp_node;
                // loop through all children with name "FUSEx", where x in [0..7]
                children = xml_node->get_children();
                for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                        // get name of current sibling...
                        node_name = (*node_iterator)->get_name();
                        // if does not contain "FUSE", proceed to next sibling...
                        if (node_name.find("FUSE") == string::npos)
                                continue;
                        // if more than 5 letters in name, proceed to next sibling...
                        if (node_name.length() > 5)
                                continue;
                        // erase constant part (FUSE)
                        node_name.erase(0,4);
                        // transform to integer
                        bit_order = atoi(node_name.c_str());
                        // get DEFAULT node
                        tmp_node = (*node_iterator)->get_first_child("DEFAULT");
                        // if not found, no default values mentioned in XML!
                        if (tmp_node == nullptr)
                                return 0;
                        bit_val = get_txt_value (tmp_node);
                        // apply bit value on fuse-byte
                        if (bit_val.find("0") == string::npos)
                                def_fusebytes[i] |= 1 << bit_order;
                        else
                                def_fusebytes[i] &= ~(1 << bit_order);
                }
        }

        //print_fuse_defaults();

        return 1;
}

// ******************************************************************************
// Parse specifications
// ******************************************************************************

int Micro::parse_specifications (xmlpp::Node *root_node)
{
        //cout << "PARSING SPECIFICATIONS..." << endl;

        DeviceSpecifications* specs = new DeviceSpecifications;
        xmlpp::Node* xml_node = root_node;
        xmlpp::Node* tmp_node = nullptr;
        string txtvalue;
        gfloat numvalue;

        // put xml filename in specifications
        specs->xml_filename = this->device_xml;

        // go to ADMIN node
        xml_node = xml_node->get_first_child("ADMIN");
        // go to SPEED node
        xml_node = xml_node->get_first_child("SPEED");
        // get node content
        txtvalue = this->get_txt_value(xml_node);
        txtvalue.resize(txtvalue.size() - 3);
        specs->max_speed = txtvalue + " MHz";
        //cout << "speed: " << specs->max_speed << endl;
        // go up to ADMIN node
        xml_node = xml_node->get_parent();
        // go to BUILD node
        xml_node = xml_node->get_first_child("BUILD");
        // get node content
        txtvalue = this->get_txt_value(xml_node);
        specs->xml_version = txtvalue;
        //cout << "build: " << specs->xml_version << endl;
        // go up to ADMIN node
        xml_node = xml_node->get_parent();
        // go to SIGNATURE node
        xml_node = xml_node->get_first_child("SIGNATURE");
        // get node content
        specs->signature = this->get_signature_bytes(xml_node);
        //cout << "signature: \"" << specs->signature << "\"" << endl;
        // go up to AVRPART node
        xml_node = xml_node->get_parent();
        xml_node = xml_node->get_parent();
        // go to MEMORY node
        xml_node = xml_node->get_first_child("MEMORY");
        // go to PROG_FLASH node
        xml_node = xml_node->get_first_child("PROG_FLASH");
        // get node content
        txtvalue = this->get_txt_value(xml_node);
        numvalue = ::atof(txtvalue.c_str()) / 1024;
        txtvalue = float_to_string(numvalue);
        specs->flash_size = txtvalue + " KB";
        //cout << "flash: " << specs->flash_size << endl;
        // go up to MEMORY node
        xml_node = xml_node->get_parent();
        // go to EEPROM node
        tmp_node = xml_node->get_first_child("EEPROM");
        // if no such node, it must be a device without EEPROM
        if (!tmp_node) {
                specs->eeprom_exists = false;
                txtvalue = "0 Bytes";
                specs->sram_size = txtvalue;
        } else {
                specs->eeprom_exists = true;
                xml_node = tmp_node;
                // get node content
                txtvalue = this->get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                // check for zero eeprom size
                if (numvalue == 0)
                        specs->eeprom_exists = false;
                // express size in KBytes
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                specs->eeprom_size = txtvalue;
                //cout << "eeprom: " << specs->eeprom_size << endl;
                // go up to MEMORY node
                xml_node = xml_node->get_parent();
        }
        // go to INT_SRAM node
        tmp_node = xml_node->get_first_child("INT_SRAM");
        // if no such node, it must be a device without SRAM!
        if (!tmp_node) {
                txtvalue = "0 Bytes";
                specs->sram_size = txtvalue;
        } else {
                xml_node = tmp_node;
                // go to SIZE node
                xml_node = xml_node->get_first_child("SIZE");
                // get node content
                txtvalue = this->get_txt_value(xml_node);
                numvalue = ::atof(txtvalue.c_str()) / 1024;
                if (numvalue > 1)
                        txtvalue = float_to_string(numvalue) + " KB";
                else
                        txtvalue += " Bytes";
                specs->sram_size = txtvalue;
                //cout << "sram_size: " << specs->eeprom_size << endl;
        }

        this->specifications = specs;
        return 1;
}

// ******************************************************************************
// Go to the specified node (by attribute)
// ******************************************************************************

xmlpp::Node* Micro::get_child_with_attr (xmlpp::Node* starting_node, string att_name, string att_value)
{
        xmlpp::Node *target = nullptr;
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = starting_node->get_children();

        // loop through child-nodes
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                // break inner loop if target has been pinpointed
                                if ((*att_iterator)->get_name().compare(att_name) == 0)
                                        if ((*att_iterator)->get_value().compare(att_value) == 0) {
                                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                                target = *node_iterator;
                                                break;
                                        }
                        }
                        // break outer loop if target has been defined
                        if (target != nullptr)
                                break;
                }
        }
        return target;
}

// ******************************************************************************
// Get text value in specified node
// ******************************************************************************

string Micro::get_txt_value (xmlpp::Node* starting_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = starting_node->get_children();

        // loop through child-nodes
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (text)
                        break;
                else
                        continue;
        }
        return text->get_content();
}

// ******************************************************************************
// Get the value of specified attribute
// ******************************************************************************

string Micro::get_att_value (xmlpp::Node* xml_node, string att_name)
{
        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        string data_string;

        if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(xml_node)) {
                // loop through attribute-nodes
                const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                        // break inner loop if target has been pinpointed
                        if ((*att_iterator)->get_name().compare(att_name) == 0) {
                                //cout << (*att_iterator)->get_name() << " = " << (*att_iterator)->get_value() << endl;
                                data_string = (*att_iterator)->get_value();
                                break;
                        }
                }
        }
        return data_string;
}

// ******************************************************************************
// Get list of enumerator members
// ******************************************************************************

list<OptionEntry>* Micro::get_enum_list (xmlpp::Node* xml_node)
{
        list<OptionEntry> *enum_listing = new list<OptionEntry>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;
        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;
        uint max_value = 0;

        // loop through child-nodes (enum nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        OptionEntry* entry = new OptionEntry;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->ename = (*att_iterator)->get_value();
                                        //cout << entry->ename << endl;
                                } else if ((*att_iterator)->get_name().compare("val") == 0) {
                                        string strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->value = stol (strvalue, nullptr, 16);
                                        if (max_value < entry->value)
                                                max_value = entry->value;
                                        //cout << entry->value << endl;
                                }
                        }
                        enum_listing->push_back(*entry);
                        delete entry;
                }
        }
        // insert pseudo enumerator-entry with maximum value
        OptionEntry* entry = new OptionEntry;
        entry->ename = "PSEUDO_ENTRY";
        entry->value = max_value;
        enum_listing->push_front(*entry);
        delete entry;
        return enum_listing;
}

// ******************************************************************************
// Get list of settings
// ******************************************************************************

list<FuseSetting>* Micro::get_fuse_list (xmlpp::Node* xml_node, uint offset)
{
        list<FuseSetting> *fuse_listing = new list<FuseSetting>;

        xmlpp::Node::NodeList::iterator node_iterator;
        xmlpp::Element::AttributeList::const_iterator att_iterator;

        xmlpp::Node::NodeList children = xml_node->get_children();
        string tmp_string;

        // loop through child-nodes (bitfield nodes)
        for (node_iterator = children.begin(); node_iterator != children.end(); ++node_iterator) {
                if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(*node_iterator)) {
                        // loop through attribute-nodes
                        FuseSetting* entry = new FuseSetting;
                        entry->fenum = ""; // must initialize to empty
                        entry->offset = offset;
                        const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();
                        for(att_iterator = attributes.begin(); att_iterator != attributes.end(); ++att_iterator) {
                                if ((*att_iterator)->get_name().compare("name") == 0) {
                                        entry->fname = (*att_iterator)->get_value();
                                        //cout << entry->fname << endl;
                                } else if ((*att_iterator)->get_name().compare("text") == 0) {
                                        entry->fdesc = (*att_iterator)->get_value();
                                        //cout << entry->fdesc << endl;
                                } else if ((*att_iterator)->get_name().compare("enum") == 0) {
                                        entry->fenum = (*att_iterator)->get_value();
                                        //cout << entry->fenum << endl;
                                } else if ((*att_iterator)->get_name().compare("mask") == 0) {
                                        string strvalue = ((*att_iterator)->get_value()).substr(2, 2);
                                        entry->fmask = stol (strvalue, nullptr, 16);
                                        //cout << entry->fmask << endl;
                                }
                        }
                        if ((entry->fenum).size() == 0)
                                entry->single_option = true;
                        else
                                entry->single_option = false;
                        fuse_listing->push_back(*entry);
                        delete entry;
                }
        }
        return fuse_listing;
}

// ******************************************************************************
// Get the device signature
// ******************************************************************************

string Micro::get_signature_bytes (xmlpp::Node* signature_node)
{
        const xmlpp::TextNode *text = NULL;
        xmlpp::Node::NodeList::iterator iter;
        xmlpp::Node::NodeList children = signature_node->get_children();
        string signatrure;

        signatrure = "0x";
        for (iter = children.begin(); iter != children.end(); ++iter) {
                text = dynamic_cast<const xmlpp::TextNode*>(*iter);
                if (!text)
                        signatrure += (get_txt_value((*iter))).substr(1, 2);
        }
        return signatrure;
}

// ******************************************************************************
// Create the string representation of a float
// ******************************************************************************

string Micro::float_to_string (gfloat number)
{
        string value(16, '\0');
        int chars_written = snprintf(&value[0], value.size(), "%.1f", number);
        value.resize(chars_written);

        return (string)value;
}

// ******************************************************************************
// List printing for debug purposes
// ******************************************************************************

void Micro::print_fuse_warnings (void)
{
        list<FuseWarning>::iterator iter;
        for (iter = warnings->begin(); iter != warnings->end(); iter++) {
                cout << endl;
                cout << "    byte: " << iter->fbyte << endl;
                cout << "    mask: " << iter->fmask << endl;
                cout << "   value: " << iter->fresult << endl;
                cout << "    text: " << iter->warning << endl;
        }
}

void Micro::print_fuse_defaults (void)
{
        for (int i = 0; i < 3; i++)
                cout << "fuse byte " << i + 1 << " : " << hex << def_fusebytes[i] << endl;
}

void Micro::print_fuse_settings (void)
{
        list<FuseSetting>::iterator iter;
        for (iter = (settings->fuse_settings)->begin(); iter != (settings->fuse_settings)->end(); ++iter) {
                cout << endl;
                cout << (*iter).fname << " ";
                cout << "mask[" << (*iter).fmask << "] ";
                cout << "offset[" << (*iter).offset << "] ";
                cout << "enum[" << (*iter).fenum << "] ";
                cout << (*iter).fdesc << endl;
        }
}

void Micro::print_option_lists (void)
{
        map<string, list<OptionEntry>*>::iterator iter;
        for (iter = (settings->option_lists)->begin(); iter != (settings->option_lists)->end(); ++iter) {
                cout << iter->first << " => " << endl;
                list<OptionEntry>::iterator iter2;
                for (iter2 = (*iter->second).begin(); iter2 != (*iter->second).end(); ++iter2)
                                cout << (*iter2).ename << endl;
        }
}
