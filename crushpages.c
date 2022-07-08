#ifdef CC_BUILD_WIN
#define CC_API __declspec(dllimport)
#define CC_VAR __declspec(dllimport)
#else
#define CC_API
#define CC_VAR
#endif

#include "Chat.h"
#include "Game.h"
#include "String.h"
#include "Server.h"
#include "Input.h"
#include "Event.h"
#include "Platform.h"

#define CRP_PLUGIN_VER "1"
#define CRP_WELCOME "&eUsing &8ya_&coi_ &fv"
#define CRP_CLIENT " +yaoiv"

static struct _ServerConnectionData *Server_;
static struct _InputEventsList *InputEvents_;

/*########################################################################################################################*
 *------------------------------------------------------Dynamic loading----------------------------------------------------*
 *#########################################################################################################################*/
#define QUOTE(x) #x

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
//#include <windows.h>
#define LoadSymbol(name) name##_ = GetProcAddress(app, QUOTE(name))
#else
#define _GNU_SOURCE
#include <dlfcn.h>
#define LoadSymbol(name) name##_ = dlsym(RTLD_DEFAULT, QUOTE(name))
#endif

static void dummy(void *obj, int c)
{
    return;
}

void debug2(int msg)
{
    char bfr2[254];
    cc_string msg2;
    String_InitArray(msg2, bfr2);
    String_AppendInt(&msg2, msg);
    Chat_Add(&msg2);
}

void Event_RaiseInput(struct Event_Input *handlers, int key, cc_bool repeating)
{
    int i;
    for (i = 0; i < handlers->Count; i++)
    {
        handlers->Handlers[i](handlers->Objs[i], key, repeating);
    }
}

int linearSearch(int arr[], int l, int r, int x)
{
    for (int i = l; i < r; i++)
    {
        if (arr[i] == x)
            return i;
    }
    return -1;
}

struct layout
{
    char monicker[4];
    int len;
    int x[64];
    int y[64];
};

struct inputState
{
    cc_bool shiftPressed;
    cc_bool tabPressed;
    cc_bool ctrlPressed;
    cc_bool chatOpen;
    cc_uint8 layoutNum;
    cc_uint8 layoutLens[8];
    cc_uint8 langSwitch;
    struct layout layouts[8];
    void *pressFunc;

} static state1;

void switchLayout()
{
    if (state1.langSwitch == 9)
        state1.langSwitch = 0;
    else
        state1.langSwitch = 9;
    return;
}

static void downMgr(void *obj, int c)
{
    switch (c)
    {
    case 94:
        return;
        break;
    case 36:
        if (state1.ctrlPressed == true)
        {
            switchLayout();
        }
        state1.shiftPressed = true;
        return;
        break;
    case 37:
        if (state1.ctrlPressed == true)
        {
            switchLayout();
        }
        state1.shiftPressed = true;
        return;
        break;
    case 38:
        if (state1.shiftPressed == true)
        {
            switchLayout();
        }
        state1.ctrlPressed = true;
        return;
    case 39:
        if (state1.shiftPressed == true)
        {
            switchLayout();
        }
        state1.ctrlPressed = true;
        return;
    case 95:
        state1.tabPressed = true;
        return;
        break;
    }

    if (state1.chatOpen == true)
    {
        if (!state1.shiftPressed && c >= KEY_A && c <= KEY_Z)
            c += 32;
        if ((state1.langSwitch) != 9)
        {

            int pos = linearSearch(state1.layouts[state1.langSwitch].x, 0,
                                   state1.layoutLens[state1.langSwitch], c);
            if (pos != -1)
            {
                InputEvents_->Press.Handlers[0] = state1.pressFunc;
                Event_RaiseInt(&InputEvents_->Press, state1.layouts[state1.langSwitch].y[pos]);
                InputEvents_->Press.Handlers[0] = dummy;
            }
        }

        else
        {
            InputEvents_->Press.Handlers[0] = state1.pressFunc;
            Event_RaiseInt(&InputEvents_->Press, c);
            InputEvents_->Press.Handlers[0] = dummy;
        }
    }
    if (KeyBind_IsPressed(KEYBIND_CHAT) && state1.chatOpen == false)
    {

        state1.chatOpen = true;
        InputEvents_->Press.Handlers[0] = dummy;
    }

    if (state1.chatOpen == true)
    {
        if ((KeyBind_IsPressed(KEYBIND_SEND_CHAT) || c == 92))
        {
            state1.chatOpen = false;
            InputEvents_->Press.Handlers[0] = state1.pressFunc;
        }
    }
}

static void upMgr(void *obj, int c)
{

    if (c == 38 || c == 39)
    {
        state1.ctrlPressed = false;
        return;
    }

    if (c == 36 || c == 37)
    {
        state1.shiftPressed = false;
        return;
    }

    if (c == 95)
    {
        state1.tabPressed = false;
        return;
    }
}

void na2layout(){
     (state1.layouts[0].monicker[0] = 'n', state1.layouts[0].monicker[1] = 'a',
     state1.layouts[0].monicker[2] = '2', state1.layouts[0].monicker[3] = '0');
    int *move = state1.layouts[0].y;
    (*move++ = 13, *move++ = 14, *move++ = 15, *move++ = 19, *move++ = 20, *move++ = 158, *move++ = 166);
    for (int i = 176; i < 218; i++)
    {
        *move++ = i;
    }

    move = state1.layouts[0].x;
    for (int i = 97; i < 123; i++)
    {
        *move++ = i;
        state1.layoutLens[0]++;
    }
    for (int i = 65; i < 90; i++)
    {
        *move++ = i;
        state1.layoutLens[0]++;
    }
    for (int i = 48; i < 57; i++)
    {
        *move++ = i;
        state1.layoutLens[0]++;
    }
}

static void crushPages_Init(void)
{
    state1.chatOpen = false;
    state1.layoutNum = 0;
    state1.shiftPressed = false;
    state1.tabPressed = false;

    for (int i = 0; i < 8; i++)
    {
        state1.layoutLens[8] = 0;
    }

    cc_string msg;
    char bfr[sizeof(CRP_WELCOME) + sizeof(CRP_PLUGIN_VER)];
    String_InitArray(msg, bfr);
    String_AppendConst(&msg, CRP_WELCOME CRP_PLUGIN_VER);
    Chat_Add(&msg);
    LoadSymbol(Server);
    String_AppendConst(&Server_->AppName, CRP_CLIENT CRP_PLUGIN_VER);
    LoadSymbol(InputEvents);
    Event_Register_(&InputEvents_->Down, NULL, downMgr);
    Event_Register_(&InputEvents_->Up, NULL, upMgr);
    state1.pressFunc = InputEvents_->Press.Handlers[0];

   na2layout();

    state1.langSwitch = 9;
}

#ifdef CC_BUILD_WIN
// special attribute to get symbols exported with Visual Studio
#define PLUGIN_EXPORT __declspec(dllexport)
#else
// public symbols already exported when compiling shared lib with GCC
#define PLUGIN_EXPORT
#endif

PLUGIN_EXPORT int Plugin_ApiVersion = 1;
PLUGIN_EXPORT struct IGameComponent Plugin_Component = {
    crushPages_Init /* Init */
};
