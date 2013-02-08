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

static struct mixer_page * const mp = &pagemem.u.mixer_page;
#define gui (&gui_objs.u.stdswitch)


typedef struct {
    u8 switches[3]; // maximum is 3(such as INP_MIX, INP_FMOD), minimun is 2(such as INP_RUD_DR, INP_ELE_DR, INP_AIL_DR, INP_GEAR)
    char name[15];
} SwitchTable;
static SwitchTable switch_table[] = {
    {{0, INP_RUD_DR1, ALWAYSOFF_SWITCH}, "RUD DR"},  // no translation for switch labels
    {{0, INP_ELE_DR1, ALWAYSOFF_SWITCH}, "ELE DR"},
    {{0, INP_AIL_DR1, ALWAYSOFF_SWITCH}, "AIL DR"},
    {{0,   INP_GEAR1,   ALWAYSOFF_SWITCH}, "GEAR"},
    {{0,   INP_MIX1,        INP_MIX2},         "MIX"},
    {{0,  INP_FMOD1,       INP_FMOD2},        "FMOD"},
};
static u8 switch_idx[SWITCHFUNC_LAST];

static u8 get_switch_idx(FunctionSwitch switch_type) {
    for (u8 i = 0; i < 6; i++) {
        for (u8 j = 0; j < 3; j++) {
            if (mapped_simple_channels.switches[switch_type] == switch_table[i].switches[j])
                return i;
        }
    }
    //printf("something is wrong in get_switch_idx()\n\n");
    return 0; // this is impossible
}

static void refresh_switches()
{
    struct Mixer *mix = MIXER_GetAllMixers();

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
        mapped_simple_channels.switches[SWITCHFUNC_HOLD] =  Model.limits[mapped_simple_channels.throttle].safetysw;
    u8 found_gyro_switch = 0;
    u8 found_flymode_switch = 0;
    u8 found_drexp_rud_switch = 0;
    u8 found_drexp_ail_switch = 0;
    u8 found_drexp_ele_switch = 0;
    for (u8 idx = 0; idx < NUM_MIXERS; idx++) {
        if (!MIXER_SRC(mix[idx].src) || mix[idx].mux != MUX_REPLACE)  // all none replace mux will be considered as program mix in the Standard mode
            continue;
        if (!found_gyro_switch && mix[idx].sw != 0 && (mix[idx].dest == mapped_simple_channels.gear || mix[idx].dest == mapped_simple_channels.aux2)) {
            found_gyro_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_GYROSENSE] = mix[idx].sw;
        } else if (!found_drexp_rud_switch && mix[idx].dest == mapped_simple_channels.rudd && mix[idx].sw != 0) {
            found_drexp_rud_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_RUD] = mix[idx].sw;
        } else if (!found_drexp_ail_switch && mix[idx].dest == mapped_simple_channels.aile && mix[idx].sw != 0) {
            found_drexp_ail_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_AIL] = mix[idx].sw;
        } else if (!found_drexp_ele_switch && mix[idx].dest == mapped_simple_channels.elev && mix[idx].sw != 0) {
            found_drexp_ele_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_DREXP_ELE] = mix[idx].sw;
        } else if (!found_flymode_switch && (mix[idx].dest == NUM_OUT_CHANNELS + 2) && mix[idx].sw != 0) { //virt3
            found_flymode_switch = 1;
            mapped_simple_channels.switches[SWITCHFUNC_FLYMODE] = mix[idx].sw;
        }
        if (found_flymode_switch && found_gyro_switch && found_drexp_rud_switch && found_drexp_ail_switch && found_drexp_ele_switch)
            break;  // don't need to check the rest
    }

    // initialize
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        switch_idx[switch_type] = get_switch_idx(switch_type);
}

// the only reason not to save changes in live is I don't want to change mixers' switch in the middle of changing options
void save_changes()
{
    MUSIC_Play(MUSIC_SAVING);
    for (FunctionSwitch switch_type = SWITCHFUNC_FLYMODE; switch_type < SWITCHFUNC_LAST; switch_type++)
        if (switch_type == SWITCHFUNC_HOLD) // bug fix: hold should use pos 1 instead of pos 0
            mapped_simple_channels.switches[switch_type] = switch_table[switch_idx[switch_type]].switches[1];
        else
            mapped_simple_channels.switches[switch_type] = switch_table[switch_idx[switch_type]].switches[0];

    if (Model.limits[mapped_simple_channels.throttle].safetysw)
       Model.limits[mapped_simple_channels.throttle].safetysw = mapped_simple_channels.switches[SWITCHFUNC_HOLD];

    u8 gyro_gear_count = 0;
    u8 gyro_aux2_count = 0;
    u8 drexp_aile_count = 0;
    u8 drexp_elev_count = 0;
    u8 drexp_rudd_count = 0;
    u8 thro_count = 0;
    u8 pit_count = 0;
    struct Mixer *mix = MIXER_GetAllMixers();
    for (u8 i = 0; i < NUM_MIXERS; i++) {
        if (!MIXER_SRC(mix[i].src) || mix[i].mux != MUX_REPLACE)  // all none replace mux will be considered as program mix in the Standard mode
            continue;

        if (mix[i].dest == mapped_simple_channels.gear) {
            if (gyro_gear_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_GYROSENSE]].switches[gyro_gear_count++];
        } else if (mix[i].dest == mapped_simple_channels.aux2) {
            if (gyro_aux2_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_GYROSENSE]].switches[gyro_aux2_count++];
        } else if (mix[i].dest == mapped_simple_channels.aile) {
            if (drexp_aile_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_DREXP_AIL]].switches[drexp_aile_count++];
        } else if (mix[i].dest == mapped_simple_channels.elev) {
            if (drexp_elev_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_DREXP_ELE]].switches[drexp_elev_count++];
        } else if (mix[i].dest == mapped_simple_channels.rudd) {
            if (drexp_rudd_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_DREXP_RUD]].switches[drexp_rudd_count++];
        } else if (mix[i].dest == mapped_simple_channels.pitch) {
            if (pit_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[pit_count++];
            else if (pit_count == 3 && mix[i].sw != ALWAYSOFF_SWITCH)
                mix[i].sw = mapped_simple_channels.switches[SWITCHFUNC_HOLD];  // hold curve
        } else if (mix[i].dest == mapped_simple_channels.throttle) {
            if (thro_count < 3)
                mix[i].sw = switch_table[switch_idx[SWITCHFUNC_FLYMODE]].switches[thro_count++];
        }
    }
}


static const char *switch_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    FunctionSwitch switch_type = (long)data;
    switch_idx[switch_type] = GUI_TextSelectHelper(switch_idx[switch_type], 0, 5, dir, 1, 1, NULL);
    strcpy(mp->tmpstr, switch_table[switch_idx[switch_type]].name);
    return mp->tmpstr;
}


