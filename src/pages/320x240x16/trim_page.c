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

#include "common.h"
#include "pages.h"
#include "gui/gui.h"

#include "../common/_trim_page.c"

static void _show_page()
{
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TRIM));
    GUI_CreateLabel(8, 40, NULL, DEFAULT_FONT, _tr("Input:"));
    GUI_CreateLabel(72, 40, NULL, DEFAULT_FONT, _tr("Trim -:"));
    GUI_CreateLabel(136, 40, NULL, DEFAULT_FONT, _tr("Trim +:"));
    GUI_CreateLabel(200, 40, NULL, DEFAULT_FONT, _tr("Trim Step:"));
    struct Trim *trim = MIXER_GetAllTrims();
    for (u8 i = 0; i < NUM_TRIMS; i++) {
        GUI_CreateButton(8, 24*i + 64, BUTTON_48x16,
            trimsource_name_cb, 0x0000, _edit_cb, (void *)((long)i));
        GUI_CreateLabel(72, 24*i + 66, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[i].neg));
        GUI_CreateLabel(136, 24*i + 66, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[i].pos));
        GUI_CreateTextSelect(200, 24*i + 64, TEXTSELECT_96, 0x0000, NULL, set_trimstep_cb, &trim[i].step);
    }
}

static void _edit_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    struct Trim *trim = MIXER_GetAllTrims();
    PAGE_SetModal(1);
    tp->index = (long)data;
    tp->trim = trim[tp->index];

    PAGE_RemoveAllObjects();
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);

    //Row 1
    GUI_CreateLabel(8, 48, NULL, DEFAULT_FONT, _tr("Input:"));
    GUI_CreateTextSelect(72, 48, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &tp->trim.src);
    //Row 2
    GUI_CreateLabel(8, 72, NULL, DEFAULT_FONT, _tr("Trim -:"));
    GUI_CreateTextSelect(72, 72, TEXTSELECT_96, 0x0000, NULL, set_trim_cb, &tp->trim.neg);
    GUI_CreateLabel(176, 72, NULL, DEFAULT_FONT, _tr("Trim +:"));
    GUI_CreateTextSelect(216, 72, TEXTSELECT_96, 0x0000, NULL, set_trim_cb, &tp->trim.pos);
    //Row 3
    GUI_CreateLabel(8, 96, NULL, DEFAULT_FONT, _tr("Trim Step:"));
    GUI_CreateTextSelect(72, 96, TEXTSELECT_96, 0x0000, NULL, set_trimstep_cb, &tp->trim.step);
}