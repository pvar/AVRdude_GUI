#include "devdesc.h"

using namespace std;

DeviceDescription::DeviceDescription ()
{
        this->warnings = new list<FuseWarning>;
        this->fuse_settings = new list<FuseSetting>;
        this->option_lists = new map <string, list<OptionEntry>*>;

        // apply default default-values
        this->fusebytes_default[0] = 255;
        this->fusebytes_default[1] = 255;
        this->fusebytes_default[2] = 255;
}

DeviceDescription::~DeviceDescription ()
{
        // delete warnings list
        delete this->warnings;
        // iterate through option_lists map and delete entries
        map<string, list<DeviceDescription::OptionEntry>*>::iterator iter = this->option_lists->begin();
        for (; iter!=this->option_lists->end(); ++iter)
                delete iter->second;
        // delete the whole option_lists map
        delete this->option_lists;
        // delete settings list
        delete this->fuse_settings;
}
