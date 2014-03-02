#include "bindwin.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

unsigned int numlock_mask = 0;
unsigned int scrolllock_mask = 0;
unsigned int capslock_mask = 0;

static const int digits[] =
    { XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9, XK_0 };
const int digits_size = sizeof(digits) / sizeof(digits[0]);	// SUDDENLY 10

void get_offending_modifiers(Display * dpy)
{
	/* pulled from xbindkeys */
	int i;
	XModifierKeymap *modmap;
	KeyCode nlock, slock;
	static int mask_table[8] = {
		ShiftMask, LockMask, ControlMask, Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};

	nlock = XKeysymToKeycode(dpy, XK_Num_Lock);
	slock = XKeysymToKeycode(dpy, XK_Scroll_Lock);

	/*
	 * Find out the masks for the NumLock and ScrollLock modifiers,
	 * so that we can bind the grabs for when they are enabled too.
	 */
	modmap = XGetModifierMapping(dpy);

	if (modmap != NULL && modmap->max_keypermod > 0) {
		for (i = 0; i < 8 * modmap->max_keypermod; i++) {
			if (modmap->modifiermap[i] == nlock && nlock != 0)
				numlock_mask =
				    mask_table[i / modmap->max_keypermod];
			else if (modmap->modifiermap[i] == slock && slock != 0)
				scrolllock_mask =
				    mask_table[i / modmap->max_keypermod];
		}
	}

	capslock_mask = LockMask;

	if (modmap)
		XFreeModifiermap(modmap);
}

void grab_keys(Display * dd, Window root_win)
{
	get_offending_modifiers(dd);
	/*
	 * Grab Ctrl+0-9 & Alt+0-9
	 * */
	for (int j = 0; j < digits_size; j++) {
		int keycode = XKeysymToKeycode(dd, digits[j]);

		for (int i = 0; i < 8; i++) {
			unsigned int modifiers =
			    (i & 1 ? capslock_mask : 0) | (i & 2 ?
							   scrolllock_mask : 0)
			    | (i & 4 ? numlock_mask : 0);

			XGrabKey(dd, keycode,
				 ControlMask | modifiers, root_win,
				 False, GrabModeAsync, GrabModeAsync);

			XGrabKey(dd, keycode,
				 Mod1Mask | modifiers, root_win,
				 False, GrabModeAsync, GrabModeAsync);
		}
	}
}
