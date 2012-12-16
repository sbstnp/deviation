/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

static struct model_page * const mp = &pagemem.u.model_page;

static void _show_buttons(int loadsave);
static void _show_list(int loadsave, u8 num_models);
static int ini_handle_icon(void* user, const char* section, const char* name, const char* value)
{
    (void)user;
    if(section[0] == '\0' && strcasecmp(name, MODEL_ICON) == 0) {
        CONFIG_ParseIconName(mp->iconstr, value);
    }
    if(section[0] == '\0' && strcasecmp(name, MODEL_TYPE) == 0) {
        mp->modeltype = CONFIG_ParseModelType(value);
    }
    return 1;
}

static int ini_handle_name(void* user, const char* section, const char* name, const char* value)
{
    long idx = (long)user;
    if(section[0] == '\0' && (strcasecmp(name, MODEL_NAME) == 0 || strcasecmp(name, MODEL_TEMPLATE) == 0)) {
        sprintf(mp->tmpstr, "%d: %s", abs(idx), idx < 0 ? _tr(value) : value);
        return -1;
    }
    return 1;
}

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    const char *ico;
    mp->selected = sel + 1;
    if(! mp->obj)
        return;
    if ((long)data == LOAD_ICON) {
        ico = CONFIG_GetIcon(mp->modeltype);
        if (sel > 0 && FS_OpenDir("modelico")) {
            char filename[13];
            int count = 0;
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".bmp", 4) == 0) {
                    count++;
                    if (sel == count) {
                        CONFIG_ParseIconName(mp->iconstr, filename);
                        ico = mp->iconstr;
                        break;
                    }
                }
            }
            FS_CloseDir();
        }
    } else {
        sprintf(mp->tmpstr, "models/model%d.ini", mp->selected);
        mp->modeltype = 0;
        mp->iconstr[0] = 0;
        ini_parse(mp->tmpstr, ini_handle_icon, NULL);
        if (mp->iconstr[0])
            ico = mp->iconstr;
        else
            ico = CONFIG_GetIcon(mp->modeltype);
    }
    GUI_ChangeImage(mp->obj, ico, 0, 0);
}
static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    FILE *fh;
    if ((long)data == LOAD_TEMPLATE) { //Template
        char filename[13];
        int type;
        int count = 0;
        if (! FS_OpenDir("template"))
            return _tr("Unknown");
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".ini", 4) == 0) {
                count++;
                if (idx + 1 == count) {
                    sprintf(mp->tmpstr, "template/%s", filename);
                    break;
                }
            }
        }
        FS_CloseDir();
    } else if ((long)data == LOAD_ICON) { //Icon
        char filename[13];
        int type;
        int count = 1;
        if (idx == 0)
            return _tr("Default");
        if (! FS_OpenDir("modelico"))
            return _tr("Unknown");
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".bmp", 4) == 0) {
                count++;
                if (idx + 1 == count) {
                    strncpy(mp->tmpstr, filename, sizeof(filename));
                    return mp->tmpstr;
                }
            }
        }
        FS_CloseDir();
        return _tr("Unknown");
    } else {
        sprintf(mp->tmpstr, "models/model%d.ini", idx + 1);
    }
    fh = fopen(mp->tmpstr, "r");
    sprintf(mp->tmpstr, "%d: NONE", idx + 1);
    if (fh) {
        long user = idx + 1;
        if ((long)data == LOAD_TEMPLATE)
            user = -user;
        ini_parse_file(fh, ini_handle_name, (void *)user);
        fclose(fh);
    }
    return mp->tmpstr;
}
static void okcancel_cb(guiObject_t *obj, const void *data)
{
    int msg = (long)data;
    (void)obj;
    if (msg == LOAD_MODEL + 1) {
        /* Load Model */
        CONFIG_SaveModelIfNeeded();
        PROTOCOL_DeInit();
        CONFIG_ReadModel(mp->selected);
        /* Need to recaclulate channels to see if we're in a safe state */
        MIXER_Init();
        MIXER_CalcChannels();
        PROTOCOL_Init(0);
    } else if (msg == SAVE_MODEL + 1) {
        /* Save Model */
        CONFIG_WriteModel(mp->selected);
        CONFIG_ReadModel(mp->selected);  //Reload the model after saving to switch (for future saves)
    } else if (msg == LOAD_TEMPLATE + 1) {
        /* Load Template */
        CONFIG_ReadTemplate(mp->selected);
    } else if (msg == LOAD_ICON + 1) {
        if (mp->selected == 1)
            Model.icon[0] = 0;
        else
            strcpy(Model.icon, mp->iconstr);
    }
    GUI_RemoveAllObjects();
    mp->return_page(-1);  // -1 for devo10 means return to the focus of previous page, which is important so that users don't need to scroll down from the 1st item
}

static const char *show_loadsave_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return ((long)data) == SAVE_MODEL + 1 ? _tr("Save") : _tr("Load");
}

/* loadsave values:
 * 0 : Load Model
 * 1 : Save Model
 * 2 : Load Template
 * 3 : Load Icon
 */
void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page))
{
    u8 num_models = 0;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    mp->return_page = return_page;
    mp->obj = NULL;
    _show_buttons(loadsave);
    if (loadsave == LOAD_TEMPLATE) { //Template
        if (FS_OpenDir("template")) {
            char filename[13];
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".ini", 4) == 0) {
                    num_models++;
                }
            }
            FS_CloseDir();
        }
        mp->selected = 1;
    } else if (loadsave == LOAD_ICON) { //Icon
        num_models = 1;
        mp->selected = 1;
        strncpy(mp->iconstr, CONFIG_GetCurrentIcon(), sizeof(mp->iconstr));
        if (FS_OpenDir("modelico")) {
            char filename[13];
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".bmp", 4) == 0) {
                    num_models++;
                    if(Model.icon[0] && strncasecmp(Model.icon+9, filename, 13) == 0) {
                        mp->selected = num_models;
                    }
                }
            }
        }
    } else {
        for (num_models = 1; num_models <= 100; num_models++) {
            sprintf(mp->tmpstr, "models/model%d.ini", num_models);
            FILE *fh = fopen(mp->tmpstr, "r");
            if (! fh)
                break;
            fclose(fh);
        }
        num_models--;
        mp->selected = CONFIG_GetCurrentModel();
        strncpy(mp->iconstr, CONFIG_GetCurrentIcon(), sizeof(mp->iconstr));
    }
    if (loadsave == SAVE_MODEL) {
        mp->selected = 0;
    } else if (loadsave == LOAD_ICON) {
    } else {
        mp->selected = CONFIG_GetCurrentModel();
    }
    _show_list(loadsave, num_models);
}